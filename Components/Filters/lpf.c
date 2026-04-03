#include "lpf.h"
#include <arm_math.h>

#define PI_F 3.14159265358979f

void Lpf_Init(Lpf_Handle *h, const Lpf_Config *cfg) {
    h->config = cfg;
    h->y_prev = 0.0f;
}

void Lpf_Step(Lpf_Handle *h, float x, float *y) {
    const Lpf_Config *c = h->config;

    float alpha = c->alpha;
    if (alpha == 0.0f && c->fc > 0.0f && c->Ts > 0.0f) {
        float omega_c = 2.0f * PI_F * c->fc;
        alpha = omega_c * c->Ts / (omega_c * c->Ts + 1.0f);
    }

    *y = alpha * x + (1.0f - alpha) * h->y_prev;
    h->y_prev = *y;
}

void Lpf_Reset(Lpf_Handle *h) {
    h->y_prev = 0.0f;
}
