#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief PLL模块配置结构体
 * 
 * 锁相环配置参数，采用二阶广义积分器(SOGI)结构，
 * 支持零电压穿越时的开环延展模式。
 */
typedef struct {
    float omega_n;      /**< 自然频率 (rad/s) */
    float zeta;         /**< 阻尼比 */
    float Ts;           /**< 采样周期 (s) */
    float freq_nom;     /**< 额定频率 (Hz) */
    float freq_min;     /**< 最小频率 (Hz) */
    float freq_max;     /**< 最大频率 (Hz) */
    float v_threshold;  /**< 电压阈值，低于此值进入开环延展 (V) */
    float hold_time;    /**< 开环保持时间 (s) */
} Pll_Config;

/**
 * @brief PLL模块句柄结构体
 * 
 * 包含锁相环内部状态和配置。
 */
typedef struct {
    const Pll_Config *config;  /**< 配置指针 */
    
    /* 内部状态变量 */
    float x[2];        /**< SOGI状态变量: [alpha, beta] */
    float theta;       /**< 估计相位 (rad) */
    float omega;       /**< 估计角频率 (rad/s) */
    float freq;        /**< 估计频率 (Hz) */
    
    /* 开环延展状态 */
    bool open_loop;           /**< 开环模式标志 */
    float open_loop_timer;    /**< 开环计时器 */
    float last_valid_freq;    /**< 最后一个有效频率 */
    float last_valid_theta;   /**< 最后一个有效相位 */
    
    /* 积分器状态 */
    float integrator;         /**< PLL积分器状态 */
    
    /* 输入历史 */
    float v_alpha_prev;       /**< 上一拍alpha电压 */
} Pll_Handle;

/**
 * @brief 初始化PLL模块
 * 
 * @param h PLL句柄指针
 * @param cfg 配置指针
 */
void Pll_Init(Pll_Handle *h, const Pll_Config *cfg);

/**
 * @brief PLL单步执行
 * 
 * @param h PLL句柄指针
 * @param v_alpha Alpha轴电压 (Clark变换后)
 * @param v_beta Beta轴电压 (Clark变换后)
 * @param theta_out 输出相位 (rad)
 * @param freq_out 输出频率 (Hz)
 * @return true 正常锁相模式
 * @return false 开环延展模式
 */
bool Pll_Step(Pll_Handle *h, float v_alpha, float v_beta, 
              float *theta_out, float *freq_out);

/**
 * @brief 重置PLL内部状态
 * 
 * @param h PLL句柄指针
 * @param init_theta 初始相位 (rad)
 * @param init_freq 初始频率 (Hz)
 */
void Pll_Reset(Pll_Handle *h, float init_theta, float init_freq);

/**
 * @brief 获取当前是否处于开环模式
 * 
 * @param h PLL句柄指针
 * @return true 开环延展模式
 * @return false 正常锁相模式
 */
bool Pll_IsOpenLoop(const Pll_Handle *h);

/**
 * @brief 获取估计的电网电压幅值
 * 
 * @param h PLL句柄指针
 * @return float 电压幅值 (V)
 */
float Pll_GetVoltageAmplitude(const Pll_Handle *h);

/**
 * @brief 强制切换到开环模式
 * 
 * @param h PLL句柄指针
 * @param duration 开环持续时间 (s)，0表示使用配置中的hold_time
 */
void Pll_ForceOpenLoop(Pll_Handle *h, float duration);

/**
 * @brief 强制切换到正常锁相模式
 * 
 * @param h PLL句柄指针
 */
void Pll_ForceNormalMode(Pll_Handle *h);