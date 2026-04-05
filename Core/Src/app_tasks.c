#include "app_tasks.h"

#include "app_events.h"
#include "inverter_types.h"
#include "inverter_ctrl.h"
#include "inverter_sampler.h"
#include "inverter_state.h"

#include "cmsis_os2.h"

#include "pi_ctrl.h"
#include "clarke.h"
#include "lpf.h"
#include "transforms.h"
#include "svpwm.h"
#include "math_utils.h"
#include "biquad.h"
#include "notch.h"

static Inv_Handle s_inv_ctrl;
static const Inv_Config s_inv_ctrl_cfg = {
    .sampler = {
        .adc_instance = 1,
        .dma_instance = 1,
        .adc_dr_addr = 0x40022040,
        .samples_per_period = SAMPLER_SAMPLES_PER_PERIOD,
        .buf_size = SAMPLER_BUF_SIZE_PERIOD,
    },
    .control_period_us = CTRL_PERIOD_US,
    .startup_delay_ms = 100,
    .fault_recover_delay_ms = 500,
    .vbus_min = 150.0f,
    .vbus_max = 420.0f,
    .i_max = 50.0f,
    .temp_max = 85.0f,
    .fault_mask = 0xFF,
};

static Transforms_Handle s_transforms;
static Math_ClarkeFactors s_clarke_factors;
static Math_ParkFactors s_park_factors;

static PiCtrl_Handle s_pi_d;
static const PiCtrl_Config s_pi_d_cfg = {
    .kp = 0.1f,
    .ki = 0.01f,
    .out_max = 1.0f,
    .out_min = -1.0f,
    .Ts = 1.0f / 48000.0f,
};

static PiCtrl_Handle s_pi_q;
static const PiCtrl_Config s_pi_q_cfg = {
    .kp = 0.1f,
    .ki = 0.01f,
    .out_max = 1.0f,
    .out_min = -1.0f,
    .Ts = 1.0f / 48000.0f,
};

static BiquadHandle s_biquad_lpf;
static BiquadConfig s_biquad_lpf_cfg = {
    .fc = 1000.0f,
    .Q = 0.707f,
    .Ts = 1.0f / 48000.0f,
};

static NotchHandle s_notch;
static NotchConfig s_notch_cfg = {
    .fc = 1000.0f,
    .Q = 10.0f,
    .depth_db = 3.0f,
    .Ts = 1.0f / 48000.0f,
};

static osMessageQueueId_t s_evt_q = NULL;
static osEventFlagsId_t s_fault_flags = NULL;

static const uint32_t kEvtQueueDepth = 64;

static void CommsTask(void *argument);
static void HousekeepingTask(void *argument);

void App_TasksCreate(void) {
    Transforms_Init(&s_transforms);
    Math_ClarkeFactors_Init(&s_clarke_factors, 1.0f / 48000.0f);
    Math_ParkFactors_Init(&s_park_factors, 0.0f);

    PiCtrl_Init(&s_pi_d, &s_pi_d_cfg);
    PiCtrl_Init(&s_pi_q, &s_pi_q_cfg);

    Biquad_LPF(&s_biquad_lpf_cfg, s_biquad_lpf_cfg.fc, s_biquad_lpf_cfg.Q, s_biquad_lpf_cfg.Ts);
    Biquad_Init(&s_biquad_lpf, &s_biquad_lpf_cfg);

    Notch_Config(&s_notch_cfg, s_notch_cfg.fc, s_notch_cfg.Q, s_notch_cfg.depth_db, s_notch_cfg.Ts);
    Notch_Init(&s_notch, &s_notch_cfg);

    Inv_Init(&s_inv_ctrl, &s_inv_ctrl_cfg);

    if (s_evt_q == NULL) {
        s_evt_q = osMessageQueueNew(kEvtQueueDepth, sizeof(app_event_t), NULL);
    }
    if (s_fault_flags == NULL) {
        s_fault_flags = osEventFlagsNew(NULL);
    }

    (void)osThreadNew(CommsTask, NULL, &(osThreadAttr_t){
                                        .name = "comms",
                                        .priority = osPriorityNormal,
                                        .stack_size = 1536,
                                    });

    (void)osThreadNew(HousekeepingTask, NULL, &(osThreadAttr_t){
                                              .name = "hk",
                                              .priority = osPriorityLow,
                                              .stack_size = 1024,
                                          });
}

static inline void PostEvtFromIsr(app_event_t *e) {
    if (s_evt_q == NULL) {
        return;
    }
    (void)osMessageQueuePut(s_evt_q, e, 0U, 0U);
}

void App_PostControlTickFromIsr(uint32_t ts) {
    app_event_t e = {
        .type = APP_EVT_CTRL_TICK,
        .ts = ts,
        .arg0 = 0,
        .arg1 = 0,
    };
    PostEvtFromIsr(&e);
}

void App_PostAdcBufReadyFromIsr(uint32_t ts, uint32_t buf_addr, uint32_t buf_index) {
    app_event_t e = {
        .type = (buf_index == 0U) ? APP_EVT_ADC_BUF0_READY : APP_EVT_ADC_BUF1_READY,
        .ts = ts,
        .arg0 = buf_addr,
        .arg1 = buf_index,
    };
    PostEvtFromIsr(&e);
}

void App_PostAdcHalfFromIsr(uint32_t ts, uint32_t dma_addr) {
    app_event_t e = {
        .type = APP_EVT_ADC_HALF,
        .ts = ts,
        .arg0 = dma_addr,
        .arg1 = 0,
    };
    PostEvtFromIsr(&e);
}

void App_PostAdcFullFromIsr(uint32_t ts, uint32_t dma_addr) {
    app_event_t e = {
        .type = APP_EVT_ADC_FULL,
        .ts = ts,
        .arg0 = dma_addr,
        .arg1 = 0,
    };
    PostEvtFromIsr(&e);
}

void App_FaultLatchFromIsr(uint32_t code, uint32_t detail) {
    (void)detail;
    if (s_fault_flags != NULL) {
        const uint32_t idx = code & 0x1FU;
        const uint32_t bit = 1U << idx;
        (void)osEventFlagsSet(s_fault_flags, bit);
    }

    app_event_t e = {
        .type = APP_EVT_FAULT_LATCHED,
        .ts = 0,
        .arg0 = code,
        .arg1 = detail,
    };
    PostEvtFromIsr(&e);
}

void App_FaultLatch(uint32_t code, uint32_t detail) {
    App_FaultLatchFromIsr(code, detail);
}

static void CommsTask(void *argument) {
    (void)argument;
    for (;;) {
        (void)osDelay(10);
    }
}

static void HousekeepingTask(void *argument) {
    (void)argument;
    for (;;) {
        (void)osDelay(100);
    }
}

void Task1ms(void) {
    Inv_ControlTask_1ms(&s_inv_ctrl);
}

void Task5ms(void) {
    Inv_ControlTask_5ms(&s_inv_ctrl);
}

void Task10ms(void) {
    Inv_ControlTask_10ms(&s_inv_ctrl);
}

void Task50ms(void) {
    Inv_ControlTask_50ms(&s_inv_ctrl);
}

void Task1000ms(void) {
    Inv_ControlTask_1000ms(&s_inv_ctrl);
}
