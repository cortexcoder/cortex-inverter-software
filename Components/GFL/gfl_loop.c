#include "gfl_loop.h"
#include <math.h>
#include <stdlib.h>

/* GFL 环路内部状态 */
typedef struct {
    Gfl_Handle base;
    
    /* 子模块句柄 */
    GflLimits_Handle limits;
    GflDcBus_Handle dc_bus;
    GflRideThrough_Handle ridethrough;
    GflWeakGrid_Handle weak_grid;
    
    /* PLL 状态 */
    Gfl_GridState grid_state;
    
    /* 电网状态 */
    float grid_voltage_pu;
    
} GflLoop_Handle_t;

void GflLoop_Init(void *h, const Gfl_Config *cfg) {
    GflLoop_Handle_t *handle = (GflLoop_Handle_t *)h;
    
    /* 初始化基类 */
    handle->base.mode = GFL_MODE_IDLE;
    handle->base.fault = GFL_FAULT_NONE;
    handle->base.rt_state = GFL_RT_NORMAL;
    handle->base.init = true;
    
    /* 初始化子模块 */
    GflLimits_Init(&handle->limits, cfg);
    GflDcBus_Init(&handle->dc_bus, cfg);
    GflRideThrough_Init(&handle->ridethrough, cfg);
    GflWeakGrid_Init(&handle->weak_grid, cfg);
}

void GflLoop_Step(void *h,
                  float v_alpha, float v_beta, float Vdc_bus,
                  float P_ref, float Q_ref,
                  GflLoop_Output *output) {
    GflLoop_Handle_t *handle = (GflLoop_Handle_t *)h;
    
    if (!handle->base.init) {
        output->Id_ref = 0.0f;
        output->Iq_ref = 0.0f;
        output->theta = 0.0f;
        output->freq = 50.0f;
        output->grid_ready = false;
        return;
    }
    
    if (handle->base.mode == GFL_MODE_IDLE || 
        handle->base.mode == GFL_MODE_FAULT) {
        output->Id_ref = 0.0f;
        output->Iq_ref = 0.0f;
        output->grid_ready = false;
        return;
    }
    
    /* ========== 1. 电网状态获取 ========== */
    float v_mag = sqrtf(v_alpha * v_alpha + v_beta * v_beta);
    float V_nominal = 311.0f; /* 假设额定相电压峰值 */
    handle->grid_voltage_pu = v_mag / V_nominal;
    
    handle->grid_state.v_alpha = v_alpha;
    handle->grid_state.v_beta = v_beta;
    handle->grid_state.amplitude = handle->grid_voltage_pu;
    
    /* ========== 2. 母线管理 ========== */
    GflDcBus_Step(&handle->dc_bus, Vdc_bus, 0.0f, 0.0f, 0.0f);
    float Vdc_ref = GflDcBus_GetRef(&handle->dc_bus);
    (void)Vdc_ref;
    
    Gfl_FaultType dc_fault = GflDcBus_CheckFault(&handle->dc_bus);
    if (dc_fault != GFL_FAULT_NONE) {
        handle->base.fault = dc_fault;
        handle->base.mode = GFL_MODE_FAULT;
    }
    
    /* ========== 3. 功率指令转电流指令 ========== */
    float Id_from_P = P_ref / (handle->grid_voltage_pu > 0.1f ? handle->grid_voltage_pu : 1.0f);
    float Iq_from_Q = Q_ref / (handle->grid_voltage_pu > 0.1f ? handle->grid_voltage_pu : 1.0f);
    
    /* ========== 4. 高低穿处理 ========== */
    GflRideThrough_Output rt_output;
    GflRideThrough_Step(&handle->ridethrough,
                       handle->grid_voltage_pu,
                       Id_from_P, Iq_from_Q,
                       P_ref,
                       &rt_output);
    
    handle->base.rt_state = rt_output.state;
    
    if (rt_output.trip_request) {
        handle->base.fault = GFL_FAULT_GRID_UNBALANCE;
        handle->base.mode = GFL_MODE_FAULT;
    }
    
    float Id_ref = rt_output.Id_ref_ramp;
    float Iq_ref = rt_output.Iq_ref_grid_code;
    
    /* ========== 5. 弱网检测 ========== */
    float I_mag = sqrtf(Id_ref * Id_ref + Iq_ref * Iq_ref);
    GflWeakGrid_Output wg_output;
    GflWeakGrid_Step(&handle->weak_grid,
                     handle->grid_voltage_pu, 50.0f,
                     I_mag, P_ref, Q_ref,
                     handle->grid_state.locked,
                     &wg_output);
    
    handle->base.weak_grid.Z_grid = wg_output.Z_grid_estimated;
    output->grid_ready = wg_output.grid_connected;
    
    /* ========== 6. 电流限幅 ========== */
    GflLimits_Update(&handle->limits, handle->grid_voltage_pu, Vdc_bus, P_ref, Q_ref);
    GflLimits_Apply(&handle->limits, Id_ref, Iq_ref, &Id_ref, &Iq_ref);
    
    /* ========== 7. 功率限幅校验 ========== */
    float P_max, Q_max;
    GflLimits_CalcPowerLimits(&handle->limits, handle->grid_voltage_pu, Vdc_bus,
                             &P_max, &Q_max);
    
    if (P_ref > P_max) P_ref = P_max;
    if (Q_ref > Q_max) Q_ref = Q_max;
    
    /* ========== 8. 输出 ========== */
    output->Id_ref = Id_ref;
    output->Iq_ref = Iq_ref;
    output->theta = handle->grid_state.theta;
    output->freq = handle->grid_state.freq;
    
    handle->base.current_ref.Id_ref = Id_ref;
    handle->base.current_ref.Iq_ref = Iq_ref;
    handle->base.power_ref.P_ref = P_ref;
    handle->base.power_ref.Q_ref = Q_ref;
}

Gfl_Mode GflLoop_GetMode(const void *h) {
    const GflLoop_Handle_t *handle = (const GflLoop_Handle_t *)h;
    return handle->base.mode;
}

Gfl_FaultType GflLoop_GetFault(const void *h) {
    const GflLoop_Handle_t *handle = (const GflLoop_Handle_t *)h;
    return handle->base.fault;
}

void GflLoop_ClearFault(void *h) {
    GflLoop_Handle_t *handle = (GflLoop_Handle_t *)h;
    handle->base.fault = GFL_FAULT_NONE;
    handle->base.mode = GFL_MODE_IDLE;
    GflRideThrough_Reset(&handle->ridethrough);
}

void GflLoop_Start(void *h) {
    GflLoop_Handle_t *handle = (GflLoop_Handle_t *)h;
    
    if (handle->base.fault != GFL_FAULT_NONE) {
        return;
    }
    
    handle->base.mode = GFL_MODE_RUNNING;
    GflDcBus_StartPrecharge(&handle->dc_bus);
}

void GflLoop_Stop(void *h) {
    GflLoop_Handle_t *handle = (GflLoop_Handle_t *)h;
    handle->base.mode = GFL_MODE_STOPPING;
}

void GflLoop_GetDcBusState(const void *h, GflDcBus_State *state) {
    const GflLoop_Handle_t *handle = (const GflLoop_Handle_t *)h;
    GflDcBus_GetState(&handle->dc_bus, state);
}

void GflLoop_GetWeakGridState(const void *h, GflWeakGrid_Output *output) {
    const GflLoop_Handle_t *handle = (const GflLoop_Handle_t *)h;
    output->level = GflWeakGrid_GetLevel(&handle->weak_grid);
    output->Z_grid_estimated = GflWeakGrid_GetImpedance(&handle->weak_grid);
    output->grid_connected = GflWeakGrid_IsGridReady(&handle->weak_grid);
}

Gfl_RideThroughState GflLoop_GetRideThroughState(const void *h) {
    const GflLoop_Handle_t *handle = (const GflLoop_Handle_t *)h;
    return handle->base.rt_state;
}