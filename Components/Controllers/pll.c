#include "pll.h"
#include <math.h>
#include <arm_math.h>

/* 内部常量定义 */
#define PI              3.14159265358979323846f
#define TWO_PI          6.28318530717958647692f
#define SQRT3           1.73205080756887729352f
#define INV_SQRT3       0.57735026918962576451f

/* 辅助函数声明 */
static float normalize_angle(float angle);
static void sogi_update(Pll_Handle *h, float v_alpha, float v_beta);
static void pll_update(Pll_Handle *h, float v_alpha_q);

/**
 * @brief 初始化PLL模块
 */
void Pll_Init(Pll_Handle *h, const Pll_Config *cfg) {
    h->config = cfg;
    
    /* 初始化状态 */
    h->x[0] = 0.0f;        /* alpha */
    h->x[1] = 0.0f;        /* beta */
    h->theta = 0.0f;
    h->omega = cfg->freq_nom * TWO_PI;
    h->freq = cfg->freq_nom;
    
    /* 开环状态 */
    h->open_loop = false;
    h->open_loop_timer = 0.0f;
    h->last_valid_freq = cfg->freq_nom;
    h->last_valid_theta = 0.0f;
    
    /* 积分器状态 */
    h->integrator = 0.0f;
    
    /* 输入历史 */
    h->v_alpha_prev = 0.0f;
}

/**
 * @brief PLL单步执行
 */
bool Pll_Step(Pll_Handle *h, float v_alpha, float v_beta, 
              float *theta_out, float *freq_out) {
    const Pll_Config *c = h->config;
    
    /* 计算电压幅值用于判断零电压穿越 */
    float v_amplitude = sqrtf(v_alpha * v_alpha + v_beta * v_beta);
    
    /* 检查是否需要进入开环模式 */
    if (v_amplitude < c->v_threshold) {
        if (!h->open_loop) {
            /* 进入开环模式 */
            h->open_loop = true;
            h->last_valid_freq = h->freq;
            h->last_valid_theta = h->theta;
            h->open_loop_timer = 0.0f;
        }
    } else {
        if (h->open_loop) {
            /* 退出开环模式 */
            h->open_loop = false;
        }
    }
    
    if (h->open_loop) {
        /* 开环延展模式：保持频率，相位继续积分 */
        h->open_loop_timer += c->Ts;
        
        if (h->open_loop_timer > c->hold_time) {
            /* 超过保持时间，重新尝试锁相 */
            h->open_loop = false;
        } else {
            /* 维持最后一个有效频率，相位继续积分 */
            h->omega = h->last_valid_freq * TWO_PI;
            h->theta = normalize_angle(h->last_valid_theta + h->omega * c->Ts);
            h->freq = h->last_valid_freq;
        }
    } else {
        /* 正常锁相模式 */
        /* 更新SOGI滤波器 */
        sogi_update(h, v_alpha, v_beta);
        
        /* 计算正交分量（beta轴作为q轴） */
        float v_alpha_q = h->x[1];  /* SOGI输出的beta分量与输入正交 */
        
        /* 更新PLL */
        pll_update(h, v_alpha_q);
        
        /* 更新最后一个有效值 */
        h->last_valid_freq = h->freq;
        h->last_valid_theta = h->theta;
        
        /* 重置开环计时器 */
        h->open_loop_timer = 0.0f;
    }
    
    /* 输出结果 */
    if (theta_out) *theta_out = h->theta;
    if (freq_out) *freq_out = h->freq;
    
    return !h->open_loop;  /* 返回是否正常锁相 */
}

/**
 * @brief 重置PLL内部状态
 */
void Pll_Reset(Pll_Handle *h, float init_theta, float init_freq) {
    /* 归一化角度 */
    h->theta = normalize_angle(init_theta);
    
    /* 限制频率范围 */
    const Pll_Config *c = h->config;
    if (init_freq < c->freq_min) init_freq = c->freq_min;
    if (init_freq > c->freq_max) init_freq = c->freq_max;
    
    h->freq = init_freq;
    h->omega = init_freq * TWO_PI;
    
    /* 重置SOGI状态 */
    h->x[0] = 0.0f;
    h->x[1] = 0.0f;
    
    /* 重置积分器状态 */
    h->integrator = 0.0f;
    
    /* 重置开环状态 */
    h->open_loop = false;
    h->open_loop_timer = 0.0f;
    h->last_valid_freq = init_freq;
    h->last_valid_theta = init_theta;
    
    h->v_alpha_prev = 0.0f;
}

/**
 * @brief 获取当前是否处于开环模式
 */
bool Pll_IsOpenLoop(const Pll_Handle *h) {
    return h->open_loop;
}

/**
 * @brief 获取估计的电网电压幅值
 */
float Pll_GetVoltageAmplitude(const Pll_Handle *h) {
    /* 使用SOGI的alpha输出计算幅值 */
    float amplitude = sqrtf(h->x[0] * h->x[0] + h->x[1] * h->x[1]);
    return amplitude;
}

/**
 * @brief 强制切换到开环模式
 */
void Pll_ForceOpenLoop(Pll_Handle *h, float duration) {
    h->open_loop = true;
    h->last_valid_freq = h->freq;
    h->last_valid_theta = h->theta;
    h->open_loop_timer = 0.0f;
    
    if (duration > 0.0f) {
        /* 使用指定的持续时间 */
        Pll_Config *c = (Pll_Config *)h->config;
        c->hold_time = duration;
    }
}

/**
 * @brief 强制切换到正常锁相模式
 */
void Pll_ForceNormalMode(Pll_Handle *h) {
    h->open_loop = false;
    h->open_loop_timer = 0.0f;
}

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 将角度归一化到[0, 2π)
 */
static float normalize_angle(float angle) {
    angle = fmodf(angle, TWO_PI);
    if (angle < 0.0f) angle += TWO_PI;
    return angle;
}

/**
 * @brief 更新SOGI滤波器状态
 * 
 * SOGI传递函数：H(s) = ω_n * s / (s² + ω_n²)
 * 状态空间实现：dx/dt = A*x + B*u
 * 其中 A = [0, -ω_n²; 1, 0], B = [ω_n; 0]
 * 
 * 使用前向欧拉离散化：x(k+1) = x(k) + Ts*(A*x(k) + B*u(k))
 */
static void sogi_update(Pll_Handle *h, float v_alpha, float v_beta) {
    const Pll_Config *c = h->config;
    float Ts = c->Ts;
    
    /* 注意：这里v_beta输入用于正交生成，但SOGI通常只处理单相 */
    /* 对于三相系统，通常先进行Clark变换得到αβ分量 */
    /* 我们假设v_alpha是主输入，v_beta是正交分量（来自Clark变换） */
    
    /* 计算SOGI系数 */
    float omega_n = c->omega_n;
    float omega_n2 = omega_n * omega_n;
    
    /* 状态空间矩阵 */
    float A00 = 0.0f;
    float A01 = -omega_n2;
    float A10 = 1.0f;
    float A11 = 0.0f;
    
    float B0 = omega_n;
    float B1 = 0.0f;
    
    /* 状态更新 */
    float x0_next = h->x[0] + Ts * (A00 * h->x[0] + A01 * h->x[1] + B0 * v_alpha);
    float x1_next = h->x[1] + Ts * (A10 * h->x[0] + A11 * h->x[1] + B1 * v_alpha);
    
    h->x[0] = x0_next;
    h->x[1] = x1_next;
}

/**
 * @brief 更新PLL核心（相位和频率跟踪）
 * 
 * 使用简单的比例积分控制器：
 * Δθ = Kp * v_q + Ki * ∫v_q dt
 * 其中v_q是电压的正交分量
 */
static void pll_update(Pll_Handle *h, float v_alpha_q) {
    const Pll_Config *c = h->config;
    
    /* 简单的PI控制器参数 */
    float kp = 2.0f * c->zeta * c->omega_n;
    float ki = c->omega_n * c->omega_n;
    
    /* 误差信号：v_q应该是0当相位锁定时 */
    float error = v_alpha_q;
    
    /* 更新频率（积分项） */
    h->integrator += ki * c->Ts * error;
    
    /* 计算频率变化 */
    float delta_omega = kp * error + h->integrator;
    
    /* 更新频率估计 */
    float omega_new = c->freq_nom * TWO_PI + delta_omega;
    
    /* 频率限制 */
    float omega_min = c->freq_min * TWO_PI;
    float omega_max = c->freq_max * TWO_PI;
    
    int limited = 0;
    if (omega_new < omega_min) {
        omega_new = omega_min;
        limited = 1;
    }
    if (omega_new > omega_max) {
        omega_new = omega_max;
        limited = 1;
    }
    
    /* 抗积分饱和：如果频率被限制，调整积分器以避免windup */
    if (limited) {
        float delta_omega_limited = omega_new - c->freq_nom * TWO_PI;
        h->integrator = delta_omega_limited - kp * error;
    }
    
    h->omega = omega_new;
    h->freq = omega_new / TWO_PI;
    
    /* 更新相位 */
    h->theta = normalize_angle(h->theta + h->omega * c->Ts);
}