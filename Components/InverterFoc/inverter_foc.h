#pragma once

#include "pi_ctrl.h"
#include "clarke.h"
#include "park.h"
#include "svpwm.h"
#include "lpf.h"

#define INV_FOC_SAMPLING_FREQ_HZ   20000.0f
#define INV_FOC_TS                 (1.0f / INV_FOC_SAMPLING_FREQ_HZ)
#define INV_FOC_VBUS               24.0f
#define INV_FOC_MAX_DUTY           0.95f
#define INV_FOC_MIN_DUTY           0.05f

typedef struct {
    PiCtrl_Config id_ctrl_cfg;
    PiCtrl_Config iq_ctrl_cfg;
    SvPwm_Config svpwm_cfg;
    MathLpfConfig i_alpha_filt_cfg;
    MathLpfConfig i_beta_filt_cfg;
} InvFoc_Config;

typedef struct {
    PiCtrl_Handle id_ctrl;
    PiCtrl_Handle iq_ctrl;
    SvPwm_Handle svpwm;
    Park_Handle park;
    MathLpfHandle i_alpha_filt;
    MathLpfHandle i_beta_filt;

    float theta;
    float speed_radps;
    float pole_pairs;
} InvFoc_Handle;

void InvFoc_Init(InvFoc_Handle *h, const InvFoc_Config *cfg);

void InvFoc_Step(InvFoc_Handle *h,
                 float i_a, float i_b, float i_c,
                 float theta_elec,
                 float v_d_ref, float v_q_ref,
                 float *ta, float *tb, float *tc);

void InvFoc_Reset(InvFoc_Handle *h);

void InvFoc_SetSpeed(InvFoc_Handle *h, float speed_radps);
