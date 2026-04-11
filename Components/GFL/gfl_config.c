#include "gfl_config.h"

/**
 * @brief 获取默认配置
 */
Gfl_Config Gfl_GetDefaultConfig(void) {
    Gfl_Config cfg = {0};
    
    /* 系统参数 */
    cfg.Ts = 1.0f / 24000.0f;               /* 24kHz 控制频率 */
    cfg.rated_power = 10000.0f;       /* 10kW */
    cfg.rated_voltage = 380.0f;       /* 380V 线电压 */
    cfg.rated_current = 30.0f;        /* 30A */
    cfg.Vdc_bus_nominal = 700.0f;     /* 700V 母线电压 */
    
    /* 电流限幅 */
    cfg.current_limits.Id_max_pos = 1.1f;   /* d轴正向最大 (pu) */
    cfg.current_limits.Id_max_neg = -0.5f;  /* d轴负向最大 (pu) */
    cfg.current_limits.Iq_max = 1.0f;      /* q轴最大 (pu) */
    cfg.current_limits.I_max = 1.2f;       /* 电流矢量最大 (pu) */
    
    /* 功率限制 */
    cfg.P_max = 1.0f;                  /* 最大有功 1.0 pu */
    cfg.Q_max = 0.5f;                  /* 最大无功 0.5 pu */
    cfg.PF_min = 0.8f;                 /* 最小功率因数 */
    
    /* 高低穿配置 */
    cfg.ridethrough.lvrt_voltage_min = 0.2f;      /* LVRT 电压下限 0.2 pu */
    cfg.ridethrough.lvrt_time_min = 0.625f;        /* LVRT 最小时间 625ms */
    cfg.ridethrough.hvrt_voltage_max = 1.3f;       /* HVRT 电压上限 1.3 pu */
    cfg.ridethrough.hvrt_time_min = 1.0f;          /* HVRT 最小时间 1s */
    cfg.ridethrough.reactive_current_k = 2.0f;     /* 无功电流系数 */
    
    /* 弱网配置 */
    cfg.weak_grid_thresh = 0.4f;      /* 弱网判定阈值 */
    cfg.Z_grid_est = 0.1f;            /* 电网阻抗估计值 */
    
    return cfg;
}

/**
 * @brief 初始化 GFL 实例
 */
void Gfl_InitInstance(Gfl_Instance *inst) {
    /* 获取默认配置 */
    inst->config = Gfl_GetDefaultConfig();
    
    /* 使用静态缓冲区初始化环路句柄 */
    GflLoop_Init(inst->loop_data, &inst->config);
}

/**
 * @brief 配置层初始化
 */
void Gfl_Config_Init(Gfl_Instance *inst) {
    Gfl_InitInstance(inst);
}

/**
 * @brief 启动 GFL
 */
void Gfl_Start(Gfl_Instance *inst) {
    GflLoop_Start(inst->loop_data);
}

/**
 * @brief 停止 GFL
 */
void Gfl_Stop(Gfl_Instance *inst) {
    GflLoop_Stop(inst->loop_data);
}

/**
 * @brief GFL 主函数
 */
void Gfl_Step(Gfl_Instance *inst,
              float v_alpha, float v_beta, float Vdc_bus,
              float P_ref, float Q_ref,
              GflLoop_Output *output) {
    GflLoop_Step(inst->loop_data, v_alpha, v_beta, Vdc_bus, P_ref, Q_ref, output);
}

/**
 * @brief 获取工作模式
 */
Gfl_Mode Gfl_GetMode(Gfl_Instance *inst) {
    return GflLoop_GetMode(inst->loop_data);
}

/**
 * @brief 获取故障状态
 */
Gfl_FaultType Gfl_GetFault(Gfl_Instance *inst) {
    return GflLoop_GetFault(inst->loop_data);
}

/**
 * @brief 清除故障
 */
void Gfl_ClearFault(Gfl_Instance *inst) {
    GflLoop_ClearFault(inst->loop_data);
}