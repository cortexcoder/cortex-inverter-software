#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @file gfl_types.h
 * @brief GFL (Grid Following) 环路类型定义
 */

/*===========================================
 * 枚举类型
 *===========================================*/

/**
 * @brief GFL 工作模式
 */
typedef enum {
    GFL_MODE_IDLE = 0,      /**< 空闲/待机模式 */
    GFL_MODE_STARTING,      /**< 启动中 */
    GFL_MODE_RUNNING,       /**< 正常运行 */
    GFL_MODE_STOPPING,      /**< 停机中 */
    GFL_MODE_FAULT,         /**< 故障模式 */
} Gfl_Mode;

/**
 * @brief 故障类型
 */
typedef enum {
    GFL_FAULT_NONE = 0,
    GFL_FAULT_OVERCURRENT,      /**< 过流 */
    GFL_FAULT_OVERVOLTAGE,      /**< 过压 */
    GFL_FAULT_UNDERVOLTAGE,     /**< 欠压 */
    GFL_FAULT_OVERFREQUENCY,    /**< 过频 */
    GFL_FAULT_UNDERFREQUENCY,   /**< 欠频 */
    GFL_FAULT_DC_BUS_OVER,      /**< 母线过压 */
    GFL_FAULT_DC_BUS_UNDER,     /**< 母线欠压 */
    GFL_FAULT_PLLA,             /**< 锁相环失锁 */
    GFL_FAULT_TEMPERATURE,      /**< 温度过高 */
    GFL_FAULT_GRID_UNBALANCE,   /**< 电网不平衡 */
} Gfl_FaultType;

/**
 * @brief 高低穿状态
 */
typedef enum {
    GFL_RT_NORMAL = 0,       /**< 正常状态 */
    GFL_RT_LVRT,             /**< 低压穿越 (Low Voltage Ride-Through) */
    GFL_RT_HVRT,             /**< 高压穿越 (High Voltage Ride-Through) */
} Gfl_RideThroughState;

/**
 * @brief 功率模式
 */
typedef enum {
    GFL_POWER_MODE_PQ = 0,   /**< PQ 控制模式 (有功/无功) */
    GFL_POWER_MODE_PF,        /**< PF 控制模式 (功率因数) */
    GFL_POWER_MODE_QV,        /**< Q-V 控制模式 (无功-电压) */
} Gfl_PowerMode;

/**
 * @brief 指令来源优先级
 */
typedef enum {
    GFL_PRIORITY_DYNAMIC = 0,    /**< 动态限制 (最低) */
    GFL_PRIORITY_POWER,          /**< 功率指令 */
    GFL_PRIORITY_GRID_CODE,      /**< 电网规范要求 */
    GFL_PRIORITY_RIDETHROUGH,    /**< 高低穿 */
    GFL_PRIORITY_DC_BUS,         /**< 母线管理 */
    GFL_PRIORITY_FAULT,          /**< 故障处理 (最高) */
} Gfl_Priority;

/*===========================================
 * 结构体类型
 *===========================================*/

/**
 * @brief 电网状态 (来自 PLL)
 */
typedef struct {
    float theta;          /**< 相位 (rad) */
    float freq;           /**< 频率 (Hz) */
    float amplitude;      /**< 电压幅值 (pu) */
    float v_alpha;        /**< α轴电压 */
    float v_beta;         /**< β轴电压 */
    bool locked;          /**< PLL 锁定标志 */
} Gfl_GridState;

/**
 * @brief 电流指令
 */
typedef struct {
    float Id_ref;         /**< d轴电流参考 (pu) */
    float Iq_ref;         /**< q轴电流参考 (pu) */
    float Id_fdb;         /**< d轴电流反馈 (pu) */
    float Iq_fdb;         /**< q轴电流反馈 (pu) */
} Gfl_CurrentRef;

/**
 * @brief 功率指令
 */
typedef struct {
    float P_ref;          /**< 有功功率参考 (pu) */
    float Q_ref;          /**< 无功功率参考 (pu) */
    float P_fdb;          /**< 有功功率反馈 (pu) */
    float Q_fdb;          /**< 无功功率反馈 (pu) */
} Gfl_PowerRef;

/**
 * @brief 电流限幅器
 */
typedef struct {
    float Id_max_pos;     /**< d轴电流正向最大值 */
    float Id_max_neg;     /**< d轴电流负向最大值 */
    float Iq_max;         /**< q轴电流最大值 */
    float I_max;          /**< 电流矢量最大幅值 */
} Gfl_CurrentLimits;

/**
 * @brief 电压状态
 */
typedef struct {
    float Vdc_bus;        /**< 母线电压 (V) */
    float Vdc_bus_ref;    /**< 母线电压参考 (V) */
    float grid_voltage;   /**< 电网电压 (pu) */
} Gfl_VoltageState;

/**
 * @brief 高低穿配置
 */
typedef struct {
    float lvrt_voltage_min;       /**< LVRT 电压下限 (pu) */
    float lvrt_time_min;          /**< LVRT 最小持续时间 (s) */
    float hvrt_voltage_max;       /**< HVRT 电压上限 (pu) */
    float hvrt_time_min;          /**< HVRT 最小持续时间 (s) */
    float reactive_current_k;     /**< 无功电流系数 */
} Gfl_RideThroughConfig;

/**
 * @brief 弱网状态
 */
typedef struct {
    float Z_grid;                 /**< 电网等效阻抗 (pu) */
    float angle_diff;             /**< 相位差 (rad) */
    float freq_offset;            /**< 频率偏差 (Hz) */
} Gfl_WeakGridState;

/*===========================================
 * 回调函数类型
 *===========================================*/

/**
 * @brief 获取电网状态回调
 */
typedef void (*Gfl_GetGridStateCb)(void *context, Gfl_GridState *grid);

/**
 * @brief 获取母线电压回调
 */
typedef float (*Gfl_GetDcBusCb)(void *context);

/**
 * @brief 获取功率反馈回调
 */
typedef void (*Gfl_GetPowerCb)(void *context, float *P, float *Q);

/**
 * @brief 获取电流反馈回调
 */
typedef void (*Gfl_GetCurrentCb)(void *context, float *Id, float *Iq);

/**
 * @brief 故障回调
 */
typedef void (*Gfl_FaultCb)(void *context, Gfl_FaultType fault, bool set);

/*===========================================
 * 配置结构体
 *===========================================*/

/**
 * @brief GFL 模块配置
 */
typedef struct {
    /* 系统参数 */
    float Ts;                    /**< 采样周期 (s) */
    float rated_power;            /**< 额定功率 (W) */
    float rated_voltage;          /**< 额定电压 (V) */
    float rated_current;          /**< 额定电流 (A) */
    float Vdc_bus_nominal;        /**< 母线电压标称值 (V) */
    
    /* 电流限幅 */
    Gfl_CurrentLimits current_limits;
    
    /* 功率限制 */
    float P_max;                  /**< 最大有功功率 (pu) */
    float Q_max;                  /**< 最大无功功率 (pu) */
    float PF_min;                 /**< 最小功率因数 */
    
    /* 高低穿配置 */
    Gfl_RideThroughConfig ridethrough;
    
    /* 弱网配置 */
    float weak_grid_thresh;       /**< 弱网判定阈值 (pu) */
    float Z_grid_est;             /**< 电网阻抗估计值 (pu) */
    
    /* 回调函数 */
    void *cb_context;             /**< 回调上下文 */
    Gfl_GetGridStateCb get_grid_state;
    Gfl_GetDcBusCb get_dc_bus;
    Gfl_GetPowerCb get_power;
    Gfl_GetCurrentCb get_current;
    Gfl_FaultCb fault_callback;
} Gfl_Config;

/*===========================================
 * 状态结构体
 *===========================================*/

/**
 * @brief GFL 模块状态
 */
typedef struct {
    Gfl_Mode mode;                /**< 工作模式 */
    Gfl_FaultType fault;          /**< 当前故障 */
    Gfl_RideThroughState rt_state; /**< 高低穿状态 */
    
    /* 内部状态 */
    float theta_pos;              /**< 正序角度 */
    float theta_neg;              /**< 负序角度 */
    
    /* 电流指令 */
    Gfl_CurrentRef current_ref;
    
    /* 功率指令 */
    Gfl_PowerRef power_ref;
    
    /* 限幅后的电流指令 */
    Gfl_CurrentRef current_ref_limited;
    
    /* 母线状态 */
    Gfl_VoltageState voltage;
    
    /* 弱网状态 */
    Gfl_WeakGridState weak_grid;
    
    /* 时间戳 */
    float rt_timer;               /**< 高低穿计时 */
    float fault_timer;            /**< 故障计时 */
    
    /* 初始化标志 */
    bool init;
} Gfl_Handle;