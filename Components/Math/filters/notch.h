#pragma once

#include <stdint.h>

typedef struct {
    float fc;
    float Q;
    float depth_db;
    float Ts;
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
} NotchConfig;

typedef struct {
    const NotchConfig *config;
    float x1;
    float x2;
    float y1;
    float y2;
} NotchHandle;

void Notch_Init(NotchHandle *h, const NotchConfig *cfg);

void Notch_Reset(NotchHandle *h);

void Notch_Step(NotchHandle *h, float x, float *y);

float Notch_StepInline(NotchHandle *h, float x);

void Notch_Config(NotchConfig *cfg, float fc, float Q, float depth_db, float Ts);
