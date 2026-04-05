#pragma once

#include <stdint.h>

typedef enum {
    INV_STATE_INIT = 0,
    INV_STATE_IDLE,
    INV_STATE_START,
    INV_STATE_RUN,
    INV_STATE_SHUTDOWN
} Inv_State_t;

typedef enum {
    INV_CMD_NONE = 0,
    INV_CMD_START,
    INV_CMD_STOP,
    INV_CMD_FAULT_CLEAR,
    INV_CMD_SHUTDOWN,
    INV_CMD_RECOVER
} Inv_Cmd_t;

typedef uint8_t (*Inv_StateHandler_t)(void);

typedef struct {
    Inv_State_t current;
    Inv_State_t previous;
    uint32_t state_entry_tick;
    uint32_t error_count;
    uint32_t fault_bits;
    uint8_t fault_mask;
} Inv_StateMachine_t;

#define INV_STATE_DECLARE(name) \
    static Inv_StateMachine_t name

#define INV_FAULT_OVERCURRENT    (1U << 0)
#define INV_FAULT_OVERVOLTAGE    (1U << 1)
#define INV_FAULT_UNDERVOLTAGE   (1U << 2)
#define INV_FAULT_OVERTEMP       (1U << 3)
#define INV_FAULT_DC_BUS_UV      (1U << 4)
#define INV_FAULT_DC_BUS_OV      (1U << 5)

void Inv_StateMachine_Init(Inv_StateMachine_t *sm);

void Inv_StateMachine_SetFault(Inv_StateMachine_t *sm, uint8_t fault_id);

void Inv_StateMachine_ClearFault(Inv_StateMachine_t *sm, uint8_t fault_id);

void Inv_StateMachine_ClearAllFaults(Inv_StateMachine_t *sm);

uint8_t Inv_StateMachine_HasFault(const Inv_StateMachine_t *sm);

void Inv_StateMachine_ProcessCommand(Inv_StateMachine_t *sm, Inv_Cmd_t cmd);

void Inv_StateMachine_Run(Inv_StateMachine_t *sm);

Inv_State_t Inv_StateMachine_GetState(const Inv_StateMachine_t *sm);

uint32_t Inv_StateMachine_GetStateDuration(const Inv_StateMachine_t *sm);

const char* Inv_StateMachine_StateName(Inv_State_t state);

uint8_t Inv_StateMachine_IsRunnable(const Inv_StateMachine_t *sm);

#define INV_STATE_ENTRY_TICK(sm)     ((sm)->state_entry_tick)
#define INV_STATE_ERROR_COUNT(sm)    ((sm)->error_count)
#define INV_STATE_FAULT_BITS(sm)     ((sm)->fault_bits)
