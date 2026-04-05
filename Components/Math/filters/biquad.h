#pragma once

#include <stdint.h>

typedef struct {
    float fc;
    float Q;
    float Ts;
    float a0;
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;
} BiquadConfig;

typedef struct {
    const BiquadConfig *config;
    float x1;
    float x2;
    float y1;
    float y2;
} BiquadHandle;

void Biquad_Init(BiquadHandle *h, const BiquadConfig *cfg);

void Biquad_Reset(BiquadHandle *h);

void Biquad_Step(BiquadHandle *h, float x, float *y);

float Biquad_StepInline(BiquadHandle *h, float x);

void Biquad_LPF(BiquadConfig *cfg, float fc, float Q, float Ts);

void Biquad_HPF(BiquadConfig *cfg, float fc, float Q, float Ts);

void Biquad_BPF(BiquadConfig *cfg, float fc, float Q, float Ts);

void Biquad_NOTCH(BiquadConfig *cfg, float fc, float Q, float Ts);
