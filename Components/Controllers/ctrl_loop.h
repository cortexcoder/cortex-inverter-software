/**
 * @file ctrl_loop.h
 * @brief Current Loop Controller for Three-Phase Inverter
 * @ingroup Controllers
 * 
 * 电流环控制器，支持正序、负序、零序控制
 * 设计用于主中断中执行，注重执行效率
 * 
 * 执行时序（每个PWM周期）：
 * 1. PWM更新中断（发波后）：CtrlLoop_PreSample - 读取电流、坐标变换
 * 2. PWM下溢中断（发波前）：CtrlLoop_Calc - PI计算、IRC计算
 * 3. PWM周期结束：CtrlLoop_PostCalc - 限幅、故障检测
 * 
 * @author DeerFlow Integration
 * @date 2026-04-10
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "pi_ctrl.h"
#include "fast_math.h"

#define CTRL_LOOP_PI_HANDLE_SIZE 4

#define SQRT2_OVER_SQRT3 0.8164965809277260f
#define INV_SQRT2        0.7071067811865475f
#define INV_SQRT3        0.5773502691896258f
#define HALF_SQRT3       0.8660254037844386f
#define TWO_PI            6.28318530717958647692f
#define INV_3             0.33333333333333333333f

/**
 * @brief 电流环相位类型
 */
typedef enum {
    LOOP_PHASE_POS = 0,
    LOOP_PHASE_NEG,
    LOOP_PHASE_ZERO,
    LOOP_PHASE_COUNT
} LoopPhase;

/**
 * @brief 电流环状态
 */
typedef enum {
    LOOP_STATE_IDLE = 0,
    LOOP_STATE_READY,
    LOOP_STATE_RUNNING,
    LOOP_STATE_FAULT
} LoopState;

/**
 * @brief 电流环输出
 */
typedef struct {
    float duty_a;
    float duty_b;
    float duty_c;
} CtrlLoop_Output;

/**
 * @brief 电流环中间结果（发波前计算）
 */
typedef struct {
    float I_alpha;
    float I_beta;
    float I_zero;
    float Id_fdb;
    float Iq_fdb;
} CtrlLoop_PreCalc;

/**
 * @brief 正序PI参数
 */
typedef struct {
    float kp_d;
    float ki_d;
    float kp_q;
    float ki_q;
} CtrlLoop_PosPI_Config;

/**
 * @brief 负序PI参数
 */
typedef struct {
    float kp_d;
    float ki_d;
    float kp_q;
    float ki_q;
} CtrlLoop_NegPI_Config;

/**
 * @brief 电流环配置
 */
typedef struct {
    CtrlLoop_PosPI_Config pos;
    CtrlLoop_NegPI_Config neg;
    float out_max;
    float out_min;
    float Ts;
    float Vdc;
    float duty_max;
    float duty_min;
    float current_limit;
    bool enable_neg;
    bool enable_gvff;
    float L;
    float R;
} CtrlLoop_Config;

/**
 * @brief PI控制器数据（内嵌，避免指针）
 */
typedef struct {
    float kp;
    float ki;
    float out_max;
    float out_min;
    float integrator;
    float out_prev;
    float ref_prev;
} CtrlLoop_PI_Data;

/**
 * @brief IRC谐振控制器数据
 */
typedef struct {
    float kp;
    float kr;
    float wc;
    float Ts;
    float integral[2];
    float out_prev;
    bool init;
} CtrlLoop_IRC_Data;

/**
 * @brief DCI直流分量控制器数据
 */
typedef struct {
    float kp;
    float ki;
    float out_max;
    float out_min;
    float integrator;
    float ref_prev;
    bool init;
} CtrlLoop_DCI_Data;

/**
 * @brief 电网电压前馈数据
 */
typedef struct {
    float L;
    float R;
    float omega;
    float Ts;
    float Vd_fbk_prev;
    float Vq_fbk_prev;
    bool init;
} CtrlLoop_GVFF_Data;

/**
 * @brief 电流环句柄
 */
typedef struct {
    const CtrlLoop_Config *config;
    
    CtrlLoop_PreCalc pre;
    
    float Vd_ref_pos;
    float Vq_ref_pos;
    float Vd_ref_neg;
    float Vq_ref_neg;
    
    CtrlLoop_Output output;
    
    CtrlLoop_PI_Data pi_pos_d;
    CtrlLoop_PI_Data pi_pos_q;
    CtrlLoop_PI_Data pi_neg_d;
    CtrlLoop_PI_Data pi_neg_q;
    
    CtrlLoop_IRC_Data irc_d;
    CtrlLoop_IRC_Data irc_q;
    
    CtrlLoop_DCI_Data dci;
    
    CtrlLoop_GVFF_Data gvff;
    
    float theta_pos;
    float theta_neg;
    float theta_elec;
    
    LoopState state;
    uint32_t fault_flags;
    uint32_t tick_count;
    
    bool init;
} CtrlLoop_Handle;

/**
 * @brief 初始化电流环
 */
void CtrlLoop_Init(CtrlLoop_Handle *h, const CtrlLoop_Config *cfg);

/**
 * @brief 重置电流环
 */
void CtrlLoop_Reset(CtrlLoop_Handle *h);

/**
 * @brief 设置角度（从PLL获取）
 */
void CtrlLoop_SetTheta(CtrlLoop_Handle *h, float theta_pos, float theta_neg);

/**
 * @brief 发波前处理 - 读取电流并执行坐标变换
 * @note 在PWM更新中断中调用，读取ADC结果并计算Id/Iq
 */
void CtrlLoop_PreSample(CtrlLoop_Handle *h, float I_a, float I_b, float I_c);

/**
 * @brief 电流环计算 - 执行PI控制和IRC谐振控制
 * @note 在PWM下溢中断中调用，在采样完成后立即执行
 */
void CtrlLoop_Calc(CtrlLoop_Handle *h, float Id_ref, float Iq_ref, float Vdc);

/**
 * @brief 发波后处理 - 限幅和故障检测
 */
void CtrlLoop_PostCalc(CtrlLoop_Handle *h);

/**
 * @brief 获取占空比输出
 */
void CtrlLoop_GetDuty(const CtrlLoop_Handle *h, float *duty_a, float *duty_b, float *duty_c);

/**
 * @brief 获取故障状态
 */
uint32_t CtrlLoop_GetFault(const CtrlLoop_Handle *h);

/**
 * @brief 清除故障
 */
void CtrlLoop_ClearFault(CtrlLoop_Handle *h);

/* ===== 内部辅助函数 ===== */

/**
 * @brief Clark变换 (ABC -> αβ)
 */
static inline void CtrlLoop_Clark(float I_a, float I_b, float I_c,
                                  float *I_alpha, float *I_beta, float *I_zero) {
    *I_alpha = SQRT2_OVER_SQRT3 * I_a - INV_SQRT3 * (I_b + I_c);
    *I_beta = INV_SQRT2 * (I_b - I_c);
    *I_zero = (I_a + I_b + I_c) * INV_3;
}

/**
 * @brief Park变换 (αβ -> dq)
 */
static inline void CtrlLoop_Park(float I_alpha, float I_beta, float theta,
                                 float *I_d, float *I_q) {
    float sin_theta, cos_theta;
    FastSinCos(theta, &sin_theta, &cos_theta);
    *I_d = I_alpha * cos_theta + I_beta * sin_theta;
    *I_q = -I_alpha * sin_theta + I_beta * cos_theta;
}

/**
 * @brief 负序Park变换 (αβ -> dq)
 */
static inline void CtrlLoop_Park_Neg(float I_alpha, float I_beta, float theta,
                                     float *I_d, float *I_q) {
    float sin_theta, cos_theta;
    FastSinCos(theta, &sin_theta, &cos_theta);
    *I_d = I_alpha * cos_theta - I_beta * sin_theta;
    *I_q = I_alpha * sin_theta + I_beta * cos_theta;
}

/**
 * @brief 逆Park变换 (dq -> αβ)
 */
static inline void CtrlLoop_InvPark(float V_d, float V_q, float theta,
                                    float *V_alpha, float *V_beta) {
    float sin_theta, cos_theta;
    FastSinCos(theta, &sin_theta, &cos_theta);
    *V_alpha = V_d * cos_theta - V_q * sin_theta;
    *V_beta = V_d * sin_theta + V_q * cos_theta;
}

/**
 * @brief 逆Park变换 (dq -> αβ)，负序
 */
static inline void CtrlLoop_InvPark_Neg(float V_d, float V_q, float theta,
                                        float *V_alpha, float *V_beta) {
    float sin_theta, cos_theta;
    FastSinCos(theta, &sin_theta, &cos_theta);
    *V_alpha = V_d * cos_theta + V_q * sin_theta;
    *V_beta = -V_d * sin_theta + V_q * cos_theta;
}

/**
 * @brief PI控制器步进
 */
static inline void CtrlLoop_PI_Step(CtrlLoop_PI_Data *pi, float ref, float fdb, float *out) {
    float err = ref - fdb;
    float u = pi->kp * err + pi->ki * (err - pi->ref_prev);
    
    if (u > pi->out_max) u = pi->out_max;
    if (u < pi->out_min) u = pi->out_min;
    
    pi->integrator += u;
    pi->out_prev = u;
    pi->ref_prev = err;
    *out = u;
}

/**
 * @brief IRC谐振控制器步进
 */
static inline void CtrlLoop_IRC_Step(CtrlLoop_IRC_Data *irc, float ref, float fdb, float theta, float *out) {
    if (!irc->init) {
        irc->integral[0] = 0.0f;
        irc->integral[1] = 0.0f;
        irc->out_prev = 0.0f;
        irc->init = true;
    }
    
    float err = ref - fdb;
    
    float cos_t, sin_t;
    FastSinCos(theta, &sin_t, &cos_t);
    
    float e_d = err * cos_t;
    float e_q = -err * sin_t;
    
    irc->integral[0] += e_d * irc->Ts;
    irc->integral[1] += e_q * irc->Ts;
    
    float kr_e = irc->kr * e_d;
    float kr_e_q = irc->kr * e_q;
    
    float u_d = irc->kp * (err + 2.0f * kr_e / (1.0f + irc->wc * irc->Ts));
    float u_q = irc->kp * (err + 2.0f * kr_e_q / (1.0f + irc->wc * irc->Ts));
    
    float u = u_d * cos_t - u_q * sin_t;
    
    if (u > 1.0f) u = 1.0f;
    if (u < -1.0f) u = -1.0f;
    
    irc->out_prev = u;
    *out = u;
}

/**
 * @brief 电网电压前馈计算
 */
static inline void CtrlLoop_GVFF_Step(CtrlLoop_GVFF_Data *gvff, float V_grid_d, float V_grid_q, 
                                       float *V_ff_d, float *V_ff_q) {
    if (!gvff->init) {
        gvff->Vd_fbk_prev = 0.0f;
        gvff->Vq_fbk_prev = 0.0f;
        gvff->init = true;
    }
    
    float omega_L = gvff->omega * gvff->L;
    float V_d_term = V_grid_d - gvff->R * V_grid_d - omega_L * gvff->Vq_fbk_prev;
    float V_q_term = V_grid_q - gvff->R * V_grid_q + omega_L * gvff->Vd_fbk_prev;
    
    *V_ff_d = V_d_term / (gvff->R + gvff->Ts / (gvff->L * gvff->Ts + 1.0f));
    *V_ff_q = V_q_term / (gvff->R + gvff->Ts / (gvff->L * gvff->Ts + 1.0f));
    
    gvff->Vd_fbk_prev = V_grid_d;
    gvff->Vq_fbk_prev = V_grid_q;
}

/**
 * @brief αβ到三相占空比转换
 */
static inline void CtrlLoop_ToDuty(float V_alpha, float V_beta, float Vdc,
                                    float duty_max, float duty_min,
                                    float *duty_a, float *duty_b, float *duty_c) {
    float V_a = V_alpha;
    float V_b = -0.5f * V_alpha + HALF_SQRT3 * V_beta;
    float V_c = -0.5f * V_alpha - HALF_SQRT3 * V_beta;
    
    float V_max = V_a;
    float V_min = V_a;
    if (V_b > V_max) V_max = V_b;
    if (V_c > V_max) V_max = V_c;
    if (V_b < V_min) V_min = V_b;
    if (V_c < V_min) V_min = V_c;
    
    float V_center = (V_max + V_min) * 0.5f;
    float V_range = V_max - V_min;
    
    if (V_range > 0.0f) {
        float scale = 0.5f / V_range;
        *duty_a = (V_a - V_center) * scale + 0.5f;
        *duty_b = (V_b - V_center) * scale + 0.5f;
        *duty_c = (V_c - V_center) * scale + 0.5f;
    } else {
        *duty_a = 0.5f;
        *duty_b = 0.5f;
        *duty_c = 0.5f;
    }
    
    if (*duty_a > duty_max) *duty_a = duty_max;
    if (*duty_a < duty_min) *duty_a = duty_min;
    if (*duty_b > duty_max) *duty_b = duty_max;
    if (*duty_b < duty_min) *duty_b = duty_min;
    if (*duty_c > duty_max) *duty_c = duty_max;
    if (*duty_c < duty_min) *duty_c = duty_min;
}
