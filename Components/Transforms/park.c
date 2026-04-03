#include "park.h"
#include <math.h>
#include <arm_math.h>

void Park_Init(Park_Handle *h, const Park_Config *cfg) {
    h->config = cfg;
    h->cos_theta = 1.0f;
    h->sin_theta = 0.0f;
}

void Park_SetTheta(Park_Handle *h, float theta) {
    h->theta = theta;
    h->cos_theta = arm_cos_f32(theta);
    h->sin_theta = arm_sin_f32(theta);
}

void Park_Step(Park_Handle *h, const Park_Input *in, Park_Output *out) {
    const float alpha = in->alpha;
    const float beta  = in->beta;
    const float cos_t = h->cos_theta;
    const float sin_t = h->sin_theta;

    out->d = alpha * cos_t + beta * sin_t;
    out->q = -alpha * sin_t + beta * cos_t;
}

void Park_Reset(Park_Handle *h) {
    h->cos_theta = 1.0f;
    h->sin_theta = 0.0f;
}
