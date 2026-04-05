#pragma once

#include <stdint.h>

#define MATH_PI         (3.14159265358979f)
#define MATH_TWO_PI     (6.28318530717959f)
#define MATH_PI_INV     (0.31830988618379f)
#define MATH_PI_DIV2    (1.57079632679490f)
#define MATH_PI_DIV3    (1.04719755119660f)
#define MATH_PI_DIV4    (0.78539816339745f)
#define MATH_PI_DIV6    (0.52359877559830f)

#define MATH_SQRT2      (1.41421356237310f)
#define MATH_SQRT3      (1.73205080756888f)
#define MATH_SQRT2_INV   (0.70710678118655f)
#define MATH_SQRT3_INV   (0.57735026918963f)
#define MATH_SQRT3_DIV2  (0.86602540378444f)

#define MATH_DEG_TO_RAD(d)  ((d) * MATH_PI / 180.0f)
#define MATH_RAD_TO_DEG(r)  ((r) * 180.0f / MATH_PI)

#define MATH_FREQ_TO_OMG(f) ((f) * MATH_TWO_PI)
#define MATH_OMG_TO_FREQ(w) ((w) / MATH_TWO_PI)

#define MATH_CLAMP(val, min, max)  \
    do { if ((val) < (min)) (val) = (min); else if ((val) > (max)) (val) = (max); } while(0)

#define MATH_ABS(x)       (((x) < 0) ? -(x) : (x))
#define MATH_MIN(a, b)    (((a) < (b)) ? (a) : (b))
#define MATH_MAX(a, b)    (((a) > (b)) ? (a) : (b))

typedef struct {
    float k12;
    float k2;
} Math_ClarkeFactors;

typedef struct {
    float cos;
    float sin;
} Math_ParkFactors;

void Math_ClarkeFactors_Init(Math_ClarkeFactors *f, float ts);

void Math_ParkFactors_Init(Math_ParkFactors *f, float theta);

float Math_LimitFloat(float val, float limit);

int32_t Math_LimitInt32(int32_t val, int32_t limit);

uint32_t Math_LimitUint32(uint32_t val, uint32_t limit);

float Math_Saturate(float val, float min, float max);

float Math_MoveAverage(float new_val, float *buf, uint8_t len, uint8_t *idx);

float Math_LinearInterpolate(float x, float x0, float y0, float x1, float y1);

float Math_FastAbs(float x);

void Math_FastSqrt(float *out, float x);
