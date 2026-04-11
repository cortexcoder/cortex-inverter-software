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
static float s_i_d = 0.0f;          /* d轴电流反馈 */
static float s_i_q = 0.0f;          /* q轴电流反馈 */

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
    .Ts = 1.0f / 48000.0f,
};

static PiCtrl_Handle s_pi_q;
static const PiCtrl_Config s_pi_q_cfg = {
    .kp = 0.1f,
    .ki = 0.01f,
    .out_max = 1.0f,
    .out_min = -1.0f,
    .Ts = 1.0f / 48000.0f,
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
void GFL_Task_1ms(void) {
    /* ========== 1. 获取采样值 ========== */
    float V_bus = Inv_GetV_Bus(&s_inv_ctrl);
    
    /* ABC 相电流采样值 */
    float i_a = Inv_GetI_A(&s_inv_ctrl);
    float i_b = Inv_GetI_B(&s_inv_ctrl);
    float i_c = Inv_GetI_C(&s_inv_ctrl);
    
    /* ABC 相电压采样值 (从 V_Out 获取 a 相，其他待硬件定义) */
    float v_a = Inv_GetV_Out(&s_inv_ctrl);
    /* 
     * TODO: v_b 和 v_c 需要从采样器获取
     * 对于三相三线制，可以使用:
     *   v_b = -0.5 * v_a - sqrt(3)/2 * v_beta (从 αβ 反推)
     *   v_c = -0.5 * v_a + sqrt(3)/2 * v_beta
     * 或者直接从 ADC 采样器获取
     */
    float v_b = 0.0f;  /* TODO: 从采样器或计算获取 */
    float v_c = -v_a - v_b;  /* 三相平衡假设 */
    
    /* ========== 2. Clarke 变换 ========== */
    /* v_alpha = v_a */
    /* v_beta = (v_a + 2*v_b) / √3 */
    s_v_alpha = v_a;
    s_v_beta = (v_a + 2.0f * v_b) * 0.5773503f;  /* 1/√3 */
    
    /* 电流 Clarke 变换 */
    s_i_alpha = i_a;
    s_i_beta = (i_a + 2.0f * i_b) * 0.5773503f;
    
    /* ========== 3. PLL 锁相 ========== */
    float theta;
    float freq;
    bool pll_locked = Pll_Step(&s_pll, s_v_alpha, s_v_beta, &theta, &freq);
    
    /* PLL 开环模式处理 */
    if (!pll_locked) {
        /* PLL 未锁定，使用保持的角度 */
        /* TODO: 可以使用 last_valid_theta 或降级控制策略 */
    }
    
    /* ========== 4. Park 变换 (电流) ========== */
    /* i_d = i_alpha * cos(theta) + i_beta * sin(theta) */
    /* i_q = -i_alpha * sin(theta) + i_beta * cos(theta) */
    float cos_theta = cosf(theta);
    float sin_theta = sinf(theta);
    s_i_d = s_i_alpha * cos_theta + s_i_beta * sin_theta;
    s_i_q = -s_i_alpha * sin_theta + s_i_beta * cos_theta;
    
    /* ========== 5. GFL 环路执行 ========== */
    GflLoop_Output gfl_output;
    Gfl_Step(&s_gfl, s_v_alpha, s_v_beta, V_bus, s_P_ref, s_Q_ref, &gfl_output);
    
    /* 更新 GFL 输出的角度为 PLL 角度 */
    gfl_output.theta = theta;
    gfl_output.freq = freq;
    
    /* ========== 6. 电流环 PI 控制 ========== */
    float Id_ref = gfl_output.Id_ref;
    float Iq_ref = gfl_output.Iq_ref;
    float Vd_out, Vq_out;
    
    PiCtrl_Step(&s_pi_d, Id_ref, s_i_d, &Vd_out);
    PiCtrl_Step(&s_pi_q, Iq_ref, s_i_q, &Vq_out);
    
    /* ========== 7. InvPark 变换 (电压) ========== */
    /* V_alpha = Vd * cos(theta) - Vq * sin(theta) */
    /* V_beta = Vd * sin(theta) + Vq * cos(theta) */
    float V_alpha = Vd_out * cos_theta - Vq_out * sin_theta;
    float V_beta = Vd_out * sin_theta + Vq_out * cos_theta;
    
    /* ========== 8. SVPWM ========== */
    SvPwm_SetTheta(&s_svpwm, theta);
    SvPwm_Step(&s_svpwm, Vd_out, Vq_out, &s_duty_a, &s_duty_b, &s_duty_c);
    
    /* ========== 9. 检查 GFL 状态 ========== */
    Gfl_Mode mode = Gfl_GetMode(&s_gfl);
    if (mode == GFL_MODE_RUNNING) {
        /* GFL 运行中，占空比已通过 SVPWM 计算 */
    }
    
    /* ========== 10. 检查故障 ========== */
    Gfl_FaultType fault = Gfl_GetFault(&s_gfl);
    if (fault != GFL_FAULT_NONE) {
        /* GFL 故障，设置逆变器故障 */
        Inv_FaultSet(&s_inv_ctrl, (uint8_t)fault);
        
        /* 故障时清零占空比 */
        s_duty_a = 0.5f;
        s_duty_b = 0.5f;
        s_duty_c = 0.5f;
    }
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
