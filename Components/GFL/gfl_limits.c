#include "gfl_limits.h"
#include <math.h>

/* 内部限幅状态 */
struct GflLimits_Internal {
    Gfl_CurrentLimits limits;
    float rated_current;
    float Vdc_bus_nominal;
    float voltage_depression;  /* 电压跌落系数 */
};

void GflLimits_Init(void *h, const Gfl_Config *cfg) {
    GflLimits_Handle *handle = (GflLimits_Handle *)h;
    
    /* 从配置初始化限幅值 */
    handle->limits = cfg->current_limits;
    handle->rated_current = cfg->rated_current;
    handle->Vdc_bus_nominal = cfg->Vdc_bus_nominal;
    handle->voltage_depression = 1.0f;
}

void GflLimits_Update(void *h, float grid_voltage, float Vdc_bus, float P, float Q) {
    (void)P;
    (void)Q;
    
    GflLimits_Handle *handle = (GflLimits_Handle *)h;
    
    /* 计算电压跌落系数 (用于低电压时的电流限幅调整) */
    /* 当电网电压跌落时，为了维持功率输出，需要增大电流 */
    if (grid_voltage > 0.01f) {
        handle->voltage_depression = 1.0f / grid_voltage;
    } else {
        handle->voltage_depression = 1.0f;
    }
    
    /* 限制电压跌落系数，防止过大的电流 */
    if (handle->voltage_depression > 1.5f) {
        handle->voltage_depression = 1.5f;
    }
    
    /* 母线电压低时也需要限制电流输出能力 */
    float Vdc_ratio = handle->Vdc_bus_nominal / Vdc_bus;
    if (Vdc_ratio > 1.0f) {
        /* 母线电压低于标称值，限制电流输出 */
        handle->voltage_depression *= Vdc_ratio;
        if (handle->voltage_depression > 1.5f) {
            handle->voltage_depression = 1.5f;
        }
    }
}

void GflLimits_Apply(void *h, float Id_ref, float Iq_ref, 
                     float *Id_out, float *Iq_out) {
    GflLimits_Handle *handle = (GflLimits_Handle *)h;
    
    float Id = Id_ref;
    float Iq = Iq_ref;
    
    /* 计算电流幅值 */
    float I_mag = sqrtf(Id * Id + Iq * Iq);
    
    /* 检查是否超出圆限幅 */
    if (I_mag > handle->limits.I_max) {
        /* 需要限幅，按比例缩小 */
        float scale = handle->limits.I_max / I_mag;
        Id *= scale;
        Iq *= scale;
    }
    
    /* 应用 d轴限幅 */
    if (Id > handle->limits.Id_max_pos) {
        Id = handle->limits.Id_max_pos;
    }
    if (Id < handle->limits.Id_max_neg) {
        Id = handle->limits.Id_max_neg;
    }
    
    /* 应用 q轴限幅 (可能需要重新计算，因为 d轴占用了部分电流裕量) */
    float I_d_margin = handle->limits.Id_max_pos - Id;
    float Iq_abs = (Iq > 0.0f) ? Iq : -Iq;
    
    if (Iq_abs > handle->limits.Iq_max) {
        Iq = (Iq > 0.0f) ? handle->limits.Iq_max : -handle->limits.Iq_max;
    }
    
    /* 确保总电流不超出圆限幅 */
    I_mag = sqrtf(Id * Id + Iq * Iq);
    if (I_mag > handle->limits.I_max) {
        /* 如果超出，需要重新分配 d/q 轴电流 */
        /* 优先保证 q轴 (无功)，限制 d轴 */
        float Iq_max_safe = sqrtf(handle->limits.I_max * handle->limits.I_max - Iq * Iq);
        if (Iq_max_safe < Iq_abs) {
            /* 需要减小 q轴电流 */
            Iq = (Iq > 0.0f) ? Iq_max_safe : -Iq_max_safe;
        }
        
        /* 重新计算 d轴 */
        Id = sqrtf(handle->limits.I_max * handle->limits.I_max - Iq * Iq);
        if (Id_ref < 0.0f) {
            Id = -Id;  /* 保持原始符号 */
        }
    }
    
    *Id_out = Id;
    *Iq_out = Iq;
}

void GflLimits_CalcPowerLimits(void *h, float grid_voltage, float Vdc_bus,
                               float *P_max_out, float *Q_max_out) {
    GflLimits_Handle *handle = (GflLimits_Handle *)h;
    (void)Vdc_bus;
    
    /* 
     * 计算最大可输出功率
     * 
     * S_max = V * I_max
     * P_max = S_max * cos(φ) = S_max * (Id / I_max) = V * Id
     * Q_max = S_max * sin(φ) = S_max * (Iq / I_max) = V * Iq
     * 
     * 在 d轴优先模式下: Id = I_max * cos(φ), Iq = I_max * sin(φ)
     */
    
    /* 考虑电网电压的影响 */
    float V = (grid_voltage > 0.01f) ? grid_voltage : 1.0f;
    
    /* 最大视在功率 */
    float S_max = V * handle->limits.I_max;
    
    /* 假设功率因数为 1 (纯有功) */
    *P_max_out = S_max;
    *Q_max_out = 0.0f;
}

void GflLimits_Get(const void *h, Gfl_CurrentLimits *limits) {
    const GflLimits_Handle *handle = (const GflLimits_Handle *)h;
    *limits = handle->limits;
}