#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 调制策略枚举
 */
typedef enum {
    MODULATION_SVPWM = 0,    /**< 空间矢量脉宽调制 */
    MODULATION_DPWM0,        /**< 不连续脉宽调制0 */
    MODULATION_DPWM1,        /**< 不连续脉宽调制1 */
    MODULATION_DPWM2,        /**< 不连续脉宽调制2 */
    MODULATION_DPWM3,        /**< 不连续脉宽调制3 */
    MODULATION_THIPWM,       /**< 三次谐波注入脉宽调制 */
    MODULATION_ADAPTIVE,     /**< 自适应调制（根据负载自动切换） */
    
    MODULATION_COUNT         /**< 调制策略数量 */
} ModulationStrategy;

/**
 * @brief 共模电压注入配置结构体
 */
typedef struct {
    float Vdc;              /**< 直流母线电压 (V) */
    float Ts;               /**< 采样周期 (s) */
    float fsw;              /**< 开关频率 (Hz) */
    
    /* THIPWM参数 */
    float thipwm_factor;    /**< 三次谐波注入系数，典型值0.2-0.3 */
    
    /* 自适应调制参数 */
    float adaptive_threshold;  /**< 切换阈值（占空比或电流） */
    float hysteresis;          /**< 迟滞带宽 */
    
    /* DPWM参数 */
    float dpwm_offset[4];   /**< DPWM偏移角度 (rad) */
    
    /* 限制参数 */
    float duty_min;         /**< 最小占空比 */
    float duty_max;         /**< 最大占空比 */
    
    /* 初始调制策略 */
    ModulationStrategy init_strategy;  /**< 初始调制策略 */
} Cmv_Config;

/**
 * @brief 共模电压注入句柄结构体
 */
typedef struct {
    const Cmv_Config *config;  /**< 配置指针 */
    
    /* 当前状态 */
    ModulationStrategy current_strategy;  /**< 当前调制策略 */
    float common_mode_voltage;            /**< 计算的共模电压 */
    
    /* 自适应调制状态 */
    float load_estimate;        /**< 负载估计 */
    bool strategy_changed;      /**< 策略是否刚刚改变 */
    int strategy_change_counter;/**< 策略改变计数器 */
    
    /* 历史值 */
    float duty_a_prev;          /**< 上一拍A相占空比 */
    float duty_b_prev;          /**< 上一拍B相占空比 */
    float duty_c_prev;          /**< 上一拍C相占空比 */
    
    /* DPWM扇区信息 */
    int dpwm_sector;            /**< 当前DPWM扇区 (0-5) */
    float dpwm_angle;           /**< 当前角度 (rad) */
    
    /* 性能统计 */
    float switching_loss_estimate;   /**< 开关损耗估计 */
    float conduction_loss_estimate;  /**< 导通损耗估计 */
} Cmv_Handle;

/**
 * @brief 初始化共模电压注入模块
 * 
 * @param h 句柄指针
 * @param cfg 配置指针
 */
void Cmv_Init(Cmv_Handle *h, const Cmv_Config *cfg);

/**
 * @brief 计算并注入共模电压
 * 
 * @param h 句柄指针
 * @param duty_a A相原始占空比 [0, 1]
 * @param duty_b B相原始占空比 [0, 1]
 * @param duty_c C相原始占空比 [0, 1]
 * @param angle 当前电角度 (rad)
 * @param modulation 调制策略（如果为MODULATION_COUNT则使用当前策略）
 * @param duty_a_out A相输出占空比（包含共模注入）
 * @param duty_b_out B相输出占空比（包含共模注入）
 * @param duty_c_out C相输出占空比（包含共模注入）
 * @return float 注入的共模电压值
 */
float Cmv_Inject(Cmv_Handle *h, float duty_a, float duty_b, float duty_c,
                 float angle, ModulationStrategy modulation,
                 float *duty_a_out, float *duty_b_out, float *duty_c_out);

/**
 * @brief 切换到指定调制策略
 * 
 * @param h 句柄指针
 * @param strategy 新调制策略
 * @param smooth 是否平滑切换（避免突变）
 */
void Cmv_SwitchStrategy(Cmv_Handle *h, ModulationStrategy strategy, bool smooth);

/**
 * @brief 获取当前调制策略
 * 
 * @param h 句柄指针
 * @return ModulationStrategy 当前调制策略
 */
ModulationStrategy Cmv_GetCurrentStrategy(const Cmv_Handle *h);

/**
 * @brief 更新自适应调制参数
 * 
 * @param h 句柄指针
 * @param load_measurement 负载测量值（电流或功率）
 * @param temperature 温度（可选）
 */
void Cmv_UpdateAdaptive(Cmv_Handle *h, float load_measurement, float temperature);

/**
 * @brief 重置模块状态
 * 
 * @param h 句柄指针
 */
void Cmv_Reset(Cmv_Handle *h);

/**
 * @brief 获取性能统计信息
 * 
 * @param h 句柄指针
 * @param switching_loss 开关损耗估计（输出）
 * @param conduction_loss 导通损耗估计（输出）
 */
void Cmv_GetPerformanceStats(const Cmv_Handle *h, 
                            float *switching_loss, float *conduction_loss);

/**
 * @brief 设置DPWM偏移角度
 * 
 * @param h 句柄指针
 * @param dpwm_index DPWM索引 (0-3)
 * @param offset_angle 偏移角度 (rad)
 */
void Cmv_SetDpwmOffset(Cmv_Handle *h, int dpwm_index, float offset_angle);

/**
 * @brief 获取调制策略名称
 * 
 * @param strategy 调制策略
 * @return const char* 策略名称字符串
 */
const char* Cmv_GetStrategyName(ModulationStrategy strategy);