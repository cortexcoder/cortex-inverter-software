#pragma once

#include <stdint.h>

typedef struct {
    float a;
    float b;
    float c;
} ABC;

typedef struct {
    float alpha;
    float beta;
} AlphaBeta;

typedef struct {
    float d;
    float q;
} DQ;

typedef struct {
    float theta;
} ParkConfig;

typedef struct {
    const ParkConfig *config;
    float theta;
    float cos_theta;
    float sin_theta;
} ParkHandle;

typedef struct {
    AlphaBeta i_ab;
    AlphaBeta v_ab;
} Transforms_Handle;

void Transforms_Init(Transforms_Handle *h);

void Transforms_ABCToAlphaBeta(const ABC *abc, AlphaBeta *ab);

void Transforms_AlphaBetaToABC(const AlphaBeta *ab, ABC *abc);

void Transforms_AlphaBetaToDQ(const AlphaBeta *ab, DQ *dq, float theta);

void Transforms_DQToAlphaBeta(const DQ *dq, AlphaBeta *ab, float theta);

void Transforms_ABCToDQ(const ABC *abc, DQ *dq, float theta);

void Transforms_DQToABC(const DQ *dq, ABC *abc, float theta);

void Transforms_SetTheta(ParkHandle *h, float theta);

void Transforms_ParkInit(ParkHandle *h, const ParkConfig *cfg);

void Transforms_ParkTransform(const ParkHandle *h, const AlphaBeta *ab, DQ *dq);

void Transforms_InvParkTransform(const ParkHandle *h, const DQ *dq, AlphaBeta *ab);
