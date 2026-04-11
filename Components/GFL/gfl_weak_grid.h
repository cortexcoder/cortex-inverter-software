#pragma once

#include "gfl_types.h"

/**
 * @file gfl_weak_grid.h
 * @brief 弱网检测与控制模块
 */

/* 内部句柄类型 */
typedef struct GflWeakGrid_Internal GflWeakGrid_Handle;

/**
 * @brief 弱网状态
 */
typedef enum {
    GFL_WEAK_GRID_STRONG = 0,    /**< 强电网 */
    GFL_WEAK_GRID_MARGINAL,       /**< 边缘电网 */
    GFL_WEAK_GRID_WEAK,           /**< 弱电网 */
} Gfl_WeakGridLevel;

/**
 * @brief 弱网状态输出
 */
typedef struct {
    Gfl_WeakGridLevel level;      /**< 弱网等级 */
    float Z_grid_estimated;       /**< 估计的电网阻抗 (pu) */
    float angle_diff;             /**< 电压电流相位差 (rad) */
    float freq_offset;             /**< 频率偏差 (Hz) */
    bool grid_connected;          /**< 电网连接状态 */
} GflWeakGrid_Output;

/**
 * @brief 初始化弱网检测模块
 * 
 * @param h GFL 句柄
 * @param cfg 配置指针
 */
void GflWeakGrid_Init(void *h, const Gfl_Config *cfg);

/**
 * @brief 弱网检测单步执行
 * 
 * @param h GFL 句柄
 * @param grid_voltage 电网电压 (pu)
 * @param grid_freq 电网频率 (Hz)
 * @param I_out 输出电流 (pu)
 * @param P_active 有功功率 (pu)
 * @param Q_reactive 无功功率 (pu)
 * @param pll_locked PLL 锁定标志
 * @param output 输出结构体
 */
void GflWeakGrid_Step(void *h, float grid_voltage, float grid_freq,
                      float I_out, float P_active, float Q_reactive,
                      bool pll_locked,
                      GflWeakGrid_Output *output);

/**
 * @brief 获取当前弱网等级
 * 
 * @param h GFL 句柄
 * @return Gfl_WeakGridLevel 弱网等级
 */
Gfl_WeakGridLevel GflWeakGrid_GetLevel(const void *h);

/**
 * @brief 获取估计的电网阻抗
 * 
 * @param h GFL 句柄
 * @return float 电网阻抗 (pu)
 */
float GflWeakGrid_GetImpedance(const void *h);

/**
 * @brief 检查电网是否适合并网
 * 
 * @param h GFL 句柄
 * @return true 可以并网
 * @return false 不可以并网
 */
bool GflWeakGrid_IsGridReady(const void *h);

/**
 * @brief 重置弱网检测状态
 * 
 * @param h GFL 句柄
 */
void GflWeakGrid_Reset(void *h);