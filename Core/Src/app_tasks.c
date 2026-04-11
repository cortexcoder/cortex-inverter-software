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
#include "gfl_split_phase.h"
#include "pll.h"
#include "park.h"
#include "clarke.h"

/*===========================================
 * 电网类型配置
 *===========================================*/
/* 选择电网类型: GRID_TYPE_ABC3 (3-leg) 或 GRID_TYPE_ABCN4 (4-leg) */
#ifndef GFL_GRID_TYPE_CFG
#define GFL_GRID_TYPE_CFG    GRID_TYPE_ABC3   /* 默认三相三线 */
#endif

/*===========================================
 * GFL 环路实例
 *===========================================*/
static Gfl_Instance s_gfl;

/* 分相功率计算结果缓存 */
static Gfl_SplitCurrentRef s_split_current;

/* PLL 实例 */
static Pll_Handle s_pll;
static const Pll_Config s_pll_cfg = {
    .omega_n = 314.159f,        /* 自然频率 50Hz*2π */
    .zeta = 0.7f,               /* 阻尼比 */
    .Ts = 1.0f / 24000.0f,      /* 24kHz 采样周期 (41.67μs) */
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

#if GFL_GRID_TYPE_CFG == GRID_TYPE_ABCN4
/* N相/零序电流 PI 控制器 (四桥臂用) */
static PiCtrl_Handle s_pi_n;
static const PiCtrl_Config s_pi_n_cfg = {
    .kp = 0.05f,             /* 零序环路增益较小 */
    .ki = 0.005f,
    .out_max = 0.5f,          /* N相电流限幅 */
    .out_min = -0.5f,
    .Ts = 1.0f / 24000.0f,
};

/* N相占空比输出 */
static float s_duty_n = 0.5f;
#endif

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
    
#if GFL_GRID_TYPE_CFG == GRID_TYPE_ABCN4
    PiCtrl_Init(&s_pi_n, &s_pi_n_cfg);
#endif
    
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
 * - Clarke 变换
 * - GFL 环路执行 (限幅、高低穿、功率分配)
 * 
 * @note PLL 已在 24kHz 快速环中执行，此处只做监督计算
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
    (void)v_b;  /* 预留 */
    
    /* ========== 2. Clarke 变换 ========== */
    s_v_alpha = v_a;
    s_v_beta = (v_a + 2.0f * v_b) * 0.5773503f;  /* 1/√3 */
    
    s_i_alpha = i_a;
    s_i_beta = (i_a + 2.0f * i_b) * 0.5773503f;
    
    /* ========== 3. NAN/INF 保护 ========== */
    s_i_alpha = clampf(s_i_alpha, -10.0f, 10.0f);
    s_i_beta = clampf(s_i_beta, -10.0f, 10.0f);
    s_v_alpha = clampf(s_v_alpha, -500.0f, 500.0f);
    s_v_beta = clampf(s_v_beta, -500.0f, 500.0f);
    
    /* ========== 4. GFL 环路执行 ========== */
    /* @note 功率分配、限幅、高低穿等监督功能在 1ms 任务中执行 */
    GflLoop_Output gfl_output;
    Gfl_Step(&s_gfl, s_v_alpha, s_v_beta, V_bus, s_P_ref, s_Q_ref, &gfl_output);
    
    /* NAN/INF 保护并限幅 */
    s_Id_ref = safe_div(gfl_output.Id_ref, 1.0f, 0.0f);
    s_Iq_ref = safe_div(gfl_output.Iq_ref, 1.0f, 0.0f);
    s_Id_ref = clampf(s_Id_ref, -2.0f, 2.0f);
    s_Iq_ref = clampf(s_Iq_ref, -2.0f, 2.0f);
    
    /* ========== 5. 检查故障 ========== */
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

/*===========================================
 * 快速电流环 - 根据电网类型选择实现
 *===========================================*/

/**
 * @brief 三相三线快速电流环 (3-leg)
 */
static void GFL_FastControl_3Leg_24kHz(void) {
    /* ========== 1. PLL 锁相 (在 24kHz 执行) ========== */
    bool pll_locked = Pll_Step(&s_pll, s_v_alpha, s_v_beta, &s_theta, &s_freq);
    (void)pll_locked;
    
    float cos_theta = cosf(s_theta);
    float sin_theta = sinf(s_theta);
    
    /* ========== 2. Park 变换 (电流反馈) ========== */
    float i_d = s_i_alpha * cos_theta + s_i_beta * sin_theta;
    float i_q = -s_i_alpha * sin_theta + s_i_beta * cos_theta;
    
    /* ========== 3. PI 控制 ========== */
    float Vd_out, Vq_out;
    PiCtrl_Step(&s_pi_d, s_Id_ref, i_d, &Vd_out);
    PiCtrl_Step(&s_pi_q, s_Iq_ref, i_q, &Vq_out);
    
    /* ========== 4. SVPWM ========== */
    SvPwm_SetTheta(&s_svpwm, s_theta);
    SvPwm_Step(&s_svpwm, Vd_out, Vq_out, &s_duty_a, &s_duty_b, &s_duty_c);
    
    /* ========== 5. 占空比限幅 ========== */
    s_duty_a = clampf(s_duty_a, 0.05f, 0.95f);
    s_duty_b = clampf(s_duty_b, 0.05f, 0.95f);
    s_duty_c = clampf(s_duty_c, 0.05f, 0.95f);
}

#if GFL_GRID_TYPE_CFG == GRID_TYPE_ABCN4
/**
 * @brief 三相四线快速电流环 (4-leg)
 * 
 * 支持 N 相独立控制，用于:
 * - 零序电流补偿
 * - 不平衡负载
 * - 谐波抑制
 */
static void GFL_FastControl_4Leg_24kHz(void) {
    /* ========== 1. PLL 锁相 (在 24kHz 执行) ========== */
    bool pll_locked = Pll_Step(&s_pll, s_v_alpha, s_v_beta, &s_theta, &s_freq);
    (void)pll_locked;
    
    float cos_theta = cosf(s_theta);
    float sin_theta = sinf(s_theta);
    
    /* ========== 2. Clarke 变换 -> Alpha-Beta ========== */
    float i_alpha = s_i_alpha;
    float i_beta = s_i_beta;
    
    /* ========== 3. 计算零序电流 (i_0 = (i_a + i_b + i_c) / 3) ========== */
    /* 假设三相电流采样已获取，此处简化处理 */
    float i_zero = 0.0f;  /* 实际应从 s_i_alpha/i_beta 推导 */
    
    /* ========== 4. Park 变换 (d/q 轴) ========== */
    float i_d = i_alpha * cos_theta + i_beta * sin_theta;
    float i_q = -i_alpha * sin_theta + i_beta * cos_theta;
    
    /* ========== 5. PI 控制 (d/q 轴) ========== */
    float Vd_out, Vq_out;
    PiCtrl_Step(&s_pi_d, s_Id_ref, i_d, &Vd_out);
    PiCtrl_Step(&s_pi_q, s_Iq_ref, i_q, &Vq_out);
    
    /* ========== 6. N相/零序 PI 控制 ========== */
    float Vn_out = 0.0f;  /* N相电压输出 */
    float i_n_fdb = 0.0f; /* N相电流反馈 (零序电流) */
    float Id_n_ref = s_split_current.Id[3];  /* N相 Id 参考 */
    float Iq_n_ref = s_split_current.Iq[3];  /* N相 Iq 参考 */
    
    if (GFL_CURRENT_IS_ASYMMETRIC(&s_gfl)) {
        PiCtrl_Step(&s_pi_n, Id_n_ref, i_n_fdb, &Vn_out);
    }
    
    /* ========== 7. 调制波生成 ========== */
    /* d/q 轴电压转为 α/β */
    float v_alpha_ref = Vd_out * cos_theta - Vq_out * sin_theta;
    float v_beta_ref = Vd_out * sin_theta + Vq_out * cos_theta;
    
    /* N 相调制波 */
    float mod_n = Vn_out + 0.5f;  /* 零序分量叠加到 N 相 */
    
    /* ========== 8. SVPWM (ABC) + N相占空比 ========== */
    SvPwm_SetTheta(&s_svpwm, s_theta);
    SvPwm_Step(&s_svpwm, Vd_out, Vq_out, &s_duty_a, &s_duty_b, &s_duty_c);
    
    /* N相占空比计算 (简化) */
    s_duty_n = clampf(mod_n, 0.05f, 0.95f);
    
    /* ========== 9. 占空比限幅 ========== */
    s_duty_a = clampf(s_duty_a, 0.05f, 0.95f);
    s_duty_b = clampf(s_duty_b, 0.05f, 0.95f);
    s_duty_c = clampf(s_duty_c, 0.05f, 0.95f);
}
#endif /* GRID_TYPE_ABCN4 */

/**
 * @brief 快速电流环控制 (在 PWM 中断或 24kHz 定时器中调用)
 * 
 * 根据 GFL_GRID_TYPE_CFG 宏选择 3-leg 或 4-leg 实现
 */
void GFL_FastControl_24kHz(void) {
#if GFL_GRID_TYPE_CFG == GRID_TYPE_ABCN4
    GFL_FastControl_4Leg_24kHz();
#else
    GFL_FastControl_3Leg_24kHz();
#endif
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
