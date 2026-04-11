#pragma once

#include "gfl_types.h"

/**
 * @file gfl_limits.h
 * @brief 电流限幅计算模块
 */

/* 内部句柄类型 */
typedef struct GflLimits_Internal GflLimits_Handle;

/**
 * @brief 初始化电流限幅器
 * 
 * @param h GFL 句柄
 * @param cfg 配置指针
 */
void GflLimits_Init(void *h, const Gfl_Config *cfg);

/**
 * @brief 更新电流限幅
 * 
 * 根据电网电压、功率等因素动态调整限幅值
 * 
 * @param h GFL 句柄
 * @param grid_voltage 电网电压 (pu)
 * @param Vdc_bus 母线电压 (V)
 * @param P 有功功率 (pu)
 * @param Q 无功功率 (pu)
 */
void GflLimits_Update(void *h, float grid_voltage, float Vdc_bus, float P, float Q);

/**
 * @brief 应用电流限幅
 * 
 * 对电流指令进行限幅，保证电流矢量不超过最大幅值
 * 同时保证 d轴和 q轴电流都在各自范围内
 * 
 * @param h GFL 句柄
 * @param Id_ref 输入 d轴电流参考
 * @param Iq_ref 输入 q轴电流参考
 * @param Id_out 输出限幅后的 d轴电流参考
 * @param Iq_out 输出限幅后的 q轴电流参考
 */
void GflLimits_Apply(void *h, float Id_ref, float Iq_ref, 
                      float *Id_out, float *Iq_out);

/**
 * @brief 计算瞬时功率限制
 * 
 * 根据电流限幅和电网电压计算最大可输出功率
 * 
 * @param h GFL 句柄
 * @param grid_voltage 电网电压 (pu)
 * @param Vdc_bus 母线电压 (V)
 * @param P_max_out 输出最大有功功率 (pu)
 * @param Q_max_out 输出最大无功功率 (pu)
 */
void GflLimits_CalcPowerLimits(void *h, float grid_voltage, float Vdc_bus,
                               float *P_max_out, float *Q_max_out);

/**
 * @brief 获取当前电流限幅值
 * 
 * @param h GFL 句柄
 * @param limits 输出限幅结构体
 */
void GflLimits_Get(const void *h, Gfl_CurrentLimits *limits);