#pragma once

#include <stdint.h>

#define BSP_HRPWM_SWITCH_FREQ_HZ         24000U
#define BSP_HRPWM_SWITCH_PERIOD_US       (1000000U / BSP_HRPWM_SWITCH_FREQ_HZ)
#define BSP_HRPWM_SWITCH_PERIOD_S        (1.0f / (float)BSP_HRPWM_SWITCH_FREQ_HZ)

#define BSP_HRPWM_SAMPLES_PER_PERIOD     8U
#define BSP_HRPWM_SAMPLE_INTERVAL_US     (BSP_HRPWM_SWITCH_PERIOD_US / BSP_HRPWM_SAMPLES_PER_PERIOD)

#define BSP_HRPWM_DEADTIME_NS            200U

#define BSP_HRPWM_TIMER_FREQ_HZ          480000000U
#define BSP_HRPWM_TIMER_PERIOD           (BSP_HRPWM_TIMER_FREQ_HZ / BSP_HRPWM_SWITCH_FREQ_HZ)

#define BSP_HRPWM_CH_A                   0U
#define BSP_HRPWM_CH_B                   1U
#define BSP_HRPWM_CH_C                   2U

#define BSP_HRPWM_CH_COUNT               3U

typedef enum {
    BSP_HRPWM_SLICE_DIV_1 = 1,
    BSP_HRPWM_SLICE_DIV_2 = 2,
    BSP_HRPWM_SLICE_DIV_4 = 4,
    BSP_HRPWM_SLICE_DIV_6 = 6,
    BSP_HRPWM_SLICE_DIV_12 = 12,
} BSP_HRPWM_SliceDiv_t;

#define BSP_HRPWM_SLICE_COUNT_DEFAULT    BSP_HRPWM_SLICE_DIV_12
#define BSP_HRPWM_SLICE_COUNT            BSP_HRPWM_SLICE_DIV_12

#define BSP_HRPWM_SLICE_US(div)         (BSP_HRPWM_SWITCH_PERIOD_US / (div))
#define BSP_HRPWM_SLICE_GET_US()         BSP_HRPWM_SLICE_US(BSP_HRPWM_SLICE_COUNT)

#define BSP_HRPWM_TIMER_CLK_HZ           240000000U
#define BSP_HRPWM_TIMER_PRESCALER        0U

#define BSP_HRPWM_ADC_TRIG_OFFSET_US     1U

#define BSP_HRPWM_PWM_FREQ_HZ            BSP_HRPWM_SWITCH_FREQ_HZ
#define BSP_HRPWM_PWM_PERIOD_CNT         BSP_HRPWM_TIMER_PERIOD
#define BSP_HRPWM_PWM_PERIOD_US          BSP_HRPWM_SWITCH_PERIOD_US

#define BSP_HRPWM_GET_SLICE_TICK(div)    (BSP_HRPWM_TIMER_FREQ_HZ / (BSP_HRPWM_SWITCH_FREQ_HZ * (div)))
#define BSP_HRPWM_GET_SAMPLE_TICK()      (BSP_HRPWM_TIMER_FREQ_HZ / (BSP_HRPWM_SWITCH_FREQ_HZ * BSP_HRPWM_SAMPLES_PER_PERIOD))

#define BSP_HRPWM_DUTY_MIN               0.02f
#define BSP_HRPWM_DUTY_MAX               0.98f

#define BSP_HRPWM_IS_VALID_DUTY(d)      (((d) >= BSP_HRPWM_DUTY_MIN) && ((d) <= BSP_HRPWM_DUTY_MAX))

#define BSP_HRPWM_COMPARE_GET_HALF(cmp)  ((cmp) >> 1)

#define BSP_HRPWM_UPDATE_DUTY(cmp, duty) \
    do { \
        uint32_t period = BSP_HRPWM_TIMER_PERIOD; \
        (cmp) = (uint32_t)((float)period * (duty)); \
        if ((cmp) < (period * BSP_HRPWM_DUTY_MIN)) { \
            (cmp) = (uint32_t)(period * BSP_HRPWM_DUTY_MIN); \
        } else if ((cmp) > (period * BSP_HRPWM_DUTY_MAX)) { \
            (cmp) = (uint32_t)(period * BSP_HRPWM_DUTY_MAX); \
        } \
    } while(0)

#define BSP_HRPWM_TIMER_TICK_TO_US(tick)  ((tick) * 1000000U / BSP_HRPWM_TIMER_FREQ_HZ)
#define BSP_HRPWM_US_TO_TIMER_TICK(us)    ((us) * BSP_HRPWM_TIMER_FREQ_HZ / 1000000U)

#define BSP_HRPWM_GET_CH_PHASE_SHIFT(ch)  ((ch) * (BSP_HRPWM_TIMER_PERIOD / 3))
