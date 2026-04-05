#include "biquad.h"
#include "fast_math.h"

#define MATH_PI (3.14159265358979f)
#define MATH_SQRT2 (1.41421356237310f)

void Biquad_Init(BiquadHandle *h, const BiquadConfig *cfg) {
    h->config = cfg;
    h->x1 = 0.0f;
    h->x2 = 0.0f;
    h->y1 = 0.0f;
    h->y2 = 0.0f;
}

void Biquad_Reset(BiquadHandle *h) {
    h->x1 = 0.0f;
    h->x2 = 0.0f;
    h->y1 = 0.0f;
    h->y2 = 0.0f;
}

void Biquad_Step(BiquadHandle *h, float x, float *y) {
    const BiquadConfig *c = h->config;

    float y0 = c->b0 * x + c->b1 * h->x1 + c->b2 * h->x2
              - c->a1 * h->y1 - c->a2 * h->y2;
    y0 /= c->a0;

    h->x2 = h->x1;
    h->x1 = x;
    h->y2 = h->y1;
    h->y1 = y0;

    *y = y0;
}

float Biquad_StepInline(BiquadHandle *h, float x) {
    const BiquadConfig *c = h->config;

    float y0 = c->b0 * x + c->b1 * h->x1 + c->b2 * h->x2
              - c->a1 * h->y1 - c->a2 * h->y2;
    y0 /= c->a0;

    h->x2 = h->x1;
    h->x1 = x;
    h->y2 = h->y1;
    h->y1 = y0;

    return y0;
}

void Biquad_LPF(BiquadConfig *cfg, float fc, float Q, float Ts) {
    float omega = 2.0f * MATH_PI * fc * Ts;
    float sn = FastSin(omega);
    float cs = FastCos(omega);
    float alpha = sn / (2.0f * Q);
    float a0_inv = 1.0f / (1.0f + alpha);

    cfg->b0 = (1.0f - cs) * 0.5f * a0_inv;
    cfg->b1 = (1.0f - cs) * a0_inv;
    cfg->b2 = (1.0f - cs) * 0.5f * a0_inv;
    cfg->a0 = 1.0f;
    cfg->a1 = -2.0f * cs * a0_inv;
    cfg->a2 = (1.0f - alpha) * a0_inv;
}

void Biquad_HPF(BiquadConfig *cfg, float fc, float Q, float Ts) {
    float omega = 2.0f * MATH_PI * fc * Ts;
    float sn = FastSin(omega);
    float cs = FastCos(omega);
    float alpha = sn / (2.0f * Q);
    float a0_inv = 1.0f / (1.0f + alpha);

    cfg->b0 = (1.0f + cs) * 0.5f * a0_inv;
    cfg->b1 = -(1.0f + cs) * a0_inv;
    cfg->b2 = (1.0f + cs) * 0.5f * a0_inv;
    cfg->a0 = 1.0f;
    cfg->a1 = -2.0f * cs * a0_inv;
    cfg->a2 = (1.0f - alpha) * a0_inv;
}

void Biquad_BPF(BiquadConfig *cfg, float fc, float Q, float Ts) {
    float omega = 2.0f * MATH_PI * fc * Ts;
    float sn = FastSin(omega);
    float cs = FastCos(omega);
    float alpha = sn / (2.0f * Q);
    float a0_inv = 1.0f / (1.0f + alpha);

    cfg->b0 = alpha * a0_inv;
    cfg->b1 = 0.0f;
    cfg->b2 = -alpha * a0_inv;
    cfg->a0 = 1.0f;
    cfg->a1 = -2.0f * cs * a0_inv;
    cfg->a2 = (1.0f - alpha) * a0_inv;
}

void Biquad_NOTCH(BiquadConfig *cfg, float fc, float Q, float Ts) {
    float omega = 2.0f * MATH_PI * fc * Ts;
    float sn = FastSin(omega);
    float cs = FastCos(omega);
    float alpha = sn / (2.0f * Q);
    float a0_inv = 1.0f / (1.0f + alpha);

    cfg->b0 = a0_inv;
    cfg->b1 = -2.0f * cs * a0_inv;
    cfg->b2 = a0_inv;
    cfg->a0 = 1.0f;
    cfg->a1 = -2.0f * cs * a0_inv;
    cfg->a2 = (1.0f - alpha) * a0_inv;
}
