#pragma once

#include <stdint.h>

typedef struct {
    float fc;
    float Ts;
    float alpha;
} Lpf_Config;

typedef struct {
    const Lpf_Config *config;
    float y_prev;
} Lpf_Handle;

void Lpf_Init(Lpf_Handle *h, const Lpf_Config *cfg);

void Lpf_Step(Lpf_Handle *h, float x, float *y);

void Lpf_Reset(Lpf_Handle *h);
