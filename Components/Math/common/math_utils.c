#include "math_utils.h"
#include "fast_math.h"

void Math_ClarkeFactors_Init(Math_ClarkeFactors *f, float ts) {
    (void)ts;
    f->k12 = 0.5f;
    f->k2 = MATH_SQRT3_INV;
}

void Math_ParkFactors_Init(Math_ParkFactors *f, float theta) {
    FastSinCos(theta, &f->sin, &f->cos);
}

float Math_LimitFloat(float val, float limit) {
    if (val > limit) return limit;
    if (val < -limit) return -limit;
    return val;
}

int32_t Math_LimitInt32(int32_t val, int32_t limit) {
    if (val > limit) return limit;
    if (val < -limit) return -limit;
    return val;
}

uint32_t Math_LimitUint32(uint32_t val, uint32_t limit) {
    if (val > limit) return limit;
    return val;
}

float Math_Saturate(float val, float min, float max) {
    if (val > max) return max;
    if (val < min) return min;
    return val;
}

float Math_MoveAverage(float new_val, float *buf, uint8_t len, uint8_t *idx) {
    buf[*idx] = new_val;
    *idx = (*idx + 1) % len;

    float sum = 0.0f;
    for (uint8_t i = 0; i < len; i++) {
        sum += buf[i];
    }
    return sum / len;
}

float Math_LinearInterpolate(float x, float x0, float y0, float x1, float y1) {
    if (x1 == x0) return y0;
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}

float Math_FastAbs(float x) {
    union {
        float f;
        uint32_t u;
    } conv;
    conv.f = x;
    conv.u &= 0x7FFFFFFFU;
    return conv.f;
}

void Math_FastSqrt(float *out, float x) {
    if (x <= 0.0f) {
        *out = 0.0f;
        return;
    }

    union {
        float f;
        uint32_t u;
    } conv;

    conv.f = x;
    conv.u = (conv.u >> 1) + 0x1FBC0000U;

    conv.f = 0.5f * (conv.f + x / conv.f);
    conv.f = 0.5f * (conv.f + x / conv.f);

    *out = conv.f;
}
