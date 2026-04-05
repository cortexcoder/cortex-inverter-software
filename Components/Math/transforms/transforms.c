#include "transforms.h"
#include "fast_math.h"

#define SQRT3_INV   (0.5773502691896258f)
#define SQRT3_DIV2  (0.8660254037844386f)
#define NEG_HALF    (-0.5f)

void Transforms_Init(Transforms_Handle *h) {
    (void)h;
}

void Transforms_ABCToAlphaBeta(const ABC *abc, AlphaBeta *ab) {
    ab->alpha = abc->a;
    ab->beta = NEG_HALF * abc->a + SQRT3_DIV2 * abc->b;
}

void Transforms_AlphaBetaToABC(const AlphaBeta *ab, ABC *abc) {
    abc->a = ab->alpha;
    abc->b = -0.5f * ab->alpha + SQRT3_DIV2 * ab->beta;
    abc->c = -0.5f * ab->alpha - SQRT3_DIV2 * ab->beta;
}

void Transforms_AlphaBetaToDQ(const AlphaBeta *ab, DQ *dq, float theta) {
    float cos_t, sin_t;
    FastSinCos(theta, &sin_t, &cos_t);

    dq->d = ab->alpha * cos_t + ab->beta * sin_t;
    dq->q = -ab->alpha * sin_t + ab->beta * cos_t;
}

void Transforms_DQToAlphaBeta(const DQ *dq, AlphaBeta *ab, float theta) {
    float cos_t, sin_t;
    FastSinCos(theta, &sin_t, &cos_t);

    ab->alpha = dq->d * cos_t - dq->q * sin_t;
    ab->beta = dq->d * sin_t + dq->q * cos_t;
}

void Transforms_ABCToDQ(const ABC *abc, DQ *dq, float theta) {
    AlphaBeta ab;
    Transforms_ABCToAlphaBeta(abc, &ab);
    Transforms_AlphaBetaToDQ(&ab, dq, theta);
}

void Transforms_DQToABC(const DQ *dq, ABC *abc, float theta) {
    AlphaBeta ab;
    Transforms_DQToAlphaBeta(dq, &ab, theta);
    Transforms_AlphaBetaToABC(&ab, abc);
}

void Transforms_SetTheta(ParkHandle *h, float theta) {
    h->theta = theta;
    FastSinCos(theta, &h->sin_theta, &h->cos_theta);
}

void Transforms_ParkInit(ParkHandle *h, const ParkConfig *cfg) {
    h->config = cfg;
    h->theta = 0.0f;
    h->cos_theta = 1.0f;
    h->sin_theta = 0.0f;
}

void Transforms_ParkTransform(const ParkHandle *h, const AlphaBeta *ab, DQ *dq) {
    const float alpha = ab->alpha;
    const float beta = ab->beta;
    const float cos_t = h->cos_theta;
    const float sin_t = h->sin_theta;

    dq->d = alpha * cos_t + beta * sin_t;
    dq->q = -alpha * sin_t + beta * cos_t;
}

void Transforms_InvParkTransform(const ParkHandle *h, const DQ *dq, AlphaBeta *ab) {
    const float d = dq->d;
    const float q = dq->q;
    const float cos_t = h->cos_theta;
    const float sin_t = h->sin_theta;

    ab->alpha = d * cos_t - q * sin_t;
    ab->beta = d * sin_t + q * cos_t;
}
