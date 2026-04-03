#pragma once

#include <stdint.h>

typedef struct {
    float vbus;
} SvPwm_Config;

typedef struct {
    const SvPwm_Config *config;
    float theta;
    float v_alpha;
    float v_beta;
    float v_d;
    float v_q;
    float sector;
    float ta;
    float tb;
    float tc;
} SvPwm_Handle;

void SvPwm_Init(SvPwm_Handle *h, const SvPwm_Config *cfg);

void SvPwm_SetTheta(SvPwm_Handle *h, float theta);

void SvPwm_Step(SvPwm_Handle *h, float v_d, float v_q, float *ta, float *tb, float *tc);

void SvPwm_Reset(SvPwm_Handle *h);
