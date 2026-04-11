#pragma once

#include "gfl_types.h"
#include "gfl_limits.h"
#include "gfl_dc_bus.h"
#include "gfl_ridethrough.h"
#include "gfl_weak_grid.h"

/**
 * @file gfl_loop.h
 * @brief GFL (Grid Following) 主环路
 */

/**
 * @brief GFL 环路输出
 */
typedef struct {
    float Id_ref;         /**< d轴电流参考 (pu) */
    float Iq_ref;         /**< q轴电流参考 (pu) */
    float theta;          /**< 锁相角度 (rad) */
    float freq;           /**< 电网频率 (Hz) */
    bool grid_ready;      /**< 电网就绪标志 */
} GflLoop_Output;

/**
 * @brief GFL 环路初始化
 * 
 * @param h GFL 句柄
 * @param cfg 配置指针
 */
void GflLoop_Init(void *h, const Gfl_Config *cfg);

/**
 * @brief GFL 环路单步执行
 * 
 * @param h GFL 句柄
 * @param v_alpha α轴电压
 * @param v_beta β轴电压
 * @param Vdc_bus 母线电压 (V)
 * @param P_ref 有功功率参考 (pu)
 * @param Q_ref 无功功率参考 (pu)
 * @param output 输出
 */
void GflLoop_Step(void *h,
                   float v_alpha, float v_beta, float Vdc_bus,
                   float P_ref, float Q_ref,
                   GflLoop_Output *output);

/**
 * @brief 获取 GFL 状态
 * 
 * @param h GFL 句柄
 * @return Gfl_Mode 当前模式
 */
Gfl_Mode GflLoop_GetMode(const void *h);

/**
 * @brief 获取故障状态
 * 
 * @param h GFL 句柄
 * @return Gfl_FaultType 当前故障
 */
Gfl_FaultType GflLoop_GetFault(const void *h);

/**
 * @brief 清除故障
 * 
 * @param h GFL 句柄
 */
void GflLoop_ClearFault(void *h);

/**
 * @brief 启动 GFL 环路
 * 
 * @param h GFL 句柄
 */
void GflLoop_Start(void *h);

/**
 * @brief 停止 GFL 环路
 * 
 * @param h GFL 句柄
 */
void GflLoop_Stop(void *h);

/**
 * @brief 获取母线状态
 * 
 * @param h GFL 句柄
 * @param state 输出状态
 */
void GflLoop_GetDcBusState(const void *h, GflDcBus_State *state);

/**
 * @brief 获取弱网状态
 * 
 * @param h GFL 句柄
 * @param output 输出
 */
void GflLoop_GetWeakGridState(const void *h, GflWeakGrid_Output *output);

/**
 * @brief 获取高低穿状态
 * 
 * @param h GFL 句柄
 * @return Gfl_RideThroughState 高低穿状态
 */
Gfl_RideThroughState GflLoop_GetRideThroughState(const void *h);