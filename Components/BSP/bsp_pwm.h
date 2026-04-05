#pragma once

#include <stdint.h>
#include "bsp_io_map.h"
#include "bsp_hrpwm.h"

typedef struct BSP_PwmCh_s {
    uint8_t ch_id;
    float duty;
    float phase_shift;
} BSP_PwmCh;

typedef struct BSP_Pwm_s {
    const BSP_PwmConfig *config;
    BSP_PwmCh ch[BSP_PWM_CH_COUNT];
    uint32_t arr;
    uint8_t enabled;
} BSP_Pwm;

void BSP_Pwm_Init(BSP_Pwm *pwm, const BSP_PwmConfig *cfg);

void BSP_Pwm_SetDuty(BSP_Pwm *pwm, uint8_t ch, float duty);

float BSP_Pwm_GetDuty(const BSP_Pwm *pwm, uint8_t ch);

void BSP_Pwm_SetPhaseShift(BSP_Pwm *pwm, uint8_t ch, float degrees);

void BSP_Pwm_SetAllDuty(BSP_Pwm *pwm, float duty_abc[3]);

void BSP_Pwm_Enable(BSP_Pwm *pwm);

void BSP_Pwm_Disable(BSP_Pwm *pwm);

uint32_t BSP_Pwm_GetArr(const BSP_Pwm *pwm);

uint32_t BSP_Pwm_GetTimerFreq(void);
