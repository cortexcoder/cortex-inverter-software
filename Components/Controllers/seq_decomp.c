#include "seq_decomp.h"
#include <math.h>

/* 对称分量法常数 */
#define SQRT3_INV     (0.5773502691896258f)   /* 1/√3 */
#define SQRT3_DIV2    (0.8660254037844386f)   /* √3/2 */
#define TWO_THIRDS    (0.6666666666666667f)   /* 2/3 */
#define ONE_THIRD     (0.3333333333333333f)   /* 1/3 */

/**
 * @brief 角度归一化到 [0, 2π)
 */
static float normalize_angle_2pi(float angle) {
    angle = fmodf(angle, 6.28318530717959f);  /* 2*PI */
    if (angle < 0.0f) {
        angle += 6.28318530717959f;
    }
    return angle;
}

/**
 * @brief 角度归一化到 (-π, π]
 */
static float normalize_angle_pi(float angle) {
    angle = fmodf(angle, 6.28318530717959f);  /* 2*PI */
    if (angle > 3.14159265358979f) {
        angle -= 6.28318530717959f;
    } else if (angle < -3.14159265358979f) {
        angle += 6.28318530717959f;
    }
    return angle;
}

void SeqDecomp_Init(SeqDecomp_Handle *h, const SeqDecomp_Config *cfg) {
    h->config = cfg;
    h->pos_d = 0.0f;
    h->pos_q = 0.0f;
    h->neg_d = 0.0f;
    h->neg_q = 0.0f;
    h->zero = 0.0f;
    h->theta_pos = 0.0f;
    h->theta_neg = 0.0f;
    h->v_alpha_prev = 0.0f;
    h->v_beta_prev = 0.0f;
    h->qsg_x[0] = 0.0f;
    h->qsg_x[1] = 0.0f;
    h->init = true;
}

/**
 * @brief Clarke 变换 (支持不平衡三相)
 * 
 * 通用 Clarke 变换 (IEEE Standard 4215):
 *   v_alpha = (2/3) * v_a - (1/3) * v_b - (1/3) * v_c
 *   v_beta  = (2/√3) * (v_b - v_c)/2 = (1/√3) * (v_b - v_c)
 *   v_zero  = (1/3) * (v_a + v_b + v_c)
 * 
 * 对于三相三线制 (无零序电流通路): v_a + v_b + v_c = 0
 * 此时简化公式:
 *   v_alpha = v_a
 *   v_beta  = (v_a + 2*v_b) / √3
 */
static void clarke_transform(float v_a, float v_b, float v_c,
                              float *v_alpha, float *v_beta, float *v_zero) {
    /* 标准 Clarke 变换 */
    *v_alpha = TWO_THIRDS * v_a - ONE_THIRD * v_b - ONE_THIRD * v_c;
    *v_beta  = SQRT3_INV * (v_b - v_c);
    
    /* 零序分量 = 三相电压的平均值 */
    *v_zero = ONE_THIRD * (v_a + v_b + v_c);
}

/**
 * @brief 正交信号发生器 (QSG) - 生成 90° 相移信号
 * 
 * 使用二阶广义积分器 (SOGI) 结构
 * 传递函数: H(s) = ω_n * s / (s² + ω_n²)
 * 
 * @param x QSG 状态 [x0, x1]
 * @param u 输入信号
 * @param omega_n 自然频率 (rad/s)
 * @param Ts 采样周期 (s)
 */
static void qsg_update(float *x, float u, float omega_n, float Ts) {
    float x0 = x[0];
    float x1 = x[1];
    
    /* 离散化状态空间实现 */
    /* A = [0, -ω_n²; 1, 0], B = [ω_n; 0] */
    float x0_next = x0 + Ts * (-omega_n * omega_n * x1 + omega_n * u);
    float x1_next = x1 + Ts * x0;  /* x1' = x0 */
    
    x[0] = x0_next;
    x[1] = x1_next;
}

void SeqDecomp_Step(SeqDecomp_Handle *h,
                    float v_a, float v_b, float v_c,
                    float theta_pll,
                    float *pos_d, float *pos_q,
                    float *neg_d, float *neg_q,
                    float *zero) {
    if (!h->init) {
        return;
    }
    
    /* Clarke 变换 */
    float v_alpha, v_beta, v_zero;
    clarke_transform(v_a, v_b, v_c, &v_alpha, &v_beta, &v_zero);
    
    /* 调用 αβ 版本的处理 */
    SeqDecomp_Step_AlphaBeta(h, v_alpha, v_beta, theta_pll,
                             pos_d, pos_q, neg_d, neg_q, zero);
    
    /* 如果需要零序分量，从 Clarke 变换获取 */
    *zero = v_zero;
}

void SeqDecomp_Step_AlphaBeta(SeqDecomp_Handle *h,
                              float v_alpha, float v_beta,
                              float theta_pll,
                              float *pos_d, float *pos_q,
                              float *neg_d, float *neg_q,
                              float *zero) {
    if (!h->init) {
        return;
    }
    
    /* 设置正序角度 (使用 PLL 输出) */
    h->theta_pos = theta_pll;
    
    /* 
     * 负序角度计算 (关键修正!)
     * 
     * 负序旋转方向与正序相反
     * 正序: 角度增加方向为反时针方向
     * 负序: 角度增加方向为顺时针方向
     * 
     * 因此: θ_neg = -θ_pos
     * 
     * 错误公式: θ_neg = θ_pos + π  (这会导致 120° 误差!)
     * 正确公式: θ_neg = -θ_pos
     */
    h->theta_neg = normalize_angle_pi(-theta_pll);
    
    /* 计算 sin/cos (可考虑从 PLL 复用以节省计算) */
    float cos_pos = cosf(theta_pll);
    float sin_pos = sinf(theta_pll);
    
    float cos_neg = cosf(h->theta_neg);
    float sin_neg = sinf(h->theta_neg);
    
    /* 
     * 正序 Park 变换: αβ -> dq (使用 +θ)
     * 
     * V_d = V_α * cos(θ) + V_β * sin(θ)
     * V_q = -V_α * sin(θ) + V_β * cos(θ)
     */
    h->pos_d = v_alpha * cos_pos + v_beta * sin_pos;
    h->pos_q = -v_alpha * sin_pos + v_beta * cos_pos;
    
    /* 
     * 负序 Park 变换: αβ -> dq (使用 -θ 或等价的 θ_neg)
     * 
     * 由于 θ_neg = -θ:
     * V_d_neg = V_α * cos(-θ) + V_β * sin(-θ)
     *           = V_α * cos(θ) - V_β * sin(θ)
     * V_q_neg = -V_α * sin(-θ) + V_β * cos(-θ)
     *           = -V_α * (-sin(θ)) + V_β * cos(θ)
     *           = V_α * sin(θ) + V_β * cos(θ)
     */
    h->neg_d = v_alpha * cos_neg + v_beta * sin_neg;
    h->neg_q = -v_alpha * sin_neg + v_beta * cos_neg;
    
    /* 更新历史值 */
    h->v_alpha_prev = v_alpha;
    h->v_beta_prev = v_beta;
    
    /* 输出结果 */
    *pos_d = h->pos_d;
    *pos_q = h->pos_q;
    *neg_d = h->neg_d;
    *neg_q = h->neg_q;
    
    /* 零序分量需要在三相输入版本中从 Clarke 变换获取 */
    /* αβ 版本无法直接计算零序 */
    (void)zero;
}

/**
 * @brief 正负序分解单步执行 - 快速版本 (复用 PLL 中间结果)
 * 
 * 此版本接受 PLL 已计算的中间结果，避免重复计算
 * 
 * @param h 句柄指针
 * @param v_alpha α轴电压
 * @param v_beta β轴电压
 * @param theta_pll PLL 输出的正序角度 (rad)
 * @param sin_pos 正序角度的 sin 值 (可从 PLL 复用)
 * @param cos_pos 正序角度的 cos 值 (可从 PLL 复用)
 * @param v_alpha_ortho α轴电压的正交分量 (可从 PLL SOGI 复用)
 * @param pos_d 输出正序 d 轴分量
 * @param pos_q 输出正序 q 轴分量
 * @param neg_d 输出负序 d 轴分量
 * @param neg_q 输出负序 q 轴分量
 */
void SeqDecomp_Step_Fast(SeqDecomp_Handle *h,
                         float v_alpha, float v_beta,
                         float theta_pll,
                         float sin_pos, float cos_pos,
                         float v_alpha_ortho,
                         float *pos_d, float *pos_q,
                         float *neg_d, float *neg_q) {
    if (!h->init) {
        return;
    }
    
    /* 设置正序角度 */
    h->theta_pos = theta_pll;
    
    /* 负序角度 */
    h->theta_neg = normalize_angle_pi(-theta_pll);
    
    /* 计算负序 sin/cos (无法从 PLL 复用，因为角度不同) */
    float cos_neg = cosf(h->theta_neg);
    float sin_neg = sinf(h->theta_neg);
    
    /* 正序 Park 变换 (复用 PLL 的 sin/cos) */
    h->pos_d = v_alpha * cos_pos + v_beta * sin_pos;
    h->pos_q = -v_alpha * sin_pos + v_beta * cos_pos;
    
    /* 负序 Park 变换 */
    h->neg_d = v_alpha * cos_neg + v_beta * sin_neg;
    h->neg_q = -v_alpha * sin_neg + v_beta * cos_neg;
    
    /* 更新历史值 */
    h->v_alpha_prev = v_alpha;
    h->v_beta_prev = v_beta;
    
    /* 输出结果 */
    *pos_d = h->pos_d;
    *pos_q = h->pos_q;
    *neg_d = h->neg_d;
    *neg_q = h->neg_q;
}

void SeqDecomp_Reset(SeqDecomp_Handle *h) {
    h->pos_d = 0.0f;
    h->pos_q = 0.0f;
    h->neg_d = 0.0f;
    h->neg_q = 0.0f;
    h->zero = 0.0f;
    h->theta_pos = 0.0f;
    h->theta_neg = 0.0f;
    h->v_alpha_prev = 0.0f;
    h->v_beta_prev = 0.0f;
    h->qsg_x[0] = 0.0f;
    h->qsg_x[1] = 0.0f;
}

void SeqDecomp_SetThetaPos(SeqDecomp_Handle *h, float theta) {
    h->theta_pos = normalize_angle_2pi(theta);
}

float SeqDecomp_GetThetaPos(const SeqDecomp_Handle *h) {
    return h->theta_pos;
}

float SeqDecomp_GetThetaNeg(const SeqDecomp_Handle *h) {
    return h->theta_neg;
}