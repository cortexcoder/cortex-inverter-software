#include "inverter_state.h"
#include "inverter_types.h"

extern uint32_t TimerISR_GetTick(void);

static const char* s_state_names[] = {
    "INIT",
    "IDLE", 
    "START",
    "RUN",
    "SHUTDOWN"
};

void Inv_StateMachine_Init(Inv_StateMachine_t *sm) {
    sm->current = INV_STATE_INIT;
    sm->previous = INV_STATE_INIT;
    sm->state_entry_tick = TimerISR_GetTick();
    sm->error_count = 0;
    sm->fault_bits = 0;
    sm->fault_mask = 0xFF;
}

void Inv_StateMachine_SetFault(Inv_StateMachine_t *sm, uint8_t fault_id) {
    if (fault_id < 8) {
        sm->fault_bits |= (1U << fault_id);
        sm->error_count++;
    }
}

void Inv_StateMachine_ClearFault(Inv_StateMachine_t *sm, uint8_t fault_id) {
    if (fault_id < 8) {
        sm->fault_bits &= ~(1U << fault_id);
    }
}

void Inv_StateMachine_ClearAllFaults(Inv_StateMachine_t *sm) {
    sm->fault_bits = 0;
}

uint8_t Inv_StateMachine_HasFault(const Inv_StateMachine_t *sm) {
    return (sm->fault_bits & sm->fault_mask) != 0;
}

Inv_State_t Inv_StateMachine_GetState(const Inv_StateMachine_t *sm) {
    return sm->current;
}

uint32_t Inv_StateMachine_GetStateDuration(const Inv_StateMachine_t *sm) {
    return TimerISR_GetTick() - sm->state_entry_tick;
}

const char* Inv_StateMachine_StateName(Inv_State_t state) {
    if (state >= sizeof(s_state_names) / sizeof(s_state_names[0])) {
        return "UNKNOWN";
    }
    return s_state_names[state];
}

uint8_t Inv_StateMachine_IsRunnable(const Inv_StateMachine_t *sm) {
    return sm->current == INV_STATE_RUN;
}

static void TransitionTo(Inv_StateMachine_t *sm, Inv_State_t new_state) {
    if (new_state == sm->current) {
        return;
    }
    sm->previous = sm->current;
    sm->current = new_state;
    sm->state_entry_tick = TimerISR_GetTick();
}

void Inv_StateMachine_ProcessCommand(Inv_StateMachine_t *sm, Inv_Cmd_t cmd) {
    Inv_State_t curr = sm->current;
    uint8_t has_fault = Inv_StateMachine_HasFault(sm);

    switch (curr) {
        case INV_STATE_INIT:
            if (cmd == INV_CMD_NONE) {
                if (!has_fault) {
                    TransitionTo(sm, INV_STATE_IDLE);
                }
            }
            break;

        case INV_STATE_IDLE:
            if (cmd == INV_CMD_START) {
                if (!has_fault) {
                    TransitionTo(sm, INV_STATE_START);
                }
            } else if (cmd == INV_CMD_SHUTDOWN || has_fault) {
                TransitionTo(sm, INV_STATE_SHUTDOWN);
            }
            break;

        case INV_STATE_START:
            if (cmd == INV_CMD_STOP || cmd == INV_CMD_SHUTDOWN || has_fault) {
                TransitionTo(sm, INV_STATE_SHUTDOWN);
            }
            break;

        case INV_STATE_RUN:
            if (cmd == INV_CMD_STOP || cmd == INV_CMD_SHUTDOWN || has_fault) {
                TransitionTo(sm, INV_STATE_SHUTDOWN);
            }
            break;

        case INV_STATE_SHUTDOWN:
            if (cmd == INV_CMD_RECOVER) {
                TransitionTo(sm, INV_STATE_RUN);
            } else if (cmd == INV_CMD_START) {
                TransitionTo(sm, INV_STATE_START);
            }
            break;

        default:
            break;
    }

    (void)has_fault;
}

void Inv_StateMachine_Run(Inv_StateMachine_t *sm) {
    Inv_State_t curr = sm->current;

    switch (curr) {
        case INV_STATE_INIT:
            if (Inv_StateMachine_GetStateDuration(sm) > 100) {
                Inv_StateMachine_ProcessCommand(sm, INV_CMD_NONE);
            }
            break;

        case INV_STATE_IDLE:
            break;

        case INV_STATE_START:
            if (Inv_StateMachine_GetStateDuration(sm) > 50) {
                TransitionTo(sm, INV_STATE_RUN);
            }
            break;

        case INV_STATE_RUN:
            break;

        case INV_STATE_SHUTDOWN:
            break;

        default:
            break;
    }
}
