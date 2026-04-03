#pragma once

#include <stdint.h>

typedef struct {
    float ia;
    float ib;
    float ic;
} Clarke_Input;

typedef struct {
    float alpha;
    float beta;
} Clarke_Output;

typedef struct {
    float alpha;
    float beta;
    float zero;
} Clarke_OutputFull;

typedef struct {
    uint8_t mode;
} Clarke_Config;

typedef struct {
    const void *config;
} Clarke_Handle;

void Clarke_Init(Clarke_Handle *h, const Clarke_Config *cfg);

void Clarke_Step(Clarke_Handle *h, const Clarke_Input *in, Clarke_Output *out);

void Clarke_StepFull(Clarke_Handle *h, const Clarke_Input *in, Clarke_OutputFull *out);

void Clarke_Reset(Clarke_Handle *h);
