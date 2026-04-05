#include "notch.h"
#include "fast_math.h"

#define MATH_PI (3.14159265358979f)

void Notch_Init(NotchHandle *h, const NotchConfig *cfg) {
    h->config = cfg;
    h->x1 = 0.0f;
    h->x2 = 0.0f;
    h->y1 = 0.0f;
    h->y2 = 0.0f;
}

void Notch_Reset(NotchHandle *h) {
    h->x1 = 0.0f;
    h->x2 = 0.0f;
    h->y1 = 0.0f;
    h->y2 = 0.0f;
}

void Notch_Step(NotchHandle *h, float x, float *y) {
    const NotchConfig *c = h->config;

    float y0 = c->b0 * x + c->b1 * h->x1 + c->b2 * h->x2
              - c->a1 * h->y1 - c->a2 * h->y2;

    h->x2 = h->x1;
    h->x1 = x;
    h->y2 = h->y1;
    h->y1 = y0;

    *y = y0;
}

float Notch_StepInline(NotchHandle *h, float x) {
    const NotchConfig *c = h->config;

    float y0 = c->b0 * x + c->b1 * h->x1 + c->b2 * h->x2
              - c->a1 * h->y1 - c->a2 * h->y2;

    h->x2 = h->x1;
    h->x1 = x;
    h->y2 = h->y1;
    h->y1 = y0;

    return y0;
}

void Notch_Config(NotchConfig *cfg, float fc, float Q, float depth_db, float Ts) {
    float omega = 2.0f * MATH_PI * fc * Ts;
    float sn = FastSin(omega);
    float cs = FastCos(omega);
    float alpha = sn / (2.0f * Q);

    float a0_inv = 1.0f / (1.0f + alpha);

    cfg->b0 = a0_inv;
    cfg->b1 = -2.0f * cs * a0_inv;
    cfg->b2 = a0_inv;
    cfg->a1 = -2.0f * cs * a0_inv;
    cfg->a2 = (1.0f - alpha) * a0_inv;

    cfg->fc = fc;
    cfg->Q = Q;
    cfg->depth_db = depth_db;
    cfg->Ts = Ts;
}
