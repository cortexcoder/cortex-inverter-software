#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 逆变器拓扑类型
 */
typedef enum {
    SEQ_DECOMP_3LEG = 0,    /**< 三相三桥臂逆变器 (三线制，无零序电流通路) */
    SEQ_DECOMP_4LEG,        /**< 三相四桥臂逆变器 (四线制，有中性点) */
} SeqDecomp_Topology;

/**
 * @brief 正负序分解模块配置
 */
typedef struct {
    float Ts;           /**< 采样周期 (s) */
    float fc_hp;        /**< 高通滤波器截止频率 (Hz)，用于提取交流分量 */
    float fc_lp;        /**< 低通滤波器截止频率 (Hz)，用于提取直流分量 */
    SeqDecomp_Topology topology;  /**< 逆变器拓扑类型 */
} SeqDecomp_Config;

/**
 * @brief 正负序分解模块句柄
 */
typedef struct {
    const SeqDecomp_Config *config;
    
    /* 正序 dq 分量 */
    float pos_d;
    float pos_q;
    
    /* 负序 dq 分量 */
    float neg_d;
    float neg_q;
    
    /* 零序分量 */
    float zero;
    
    /* 内部状态 */
    float theta_pos;        /**< 正序同步角度 */
    float theta_neg;        /**< 负序同步角度 */
    
    /* αβ 历史值用于差分计算 */
    float v_alpha_prev;
    float v_beta_prev;
    
    /* 正交信号发生器状态 (用于负序提取) */
    float qsg_x[2];         /**< 正交信号发生器状态 */
    
    /* 标志位 */
    bool init;
} SeqDecomp_Handle;

/**
 * @brief 初始化正负序分解模块
 * 
 * @param h 句柄指针
 * @param cfg 配置指针
 */
void SeqDecomp_Init(SeqDecomp_Handle *h, const SeqDecomp_Config *cfg);

/**
 * @brief 正负序分解单步执行
 * 
 * 使用对称分量法 (Fortescue transformation) 分解三相电压/电流
 * 到正序、负序和零序分量。
 * 
 * 正负序分解公式：
 *   V_pos = (V_a + a*V_b + a^2*V_c) / 3
 *   V_neg = (V_a + a^2*V_b + a*V_c) / 3
 *   V_zero = (V_a + V_b + V_c) / 3
 * 
 * 其中 a = e^(j*120°) = -0.5 + j*sqrt(3)/2
 * 
 * @param h 句柄指针
 * @param v_a A相电压 (V)
 * @param v_b B相电压 (V)
 * @param v_c C相电压 (V)
 * @param theta_pll PLL输出的正序角度 (rad)
 * @param pos_d 输出正序d轴分量
 * @param pos_q 输出正序q轴分量
 * @param neg_d 输出负序d轴分量
 * @param neg_q 输出负序q轴分量
 * @param zero 输出零序分量
 */
void SeqDecomp_Step(SeqDecomp_Handle *h,
                    float v_a, float v_b, float v_c,
                    float theta_pll,
                    float *pos_d, float *pos_q,
                    float *neg_d, float *neg_q,
                    float *zero);

/**
 * @brief 使用 αβ 坐标系进行正负序分解
 * 
 * 适用于已经过 Clarke 变换的 αβ 信号
 * 
 * @param h 句柄指针
 * @param v_alpha α轴电压
 * @param v_beta β轴电压
 * @param theta_pll PLL输出的正序角度
 * @param pos_d 输出正序d轴分量
 * @param pos_q 输出正序q轴分量
 * @param neg_d 输出负序d轴分量
 * @param neg_q 输出负序q轴分量
 */
void SeqDecomp_Step_AlphaBeta(SeqDecomp_Handle *h,
                              float v_alpha, float v_beta,
                              float theta_pll,
                              float *pos_d, float *pos_q,
                              float *neg_d, float *neg_q,
                              float *zero);

/**
 * @brief 正负序分解快速版本 (复用 PLL 中间结果)
 * 
 * 此版本接受 PLL 已计算的中间结果 (sin/cos)，避免重复三角函数计算
 * 可显著减少计算量
 * 
 * @param h 句柄指针
 * @param v_alpha α轴电压
 * @param v_beta β轴电压
 * @param theta_pll PLL 输出的正序角度 (rad)
 * @param sin_pos 正序角度的 sin 值 (可从 PLL 复用)
 * @param cos_pos 正序角度的 cos 值 (可从 PLL 复用)
 * @param v_alpha_ortho α轴电压的正交分量 (可从 PLL SOGI 复用，当前未使用，预留)
 * @param pos_d 输出正序d轴分量
 * @param pos_q 输出正序q轴分量
 * @param neg_d 输出负序d轴分量
 * @param neg_q 输出负序q轴分量
 */
void SeqDecomp_Step_Fast(SeqDecomp_Handle *h,
                         float v_alpha, float v_beta,
                         float theta_pll,
                         float sin_pos, float cos_pos,
                         float v_alpha_ortho,
                         float *pos_d, float *pos_q,
                         float *neg_d, float *neg_q);

/**
 * @brief 重置正负序分解模块
 * 
 * @param h 句柄指针
 */
void SeqDecomp_Reset(SeqDecomp_Handle *h);

/**
 * @brief 设置正序角度（用于开环模式或外部角度源）
 * 
 * @param h 句柄指针
 * @param theta 正序角度 (rad)
 */
void SeqDecomp_SetThetaPos(SeqDecomp_Handle *h, float theta);

/**
 * @brief 获取当前正序角度
 * 
 * @param h 句柄指针
 * @return float 正序角度 (rad)
 */
float SeqDecomp_GetThetaPos(const SeqDecomp_Handle *h);

/**
 * @brief 获取当前负序角度
 * 
 * @param h 句柄指针
 * @return float 负序角度 (rad)
 */
float SeqDecomp_GetThetaNeg(const SeqDecomp_Handle *h);