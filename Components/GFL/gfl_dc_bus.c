#include "gfl_dc_bus.h"
#include <math.h>

/* 母线管理内部状态 */
struct GflDcBus_Internal {
    float Vdc_ref;               /* 母线电压参考 (V) */
    float Vdc_ref_target;        /* 目标母线电压参考 (V) */
    
    /* 故障阈值 */
    float Vdc_over;              /* 过压阈值 (V) */
    float Vdc_under;             /* 欠压阈值 (V) */
    float Vdc_precharge;         /* 预充电目标电压 (V) */
    
    /* 状态 */
    bool precharge_done;
    bool overvoltage_fault;
    bool undervoltage_fault;
    
    /* PI 控制器状态 */
    float integrator;
    float out_prev;
    
    /* 时间 */
    float precharge_timer;
    float Ts;
};

void GflDcBus_Init(void *h, const Gfl_Config *cfg) {
    GflDcBus_Handle *handle = (GflDcBus_Handle *)h;
    
    handle->Ts = cfg->Ts;
    
    /* 初始化母线电压参考 */
    handle->Vdc_ref = 0.0f;
    handle->Vdc_ref_target = cfg->Vdc_bus_nominal;
    
    /* 设置故障阈值 (标称值的 ±10%) */
    handle->Vdc_over = cfg->Vdc_bus_nominal * 1.10f;
    handle->Vdc_under = cfg->Vdc_bus_nominal * 0.90f;
    handle->Vdc_precharge = cfg->Vdc_bus_nominal * 0.95f;
    
    /* 状态初始化 */
    handle->precharge_done = false;
    handle->overvoltage_fault = false;
    handle->undervoltage_fault = false;
    
    /* PI 控制器状态 */
    handle->integrator = 0.0f;
    handle->out_prev = 0.0f;
    
    handle->precharge_timer = 0.0f;
}

void GflDcBus_Step(void *h, float Vdc_bus, float I_dc_bus, 
                   float grid_power, float P_injected) {
    (void)I_dc_bus;
    (void)grid_power;
    (void)P_injected;
    
    GflDcBus_Handle *handle = (GflDcBus_Handle *)h;
    
    /* 更新当前母线电压 */
    /* 检测过压/欠压故障 */
    if (Vdc_bus > handle->Vdc_over) {
        handle->overvoltage_fault = true;
    } else {
        handle->overvoltage_fault = false;
    }
    
    if (Vdc_bus < handle->Vdc_under) {
        handle->undervoltage_fault = true;
    } else {
        handle->undervoltage_fault = false;
    }
    
    /* 预充电阶段 */
    if (!handle->precharge_done) {
        /* 预充电阶段：缓慢提升母线电压 */
        handle->precharge_timer += handle->Ts;
        
        /* 简单的预充电斜坡控制 */
        float precharge_rate = 50.0f; /* V/s */
        float Vdc_target = precharge_rate * handle->precharge_timer;
        
        if (Vdc_target > handle->Vdc_precharge) {
            Vdc_target = handle->Vdc_precharge;
        }
        
        /* 检查是否达到预充电目标 */
        if (Vdc_bus >= handle->Vdc_precharge * 0.98f) {
            handle->precharge_done = true;
            handle->Vdc_ref = handle->Vdc_ref_target;
        } else {
            handle->Vdc_ref = Vdc_target;
        }
    } else {
        /* 正常运行阶段：使用 PI 控制器稳定母线电压 */
        /* 这部分通常由外层环控制，这里仅做监控 */
        handle->Vdc_ref = handle->Vdc_ref_target;
    }
}

float GflDcBus_GetRef(const void *h) {
    const GflDcBus_Handle *handle = (const GflDcBus_Handle *)h;
    return handle->Vdc_ref;
}

void GflDcBus_GetState(const void *h, GflDcBus_State *state) {
    const GflDcBus_Handle *handle = (const GflDcBus_Handle *)h;
    
    state->Vdc_ref = handle->Vdc_ref;
    state->precharge_done = handle->precharge_done;
    state->overvoltage_fault = handle->overvoltage_fault;
    state->undervoltage_fault = handle->undervoltage_fault;
}

Gfl_FaultType GflDcBus_CheckFault(const void *h) {
    const GflDcBus_Handle *handle = (const GflDcBus_Handle *)h;
    
    if (handle->overvoltage_fault) {
        return GFL_FAULT_DC_BUS_OVER;
    }
    
    if (handle->undervoltage_fault) {
        return GFL_FAULT_DC_BUS_UNDER;
    }
    
    return GFL_FAULT_NONE;
}

void GflDcBus_SetRef(void *h, float Vdc_ref) {
    GflDcBus_Handle *handle = (GflDcBus_Handle *)h;
    handle->Vdc_ref_target = Vdc_ref;
    
    if (handle->precharge_done) {
        handle->Vdc_ref = Vdc_ref;
    }
}

void GflDcBus_StartPrecharge(void *h) {
    GflDcBus_Handle *handle = (GflDcBus_Handle *)h;
    handle->precharge_done = false;
    handle->precharge_timer = 0.0f;
    handle->Vdc_ref = 0.0f;
}

bool GflDcBus_IsPrechargeDone(const void *h) {
    const GflDcBus_Handle *handle = (const GflDcBus_Handle *)h;
    return handle->precharge_done;
}