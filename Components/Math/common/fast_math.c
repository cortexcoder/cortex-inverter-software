#include "fast_math.h"

#define FAST_MATH_PI     (3.14159265358979f)
#define FAST_MATH_TWO_PI  (6.28318530717959f)
#define FAST_MATH_PI_2    (1.57079632679490f)
#define FAST_MATH_PI_4   (0.78539816339745f)

#define FAST_MATH_SIN_C1  (1.0f)
#define FAST_MATH_SIN_C2  (-0.16666666666666667f)
#define FAST_MATH_SIN_C3  (0.00833333333333333f)
#define FAST_MATH_SIN_C4  (-0.000198412698412698f)
#define FAST_MATH_SIN_C5  (0.00000275573192239859f)
#define FAST_MATH_SIN_C6  (-0.000000025052108102f)

static float FastReduceAngle(float x) {
    x = x / FAST_MATH_TWO_PI;
    x = x - (int)(x);
    if (x < 0.0f) {
        x += 1.0f;
    }
    return x * FAST_MATH_TWO_PI;
}

static float FastSinPoly(float x) {
    float x2 = x * x;
    float x3 = x2 * x;
    float x6 = x3 * x3;
    
    return FAST_MATH_SIN_C1 * x +
           FAST_MATH_SIN_C2 * x3 +
           FAST_MATH_SIN_C3 * x3 * x2 +
           FAST_MATH_SIN_C4 * x6 * x +
           FAST_MATH_SIN_C5 * x6 * x2 +
           FAST_MATH_SIN_C6 * x6 * x3;
}

static float FastCosPoly(float x) {
    float x2 = x * x;
    float x4 = x2 * x2;
    float x6 = x4 * x2;
    
    return 1.0f - 0.5f * x2 + 0.0416666667f * x4 - 0.00138888889f * x6;
}

float FastSin(float x) {
    x = FastReduceAngle(x);
    
    if (x > FAST_MATH_PI) {
        x = FAST_MATH_TWO_PI - x;
        return -FastSinPoly(x);
    } else if (x > FAST_MATH_PI_2) {
        x = FAST_MATH_PI - x;
        return FastSinPoly(x);
    } else {
        return FastSinPoly(x);
    }
}

float FastCos(float x) {
    x = FastReduceAngle(x);
    
    if (x > FAST_MATH_PI) {
        x = FAST_MATH_TWO_PI - x;
        return FastCosPoly(x);
    } else if (x > FAST_MATH_PI_2) {
        x = FAST_MATH_PI - x;
        return -FastCosPoly(x);
    } else {
        return FastCosPoly(x);
    }
}

void FastSinCos(float x, float *sin_out, float *cos_out) {
    x = FastReduceAngle(x);
    
    if (x > FAST_MATH_PI) {
        x = FAST_MATH_TWO_PI - x;
        *sin_out = -FastSinPoly(x);
        *cos_out = FastCosPoly(x);
    } else if (x > FAST_MATH_PI_2) {
        x = FAST_MATH_PI - x;
        *sin_out = FastSinPoly(x);
        *cos_out = -FastCosPoly(x);
    } else {
        *sin_out = FastSinPoly(x);
        *cos_out = FastCosPoly(x);
    }
}
