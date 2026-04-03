#include "clarke.h"
#include <math.h>

#define SQRT3_INV  (0.5773502691896258f)
#define SQRT3_DIV2 (0.8660254037844386f)
#define TWO_DIV3   (0.6666666666666667f)
#define NEG_HALF   (-0.5f)

void Clarke_Init(Clarke_Handle *h, const Clarke_Config *cfg) {
    (void)h;
    (void)cfg;
}

void Clarke_Step(Clarke_Handle *h, const Clarke_Input *in, Clarke_Output *out) {
    (void)h;

    out->alpha = in->ia;

    out->beta = NEG_HALF * in->ia + SQRT3_DIV2 * in->ib;
}

void Clarke_StepFull(Clarke_Handle *h, const Clarke_Input *in, Clarke_OutputFull *out) {
    (void)h;

    out->alpha = in->ia;
    out->beta  = NEG_HALF * in->ia + SQRT3_DIV2 * in->ib;
    out->zero  = TWO_DIV3 * (in->ia + in->ib + in->ic);
}

void Clarke_Reset(Clarke_Handle *h) {
    (void)h;
}
