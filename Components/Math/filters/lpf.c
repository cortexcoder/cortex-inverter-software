#include "lpf.h"
#include <math.h>

#define MATH_PI (3.14159265358979f)

void MathLpf_Init(MathLpfHandle *h, const MathLpfConfig *cfg) {
    h->config = cfg;
    h->y_prev = 0.0f;
}

void MathLpf_Reset(MathLpfHandle *h) {
    h->y_prev = 0.0f;
}

void MathLpf_Step(MathLpfHandle *h, float x, float *y) {
    const MathLpfConfig *c = h->config;

    float alpha = c->alpha;
    if (alpha == 0.0f && c->fc > 0.0f && c->Ts > 0.0f) {
        float omega_c = 2.0f * MATH_PI * c->fc;
        alpha = omega_c * c->Ts / (omega_c * c->Ts + 1.0f);
    }

    *y = alpha * x + (1.0f - alpha) * h->y_prev;
    h->y_prev = *y;
}

float MathLpf_StepInline(MathLpfHandle *h, float x) {
    const MathLpfConfig *c = h->config;

    float alpha = c->alpha;
    if (alpha == 0.0f && c->fc > 0.0f && c->Ts > 0.0f) {
        float omega_c = 2.0f * MATH_PI * c->fc;
        alpha = omega_c * c->Ts / (omega_c * c->Ts + 1.0f);
    }

    float y = alpha * x + (1.0f - alpha) * h->y_prev;
    h->y_prev = y;
    return y;
}
