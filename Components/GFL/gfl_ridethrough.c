#include "gfl_ridethrough.h"
#include <math.h>

/* 高低穿内部状态 */
struct GflRideThrough_Internal {
    Gfl_RideThroughState state;
    Gfl_RideThroughConfig config;
    
    /* 状态机 */
    float state_timer;
    float hold_time;
    
    /* 斜率限制 */
    float Id_ref_prev;
    float dI_dt_max;        /* 最大电流变化率 (pu/s) */
    
    /* 故障 */
    bool trip_request;
    
    /* 欠压/过压计数 */
    uint32_t undervoltage_count;
    uint32_t overvoltage_count;
    
    float Ts;
};

void GflRideThrough_Init(void *h, const Gfl_Config *cfg) {
    GflRideThrough_Handle *handle = (GflRideThrough_Handle *)h;
    
    handle->config = cfg->ridethrough;
    handle->Ts = cfg->Ts;
    
    handle->state = GFL_RT_NORMAL;
    handle->state_timer = 0.0f;
    handle->hold_time = 0.0f;
    handle->Id_ref_prev = 0.0f;
    handle->dI_dt_max = 2.0f; /* 2 pu/s 电流变化率限制 */
    handle->trip_request = false;
    handle->undervoltage_count = 0;
    handle->overvoltage_count = 0;
}

void GflRideThrough_Step(void *h, float grid_voltage, 
                         float Id_ref_input, float Iq_ref_input,
                         float P_active,
                         GflRideThrough_Output *output) {
    GflRideThrough_Handle *handle = (GflRideThrough_Handle *)h;
    (void)P_active;
    
    output->state = handle->state;
    
    /* 根据电压判断状态 */
    switch (handle->state) {
        case GFL_RT_NORMAL: {
            /* 正常状态：检测电压是否越限 */
            if (grid_voltage < handle->config.lvrt_voltage_min) {
                /* 进入 LVRT */
                handle->state = GFL_RT_LVRT;
                handle->state_timer = 0.0f;
                handle->hold_time = handle->config.lvrt_time_min;
            } else if (grid_voltage > handle->config.hvrt_voltage_max) {
                /* 进入 HVRT */
                handle->state = GFL_RT_HVRT;
                handle->state_timer = 0.0f;
                handle->hold_time = handle->config.hvrt_time_min;
            }
            
            /* 正常状态：直接输出原始参考 */
            output->Id_ref_ramp = Id_ref_input;
            output->Iq_ref_grid_code = Iq_ref_input;
            output->trip_request = false;
            break;
        }
        
        case GFL_RT_LVRT: {
            /* 低压穿越状态 */
            handle->state_timer += handle->Ts;
            
            /* LVRT 逻辑：
             * 1. 电压跌落时，需要注入无功电流支撑电网
             * 2. 无功电流参考: I_q = k * (1 - V)
             * 3. 有功电流需要限制，以防止过流
             */
            float V_diff = 1.0f - grid_voltage;
            if (V_diff < 0.0f) V_diff = 0.0f;
            
            /* 电网规范要求的无功电流 */
            output->Iq_ref_grid_code = handle->config.reactive_current_k * V_diff;
            
            /* 限制 q轴电流最大值 */
            if (output->Iq_ref_grid_code > 1.0f) {
                output->Iq_ref_grid_code = 1.0f;
            }
            
            /* 有功电流需要缩减 (因为总电流有限) */
            float I_total_max = 1.0f; /* pu */
            float Iq_sq = output->Iq_ref_grid_code * output->Iq_ref_grid_code;
            float Id_max_available = sqrtf(I_total_max * I_total_max - Iq_sq);
            
            /* 斜率限制 */
            float Id_ramp_limited = Id_ref_input;
            float dI = Id_ramp_limited - handle->Id_ref_prev;
            float dI_max = handle->dI_dt_max * handle->Ts;
            
            if (dI > dI_max) {
                Id_ramp_limited = handle->Id_ref_prev + dI_max;
            } else if (dI < -dI_max) {
                Id_ramp_limited = handle->Id_ref_prev - dI_max;
            }
            
            /* 应用可用电流限制 */
            if (Id_ramp_limited > Id_max_available) {
                Id_ramp_limited = Id_max_available;
            } else if (Id_ramp_limited < -Id_max_available) {
                Id_ramp_limited = -Id_max_available;
            }
            
            output->Id_ref_ramp = Id_ramp_limited;
            handle->Id_ref_prev = Id_ramp_limited;
            
            /* 检查是否恢复 */
            if (handle->state_timer >= handle->hold_time) {
                if (grid_voltage >= handle->config.lvrt_voltage_min * 1.05f) {
                    handle->state = GFL_RT_NORMAL;
                    handle->trip_request = false;
                }
            }
            
            /* LVRT 期间持续时间超限则跳闸 */
            if (handle->state_timer > 20.0f) { /* 20秒最大 LVRT */
                output->trip_request = true;
                handle->trip_request = true;
            }
            
            break;
        }
        
        case GFL_RT_HVRT: {
            /* 高压穿越状态 */
            handle->state_timer += handle->Ts;
            
            /* HVRT 逻辑：
             * 1. 电压抬升时，需要吸收无功电流
             * 2. 限制有功输出
             */
            float V_diff = grid_voltage - 1.0f;
            if (V_diff < 0.0f) V_diff = 0.0f;
            
            /* 吸收无功电流 */
            output->Iq_ref_grid_code = -handle->config.reactive_current_k * V_diff;
            
            /* 限制 q轴电流 */
            if (output->Iq_ref_grid_code < -1.0f) {
                output->Iq_ref_grid_code = -1.0f;
            }
            
            /* 有功电流缩减 */
            float I_total_max = 1.0f;
            float Iq_sq = output->Iq_ref_grid_code * output->Iq_ref_grid_code;
            float Id_max_available = sqrtf(I_total_max * I_total_max - Iq_sq);
            
            float Id_ramp_limited = Id_ref_input;
            float dI = Id_ramp_limited - handle->Id_ref_prev;
            float dI_max = handle->dI_dt_max * handle->Ts;
            
            if (dI > dI_max) {
                Id_ramp_limited = handle->Id_ref_prev + dI_max;
            } else if (dI < -dI_max) {
                Id_ramp_limited = handle->Id_ref_prev - dI_max;
            }
            
            if (Id_ramp_limited > Id_max_available) {
                Id_ramp_limited = Id_max_available;
            } else if (Id_ramp_limited < -Id_max_available) {
                Id_ramp_limited = -Id_max_available;
            }
            
            output->Id_ref_ramp = Id_ramp_limited;
            handle->Id_ref_prev = Id_ramp_limited;
            
            /* 检查是否恢复 */
            if (handle->state_timer >= handle->hold_time) {
                if (grid_voltage <= handle->config.hvrt_voltage_max * 0.95f) {
                    handle->state = GFL_RT_NORMAL;
                    handle->trip_request = false;
                }
            }
            
            break;
        }
    }
    
    output->voltage_threshold = (handle->state == GFL_RT_LVRT) ? 
                               handle->config.lvrt_voltage_min :
                               handle->config.hvrt_voltage_max;
    output->hold_time_remaining = handle->hold_time - handle->state_timer;
}

Gfl_RideThroughState GflRideThrough_GetState(const void *h) {
    const GflRideThrough_Handle *handle = (const GflRideThrough_Handle *)h;
    return handle->state;
}

bool GflRideThrough_IsTripRequested(const void *h) {
    const GflRideThrough_Handle *handle = (const GflRideThrough_Handle *)h;
    return handle->trip_request;
}

void GflRideThrough_Reset(void *h) {
    GflRideThrough_Handle *handle = (GflRideThrough_Handle *)h;
    handle->state = GFL_RT_NORMAL;
    handle->state_timer = 0.0f;
    handle->hold_time = 0.0f;
    handle->trip_request = false;
    handle->Id_ref_prev = 0.0f;
}

float GflRideThrough_GetReactiveCurrentRef(const void *h, float grid_voltage) {
    const GflRideThrough_Handle *handle = (const GflRideThrough_Handle *)h;
    
    float V_diff = 1.0f - grid_voltage;
    if (V_diff < 0.0f) V_diff = 0.0f;
    
    float Iq = handle->config.reactive_current_k * V_diff;
    
    if (Iq > 1.0f) Iq = 1.0f;
    if (Iq < -1.0f) Iq = -1.0f;
    
    return Iq;
}