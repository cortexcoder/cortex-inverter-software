#include "pi_ctrl.h"
#include <arm_math.h>

void PiCtrl_Init(PiCtrl_Handle *h, const PiCtrl_Config *cfg) {
    h->config = cfg;
    h->integrator = 0.0f;
    h->out_prev = 0.0f;
    h->ref_prev = 0.0f;
}

void PiCtrl_Step(PiCtrl_Handle *h, float ref, float fdb, float *out) {
    const PiCtrl_Config *c = h->config;

    const float error = ref - fdb;

    const float kp = c->kp;
    const float ki = c->ki;
    const float Ts = c->Ts;

    const float out_max = c->out_max;
    const float out_min = c->out_min;

    float proportional = kp * error;

    h->integrator += ki * Ts * error;

    float u = proportional + h->integrator;

    if (u > out_max) {
        u = out_max;
    } else if (u < out_min) {
        u = out_min;
    }

    h->out_prev = u;
    h->ref_prev = ref;
    *out = u;
}

void PiCtrl_Reset(PiCtrl_Handle *h) {
    h->integrator = 0.0f;
    h->out_prev = 0.0f;
    h->ref_prev = 0.0f;
}

void PiCtrl_SetIntegral(PiCtrl_Handle *h, float value) {
    h->integrator = value;
}

float PiCtrl_GetIntegral(const PiCtrl_Handle *h) {
    return h->integrator;
}
