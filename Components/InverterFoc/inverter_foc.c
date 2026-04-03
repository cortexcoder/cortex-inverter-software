#include "inverter_foc.h"
#include <stddef.h>

#define PI_CTRL_D_KP       0.05f
#define PI_CTRL_D_KI       0.001f
#define PI_CTRL_Q_KP       0.05f
#define PI_CTRL_Q_KI       0.001f
#define PI_CTRL_OUT_MAX    (INV_FOC_VBUS * INV_FOC_MAX_DUTY)
#define PI_CTRL_OUT_MIN    (INV_FOC_VBUS * INV_FOC_MIN_DUTY)

#define LPF_FC_HZ          1000.0f

void InvFoc_Init(InvFoc_Handle *h, const InvFoc_Config *cfg) {
    h->theta = 0.0f;
    h->speed_radps = 0.0f;
    h->pole_pairs = 4.0f;

    PiCtrl_Init(&h->id_ctrl, &cfg->id_ctrl_cfg);
    PiCtrl_Init(&h->iq_ctrl, &cfg->iq_ctrl_cfg);
    Park_Init(&h->park, NULL);
    SvPwm_Init(&h->svpwm, &cfg->svpwm_cfg);
    Lpf_Init(&h->i_alpha_filt, &cfg->i_alpha_filt_cfg);
    Lpf_Init(&h->i_beta_filt, &cfg->i_beta_filt_cfg);
}

void InvFoc_Step(InvFoc_Handle *h,
                 float i_a, float i_b, float i_c,
                 float theta_elec,
                 float v_d_ref, float v_q_ref,
                 float *ta, float *tb, float *tc) {

    Clarke_Input clarke_in = { i_a, i_b, i_c };
    Clarke_Output clarke_out;
    Clarke_Step(NULL, &clarke_in, &clarke_out);

    float i_alpha_filt, i_beta_filt;
    Lpf_Step(&h->i_alpha_filt, clarke_out.alpha, &i_alpha_filt);
    Lpf_Step(&h->i_beta_filt, clarke_out.beta, &i_beta_filt);

    Park_SetTheta(&h->park, theta_elec);
    Park_Input park_in = { i_alpha_filt, i_beta_filt };
    Park_Output park_out;
    Park_Step(&h->park, &park_in, &park_out);

    float v_d, v_q;
    PiCtrl_Step(&h->id_ctrl, v_d_ref, park_out.d, &v_d);
    PiCtrl_Step(&h->iq_ctrl, v_q_ref, park_out.q, &v_q);

    SvPwm_SetTheta(&h->svpwm, theta_elec);
    SvPwm_Step(&h->svpwm, v_d, v_q, ta, tb, tc);
}

void InvFoc_Reset(InvFoc_Handle *h) {
    PiCtrl_Reset(&h->id_ctrl);
    PiCtrl_Reset(&h->iq_ctrl);
    Park_Reset(&h->park);
    SvPwm_Reset(&h->svpwm);
    Lpf_Reset(&h->i_alpha_filt);
    Lpf_Reset(&h->i_beta_filt);
    h->theta = 0.0f;
    h->speed_radps = 0.0f;
}

void InvFoc_SetSpeed(InvFoc_Handle *h, float speed_radps) {
    h->speed_radps = speed_radps;
}
