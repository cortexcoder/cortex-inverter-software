#include "svpwm.h"
#include "fast_math.h"
#include <math.h>

#define SQRT3_INV  (0.5773502691896258f)
#define SQRT3_DIV2 (0.8660254037844386f)
#define TWO_DIV_SQRT3 (1.1547005383793f)
#define INV_SQRT3  (0.5773502691896258f)

#define PI         (3.14159265358979f)
#define TWO_PI     (6.28318530717959f)
#define PI_DIV3    (1.04719755119660f)
#define PI_2DIV3   (2.09439510239320f)

void SvPwm_Init(SvPwm_Handle *h, const SvPwm_Config *cfg) {
    h->config = cfg;
    h->theta = 0.0f;
    h->v_alpha = 0.0f;
    h->v_beta = 0.0f;
    h->v_d = 0.0f;
    h->v_q = 0.0f;
    h->sector = 0.0f;
    h->ta = 0.5f;
    h->tb = 0.5f;
    h->tc = 0.5f;
}

void SvPwm_SetTheta(SvPwm_Handle *h, float theta) {
    h->theta = theta;
}

static float SvPwm_CalcSector(float theta) {
    theta = fmodf(theta, TWO_PI);
    if (theta < 0.0f) {
        theta += TWO_PI;
    }

    if (theta < PI_DIV3) {
        return 0.0f;
    } else if (theta < PI_2DIV3) {
        return 1.0f;
    } else if (theta < PI) {
        return 2.0f;
    } else if (theta < PI + PI_DIV3) {
        return 3.0f;
    } else if (theta < PI + PI_2DIV3) {
        return 4.0f;
    } else {
        return 5.0f;
    }
}

void SvPwm_Step(SvPwm_Handle *h, float v_d, float v_q, float *ta, float *tb, float *tc) {
    const float vbus = h->config->vbus;
    const float theta = h->theta;

    h->v_d = v_d;
    h->v_q = v_q;

    const float U_dc_inv = 1.0f / vbus;

    float sin_theta, cos_theta;
    FastSinCos(theta, &sin_theta, &cos_theta);
    h->v_alpha = v_d * cos_theta - v_q * sin_theta;
    h->v_beta  = v_d * sin_theta + v_q * cos_theta;

    float v_squared = h->v_alpha * h->v_alpha + h->v_beta * h->v_beta;
    float v_ref = SQRT3_INV * sqrtf(v_squared);

    if (v_ref > (U_dc_inv * 0.5f)) {
        h->ta = 0.5f;
        h->tb = 0.5f;
        h->tc = 0.5f;
        *ta = 0.5f;
        *tb = 0.5f;
        *tc = 0.5f;
        return;
    }

    const float sector = SvPwm_CalcSector(theta);
    h->sector = sector;

    float T1, T2, T0;
    float t_a, t_b, t_c;

    const float U1 = h->v_beta;
    const float U2 = -0.5f * h->v_beta + SQRT3_DIV2 * h->v_alpha;
    const float U3 = -0.5f * h->v_beta - SQRT3_DIV2 * h->v_alpha;

    float Ta, Tb, Tc;

    if (sector < 0.5f) {
        T1 = U2 * TWO_DIV_SQRT3;
        T2 = U1 * INV_SQRT3;
        Ta = INV_SQRT3 * (U1 - U2);
        Tb = SQRT3_INV * U2;
        Tc = 0.0f;
    } else if (sector < 1.5f) {
        T1 = -U2 * TWO_DIV_SQRT3;
        T2 = -U3 * INV_SQRT3;
        Ta = 0.0f;
        Tb = SQRT3_INV * (-U2 + U3);
        Tc = INV_SQRT3 * U2;
    } else if (sector < 2.5f) {
        T1 = U3 * TWO_DIV_SQRT3;
        T2 = U1 * INV_SQRT3;
        Ta = 0.0f;
        Tb = INV_SQRT3 * U3;
        Tc = SQRT3_INV * (-U1 + U3);
    } else if (sector < 3.5f) {
        T1 = -U3 * TWO_DIV_SQRT3;
        T2 = -U1 * INV_SQRT3;
        Ta = INV_SQRT3 * (-U3 + U1);
        Tb = 0.0f;
        Tc = SQRT3_INV * U1;
    } else if (sector < 4.5f) {
        T1 = U1 * TWO_DIV_SQRT3;
        T2 = U2 * INV_SQRT3;
        Ta = INV_SQRT3 * (-U1 + U2);
        Tb = 0.0f;
        Tc = SQRT3_INV * (-U2 + U1);
    } else {
        T1 = -U1 * TWO_DIV_SQRT3;
        T2 = -U2 * INV_SQRT3;
        Ta = SQRT3_INV * U1;
        Tb = INV_SQRT3 * (U1 - U2);
        Tc = 0.0f;
    }

    T0 = 1.0f - T1 - T2;

    float T_pwm = 1.0f;
    t_a = Ta + T1 + T2 * 0.5f;
    t_b = Tb + T2 + T1 * 0.5f;
    t_c = Tc + T0 * 0.5f;

    h->ta = t_a * T_pwm;
    h->tb = t_b * T_pwm;
    h->tc = t_c * T_pwm;

    *ta = t_a * T_pwm;
    *tb = t_b * T_pwm;
    *tc = t_c * T_pwm;
}

void SvPwm_Reset(SvPwm_Handle *h) {
    h->theta = 0.0f;
    h->ta = 0.5f;
    h->tb = 0.5f;
    h->tc = 0.5f;
}
