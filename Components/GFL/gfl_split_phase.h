#pragma once

#include <math.h>
#include "gfl_types.h"

/**
 * @file gfl_split_phase.h
 * @brief 分相功率控制和电流仲裁模块
 */

/**
 * @brief 分相功率控制初始化
 * 
 * @param h 句柄
 * @param cfg 配置
 */
void GflSplitPhase_Init(void *h, const Gfl_Config *cfg);

/**
 * @brief 分相功率转电流计算
 * 
 * 统一底层函数，根据配置层参数区分对称/不对称模式
 * 
 * @param h 句柄
 * @param power 相功率数组
 * @param mode 电流模式
 * @param output 分相电流输出
 */
void GflSplitPhase_CalcCurrent(void *h,
                               const Gfl_PhasePower *power,
                               Gfl_CurrentMode mode,
                               Gfl_SplitCurrentRef *output);

/**
 * @brief 电流指令竞争仲裁
 * 
 * 多请求优先级仲裁，高优先级覆盖低优先级
 * 同优先级按权重加权平均
 * 
 * @param requests 请求数组
 * @param num_requests 请求数量
 * @param limits 电流限幅
 * @param output 仲裁结果
 */
void Gfl_CurrentArbitrate(const Gfl_CurrentRequest *requests,
                          uint8_t num_requests,
                          const Gfl_CurrentLimits *limits,
                          Gfl_SplitCurrentRef *output);

/**
 * @brief 两阶段电流限幅
 * 
 * 阶段1: 圆形限幅 (总电流矢量 ≤ I_max)
 * 阶段2: 矩形限幅 (每相 Id/Iq 在范围内)
 * 
 * @param input 输入电流
 * @param limits 限幅参数
 * @param mode 电流模式
 * @param output 限幅后电流
 */
void Gfl_CurrentLimit(const Gfl_SplitCurrentRef *input,
                      const Gfl_CurrentLimits *limits,
                      Gfl_CurrentMode mode,
                      Gfl_SplitCurrentRef *output);

/**
 * @brief 单相电流限幅
 * 
 * @param Id d轴电流
 * @param Iq q轴电流
 * @param limits 限幅参数
 * @return true 如果电流在限幅内
 */
bool Gfl_LimitPhaseCurrent(float *Id, float *Iq, const Gfl_CurrentLimits *limits);

/**
 * @brief 计算电流矢量幅值
 * 
 * @param Id d轴电流
 * @param Iq q轴电流
 * @return float 电流幅值
 */
static inline float Gfl_CalcCurrentMag(float Id, float Iq) {
    return sqrtf(Id * Id + Iq * Iq);
}

/**
 * @brief 浮点数有效性检查 (防 NAN/INF)
 */
static inline bool Gfl_IsValidFloat(float x) {
    uint32_t u = *(uint32_t *)&x;
    return (u & 0x7F800000) != 0x7F800000;
}