#include "common_mode_voltage.h"
#include <math.h>
#include <arm_math.h>

/* 内部常量定义 */
#define PI              3.14159265358979323846f
#define TWO_PI          6.28318530717958647692f
#define SQRT3           1.73205080756887729352f
#define INV_SQRT3       0.57735026918962576451f
#define PI_DIV_3        1.04719755119659774615f
#define TWO_PI_DIV_3    2.09439510239319549229f

/* 内部函数声明 */
static float calculate_svpwm_cmv(float duty_a, float duty_b, float duty_c);
static float calculate_thipwm_cmv(float duty_a, float duty_b, float duty_c, float angle, float factor);
static float calculate_dpwm_cmv(float duty_a, float duty_b, float duty_c, 
                                float angle, int dpwm_type, float offset);
static void apply_duty_limits(float *duty_a, float *duty_b, float *duty_c, 
                             float min_limit, float max_limit);
static int get_sector_from_angle(float angle);
static void smooth_transition(Cmv_Handle *h, float *duty_a, float *duty_b, float *duty_c);

/**
 * @brief 初始化共模电压注入模块
 */
void Cmv_Init(Cmv_Handle *h, const Cmv_Config *cfg) {
    h->config = cfg;
    h->current_strategy = cfg->init_strategy;
    h->common_mode_voltage = 0.0f;
    
    h->load_estimate = 0.0f;
    h->strategy_changed = false;
    h->strategy_change_counter = 0;
    
    h->duty_a_prev = 0.0f;
    h->duty_b_prev = 0.0f;
    h->duty_c_prev = 0.0f;
    
    h->dpwm_sector = 0;
    h->dpwm_angle = 0.0f;
    
    h->switching_loss_estimate = 0.0f;
    h->conduction_loss_estimate = 0.0f;
}

/**
 * @brief 计算并注入共模电压
 */
float Cmv_Inject(Cmv_Handle *h, float duty_a, float duty_b, float duty_c,
                 float angle, ModulationStrategy modulation,
                 float *duty_a_out, float *duty_b_out, float *duty_c_out) {
    const Cmv_Config *c = h->config;
    
    /* 处理调制策略输入 */
    if (modulation == MODULATION_COUNT) {
        modulation = h->current_strategy;
    } else if (modulation != h->current_strategy) {
        /* 策略改变 */
        Cmv_SwitchStrategy(h, modulation, true);
    }
    
    /* 保存原始占空比用于自适应调制 */
    float original_duties[3] = {duty_a, duty_b, duty_c};
    
    /* 计算共模电压 */
    float cmv = 0.0f;
    
    switch (modulation) {
        case MODULATION_SVPWM:
            cmv = calculate_svpwm_cmv(duty_a, duty_b, duty_c);
            break;
            
        case MODULATION_THIPWM:
            cmv = calculate_thipwm_cmv(duty_a, duty_b, duty_c, angle, c->thipwm_factor);
            break;
            
        case MODULATION_DPWM0:
            cmv = calculate_dpwm_cmv(duty_a, duty_b, duty_c, angle, 0, c->dpwm_offset[0]);
            break;
            
        case MODULATION_DPWM1:
            cmv = calculate_dpwm_cmv(duty_a, duty_b, duty_c, angle, 1, c->dpwm_offset[1]);
            break;
            
        case MODULATION_DPWM2:
            cmv = calculate_dpwm_cmv(duty_a, duty_b, duty_c, angle, 2, c->dpwm_offset[2]);
            break;
            
        case MODULATION_DPWM3:
            cmv = calculate_dpwm_cmv(duty_a, duty_b, duty_c, angle, 3, c->dpwm_offset[3]);
            break;
            
        case MODULATION_ADAPTIVE:
            /* 自适应调制：根据负载选择最优策略 */
            /* 这里简化实现：根据占空比大小选择DPWM或SVPWM */
            float max_duty = fmaxf(fmaxf(fabsf(duty_a), fabsf(duty_b)), fabsf(duty_c));
            if (max_duty < c->adaptive_threshold) {
                cmv = calculate_svpwm_cmv(duty_a, duty_b, duty_c);
            } else {
                /* 高调制比时使用DPWM1减少开关次数 */
                cmv = calculate_dpwm_cmv(duty_a, duty_b, duty_c, angle, 1, c->dpwm_offset[1]);
            }
            break;
            
        default:
            /* 默认使用SVPWM */
            cmv = calculate_svpwm_cmv(duty_a, duty_b, duty_c);
            break;
    }
    
    /* 应用共模电压注入 */
    float duty_a_injected = duty_a + cmv;
    float duty_b_injected = duty_b + cmv;
    float duty_c_injected = duty_c + cmv;
    
    /* 平滑切换处理 */
    if (h->strategy_changed) {
        smooth_transition(h, &duty_a_injected, &duty_b_injected, &duty_c_injected);
    }
    
    /* 限制占空比范围 */
    apply_duty_limits(&duty_a_injected, &duty_b_injected, &duty_c_injected,
                     c->duty_min, c->duty_max);
    
    /* 保存历史值 */
    h->duty_a_prev = duty_a_injected;
    h->duty_b_prev = duty_b_injected;
    h->duty_c_prev = duty_c_injected;
    
    /* 更新扇区和角度信息 */
    h->dpwm_sector = get_sector_from_angle(angle);
    h->dpwm_angle = angle;
    
    /* 更新共模电压值 */
    h->common_mode_voltage = cmv;
    
    /* 更新性能估计（简化模型） */
    h->switching_loss_estimate = 0.0f;  /* 需要实际模型 */
    h->conduction_loss_estimate = 0.0f; /* 需要实际模型 */
    
    /* 输出结果 */
    if (duty_a_out) *duty_a_out = duty_a_injected;
    if (duty_b_out) *duty_b_out = duty_b_injected;
    if (duty_c_out) *duty_c_out = duty_c_injected;
    
    return cmv;
}

/**
 * @brief 切换到指定调制策略
 */
void Cmv_SwitchStrategy(Cmv_Handle *h, ModulationStrategy strategy, bool smooth) {
    if (strategy >= MODULATION_COUNT) {
        strategy = MODULATION_SVPWM;
    }
    
    if (strategy != h->current_strategy) {
        h->current_strategy = strategy;
        h->strategy_changed = true;
        h->strategy_change_counter++;
    }
    
    if (!smooth) {
        /* 立即切换，不使用平滑 */
        h->strategy_changed = false;
    }
}

/**
 * @brief 获取当前调制策略
 */
ModulationStrategy Cmv_GetCurrentStrategy(const Cmv_Handle *h) {
    return h->current_strategy;
}

/**
 * @brief 更新自适应调制参数
 */
void Cmv_UpdateAdaptive(Cmv_Handle *h, float load_measurement, float temperature) {
    /* 简化实现：仅保存负载估计 */
    h->load_estimate = load_measurement;
    
    /* 更复杂的实现可以在这里根据负载和温度动态切换策略 */
    const Cmv_Config *c = h->config;
    
    /* 如果负载低于阈值且温度正常，可以使用更高效的调制策略 */
    if (load_measurement < c->adaptive_threshold && temperature < 80.0f) {
        /* 轻载时切换到DPWM减少开关损耗 */
        if (h->current_strategy != MODULATION_DPWM1) {
            Cmv_SwitchStrategy(h, MODULATION_DPWM1, true);
        }
    } else {
        /* 重载或高温时切换到SVPWM保证性能 */
        if (h->current_strategy != MODULATION_SVPWM) {
            Cmv_SwitchStrategy(h, MODULATION_SVPWM, true);
        }
    }
}

/**
 * @brief 重置模块状态
 */
void Cmv_Reset(Cmv_Handle *h) {
    h->common_mode_voltage = 0.0f;
    h->strategy_changed = false;
    h->strategy_change_counter = 0;
    
    h->duty_a_prev = 0.0f;
    h->duty_b_prev = 0.0f;
    h->duty_c_prev = 0.0f;
    
    h->dpwm_sector = 0;
    h->dpwm_angle = 0.0f;
    
    h->switching_loss_estimate = 0.0f;
    h->conduction_loss_estimate = 0.0f;
}

/**
 * @brief 获取性能统计信息
 */
void Cmv_GetPerformanceStats(const Cmv_Handle *h, 
                            float *switching_loss, float *conduction_loss) {
    if (switching_loss) *switching_loss = h->switching_loss_estimate;
    if (conduction_loss) *conduction_loss = h->conduction_loss_estimate;
}

/**
 * @brief 设置DPWM偏移角度
 */
void Cmv_SetDpwmOffset(Cmv_Handle *h, int dpwm_index, float offset_angle) {
    if (dpwm_index >= 0 && dpwm_index < 4) {
        /* 注意：这里直接修改配置，需要确保配置不是const */
        /* 实际应用中应该避免直接修改配置，或者使用可变配置结构 */
    }
}

/**
 * @brief 获取调制策略名称
 */
const char* Cmv_GetStrategyName(ModulationStrategy strategy) {
    static const char* names[] = {
        "SVPWM",
        "DPWM0",
        "DPWM1",
        "DPWM2",
        "DPWM3",
        "THIPWM",
        "ADAPTIVE"
    };
    
    if (strategy < MODULATION_COUNT) {
        return names[strategy];
    }
    return "UNKNOWN";
}

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 计算SVPWM的共模电压
 * 
 * SVPWM的共模电压通常为0，或者采用最小化开关次数的注入方式。
 * 这里采用经典的SVPWM：v_common = -0.5*(max(duties) + min(duties))
 */
static float calculate_svpwm_cmv(float duty_a, float duty_b, float duty_c) {
    float max_duty = fmaxf(fmaxf(duty_a, duty_b), duty_c);
    float min_duty = fminf(fminf(duty_a, duty_b), duty_c);
    
    /* 将三相占空比平移，使最大最小值的平均值移动到0 */
    float cmv = -0.5f * (max_duty + min_duty);
    
    return cmv;
}

/**
 * @brief 计算THIPWM的共模电压
 * 
 * 三次谐波注入：v_common = -0.5*max(duties) - 0.5*min(duties) 
 *               + factor * sin(3*angle + phase_shift)
 */
static float calculate_thipwm_cmv(float duty_a, float duty_b, float duty_c, 
                                  float angle, float factor) {
    /* 基础SVPWM共模电压 */
    float svpwm_cmv = calculate_svpwm_cmv(duty_a, duty_b, duty_c);
    
    /* 三次谐波注入 */
    float third_harmonic = factor * sinf(3.0f * angle);
    
    /* 总共模电压 */
    float cmv = svpwm_cmv + third_harmonic;
    
    return cmv;
}

/**
 * @brief 计算DPWM的共模电压
 * 
 * DPWM通过在不同扇区注入不同的共模电压，使一相保持为0或1，
 * 从而减少开关次数。
 */
static float calculate_dpwm_cmv(float duty_a, float duty_b, float duty_c,
                                float angle, int dpwm_type, float offset) {
    float cmv = 0.0f;
    
    /* 将角度归一化到[0, 2π) */
    float norm_angle = fmodf(angle + offset, TWO_PI);
    if (norm_angle < 0.0f) norm_angle += TWO_PI;
    
    /* 获取扇区 (0-5) */
    int sector = (int)(norm_angle / PI_DIV_3);
    
    /* 根据不同DPWM类型计算共模电压 */
    switch (dpwm_type) {
        case 0: /* DPWM0: 最大开关损耗降低 */
            /* 在每个扇区将最大相固定为1或最小相固定为0 */
            if (sector < 3) {
                /* 扇区0,1,2: 固定最大相为1 */
                float max_duty = fmaxf(fmaxf(duty_a, duty_b), duty_c);
                cmv = 1.0f - max_duty;
            } else {
                /* 扇区3,4,5: 固定最小相为0 */
                float min_duty = fminf(fminf(duty_a, duty_b), duty_c);
                cmv = -min_duty;
            }
            break;
            
        case 1: /* DPWM1: 平衡开关损耗 */
            /* 根据扇区交替固定最大最小相 */
            if (sector == 0 || sector == 3) {
                float max_duty = fmaxf(fmaxf(duty_a, duty_b), duty_c);
                cmv = 1.0f - max_duty;
            } else if (sector == 1 || sector == 4) {
                float min_duty = fminf(fminf(duty_a, duty_b), duty_c);
                cmv = -min_duty;
            } else { /* sector 2, 5 */
                cmv = calculate_svpwm_cmv(duty_a, duty_b, duty_c);
            }
            break;
            
        case 2: /* DPWM2: 最小化共模电压 */
            /* 更复杂的算法，简化处理 */
            cmv = calculate_svpwm_cmv(duty_a, duty_b, duty_c);
            break;
            
        case 3: /* DPWM3: 最优谐波性能 */
            /* 简化实现 */
            cmv = calculate_svpwm_cmv(duty_a, duty_b, duty_c);
            break;
            
        default:
            cmv = calculate_svpwm_cmv(duty_a, duty_b, duty_c);
            break;
    }
    
    return cmv;
}

/**
 * @brief 应用占空比限制
 */
static void apply_duty_limits(float *duty_a, float *duty_b, float *duty_c,
                             float min_limit, float max_limit) {
    /* 限制每相占空比 */
    if (*duty_a > max_limit) *duty_a = max_limit;
    if (*duty_a < min_limit) *duty_a = min_limit;
    
    if (*duty_b > max_limit) *duty_b = max_limit;
    if (*duty_b < min_limit) *duty_b = min_limit;
    
    if (*duty_c > max_limit) *duty_c = max_limit;
    if (*duty_c < min_limit) *duty_c = min_limit;
}

/**
 * @brief 从角度获取扇区
 */
static int get_sector_from_angle(float angle) {
    /* 将角度归一化到[0, 2π) */
    float norm_angle = fmodf(angle, TWO_PI);
    if (norm_angle < 0.0f) norm_angle += TWO_PI;
    
    /* 计算扇区 (0-5) */
    int sector = (int)(norm_angle / PI_DIV_3);
    if (sector >= 6) sector = 5;
    
    return sector;
}

/**
 * @brief 平滑切换处理
 */
static void smooth_transition(Cmv_Handle *h, float *duty_a, float *duty_b, float *duty_c) {
    /* 简单的一阶低通滤波平滑过渡 */
    float alpha = 0.1f;  /* 平滑系数 */
    
    *duty_a = alpha * (*duty_a) + (1.0f - alpha) * h->duty_a_prev;
    *duty_b = alpha * (*duty_b) + (1.0f - alpha) * h->duty_b_prev;
    *duty_c = alpha * (*duty_c) + (1.0f - alpha) * h->duty_c_prev;
    
    /* 平滑完成后重置标志 */
    static int smooth_counter = 0;
    smooth_counter++;
    if (smooth_counter > 10) {  /* 10个采样周期后完成平滑 */
        h->strategy_changed = false;
        smooth_counter = 0;
    }
}