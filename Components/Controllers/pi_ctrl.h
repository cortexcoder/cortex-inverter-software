#pragma once

#include <stdint.h>

typedef struct {
    float kp;
    float ki;
    float out_max;
    float out_min;
    float Ts;
} PiCtrl_Config;

typedef struct {
    const PiCtrl_Config *config;
    float integrator;
    float out_prev;
    float ref_prev;
} PiCtrl_Handle;

void PiCtrl_Init(PiCtrl_Handle *h, const PiCtrl_Config *cfg);

void PiCtrl_Step(PiCtrl_Handle *h, float ref, float fdb, float *out);

void PiCtrl_Reset(PiCtrl_Handle *h);

void PiCtrl_SetIntegral(PiCtrl_Handle *h, float value);

float PiCtrl_GetIntegral(const PiCtrl_Handle *h);
