#include "app_tasks.h"

#include "app_events.h"
#include "inverter_types.h"
#include "inverter_ctrl.h"
#include "inverter_sampler.h"
#include "inverter_state.h"

#include "cmsis_os2.h"

#include "pi_ctrl.h"
#include "clarke.h"
#include "lpf.h"
#include "transforms.h"
#include "svpwm.h"
#include "math_utils.h"
#include "biquad.h"
#include "notch.h"

/* GFL 环路 */
#include "gfl_config.h"
#include "pll.h"
#include "park.h"
#include "clarke.h"

/* GFL 环路实例 */
static Gfl_Instance s_gfl;

/* PLL 实例 */
static Pll_Handle s_pll;
static const Pll_Config s_pll_cfg = {
    .omega_n = 314.159f,        /* 自然频率 50Hz*2π */
    .zeta = 0.7f,               /* 阻尼比 */
    .Ts = 0.001f,                /* 1ms 采样周期 */
    .freq_nom = 50.0f,          /* 额定频率 50Hz */
    .freq_min = 45.0f,          /* 最小频率 45Hz */
    .freq_max = 55.0f,          /* 最大频率 55Hz */
    .v_threshold = 0.5f,         /* 电压阈值 */
    .hold_time = 0.5f,          /* 开环保持时间 */
};

/* Park 变换句柄 */
static Park_Handle s_park;

/* SVPWM 句柄 */
static SvPwm_Handle s_svpwm;
static const SvPwm_Config s_svpwm_cfg = {
    .vbus = 700.0f,  /* 母线电压 700V */
};

/* GFL 内部状态 */
static float s_v_alpha = 0.0f;     /* α轴电压 */
static float s_v_beta = 0.0f;      /* β轴电压 */
static float s_i_alpha = 0.0f;     /* α轴电流 */
static float s_i_beta = 0.0f;      /* β轴电流 */

/* 电流环共享状态 - 用于快速中断和慢速任务间传递 */
static float s_i_d = 0.0f;          /* d轴电流反馈 */
static float s_i_q = 0.0f;          /* q轴电流反馈 */
static float s_Id_ref = 0.0f;       /* d轴电流参考 (来自 GFL) */
static float s_Iq_ref = 0.0f;       /* q轴电流参考 (来自 GFL) */
static float s_theta = 0.0f;         /* 锁相角度 */
static float s_freq = 50.0f;        /* 锁相频率 */

/* 输出占空比 */
static float s_duty_a = 0.5f;
static float s_duty_b = 0.5f;
static float s_duty_c = 0.5f;

/* 功率指令 (可由上层设置) */
static float s_P_ref = 0.0f;   /* 有功功率参考 0 pu */
static float s_Q_ref = 0.0f;   /* 无功功率参考 0 pu */

static Inv_Handle s_inv_ctrl;
static const Inv_Config s_inv_ctrl_cfg = {
    .sampler = {
        .adc_instance = 1,
        .dma_instance = 1,
        .adc_dr_addr = 0x40022040,
        .samples_per_period = SAMPLER_SAMPLES_PER_PERIOD,
        .buf_size = SAMPLER_BUF_SIZE_PERIOD,
    },
    .control_period_us = CTRL_PERIOD_US,
    .startup_delay_ms = 100,
    .fault_recover_delay_ms = 500,
    .vbus_min = 150.0f,
    .vbus_max = 420.0f,
    .i_max = 50.0f,
    .temp_max = 85.0f,
    .fault_mask = 0xFF,
};

static Transforms_Handle s_transforms;
static Math_ClarkeFactors s_clarke_factors;
static Math_ParkFactors s_park_factors;

static PiCtrl_Handle s_pi_d;
static const PiCtrl_Config s_pi_d_cfg = {
    .kp = 0.1f,
    .ki = 0.01f,
    .out_max = 1.0f,
    .out_min = -1.0f,
    .Ts = 1.0f / 24000.0f,  /* 41.67μs = 24kHz 快速电流环 */
};

static PiCtrl_Handle s_pi_q;
static const PiCtrl_Config s_pi_q_cfg = {
    .kp = 0.1f,
    .ki = 0.01f,
    .out_max = 1.0f,
    .out_min = -1.0f,
    .Ts = 1.0f / 24000.0f,  /* 41.67μs = 24kHz 快速电流环 */
};

static BiquadHandle s_biquad_lpf;
static BiquadConfig s_biquad_lpf_cfg = {
    .fc = 1000.0f,
    .Q = 0.707f,
    .Ts = 1.0f / 48000.0f,
};

static NotchHandle s_notch;
static NotchConfig s_notch_cfg = {
    .fc = 1000.0f,
    .Q = 10.0f,
    .depth_db = 3.0f,
    .Ts = 1.0f / 48000.0f,
};

static osMessageQueueId_t s_evt_q = NULL;
static osEventFlagsId_t s_fault_flags = NULL;

static const uint32_t kEvtQueueDepth = 64;

static void CommsTask(void *argument);
static void HousekeepingTask(void *argument);

void App_TasksCreate(void) {
    Transforms_Init(&s_transforms);
    Math_ClarkeFactors_Init(&s_clarke_factors, 1.0f / 48000.0f);
    Math_ParkFactors_Init(&s_park_factors, 0.0f);

    PiCtrl_Init(&s_pi_d, &s_pi_d_cfg);
    PiCtrl_Init(&s_pi_q, &s_pi_q_cfg);

    Biquad_LPF(&s_biquad_lpf_cfg, s_biquad_lpf_cfg.fc, s_biquad_lpf_cfg.Q, s_biquad_lpf_cfg.Ts);
    Biquad_Init(&s_biquad_lpf, &s_biquad_lpf_cfg);

    Notch_Config(&s_notch_cfg, s_notch_cfg.fc, s_notch_cfg.Q, s_notch_cfg.depth_db, s_notch_cfg.Ts);
    Notch_Init(&s_notch, &s_notch_cfg);

    Inv_Init(&s_inv_ctrl, &s_inv_ctrl_cfg);

    /* PLL 初始化 */
    Pll_Init(&s_pll, &s_pll_cfg);

    /* Park 变换初始化 */
    Park_Init(&s_park, NULL);

    /* SVPWM 初始化 */
    SvPwm_Init(&s_svpwm, &s_svpwm_cfg);

    /* GFL 环路初始化 */
    Gfl_Config_Init(&s_gfl);

    if (s_evt_q == NULL) {
        s_evt_q = osMessageQueueNew(kEvtQueueDepth, sizeof(app_event_t), NULL);
    }
    if (s_fault_flags == NULL) {
        s_fault_flags = osEventFlagsNew(NULL);
    }

    (void)osThreadNew(CommsTask, NULL, &(osThreadAttr_t){
                                        .name = "comms",
                                        .priority = osPriorityNormal,
                                        .stack_size = 1536,
                                    });

    (void)osThreadNew(HousekeepingTask, NULL, &(osThreadAttr_t){
                                              .name = "hk",
                                              .priority = osPriorityLow,
                                              .stack_size = 1024,
                                          });
}

static inline void PostEvtFromIsr(app_event_t *e) {
    if (s_evt_q == NULL) {
        return;
    }
    (void)osMessageQueuePut(s_evt_q, e, 0U, 0U);
}

void App_PostControlTickFromIsr(uint32_t ts) {
    app_event_t e = {
        .type = APP_EVT_CTRL_TICK,
        .ts = ts,
        .arg0 = 0,
        .arg1 = 0,
    };
    PostEvtFromIsr(&e);
}

void App_PostAdcBufReadyFromIsr(uint32_t ts, uint32_t buf_addr, uint32_t buf_index) {
    app_event_t e = {
        .type = (buf_index == 0U) ? APP_EVT_ADC_BUF0_READY : APP_EVT_ADC_BUF1_READY,
        .ts = ts,
        .arg0 = buf_addr,
        .arg1 = buf_index,
    };
    PostEvtFromIsr(&e);
}

void App_PostAdcHalfFromIsr(uint32_t ts, uint32_t dma_addr) {
    app_event_t e = {
        .type = APP_EVT_ADC_HALF,
        .ts = ts,
        .arg0 = dma_addr,
        .arg1 = 0,
    };
    PostEvtFromIsr(&e);
}

void App_PostAdcFullFromIsr(uint32_t ts, uint32_t dma_addr) {
    app_event_t e = {
        .type = APP_EVT_ADC_FULL,
        .ts = ts,
        .arg0 = dma_addr,
        .arg1 = 0,
    };
    PostEvtFromIsr(&e);
}

void App_FaultLatchFromIsr(uint32_t code, uint32_t detail) {
    (void)detail;
    if (s_fault_flags != NULL) {
        const uint32_t idx = code & 0x1FU;
        const uint32_t bit = 1U << idx;
        (void)osEventFlagsSet(s_fault_flags, bit);
    }

    app_event_t e = {
        .type = APP_EVT_FAULT_LATCHED,
        .ts = 0,
        .arg0 = code,
        .arg1 = detail,
    };
    PostEvtFromIsr(&e);
}

void App_FaultLatch(uint32_t code, uint32_t detail) {
    App_FaultLatchFromIsr(code, detail);
}

static void CommsTask(void *argument) {
    (void)argument;
    for (;;) {
        (void)osDelay(10);
    }
}

static void HousekeepingTask(void *argument) {
    (void)argument;
    for (;;) {
        (void)osDelay(100);
    }
}

void Task1ms(void) {
    Inv_ControlTask_1ms(&s_inv_ctrl);
    
    /* GFL 环路 1ms 任务 */
    GFL_Task_1ms();
}

/**
 * @brief GFL 1ms 快速控制任务
 * 
 * 执行 GFL 环路的快速控制计算:
 * - 获取 ABC 采样值
 * - 执行 Clarke 变换得到 αβ
 * - PLL 锁相获取角度
 * - 计算电流参考 (Id/Iq)
 * - 高低穿处理
 * - 限幅
 * - 电流环 PI 控制
 * - InvPark 变换
 * - SVPWM 占空比输出
 */
/**
 * @brief 安全除法，避免除零和 NAN/INF
 */
static inline float safe_div(float a, float b, float def) {
    if (b < 0.0001f && b > -0.0001f) {
        return def;
    }
    float result = a / b;
    if (result != result) {  /* NAN 检测 */
        return def;
    }
    if (result > 1e10f || result < -1e10f) {  /* INF 检测 */
        return def;
    }
    return result;
}

/**
 * @brief 限幅函数
 */
static inline float clampf(float val, float min, float max) {
    if (val > max) return max;
    if (val < min) return min;
    return val;
}

/**
 * @brief GFL 1ms 监督控制任务
 * 
 * 此函数在 1ms 任务中调用，负责:
 * - 获取采样值
 * - PLL 锁相
 * - Clarke 变换
 * - GFL 环路执行 (限幅、高低穿、功率分配)
 * 
 * 输出 Id_ref/Iq_ref 供快速电流环使用
 */
void GFL_Task_1ms(void) {
    /* ========== 1. 获取采样值 ========== */
    float V_bus = Inv_GetV_Bus(&s_inv_ctrl);
    
    /* ABC 相电流采样值 */
    float i_a = Inv_GetI_A(&s_inv_ctrl);
    float i_b = Inv_GetI_B(&s_inv_ctrl);
    /* 当前仅使用 A/B 相，假设平衡 */
    
    /* ABC 相电压采样值 */
    float v_a = Inv_GetV_Out(&s_inv_ctrl);  /* a相电压 */
    
    /* 估算 b/c 相电压 (假设三相平衡) */
    float v_b = -0.5f * v_a;
    float v_c = -0.5f * v_a;
    
    /* ========== 2. Clarke 变换 ========== */
    s_v_alpha = v_a;
    s_v_beta = (v_a + 2.0f * v_b) * 0.5773503f;  /* 1/√3 */
    
    s_i_alpha = i_a;
    s_i_beta = (i_a + 2.0f * i_b) * 0.5773503f;
    
    /* ========== 3. PLL 锁相 ========== */
    bool pll_locked = Pll_Step(&s_pll, s_v_alpha, s_v_beta, &s_theta, &s_freq);
    
    (void)pll_locked;  /* 预留用于开环模式处理 */
    
    /* ========== 4. NAN/INF 保护 ========== */
    s_i_alpha = clampf(s_i_alpha, -10.0f, 10.0f);
    s_i_beta = clampf(s_i_beta, -10.0f, 10.0f);
    s_v_alpha = clampf(s_v_alpha, -500.0f, 500.0f);
    s_v_beta = clampf(s_v_beta, -500.0f, 500.0f);
    
    /* ========== 5. GFL 环路执行 ========== */
    GflLoop_Output gfl_output;
    Gfl_Step(&s_gfl, s_v_alpha, s_v_beta, V_bus, s_P_ref, s_Q_ref, &gfl_output);
    
    /* NAN/INF 保护并限幅 */
    s_Id_ref = safe_div(gfl_output.Id_ref, 1.0f, 0.0f);
    s_Iq_ref = safe_div(gfl_output.Iq_ref, 1.0f, 0.0f);
    s_Id_ref = clampf(s_Id_ref, -2.0f, 2.0f);
    s_Iq_ref = clampf(s_Iq_ref, -2.0f, 2.0f);
    
    /* ========== 6. 检查故障 ========== */
    Gfl_Mode mode = GFL_GET_MODE(&s_gfl);
    Gfl_FaultType fault = GFL_GET_FAULT(&s_gfl);
    
    if (GFL_HAS_FAULT(&s_gfl) || mode == GFL_MODE_FAULT) {
        /* 故障时清零电流参考 */
        Inv_FaultSet(&s_inv_ctrl, (uint8_t)fault);
        s_Id_ref = 0.0f;
        s_Iq_ref = 0.0f;
        PiCtrl_Reset(&s_pi_d);
        PiCtrl_Reset(&s_pi_q);
    }
}

/**
 * @brief 快速电流环控制 (在 PWM 中断或 24kHz 定时器中调用)
 * 
 * 此函数在快速中断 (24kHz) 中调用，负责:
 * - Park 变换 (电流)
 * - PI 控制
 * - InvPark 变换 (电压)
 * - SVPWM
 * 
 * @note 此函数使用 GFL_Task_1ms 输出的 s_Id_ref/s_Iq_ref
 */
void GFL_FastControl_24kHz(void) {
    float cos_theta = cosf(s_theta);
    float sin_theta = sinf(s_theta);
    
    /* Park 变换 (电流反馈) */
    float i_d = s_i_alpha * cos_theta + s_i_beta * sin_theta;
    float i_q = -s_i_alpha * sin_theta + s_i_beta * cos_theta;
    
    /* PI 控制 */
    float Vd_out, Vq_out;
    PiCtrl_Step(&s_pi_d, s_Id_ref, i_d, &Vd_out);
    PiCtrl_Step(&s_pi_q, s_Iq_ref, i_q, &Vq_out);
    
    /* SVPWM (使用电压参考 Vd/Vq) */
    SvPwm_SetTheta(&s_svpwm, s_theta);
    SvPwm_Step(&s_svpwm, Vd_out, Vq_out, &s_duty_a, &s_duty_b, &s_duty_c);
    
    /* 占空比限幅 */
    s_duty_a = clampf(s_duty_a, 0.05f, 0.95f);
    s_duty_b = clampf(s_duty_b, 0.05f, 0.95f);
    s_duty_c = clampf(s_duty_c, 0.05f, 0.95f);
}

/**
 * @brief 设置 GFL 有功功率参考
 */
void GFL_SetPowerRef(float P_ref, float Q_ref) {
    s_P_ref = P_ref;
    s_Q_ref = Q_ref;
}

/**
 * @brief 获取 GFL 当前模式
 */
Gfl_Mode GFL_GetMode(void) {
    return Gfl_GetMode(&s_gfl);
}

/**
 * @brief 获取 GFL 当前故障
 */
Gfl_FaultType GFL_GetFault(void) {
    return Gfl_GetFault(&s_gfl);
}

/**
 * @brief 清除 GFL 故障
 */
void GFL_ClearFault(void) {
    Gfl_ClearFault(&s_gfl);
}

/**
 * @brief 获取占空比 A
 */
float GFL_GetDutyA(void) {
    return s_duty_a;
}

/**
 * @brief 获取占空比 B
 */
float GFL_GetDutyB(void) {
    return s_duty_b;
}

/**
 * @brief 获取占空比 C
 */
float GFL_GetDutyC(void) {
    return s_duty_c;
}

/**
 * @brief 获取 PLL 锁定状态
 */
bool GFL_IsPllLocked(void) {
    return Pll_IsOpenLoop(&s_pll) == false;
}

/**
 * @brief 获取电网频率
 */
float GFL_GetFreq(void) {
    return Pll_GetFrequency(&s_pll);
}

/**
 * @brief 获取电网电压幅值
 */
float GFL_GetGridVoltage(void) {
    return Pll_GetVoltageAmplitude(&s_pll);
}

void Task5ms(void) {
    Inv_ControlTask_5ms(&s_inv_ctrl);
}

void Task10ms(void) {
    Inv_ControlTask_10ms(&s_inv_ctrl);
}

void Task50ms(void) {
    Inv_ControlTask_50ms(&s_inv_ctrl);
}

void Task1000ms(void) {
    Inv_ControlTask_1000ms(&s_inv_ctrl);
}
