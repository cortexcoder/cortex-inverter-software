#pragma once

#include <stdint.h>
#include "bsp_hrpwm.h"

#define SAMPLER_CHANNEL_COUNT       6
#define SAMPLER_SAMPLES_PER_PERIOD   BSP_HRPWM_SAMPLES_PER_PERIOD

#define SAMPLER_CH_I_A           0
#define SAMPLER_CH_I_B           1
#define SAMPLER_CH_I_C           2
#define SAMPLER_CH_V_BUS         3
#define SAMPLER_CH_V_OUT         4
#define SAMPLER_CH_TEMP          5

typedef struct {
    float offset;
    float gain;
} Sampler_CalParam;

typedef struct {
    uint16_t raw[SAMPLER_SAMPLES_PER_PERIOD][SAMPLER_CHANNEL_COUNT];
    float cal[SAMPLER_SAMPLES_PER_PERIOD][SAMPLER_CHANNEL_COUNT];
} Sampler_Buffer;

typedef struct {
    uint16_t raw_min;
    uint16_t raw_max;
    float offset;
    float gain;
    uint8_t valid;
} Sampler_CalData;

typedef struct {
    Sampler_Buffer buf[2];
    volatile uint8_t buf_active;
    volatile uint8_t buf_ready;
    volatile uint8_t fault_flags;

    Sampler_CalData cal[SAMPLER_CHANNEL_COUNT];

    uint32_t period_count;
    uint32_t error_count;
} Sampler_Handle;

typedef struct {
    uint8_t adc_instance;
    uint8_t dma_instance;
    uint32_t adc_dr_addr;
    uint16_t samples_per_period;
    uint16_t buf_size;
} Sampler_Config;

#define SAMPLER_DECLARE_HANDLE(name) \
    static Sampler_Handle name

#define SAMPLER_DECLARE_CONFIG(cfg) \
    static const Sampler_Config cfg

#define SAMPLER_BUF_SIZE_PERIOD  (SAMPLER_SAMPLES_PER_PERIOD * SAMPLER_CHANNEL_COUNT)

void Sampler_Init(Sampler_Handle *h, const Sampler_Config *cfg);

void Sampler_SetCalOffset(Sampler_Handle *h, uint8_t ch, float offset);

void Sampler_SetCalGain(Sampler_Handle *h, uint8_t ch, float gain);

float Sampler_GetCalOffset(const Sampler_Handle *h, uint8_t ch);

float Sampler_GetCalGain(const Sampler_Handle *h, uint8_t ch);

void Sampler_DoCalibrate(Sampler_Handle *h, uint8_t ch, uint16_t raw_zero);

void Sampler_ProcessBuffer(Sampler_Handle *h);

uint8_t Sampler_IsBufReady(const Sampler_Handle *h);

void Sampler_AckBufReady(Sampler_Handle *h);

float Sampler_GetI_A_Avg(const Sampler_Handle *h);

float Sampler_GetI_B_Avg(const Sampler_Handle *h);

float Sampler_GetI_C_Avg(const Sampler_Handle *h);

float Sampler_GetV_Bus_Avg(const Sampler_Handle *h);

float Sampler_GetV_Out_Avg(const Sampler_Handle *h);

float Sampler_GetTemp_Avg(const Sampler_Handle *h);

float Sampler_GetI_A_Sample(const Sampler_Handle *h, uint8_t sample_idx);

float Sampler_GetI_B_Sample(const Sampler_Handle *h, uint8_t sample_idx);

float Sampler_GetI_C_Sample(const Sampler_Handle *h, uint8_t sample_idx);

float Sampler_GetV_Bus_Sample(const Sampler_Handle *h, uint8_t sample_idx);

float Sampler_GetV_Out_Sample(const Sampler_Handle *h, uint8_t sample_idx);

float Sampler_GetTemp_Sample(const Sampler_Handle *h, uint8_t sample_idx);

uint16_t Sampler_GetRaw(const Sampler_Handle *h, uint8_t sample_idx, uint8_t ch);

uint32_t Sampler_GetPeriodCount(const Sampler_Handle *h);

uint32_t Sampler_GetErrorCount(const Sampler_Handle *h);

void Sampler_OnDmaBuf0(Sampler_Handle *h, uint32_t timestamp);

void Sampler_OnDmaBuf1(Sampler_Handle *h, uint32_t timestamp);

void Sampler_OnFault(Sampler_Handle *h, uint32_t timestamp, uint8_t fault_id);

#define SAMPLER_GET_I_A(h)    ((h)->cal[SAMPLER_CH_I_A].offset)
#define SAMPLER_GET_I_B(h)    ((h)->cal[SAMPLER_CH_I_B].offset)
#define SAMPLER_GET_I_C(h)    ((h)->cal[SAMPLER_CH_I_C].offset)
#define SAMPLER_GET_V_BUS(h)  ((h)->cal[SAMPLER_CH_V_BUS].offset)
#define SAMPLER_GET_V_OUT(h)  ((h)->cal[SAMPLER_CH_V_OUT].offset)
#define SAMPLER_GET_TEMP(h)   ((h)->cal[SAMPLER_CH_TEMP].offset)

#define SAMPLER_SET_OFFSET_I_A(h, v)    do { (h)->cal[SAMPLER_CH_I_A].offset = (v); } while(0)
#define SAMPLER_SET_OFFSET_I_B(h, v)    do { (h)->cal[SAMPLER_CH_I_B].offset = (v); } while(0)
#define SAMPLER_SET_OFFSET_I_C(h, v)    do { (h)->cal[SAMPLER_CH_I_C].offset = (v); } while(0)
#define SAMPLER_SET_OFFSET_V_BUS(h, v)  do { (h)->cal[SAMPLER_CH_V_BUS].offset = (v); } while(0)
#define SAMPLER_SET_OFFSET_V_OUT(h, v)  do { (h)->cal[SAMPLER_CH_V_OUT].offset = (v); } while(0)
#define SAMPLER_SET_OFFSET_TEMP(h, v)   do { (h)->cal[SAMPLER_CH_TEMP].offset = (v); } while(0)

#define SAMPLER_SET_GAIN_I_A(h, v)    do { (h)->cal[SAMPLER_CH_I_A].gain = (v); } while(0)
#define SAMPLER_SET_GAIN_I_B(h, v)    do { (h)->cal[SAMPLER_CH_I_B].gain = (v); } while(0)
#define SAMPLER_SET_GAIN_I_C(h, v)    do { (h)->cal[SAMPLER_CH_I_C].gain = (v); } while(0)
#define SAMPLER_SET_GAIN_V_BUS(h, v)  do { (h)->cal[SAMPLER_CH_V_BUS].gain = (v); } while(0)
#define SAMPLER_SET_GAIN_V_OUT(h, v)  do { (h)->cal[SAMPLER_CH_V_OUT].gain = (v); } while(0)
#define SAMPLER_SET_GAIN_TEMP(h, v)   do { (h)->cal[SAMPLER_CH_TEMP].gain = (v); } while(0)

#define SAMPLER_DEFAULT_GAIN    (1.0f)
#define SAMPLER_ADC_MAX         (4095.0f)
#define SAMPLER_ADC_VREF        (3.3f)
#define SAMPLER_SHUNT_RESISTOR   (0.001f)
#define SAMPLER_OPAMP_GAIN       (10.0f)
#define SAMPLER_VDIV_R1         (100.0f)
#define SAMPLER_VDIV_R2         (100.0f)
