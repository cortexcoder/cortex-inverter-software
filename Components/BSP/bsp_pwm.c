#include "bsp_pwm.h"
#include "bsp_hrpwm.h"

void BSP_Pwm_Init(BSP_Pwm *pwm, const BSP_PwmConfig *cfg) {
    pwm->config = cfg;
    pwm->arr = BSP_HRPWM_TIMER_FREQ_HZ / BSP_HRPWM_SWITCH_FREQ_HZ / 2;
    pwm->enabled = 0;

    for (int i = 0; i < BSP_PWM_CH_COUNT; i++) {
        pwm->ch[i].ch_id = i;
        pwm->ch[i].duty = 0.5f;
        pwm->ch[i].phase_shift = 0.0f;
    }
}

void BSP_Pwm_SetDuty(BSP_Pwm *pwm, uint8_t ch, float duty) {
    if (ch >= BSP_PWM_CH_COUNT) return;
    if (duty < BSP_HRPWM_DUTY_MIN) duty = BSP_HRPWM_DUTY_MIN;
    if (duty > BSP_HRPWM_DUTY_MAX) duty = BSP_HRPWM_DUTY_MAX;
    pwm->ch[ch].duty = duty;
}

float BSP_Pwm_GetDuty(const BSP_Pwm *pwm, uint8_t ch) {
    if (ch >= BSP_PWM_CH_COUNT) return 0.0f;
    return pwm->ch[ch].duty;
}

void BSP_Pwm_SetPhaseShift(BSP_Pwm *pwm, uint8_t ch, float degrees) {
    if (ch >= BSP_PWM_CH_COUNT) return;
    pwm->ch[ch].phase_shift = degrees;
}

void BSP_Pwm_SetAllDuty(BSP_Pwm *pwm, float duty_abc[3]) {
    for (int i = 0; i < BSP_PWM_CH_COUNT; i++) {
        BSP_Pwm_SetDuty(pwm, i, duty_abc[i]);
    }
}

void BSP_Pwm_Enable(BSP_Pwm *pwm) {
    (void)pwm;
    pwm->enabled = 1;
}

void BSP_Pwm_Disable(BSP_Pwm *pwm) {
    (void)pwm;
    pwm->enabled = 0;
}

uint32_t BSP_Pwm_GetArr(const BSP_Pwm *pwm) {
    return pwm->arr;
}

uint32_t BSP_Pwm_GetTimerFreq(void) {
    return BSP_HRPWM_TIMER_FREQ_HZ;
}
