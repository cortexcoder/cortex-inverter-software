#pragma once

#include "gfl_types.h"

/**
 * @file gfl_ridethrough.h
 * @brief 高低压穿越 (Ride-Through) 模块
 */

/* 内部句柄类型 */
typedef struct GflRideThrough_Internal GflRideThrough_Handle;

/**
 * @brief 高低穿状态输出
 */
typedef struct {
    Gfl_RideThroughState state;   /**< 当前状态 */
    float Id_ref_ramp;           /**< 斜率限制后的电流参考 */
    float Iq_ref_grid_code;      /**< 电网规范要求的无功电流 */
    float voltage_threshold;      /**< 当前电压阈值 */
    float hold_time_remaining;    /**< 保持时间剩余 */
    bool trip_request;           /**< 跳闸请求 */
} GflRideThrough_Output;

/**
 * @brief 初始化高低穿模块
 * 
 * @param h GFL 句柄
 * @param cfg 配置指针
 */
void GflRideThrough_Init(void *h, const Gfl_Config *cfg);

/**
 * @brief 高低穿单步执行
 * 
 * 根据电网电压判断当前状态，计算相应的电流参考
 * 
 * @param h GFL 句柄
 * @param grid_voltage 电网电压 (pu)
 * @param Id_ref_input 输入 d轴电流参考 (pu)
 * @param Iq_ref_input 输入 q轴电流参考 (pu)
 * @param P_active 当前有功功率 (pu)
 * @param output 输出结构体
 */
void GflRideThrough_Step(void *h, float grid_voltage, 
                         float Id_ref_input, float Iq_ref_input,
                         float P_active,
                         GflRideThrough_Output *output);

/**
 * @brief 获取当前高低穿状态
 * 
 * @param h GFL 句柄
 * @return Gfl_RideThroughState 当前状态
 */
Gfl_RideThroughState GflRideThrough_GetState(const void *h);

/**
 * @brief 检查是否请求跳闸
 * 
 * @param h GFL 句柄
 * @return true 请求跳闸
 * @return false 不请求跳闸
 */
bool GflRideThrough_IsTripRequested(const void *h);

/**
 * @brief 重置高低穿状态
 * 
 * @param h GFL 句柄
 */
void GflRideThrough_Reset(void *h);

/**
 * @brief 获取无功电流参考 (用于电网规范要求)
 * 
 * @param h GFL 句柄
 * @param grid_voltage 电网电压 (pu)
 * @return float 要求的无功电流参考 (pu)
 */
float GflRideThrough_GetReactiveCurrentRef(const void *h, float grid_voltage);