#include "gfl_split_phase.h"
#include <string.h>

/*===========================================
 * 内部常量
 *===========================================*/

#define EPSILON 0.01f           /**< 防止除零的小值 */
#define MAX_GRID_PHASES 3       /**< 最大电网相数 (ABC) */
#define PHASE_INDEX_A 0         /**< A相索引 */
#define PHASE_INDEX_B 1         /**< B相索引 */
#define PHASE_INDEX_C 2         /**< C相索引 */
#define PHASE_INDEX_N 3         /**< N相索引 */

/*===========================================
 * 分相功率转电流 (统一底层函数)
 *===========================================*/

/**
 * @brief 单相功率转电流
 * 
 * @param P 有功功率 (pu)
 * @param Q 无功功率 (pu)
 * @param V 相电压 (pu)
 * @param Id_out 输出 Id
 * @param Iq_out 输出 Iq
 */
static void calc_single_phase_current(float P, float Q, float V,
                                      float *Id_out, float *Iq_out) {
    /* 防除零保护 */
    float V_safe = (fabsf(V) > EPSILON) ? V : 1.0f;
    
    /* 功率方程: Id = (2/3) * P / V, Iq = -(2/3) * Q / V */
    float Id = (2.0f / 3.0f) * P / V_safe;
    float Iq = -(2.0f / 3.0f) * Q / V_safe;
    
    /* NAN/INF 保护 */
    *Id_out = Gfl_IsValidFloat(Id) ? Id : 0.0f;
    *Iq_out = Gfl_IsValidFloat(Iq) ? Iq : 0.0f;
}

void GflSplitPhase_CalcCurrent(void *h,
                               const Gfl_PhasePower *power,
                               Gfl_CurrentMode mode,
                               Gfl_SplitCurrentRef *output) {
    (void)h;  /* 目前不需要内部状态 */
    
    uint8_t num_phases = output->num_phases;
    
    if (mode == CURRENT_MODE_SYMMETRIC) {
        /* 对称模式: 三相功率均分 */
        float P_phase = 0.0f;
        float Q_phase = 0.0f;
        float V_avg = 0.0f;
        
        for (uint8_t i = 0; i < num_phases && i < MAX_GRID_PHASES; i++) {
            P_phase += power[i].P / 3.0f;
            Q_phase += power[i].Q / 3.0f;
            V_avg += (fabsf(power[i].V) > EPSILON) ? power[i].V : 1.0f;
        }
        V_avg /= (num_phases > 3) ? 3 : num_phases;
        
        float Id, Iq;
        calc_single_phase_current(P_phase, Q_phase, V_avg, &Id, &Iq);
        
        /* 三相使用相同的 Id/Iq */
        for (uint8_t i = 0; i < num_phases && i < MAX_GRID_PHASES; i++) {
            output->Id[i] = Id;
            output->Iq[i] = Iq;
        }
        
        /* N相电流为零 (平衡系统) */
        if (num_phases > 3) {
            output->Id[PHASE_INDEX_N] = 0.0f;
            output->Iq[PHASE_INDEX_N] = 0.0f;
        }
        
    } else {
        /* 不对称模式: 各相独立计算 */
        for (uint8_t i = 0; i < num_phases && i < MAX_GRID_PHASES; i++) {
            calc_single_phase_current(power[i].P, power[i].Q, power[i].V,
                                     &output->Id[i], &output->Iq[i]);
        }
        
        /* N相电流计算 (KCL: I_N = -(I_A + I_B + I_C)) */
        if (num_phases > 3) {
            output->Id[PHASE_INDEX_N] = 0.0f;
            output->Iq[PHASE_INDEX_N] = 0.0f;
            for (uint8_t i = 0; i < 3 && i < num_phases; i++) {
                output->Id[PHASE_INDEX_N] -= output->Id[i];
                output->Iq[PHASE_INDEX_N] -= output->Iq[i];
            }
        }
    }
}

/*===========================================
 * 电流指令竞争仲裁
 *===========================================*/

void Gfl_CurrentArbitrate(const Gfl_CurrentRequest *requests,
                          uint8_t num_requests,
                          const Gfl_CurrentLimits *limits,
                          Gfl_SplitCurrentRef *output) {
    if (num_requests == 0 || output == NULL) {
        return;
    }
    
    /* 初始化输出为 0 */
    memset(output, 0, sizeof(Gfl_SplitCurrentRef));
    
    /* 按优先级分组处理 */
    Gfl_Priority highest_priority = GFL_PRIORITY_FAULT;  /* 最高优先级 */
    Gfl_CurrentRequest same_prio_requests[6] = {0};   /* 同优先级请求缓存 */
    uint8_t same_prio_count = 0;
    
    /* 找到最高优先级的请求 */
    for (uint8_t i = 0; i < num_requests; i++) {
        if (!requests[i].valid) {
            continue;
        }
        
        if (requests[i].priority < highest_priority) {
            /* 发现更高优先级请求，切换 */
            highest_priority = requests[i].priority;
            same_prio_count = 0;
            memset(same_prio_requests, 0, sizeof(same_prio_requests));
        }
        
        if (requests[i].priority == highest_priority) {
            /* 同优先级请求，收集 */
            if (same_prio_count < 6) {
                same_prio_requests[same_prio_count++] = requests[i];
            }
        }
    }
    
    /* 处理同优先级请求: 加权平均 */
    if (same_prio_count > 0) {
        float total_weight = 0.0f;
        float Id_sum = 0.0f;
        float Iq_sum = 0.0f;
        
        for (uint8_t i = 0; i < same_prio_count; i++) {
            float weight = same_prio_requests[i].weight;
            weight = (weight > 0.0f) ? weight : 1.0f;  /* 默认权重 1.0 */
            
            Id_sum += same_prio_requests[i].Id_req * weight;
            Iq_sum += same_prio_requests[i].Iq_req * weight;
            total_weight += weight;
        }
        
        if (total_weight > EPSILON) {
            float Id_avg = Id_sum / total_weight;
            float Iq_avg = Iq_sum / total_weight;
            
            /* 应用到所有相 (对称模式) */
            for (uint8_t i = 0; i < output->num_phases && i < 3; i++) {
                output->Id[i] = Id_avg;
                output->Iq[i] = Iq_avg;
            }
        }
    }
}

/*===========================================
 * 两阶段电流限幅
 *===========================================*/

void Gfl_CurrentLimit(const Gfl_SplitCurrentRef *input,
                      const Gfl_CurrentLimits *limits,
                      Gfl_CurrentMode mode,
                      Gfl_SplitCurrentRef *output) {
    if (input == NULL || limits == NULL || output == NULL) {
        return;
    }
    
    *output = *input;
    
    if (mode == CURRENT_MODE_SYMMETRIC) {
        /* 对称模式: 先圆形限幅，再矩形限幅 */
        
        /* 阶段1: 圆形限幅 (总电流矢量) */
        float I_mag = Gfl_CalcCurrentMag(output->Id[0], output->Iq[0]);
        
        if (I_mag > limits->I_max) {
            float scale = limits->I_max / I_mag;
            for (uint8_t i = 0; i < output->num_phases && i < 3; i++) {
                output->Id[i] *= scale;
                output->Iq[i] *= scale;
            }
        }
        
        /* 阶段2: 矩形限幅 (d/q轴分别限幅) */
        for (uint8_t i = 0; i < output->num_phases && i < 3; i++) {
            (void)Gfl_LimitPhaseCurrent(&output->Id[i], &output->Iq[i], limits);
        }
        
        /* N相电流置零 */
        if (output->num_phases > 3) {
            output->Id[PHASE_INDEX_N] = 0.0f;
            output->Iq[PHASE_INDEX_N] = 0.0f;
        }
        
    } else {
        /* 不对称模式: 每相独立限幅 */
        for (uint8_t i = 0; i < output->num_phases; i++) {
            (void)Gfl_LimitPhaseCurrent(&output->Id[i], &output->Iq[i], limits);
        }
    }
}

/*===========================================
 * 单相电流限幅
 *===========================================*/

bool Gfl_LimitPhaseCurrent(float *Id, float *Iq, const Gfl_CurrentLimits *limits) {
    if (Id == NULL || Iq == NULL || limits == NULL) {
        return false;
    }
    
    bool limited = false;
    
    /* d轴限幅 */
    if (*Id > limits->Id_max_pos) {
        *Id = limits->Id_max_pos;
        limited = true;
    } else if (*Id < limits->Id_max_neg) {
        *Id = limits->Id_max_neg;
        limited = true;
    }
    
    /* q轴限幅 */
    if (*Iq > limits->Iq_max) {
        *Iq = limits->Iq_max;
        limited = true;
    } else if (*Iq < -limits->Iq_max) {
        *Iq = -limits->Iq_max;
        limited = true;
    }
    
    /* 圆形限幅 (确保矢量幅值不超过限制) */
    float I_mag = Gfl_CalcCurrentMag(*Id, *Iq);
    if (I_mag > limits->I_max) {
        float scale = limits->I_max / I_mag;
        *Id *= scale;
        *Iq *= scale;
        limited = true;
    }
    
    return limited;
}