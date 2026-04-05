#pragma once

#include <stdint.h>
#include "bsp_hrpwm.h"

#define CTRL_FREQ_HZ         BSP_HRPWM_SWITCH_FREQ_HZ
#define CTRL_PERIOD_US       BSP_HRPWM_SWITCH_PERIOD_US
#define CTRL_SLICE_COUNT     BSP_HRPWM_SLICE_COUNT
#define CTRL_SLICE_US        BSP_HRPWM_SLICE_GET_US()

#define TASK_FREQ_1MS        1000U
#define TASK_FREQ_5MS       200U
#define TASK_FREQ_10MS       100U
#define TASK_FREQ_50MS       20U
#define TASK_FREQ_1000MS     1U

typedef void (*TaskFunc_t)(void);

typedef struct {
    uint32_t interval_ticks;
    uint32_t last_tick;
    TaskFunc_t func;
    const char *name;
} TaskDescriptor;

#define TASK_DECLARE(name, freq, func) \
    static const TaskDescriptor name = { \
        .interval_ticks = freq, \
        .last_tick = 0, \
        .func = func, \
        .name = #name \
    }

void TimerISR_Init(void);

void TimerISR_MasterHandler(void);

void TimerISR_OnSysTick(void);

void TimerISR_AckSlice(uint8_t slice);

uint8_t TimerISR_GetSlice(void);

uint32_t TimerISR_GetTick(void);

uint32_t TimerISR_GetTick64(void);

void TaskScheduler_Init(void);

void TaskScheduler_Run(void);

void TaskScheduler_SetTimeBase(uint32_t tick);

void Task1ms(void);
void Task5ms(void);
void Task10ms(void);
void Task50ms(void);
void Task1000ms(void);
