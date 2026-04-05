#include "inverter_types.h"
#include <string.h>

static volatile uint32_t s_systick = 0;
static volatile uint8_t s_current_slice = 0;
static volatile uint32_t s_slice_timestamps[CTRL_SLICE_COUNT];
static volatile uint32_t s_slice_durations[CTRL_SLICE_COUNT];

static TaskDescriptor *s_tasks[] = {
    &(TaskDescriptor){ TASK_FREQ_1MS, 0, Task1ms, "1ms" },
    &(TaskDescriptor){ TASK_FREQ_5MS, 0, Task5ms, "5ms" },
    &(TaskDescriptor){ TASK_FREQ_10MS, 0, Task10ms, "10ms" },
    &(TaskDescriptor){ TASK_FREQ_50MS, 0, Task50ms, "50ms" },
    &(TaskDescriptor){ TASK_FREQ_1000MS, 0, Task1000ms, "1000ms" },
};

#define TASK_COUNT (sizeof(s_tasks) / sizeof(s_tasks[0]))

void TimerISR_Init(void) {
    memset((void*)s_slice_timestamps, 0, sizeof(s_slice_timestamps));
    memset((void*)s_slice_durations, 0, sizeof(s_slice_durations));
    s_current_slice = 0;
    s_systick = 0;
}

void TimerISR_OnSysTick(void) {
    s_systick++;
}

uint32_t TimerISR_GetTick(void) {
    return s_systick;
}

uint32_t TimerISR_GetTick64(void) {
    return s_systick;
}

uint8_t TimerISR_GetSlice(void) {
    return s_current_slice;
}

void TimerISR_AckSlice(uint8_t slice) {
    if (slice < CTRL_SLICE_COUNT) {
        uint32_t now = s_systick;
        s_slice_durations[slice] = now - s_slice_timestamps[slice];
        s_slice_timestamps[slice] = now;
    }
    s_current_slice = slice;
}

void TimerISR_MasterHandler(void) {
    uint8_t slice = s_current_slice;
    uint32_t timestamp = s_systick;
    (void)timestamp;

    TimerISR_AckSlice(slice);

    if (slice == 0) {
        for (uint32_t i = 0; i < TASK_COUNT; i++) {
            TaskDescriptor *t = (TaskDescriptor*)s_tasks[i];
            uint32_t interval = t->interval_ticks;
            if (interval == 0) continue;
            if ((s_systick - t->last_tick) >= interval) {
                t->last_tick = s_systick;
                t->func();
            }
        }
    }

    (void)slice;
}

void TaskScheduler_Init(void) {
    TimerISR_Init();
    for (uint32_t i = 0; i < TASK_COUNT; i++) {
        TaskDescriptor *t = (TaskDescriptor*)s_tasks[i];
        t->last_tick = 0;
    }
}

void TaskScheduler_SetTimeBase(uint32_t tick) {
    s_systick = tick;
    for (uint32_t i = 0; i < TASK_COUNT; i++) {
        TaskDescriptor *t = (TaskDescriptor*)s_tasks[i];
        t->last_tick = tick;
    }
}

void TaskScheduler_Run(void) {
    TaskScheduler_Init();
    while (1) {
        __asm("wfi");
    }
}

__attribute__((weak)) void Task1ms(void) {
}

__attribute__((weak)) void Task5ms(void) {
}

__attribute__((weak)) void Task10ms(void) {
}

__attribute__((weak)) void Task50ms(void) {
}

__attribute__((weak)) void Task1000ms(void) {
}
