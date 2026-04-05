#include "inverter_ctrl.h"
#include "inverter_types.h"

extern void Sampler_Init(Sampler_Handle *h, const Sampler_Config *cfg);
extern void Sampler_OnDmaBuf0(Sampler_Handle *h, uint32_t timestamp);
extern void Sampler_OnDmaBuf1(Sampler_Handle *h, uint32_t timestamp);
extern void Sampler_OnFault(Sampler_Handle *h, uint32_t timestamp, uint8_t fault_id);

void Inv_Init(Inv_Handle *h, const Inv_Config *cfg) {
    Inv_StateMachine_Init(&h->state_machine);
    Sampler_Init(&h->sampler, &cfg->sampler);
    h->control_period_us = cfg->control_period_us;
    h->startup_delay_ms = cfg->startup_delay_ms;
    h->fault_recover_delay_ms = cfg->fault_recover_delay_ms;
    h->vbus_min = cfg->vbus_min;
    h->vbus_max = cfg->vbus_max;
    h->i_max = cfg->i_max;
    h->temp_max = cfg->temp_max;
}

void Inv_Start(Inv_Handle *h) {
    Inv_StateMachine_ProcessCommand(&h->state_machine, INV_CMD_START);
}

void Inv_Stop(Inv_Handle *h) {
    Inv_StateMachine_ProcessCommand(&h->state_machine, INV_CMD_STOP);
}

void Inv_Shutdown(Inv_Handle *h) {
    Inv_StateMachine_ProcessCommand(&h->state_machine, INV_CMD_SHUTDOWN);
}

void Inv_Recover(Inv_Handle *h) {
    Inv_StateMachine_ProcessCommand(&h->state_machine, INV_CMD_RECOVER);
}

void Inv_FaultSet(Inv_Handle *h, uint8_t fault_id) {
    Inv_StateMachine_SetFault(&h->state_machine, fault_id);
    Inv_StateMachine_ProcessCommand(&h->state_machine, INV_CMD_SHUTDOWN);
}

void Inv_FaultClear(Inv_Handle *h, uint8_t fault_id) {
    Inv_StateMachine_ClearFault(&h->state_machine, fault_id);
}

void Inv_FaultClearAll(Inv_Handle *h) {
    Inv_StateMachine_ClearAllFaults(&h->state_machine);
}

uint8_t Inv_HasFault(const Inv_Handle *h) {
    return Inv_StateMachine_HasFault(&h->state_machine);
}

Inv_State_t Inv_GetState(const Inv_Handle *h) {
    return Inv_StateMachine_GetState(&h->state_machine);
}

const char* Inv_GetStateName(const Inv_Handle *h) {
    return Inv_StateMachine_StateName(Inv_GetState(h));
}

float Inv_GetI_A(const Inv_Handle *h) {
    return Sampler_GetI_A_Avg(&h->sampler);
}

float Inv_GetI_B(const Inv_Handle *h) {
    return Sampler_GetI_B_Avg(&h->sampler);
}

float Inv_GetI_C(const Inv_Handle *h) {
    return Sampler_GetI_C_Avg(&h->sampler);
}

float Inv_GetV_Bus(const Inv_Handle *h) {
    return Sampler_GetV_Bus_Avg(&h->sampler);
}

float Inv_GetV_Out(const Inv_Handle *h) {
    return Sampler_GetV_Out_Avg(&h->sampler);
}

float Inv_GetTemp(const Inv_Handle *h) {
    return Sampler_GetTemp_Avg(&h->sampler);
}

void Inv_OnAdcBuf0(Inv_Handle *h, uint32_t timestamp) {
    Sampler_OnDmaBuf0(&h->sampler, timestamp);
    Sampler_ProcessBuffer(&h->sampler);
}

void Inv_OnAdcBuf1(Inv_Handle *h, uint32_t timestamp) {
    Sampler_OnDmaBuf1(&h->sampler, timestamp);
    Sampler_ProcessBuffer(&h->sampler);
}

void Inv_OnDmaFault(Inv_Handle *h, uint32_t timestamp, uint8_t fault_id) {
    (void)h;
    Sampler_OnFault(&h->sampler, timestamp, fault_id);
    Inv_FaultSet(h, fault_id);
}

void Inv_ControlTask_1ms(Inv_Handle *h) {
    (void)h;
}

void Inv_ControlTask_5ms(Inv_Handle *h) {
    Inv_StateMachine_Run(&h->state_machine);

    float vbus = Inv_GetV_Bus(h);
    float i_a = Inv_GetI_A(h);
    float i_b = Inv_GetI_B(h);
    float i_c = Inv_GetI_C(h);
    float temp = Inv_GetTemp(h);

    if (vbus > h->vbus_max || vbus < h->vbus_min) {
        if (vbus > h->vbus_max) {
            Inv_FaultSet(h, INV_FAULT_OVERVOLTAGE);
        } else {
            Inv_FaultSet(h, INV_FAULT_UNDERVOLTAGE);
        }
        return;
    }

    if (i_a > h->i_max || i_b > h->i_max || i_c > h->i_max) {
        Inv_FaultSet(h, INV_FAULT_OVERCURRENT);
        return;
    }

    if (temp > h->temp_max) {
        Inv_FaultSet(h, INV_FAULT_OVERTEMP);
        return;
    }
}

void Inv_ControlTask_10ms(Inv_Handle *h) {
    (void)h;
}

void Inv_ControlTask_50ms(Inv_Handle *h) {
    (void)h;
}

void Inv_ControlTask_1000ms(Inv_Handle *h) {
    (void)h;
}
