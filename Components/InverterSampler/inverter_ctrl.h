#pragma once

#include "inverter_types.h"
#include "inverter_state.h"
#include "inverter_sampler.h"

#define INV_SAMPLER_DECLARE(name) \
    SAMPLER_DECLARE_HANDLE(name)

#define INV_SAMPLER_DECLARE_CONFIG(cfg) \
    SAMPLER_DECLARE_CONFIG(cfg)

#define INV_TASK_MAX_STACK_SIZE   256

typedef struct {
    Inv_StateMachine_t state_machine;
    Sampler_Handle sampler;
    uint32_t control_period_us;
    uint32_t startup_delay_ms;
    uint32_t fault_recover_delay_ms;
    float vbus_min;
    float vbus_max;
    float i_max;
    float temp_max;
} Inv_Handle;

typedef struct {
    Sampler_Config sampler;
    uint32_t control_period_us;
    uint32_t startup_delay_ms;
    uint32_t fault_recover_delay_ms;
    float vbus_min;
    float vbus_max;
    float i_max;
    float temp_max;
    uint8_t fault_mask;
} Inv_Config;

#define INV_DECLARE_HANDLE(name) \
    static Inv_Handle name

#define INV_DECLARE_CONFIG(cfg) \
    static const Inv_Config cfg

void Inv_Init(Inv_Handle *h, const Inv_Config *cfg);

void Inv_Start(Inv_Handle *h);

void Inv_Stop(Inv_Handle *h);

void Inv_Shutdown(Inv_Handle *h);

void Inv_Recover(Inv_Handle *h);

void Inv_FaultSet(Inv_Handle *h, uint8_t fault_id);

void Inv_FaultClear(Inv_Handle *h, uint8_t fault_id);

void Inv_FaultClearAll(Inv_Handle *h);

uint8_t Inv_HasFault(const Inv_Handle *h);

Inv_State_t Inv_GetState(const Inv_Handle *h);

const char* Inv_GetStateName(const Inv_Handle *h);

float Inv_GetI_A(const Inv_Handle *h);

float Inv_GetI_B(const Inv_Handle *h);

float Inv_GetI_C(const Inv_Handle *h);

float Inv_GetV_Bus(const Inv_Handle *h);

float Inv_GetV_Out(const Inv_Handle *h);

float Inv_GetTemp(const Inv_Handle *h);

void Inv_OnAdcBuf0(Inv_Handle *h, uint32_t timestamp);

void Inv_OnAdcBuf1(Inv_Handle *h, uint32_t timestamp);

void Inv_OnDmaFault(Inv_Handle *h, uint32_t timestamp, uint8_t fault_id);

void Inv_ControlTask_1ms(Inv_Handle *h);

void Inv_ControlTask_5ms(Inv_Handle *h);

void Inv_ControlTask_10ms(Inv_Handle *h);

void Inv_ControlTask_50ms(Inv_Handle *h);

void Inv_ControlTask_1000ms(Inv_Handle *h);
