#pragma once

#include <stdint.h>

typedef struct {
    float fc;
    float Ts;
    float alpha;
} MathLpfConfig;

typedef struct {
    const MathLpfConfig *config;
    float y_prev;
} MathLpfHandle;

void MathLpf_Init(MathLpfHandle *h, const MathLpfConfig *cfg);

void MathLpf_Reset(MathLpfHandle *h);

void MathLpf_Step(MathLpfHandle *h, float x, float *y);

float MathLpf_StepInline(MathLpfHandle *h, float x);
