#include "gfl_split_power.h"
#include <string.h>

/*===========================================
 * 内部常量
 *===========================================*/

#define EPSILON 0.01f           /**< 防止除零的小值 */
#define MAX_GRID_PHASES 3       /**< 最大电网相数 (ABC) */

/*===========================================
 * 功率指令竞争仲裁
 *===========================================*/

void Gfl_PowerArbitrate(const Gfl_PowerRequest *requests,
                        uint8_t num_requests,
                        Gfl_PowerDistribution dist_mode,
                        Gfl_SplitPowerRef *output) {
    if (num_requests == 0 || requests == NULL || output == NULL) {
        return;
    }
    
    /* 初始化输出 */
    memset(output, 0, sizeof(Gfl_SplitPowerRef));
    output->num_phases = 3;  /* 默认三相 */
    
    /* 按优先级收集同优先级请求 */
    Gfl_Priority highest_priority = GFL_PRIORITY_FAULT;
    Gfl_PowerRequest same_prio[6] = {0};
    uint8_t same_prio_count = 0;
    
    /* 找到最高优先级请求 */
    for (uint8_t i = 0; i < num_requests; i++) {
        if (!requests[i].valid) {
            continue;
        }
        
        if (requests[i].priority < highest_priority) {
            highest_priority = requests[i].priority;
            same_prio_count = 0;
            memset(same_prio, 0, sizeof(same_prio));
        }
        
        if (requests[i].priority == highest_priority) {
            if (same_prio_count < 6) {
                same_prio[same_prio_count++] = requests[i];
            }
        }
    }
    
    /* 处理同优先级请求: 按相掩码和权重合并 */
    if (same_prio_count > 0) {
        float P_sum[6] = {0};
        float Q_sum[6] = {0};
        float weight_sum[6] = {0};
        
        for (uint8_t i = 0; i < same_prio_count; i++) {
            float weight = (same_prio[i].weight > 0.0f) ? same_prio[i].weight : 1.0f;
            uint8_t mask = same_prio[i].phase_mask;
            
            for (uint8_t phase = 0; phase < MAX_GRID_PHASES; phase++) {
                if (mask & (1U << phase)) {
                    P_sum[phase] += same_prio[i].P_req * weight;
                    Q_sum[phase] += same_prio[i].Q_req * weight;
                    weight_sum[phase] += weight;
                }
            }
        }
        
        /* 计算加权平均 */
        for (uint8_t phase = 0; phase < MAX_GRID_PHASES; phase++) {
            if (weight_sum[phase] > EPSILON) {
                output->P[phase] = P_sum[phase] / weight_sum[phase];
                output->Q[phase] = Q_sum[phase] / weight_sum[phase];
            }
        }
        
        /* 根据分配模式处理 */
        if (dist_mode == POWER_DIST_SYMMETRIC) {
            /* 对称模式: 三相功率相同 */
            float P_avg = 0.0f;
            float Q_avg = 0.0f;
            uint8_t valid_count = 0;
            
            for (uint8_t phase = 0; phase < MAX_GRID_PHASES; phase++) {
                if (weight_sum[phase] > EPSILON) {
                    P_avg += output->P[phase];
                    Q_avg += output->Q[phase];
                    valid_count++;
                }
            }
            
            if (valid_count > 0) {
                P_avg /= valid_count;
                Q_avg /= valid_count;
                
                for (uint8_t phase = 0; phase < MAX_GRID_PHASES; phase++) {
                    output->P[phase] = P_avg;
                    output->Q[phase] = Q_avg;
                }
            }
        }
        /* POWER_DIST_PROPORTIONAL: 按电压比例分配 (预留) */
        /* POWER_DIST_ASYMMETRIC: 保持各相独立值 (已完成) */
    }
}

/*===========================================
 * 分相功率 PI 控制器
 *===========================================*/

void Gfl_SplitPowerPI_Init(Gfl_SplitPowerPI_Handle *h, 
                           const Gfl_SplitPowerPI_Config *cfg) {
    if (h == NULL || cfg == NULL) {
        return;
    }
    
    h->config = *cfg;
    memset(h->integrator_P, 0, sizeof(h->integrator_P));
    memset(h->integrator_Q, 0, sizeof(h->integrator_Q));
    memset(h->out_prev_P, 0, sizeof(h->out_prev_P));
    memset(h->out_prev_Q, 0, sizeof(h->out_prev_Q));
    memset(h->windup_P, 0, sizeof(h->windup_P));
    memset(h->windup_Q, 0, sizeof(h->windup_Q));
    memset(&h->power_ref, 0, sizeof(h->power_ref));
    memset(&h->power_fdb, 0, sizeof(h->power_fdb));
}

void Gfl_SplitPowerPI_Step(Gfl_SplitPowerPI_Handle *h,
                           const Gfl_SplitPowerRef *power_ref,
                           const Gfl_PowerFeedback *power_fdb,
                           Gfl_SplitCurrentRef *output) {
    if (h == NULL || power_ref == NULL || power_fdb == NULL || output == NULL) {
        return;
    }
    
    /* 保存参考和反馈 */
    h->power_ref = *power_ref;
    h->power_fdb = *power_fdb;
    
    uint8_t num_phases = (power_ref->num_phases < 6) ? power_ref->num_phases : 6;
    output->num_phases = num_phases;
    
    for (uint8_t phase = 0; phase < num_phases && phase < MAX_GRID_PHASES; phase++) {
        /* 计算功率误差 */
        float err_P = power_ref->P[phase] - power_fdb->P_fdb[phase];
        float err_Q = power_ref->Q[phase] - power_fdb->Q_fdb[phase];
        
        /* PI 控制器 (有功) - 标准离散PI: u = kp*e + ki*Ts*Σe */
        float u_P = h->config.kp_P * err_P + h->config.ki_P * h->config.Ts * h->integrator_P[phase];
        
        /* 抗积分饱和 (条件积分) */
        if (u_P > h->config.out_max_P) {
            u_P = h->config.out_max_P;
            h->windup_P[phase] = true;
        } else if (u_P < h->config.out_min_P) {
            u_P = h->config.out_min_P;
            h->windup_P[phase] = true;
        } else {
            h->windup_P[phase] = false;
        }
        /* 积分项累加 */
        h->integrator_P[phase] += err_P;
        
        /* PI 控制器 (无功) */
        float u_Q = h->config.kp_Q * err_Q + h->config.ki_Q * h->config.Ts * h->integrator_Q[phase];
        
        if (u_Q > h->config.out_max_Q) {
            u_Q = h->config.out_max_Q;
            h->windup_Q[phase] = true;
        } else if (u_Q < h->config.out_min_Q) {
            u_Q = h->config.out_min_Q;
            h->windup_Q[phase] = true;
        } else {
            h->windup_Q[phase] = false;
        }
        /* 积分项累加 */
        h->integrator_Q[phase] += err_Q;
        
        /* 输出: 功率误差 -> 电流参考 */
        /* Id = (2/3) * P / V, Iq = -(2/3) * Q / V */
        /* V_phase 需要从外部传入，此处使用标幺值 1.0 pu */
        float V_phase = 1.0f;  /* TODO: 需要从电压采样获取 */
        
        output->Id[phase] = (2.0f / 3.0f) * u_P / V_phase;
        output->Iq[phase] = -(2.0f / 3.0f) * u_Q / V_phase;
        
        /* NAN/INF 保护 */
        if (!Gfl_IsValidFloat(output->Id[phase])) output->Id[phase] = 0.0f;
        if (!Gfl_IsValidFloat(output->Iq[phase])) output->Iq[phase] = 0.0f;
        
        h->out_prev_P[phase] = u_P;
        h->out_prev_Q[phase] = u_Q;
    }
    
    /* N相电流置零 (平衡系统) */
    if (num_phases > MAX_GRID_PHASES) {
        output->Id[3] = 0.0f;
        output->Iq[3] = 0.0f;
    }
}

void Gfl_SplitPowerPI_Reset(Gfl_SplitPowerPI_Handle *h) {
    if (h == NULL) {
        return;
    }
    
    memset(h->integrator_P, 0, sizeof(h->integrator_P));
    memset(h->integrator_Q, 0, sizeof(h->integrator_Q));
    memset(h->windup_P, 0, sizeof(h->windup_P));
    memset(h->windup_Q, 0, sizeof(h->windup_Q));
}

/*===========================================
 * 分相功率反馈计算
 *===========================================*/

void Gfl_CalcPowerFeedback(float v_a, float v_b, float v_c,
                           float i_a, float i_b, float i_c,
                           float pf,
                           Gfl_PowerFeedback *output) {
    (void)pf;  /* pf 参数暂未使用，使用瞬时功率计算 */
    
    if (output == NULL) {
        return;
    }
    
    /* Clarke 变换: abc -> αβ
     * v_alpha = v_a
     * v_beta = (v_a + 2*v_b) / sqrt(3)
     */
    float v_alpha = v_a;
    float v_beta = (v_a + 2.0f * v_b) * 0.5773503f;  /* 1/sqrt(3) */
    
    float i_alpha = i_a;
    float i_beta = (i_a + 2.0f * i_b) * 0.5773503f;
    
    /* αβ坐标系瞬时功率:
     * P = v_alpha * i_alpha + v_beta * i_beta
     * Q = v_beta * i_alpha - v_alpha * i_beta
     */
    float P_alpha_beta = v_alpha * i_alpha + v_beta * i_beta;
    float Q_alpha_beta = v_beta * i_alpha - v_alpha * i_beta;
    
    /* 三相三线制: P_total = 3/2 * P_alpha_beta, Q_total = 3/2 * Q_alpha_beta */
    float P_inst = (3.0f / 2.0f) * P_alpha_beta;
    float Q_inst = (3.0f / 2.0f) * Q_alpha_beta;
    
    /* 分配到各相 (对称系统各相相等) */
    for (uint8_t i = 0; i < 3; i++) {
        output->P_fdb[i] = P_inst / 3.0f;
        output->Q_fdb[i] = Q_inst / 3.0f;
    }
    
    /* 总功率 */
    output->P_total = P_inst;
    output->Q_total = Q_inst;
    
    /* NAN/INF 保护 */
    for (uint8_t i = 0; i < 3; i++) {
        if (!Gfl_IsValidFloat(output->P_fdb[i])) output->P_fdb[i] = 0.0f;
        if (!Gfl_IsValidFloat(output->Q_fdb[i])) output->Q_fdb[i] = 0.0f;
    }
    if (!Gfl_IsValidFloat(output->P_total)) output->P_total = 0.0f;
    if (!Gfl_IsValidFloat(output->Q_total)) output->Q_total = 0.0f;
}