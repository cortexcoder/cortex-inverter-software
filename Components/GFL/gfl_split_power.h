#pragma once

#include <math.h>
#include "gfl_types.h"
#include "gfl_split_phase.h"

/**
 * @file gfl_split_power.h
 * @brief 分相功率指令和竞争仲裁模块
 */

/*===========================================
 * 功率请求结构体
 *===========================================*/

/**
 * @brief 功率请求 (用于功率指令竞争仲裁)
 */
typedef struct {
    Gfl_Priority priority;     /**< 优先级 */
    float P_req;               /**< 有功功率请求 (pu) */
    float Q_req;               /**< 无功功率请求 (pu) */
    uint8_t phase_mask;        /**< 作用相掩码: bit0=A, bit1=B, bit2=C, bit3=N */
    bool valid;                /**< 请求是否有效 */
    uint32_t timestamp;        /**< 请求时间戳 */
    float weight;              /**< 同优先级权重 (0-1) */
} Gfl_PowerRequest;

/**
 * @brief 分相功率指令
 */
typedef struct {
    float P[6];       /**< 各相有功功率 (pu), 索引: 0=A, 1=B, 2=C, 3=N */
    float Q[6];       /**< 各相无功功率 (pu) */
    uint8_t num_phases;
} Gfl_SplitPowerRef;

/**
 * @brief 功率反馈
 */
typedef struct {
    float P_fdb[6];   /**< 各相有功功率反馈 (pu) */
    float Q_fdb[6];   /**< 各相无功功率反馈 (pu) */
    float P_total;    /**< 总有功功率 (pu) */
    float Q_total;    /**< 总无功功率 (pu) */
} Gfl_PowerFeedback;

/**
 * @brief 功率分配模式
 */
typedef enum {
    POWER_DIST_SYMMETRIC = 0,   /**< 对称分配: 各相功率相等 */
    POWER_DIST_PROPORTIONAL,    /**< 比例分配: 按电压比例分配 */
    POWER_DIST_ASYMMETRIC,      /**< 不对称分配: 各相独立控制 */
} Gfl_PowerDistribution;

/**
 * @brief 功率 PI 控制配置
 */
typedef struct {
    float kp_P;         /**< 有功功率 P 参数 */
    float ki_P;         /**< 有功功率 I 参数 */
    float kp_Q;         /**< 无功功率 P 参数 */
    float ki_Q;         /**< 无功功率 I 参数 */
    float out_max_P;    /**< 有功输出上限 */
    float out_min_P;    /**< 有功输出下限 */
    float out_max_Q;    /**< 无功输出上限 */
    float out_min_Q;    /**< 无功输出下限 */
    float Ts;           /**< 采样周期 */
} Gfl_SplitPowerPI_Config;

/**
 * @brief 功率 PI 控制句柄
 */
typedef struct {
    Gfl_SplitPowerPI_Config config;
    
    /* 各相 PI 状态 */
    float integrator_P[6];     /**< 各相有功积分器 */
    float integrator_Q[6];     /**< 各相无功积分器 */
    float out_prev_P[6];       /**< 各相有功上一时刻输出 */
    float out_prev_Q[6];       /**< 各相无功上一时刻输出 */
    
    /* 功率参考 */
    Gfl_SplitPowerRef power_ref;
    
    /* 功率反馈 */
    Gfl_PowerFeedback power_fdb;
    
    /* 限幅状态 */
    bool windup_P[6];
    bool windup_Q[6];
} Gfl_SplitPowerPI_Handle;

/*===========================================
 * 函数声明
 *===========================================*/

/**
 * @brief 功率指令竞争仲裁
 * 
 * 多源功率请求的优先级仲裁，高优先级覆盖低优先级
 * 同优先级按权重加权平均
 * 
 * @param requests 请求数组
 * @param num_requests 请求数量
 * @param dist_mode 分配模式
 * @param output 仲裁后的分相功率指令
 */
void Gfl_PowerArbitrate(const Gfl_PowerRequest *requests,
                        uint8_t num_requests,
                        Gfl_PowerDistribution dist_mode,
                        Gfl_SplitPowerRef *output);

/**
 * @brief 分相功率 PI 控制器初始化
 */
void Gfl_SplitPowerPI_Init(Gfl_SplitPowerPI_Handle *h, 
                           const Gfl_SplitPowerPI_Config *cfg);

/**
 * @brief 分相功率 PI 控制单步
 * 
 * @param h 句柄
 * @param power_ref 功率参考
 * @param power_fdb 功率反馈
 * @param output 电流指令输出
 */
void Gfl_SplitPowerPI_Step(Gfl_SplitPowerPI_Handle *h,
                           const Gfl_SplitPowerRef *power_ref,
                           const Gfl_PowerFeedback *power_fdb,
                           Gfl_SplitCurrentRef *output);

/**
 * @brief 计算分相功率反馈
 * 
 * 基于电压电流采样计算各相瞬时功率
 * 
 * @param v_a A相电压 (pu)
 * @param v_b B相电压 (pu)
 * @param v_c C相电压 (pu)
 * @param i_a A相电流 (pu)
 * @param i_b B相电流 (pu)
 * @param i_c C相电流 (pu)
 * @param pf 功率因数 (用于无功估算)
 * @param output 功率反馈输出
 */
void Gfl_CalcPowerFeedback(float v_a, float v_b, float v_c,
                           float i_a, float i_b, float i_c,
                           float pf,
                           Gfl_PowerFeedback *output);

/**
 * @brief 重置功率 PI 控制器
 */
void Gfl_SplitPowerPI_Reset(Gfl_SplitPowerPI_Handle *h);

/*===========================================
 * 辅助宏
 *===========================================*/

/**
 * @brief 检查相掩码中是否包含某相
 */
#define PHASE_MASK_HAS(mask, phase)    (((mask) >> (phase)) & 0x01)

/**
 * @brief 获取相数对应的掩码
 */
#define PHASE_TO_MASK(phase)          (1U << (phase))

/**
 * @brief 所有三相掩码 (ABC)
 */
#define PHASE_MASK_ABC                (PHASE_TO_MASK(0) | PHASE_TO_MASK(1) | PHASE_TO_MASK(2))

/**
 * @brief 所有四相掩码 (ABCN)
 */
#define PHASE_MASK_ABCN               (PHASE_MASK_ABC | PHASE_TO_MASK(3))