#pragma once

#include <stdint.h>

typedef struct {
    float alpha;
    float beta;
} Park_Input;

typedef struct {
    float d;
    float q;
} Park_Output;

typedef struct {
    float theta;
} Park_Config;

typedef struct {
    const Park_Config *config;
    float theta;
    float cos_theta;
    float sin_theta;
} Park_Handle;

void Park_Init(Park_Handle *h, const Park_Config *cfg);

void Park_Step(Park_Handle *h, const Park_Input *in, Park_Output *out);

void Park_SetTheta(Park_Handle *h, float theta);

void Park_Reset(Park_Handle *h);
