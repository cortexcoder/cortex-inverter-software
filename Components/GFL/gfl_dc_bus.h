#pragma once

#include "gfl_types.h"

/**
 * @file gfl_dc_bus.h
 * @brief 母线电压管理模块
 */

/* 内部句柄类型 */
typedef struct GflDcBus_Internal GflDcBus_Handle;

/**
 * @brief 母线管理状态
 */
typedef struct {
    float Vdc_bus;           /**< 当前母线电压 (V) */
    float Vdc_ref;           /**< 母线电压参考 (V) */
    float Vdc_error;         /**< 母线电压误差 (V) */
    float I_dc_charge;       /**< 充电电流 (A) */
    bool precharge_done;      /**< 预充电完成标志 */
    bool overvoltage_fault;   /**< 过压故障标志 */
    bool undervoltage_fault;  /**< 欠压故障标志 */
} GflDcBus_State;

/**
 * @brief 初始化母线管理器
 * 
 * @param h GFL 句柄
 * @param cfg 配置指针
 */
void GflDcBus_Init(void *h, const Gfl_Config *cfg);

/**
 * @brief 母线电压管理单步执行
 * 
 * @param h GFL 句柄
 * @param Vdc_bus 实际母线电压 (V)
 * @param I_dc_bus 母线电流 (A)
 * @param grid_power 电网侧功率 (pu，负值表示逆变)
 * @param P_injected 注入功率 (pu)
 */
void GflDcBus_Step(void *h, float Vdc_bus, float I_dc_bus, 
                   float grid_power, float P_injected);

/**
 * @brief 获取母线电压参考
 * 
 * @param h GFL 句柄
 * @return float 母线电压参考 (V)
 */
float GflDcBus_GetRef(const void *h);

/**
 * @brief 获取母线状态
 * 
 * @param h GFL 句柄
 * @param state 输出状态
 */
void GflDcBus_GetState(const void *h, GflDcBus_State *state);

/**
 * @brief 检查母线是否有故障
 * 
 * @param h GFL 句柄
 * @return Gfl_FaultType 故障类型，无故障返回 GFL_FAULT_NONE
 */
Gfl_FaultType GflDcBus_CheckFault(const void *h);

/**
 * @brief 设置母线电压参考
 * 
 * @param h GFL 句柄
 * @param Vdc_ref 母线电压参考 (V)
 */
void GflDcBus_SetRef(void *h, float Vdc_ref);

/**
 * @brief 启动预充电
 * 
 * @param h GFL 句柄
 */
void GflDcBus_StartPrecharge(void *h);

/**
 * @brief 检查预充电是否完成
 * 
 * @param h GFL 句柄
 * @return true 预充电完成
 * @return false 预充电未完成
 */
bool GflDcBus_IsPrechargeDone(const void *h);