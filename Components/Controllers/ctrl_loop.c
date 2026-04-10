/**
 * @file ctrl_loop.c
 * @brief Current Loop Controller Implementation
 */

#include "ctrl_loop.h"
#include <stddef.h>

void CtrlLoop_Init(CtrlLoop_Handle *h, const CtrlLoop_Config *cfg) {
    if (h == NULL || cfg == NULL) {
        return;
    }
    
    h->config = cfg;
    h->state = LOOP_STATE_IDLE;
    h->fault_flags = 0;
    h->tick_count = 0;
    
    h->pi_pos_d.kp = cfg->pos.kp_d;
    h->pi_pos_d.ki = cfg->pos.ki_d;
    h->pi_pos_d.out_max = cfg->out_max;
    h->pi_pos_d.out_min = cfg->out_min;
    h->pi_pos_d.integrator = 0.0f;
    h->pi_pos_d.out_prev = 0.0f;
    h->pi_pos_d.ref_prev = 0.0f;
    
    h->pi_pos_q.kp = cfg->pos.kp_q;
    h->pi_pos_q.ki = cfg->pos.ki_q;
    h->pi_pos_q.out_max = cfg->out_max;
    h->pi_pos_q.out_min = cfg->out_min;
    h->pi_pos_q.integrator = 0.0f;
    h->pi_pos_q.out_prev = 0.0f;
    h->pi_pos_q.ref_prev = 0.0f;
    
    if (cfg->enable_neg) {
        h->pi_neg_d.kp = cfg->neg.kp_d;
        h->pi_neg_d.ki = cfg->neg.ki_d;
        h->pi_neg_d.out_max = cfg->out_max;
        h->pi_neg_d.out_min = cfg->out_min;
        h->pi_neg_d.integrator = 0.0f;
        h->pi_neg_d.out_prev = 0.0f;
        h->pi_neg_d.ref_prev = 0.0f;
        
        h->pi_neg_q.kp = cfg->neg.kp_q;
        h->pi_neg_q.ki = cfg->neg.ki_q;
        h->pi_neg_q.out_max = cfg->out_max;
        h->pi_neg_q.out_min = cfg->out_min;
        h->pi_neg_q.integrator = 0.0f;
        h->pi_neg_q.out_prev = 0.0f;
        h->pi_neg_q.ref_prev = 0.0f;
    }
    
    if (cfg->enable_gvff) {
        h->gvff.L = cfg->L;
        h->gvff.R = cfg->R;
        h->gvff.omega = TWO_PI * 50.0f;
        h->gvff.Ts = cfg->Ts;
        h->gvff.init = false;
    }
    
    h->irc_d.init = false;
    h->irc_q.init = false;
    h->dci.init = false;
    
    h->init = true;
    h->state = LOOP_STATE_READY;
}

void CtrlLoop_Reset(CtrlLoop_Handle *h) {
    if (h == NULL || !h->init) {
        return;
    }
    
    h->pi_pos_d.integrator = 0.0f;
    h->pi_pos_d.out_prev = 0.0f;
    h->pi_pos_d.ref_prev = 0.0f;
    
    h->pi_pos_q.integrator = 0.0f;
    h->pi_pos_q.out_prev = 0.0f;
    h->pi_pos_q.ref_prev = 0.0f;
    
    if (h->config->enable_neg) {
        h->pi_neg_d.integrator = 0.0f;
        h->pi_neg_d.out_prev = 0.0f;
        h->pi_neg_d.ref_prev = 0.0f;
        
        h->pi_neg_q.integrator = 0.0f;
        h->pi_neg_q.out_prev = 0.0f;
        h->pi_neg_q.ref_prev = 0.0f;
    }
    
    h->irc_d.integral[0] = 0.0f;
    h->irc_d.integral[1] = 0.0f;
    h->irc_d.out_prev = 0.0f;
    h->irc_d.init = false;
    
    h->irc_q.integral[0] = 0.0f;
    h->irc_q.integral[1] = 0.0f;
    h->irc_q.out_prev = 0.0f;
    h->irc_q.init = false;
    
    h->dci.integrator = 0.0f;
    h->dci.ref_prev = 0.0f;
    h->dci.init = false;
    
    h->gvff.Vd_fbk_prev = 0.0f;
    h->gvff.Vq_fbk_prev = 0.0f;
    h->gvff.init = false;
    
    h->fault_flags = 0;
    h->state = LOOP_STATE_READY;
}

void CtrlLoop_SetTheta(CtrlLoop_Handle *h, float theta_pos, float theta_neg) {
    if (h == NULL || !h->init) {
        return;
    }
    h->theta_pos = theta_pos;
    h->theta_neg = theta_neg;
}

void CtrlLoop_PreSample(CtrlLoop_Handle *h, float I_a, float I_b, float I_c) {
    if (h == NULL || !h->init) {
        return;
    }
    
    CtrlLoop_Clark(I_a, I_b, I_c, 
                   &h->pre.I_alpha, &h->pre.I_beta, &h->pre.I_zero);
    
    CtrlLoop_Park(h->pre.I_alpha, h->pre.I_beta, h->theta_pos,
                   &h->pre.Id_fdb, &h->pre.Iq_fdb);
}

void CtrlLoop_Calc(CtrlLoop_Handle *h, float Id_ref, float Iq_ref, float Vdc) {
    if (h == NULL || !h->init) {
        return;
    }
    
    float V_ff_d = 0.0f;
    float V_ff_q = 0.0f;
    
    if (h->config->enable_gvff) {
        CtrlLoop_GVFF_Step(&h->gvff, Id_ref, Iq_ref, &V_ff_d, &V_ff_q);
    }
    
    float Vd_pos, Vq_pos;
    CtrlLoop_PI_Step(&h->pi_pos_d, Id_ref, h->pre.Id_fdb, &Vd_pos);
    CtrlLoop_PI_Step(&h->pi_pos_q, Iq_ref, h->pre.Iq_fdb, &Vq_pos);
    
    Vd_pos += V_ff_d;
    Vq_pos += V_ff_q;
    
    if (h->config->enable_neg) {
        float Vd_neg, Vq_neg;
        CtrlLoop_PI_Step(&h->pi_neg_d, 0.0f, h->pre.Id_fdb, &Vd_neg);
        CtrlLoop_PI_Step(&h->pi_neg_q, 0.0f, h->pre.Iq_fdb, &Vq_neg);
        
        h->Vd_ref_neg = Vd_neg;
        h->Vq_ref_neg = Vq_neg;
    }
    
    h->Vd_ref_pos = Vd_pos;
    h->Vq_ref_pos = Vq_pos;
    
    h->theta_elec = h->theta_pos;
    
    (void)Vdc;
    
    h->state = LOOP_STATE_RUNNING;
}

void CtrlLoop_PostCalc(CtrlLoop_Handle *h) {
    if (h == NULL || !h->init) {
        return;
    }
    
    float V_alpha_pos, V_beta_pos;
    CtrlLoop_InvPark(h->Vd_ref_pos, h->Vq_ref_pos, h->theta_elec,
                      &V_alpha_pos, &V_beta_pos);
    
    float V_alpha = V_alpha_pos;
    float V_beta = V_beta_pos;
    
    if (h->config->enable_neg) {
        float V_alpha_neg, V_beta_neg;
        CtrlLoop_InvPark_Neg(h->Vd_ref_neg, h->Vq_ref_neg, h->theta_neg,
                              &V_alpha_neg, &V_beta_neg);
        V_alpha += V_alpha_neg;
        V_beta += V_beta_neg;
    }
    
    CtrlLoop_ToDuty(V_alpha, V_beta, h->config->Vdc,
                    h->config->duty_max, h->config->duty_min,
                    &h->output.duty_a, &h->output.duty_b, &h->output.duty_c);
    
    float I_mag_sq = h->pre.I_alpha * h->pre.I_alpha + 
                     h->pre.I_beta * h->pre.I_beta;
    
    if (h->config->current_limit > 0.0f) {
        if (I_mag_sq > h->config->current_limit * h->config->current_limit) {
            h->fault_flags |= 1U;
            h->state = LOOP_STATE_FAULT;
        }
    }
    
    h->tick_count++;
}

void CtrlLoop_GetDuty(const CtrlLoop_Handle *h, float *duty_a, float *duty_b, float *duty_c) {
    if (h == NULL) {
        return;
    }
    *duty_a = h->output.duty_a;
    *duty_b = h->output.duty_b;
    *duty_c = h->output.duty_c;
}

uint32_t CtrlLoop_GetFault(const CtrlLoop_Handle *h) {
    return (h != NULL) ? h->fault_flags : 0U;
}

void CtrlLoop_ClearFault(CtrlLoop_Handle *h) {
    if (h == NULL) {
        return;
    }
    h->fault_flags = 0U;
    if (h->init) {
        h->state = LOOP_STATE_READY;
    }
}
