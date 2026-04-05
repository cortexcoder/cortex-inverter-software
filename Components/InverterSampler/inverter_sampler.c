#include "inverter_sampler.h"
#include <string.h>

#define ADC_TO_VOLTS(adc_val, vref, adc_max)   ((float)(adc_val) * (vref) / (adc_max))
#define VOLTS_TO_AMPS(volts, shunt, gain)     ((volts) / (shunt) / (gain))
#define VOLTS_TO_VOLTS_DIVIDER(volts, r1, r2) ((volts) * ((r1) + (r2)) / (r2))

static void ApplyCalibration(const Sampler_CalData *cal, uint16_t raw, float *out) {
    if (!cal->valid) {
        *out = (float)raw;
        return;
    }
    float volts = ADC_TO_VOLTS((float)raw, SAMPLER_ADC_VREF, SAMPLER_ADC_MAX);
    volts = (volts - cal->offset) * cal->gain;
    *out = volts;
}

static float RawToCurrent(float volts) {
    return volts / SAMPLER_SHUNT_RESISTOR / SAMPLER_OPAMP_GAIN;
}

static float RawToVoltage(float volts) {
    return VOLTS_TO_VOLTS_DIVIDER(volts, SAMPLER_VDIV_R1, SAMPLER_VDIV_R2);
}

void Sampler_Init(Sampler_Handle *h, const Sampler_Config *cfg) {
    (void)cfg;
    memset(h, 0, sizeof(Sampler_Handle));
    h->buf_active = 0;
    h->buf_ready = 0;
    h->fault_flags = 0;
    h->period_count = 0;
    h->error_count = 0;

    for (int i = 0; i < SAMPLER_CHANNEL_COUNT; i++) {
        h->cal[i].offset = 0.0f;
        h->cal[i].gain = SAMPLER_DEFAULT_GAIN;
        h->cal[i].valid = 0;
        h->cal[i].raw_min = 0;
        h->cal[i].raw_max = 0xFFFF;
    }
}

void Sampler_SetCalOffset(Sampler_Handle *h, uint8_t ch, float offset) {
    if (ch >= SAMPLER_CHANNEL_COUNT) return;
    h->cal[ch].offset = offset;
    h->cal[ch].valid = 1;
}

void Sampler_SetCalGain(Sampler_Handle *h, uint8_t ch, float gain) {
    if (ch >= SAMPLER_CHANNEL_COUNT) return;
    h->cal[ch].gain = gain;
    h->cal[ch].valid = 1;
}

float Sampler_GetCalOffset(const Sampler_Handle *h, uint8_t ch) {
    if (ch >= SAMPLER_CHANNEL_COUNT) return 0.0f;
    return h->cal[ch].offset;
}

float Sampler_GetCalGain(const Sampler_Handle *h, uint8_t ch) {
    if (ch >= SAMPLER_CHANNEL_COUNT) return 1.0f;
    return h->cal[ch].gain;
}

void Sampler_DoCalibrate(Sampler_Handle *h, uint8_t ch, uint16_t raw_zero) {
    if (ch >= SAMPLER_CHANNEL_COUNT) return;
    float volts_zero = ADC_TO_VOLTS((float)raw_zero, SAMPLER_ADC_VREF, SAMPLER_ADC_MAX);
    h->cal[ch].offset = volts_zero;
    h->cal[ch].gain = 1.0f;
    h->cal[ch].valid = 1;
}

void Sampler_ProcessBuffer(Sampler_Handle *h) {
    uint8_t idx = h->buf_active ^ 1;
    Sampler_Buffer *buf = &h->buf[idx];

    for (uint8_t s = 0; s < SAMPLER_SAMPLES_PER_PERIOD; s++) {
        float volts;

        volts = buf->raw[s][SAMPLER_CH_I_A];
        ApplyCalibration(&h->cal[SAMPLER_CH_I_A], (uint16_t)volts, &buf->cal[s][SAMPLER_CH_I_A]);
        volts = RawToCurrent(buf->cal[s][SAMPLER_CH_I_A]);

        volts = buf->raw[s][SAMPLER_CH_I_B];
        ApplyCalibration(&h->cal[SAMPLER_CH_I_B], (uint16_t)volts, &buf->cal[s][SAMPLER_CH_I_B]);
        volts = RawToCurrent(buf->cal[s][SAMPLER_CH_I_B]);

        volts = buf->raw[s][SAMPLER_CH_I_C];
        ApplyCalibration(&h->cal[SAMPLER_CH_I_C], (uint16_t)volts, &buf->cal[s][SAMPLER_CH_I_C]);
        volts = RawToCurrent(buf->cal[s][SAMPLER_CH_I_C]);

        volts = buf->raw[s][SAMPLER_CH_V_BUS];
        ApplyCalibration(&h->cal[SAMPLER_CH_V_BUS], (uint16_t)volts, &buf->cal[s][SAMPLER_CH_V_BUS]);
        volts = RawToVoltage(buf->cal[s][SAMPLER_CH_V_BUS]);

        volts = buf->raw[s][SAMPLER_CH_V_OUT];
        ApplyCalibration(&h->cal[SAMPLER_CH_V_OUT], (uint16_t)volts, &buf->cal[s][SAMPLER_CH_V_OUT]);
        volts = RawToVoltage(buf->cal[s][SAMPLER_CH_V_OUT]);

        volts = buf->raw[s][SAMPLER_CH_TEMP];
        ApplyCalibration(&h->cal[SAMPLER_CH_TEMP], (uint16_t)volts, &buf->cal[s][SAMPLER_CH_TEMP]);
    }
}

uint8_t Sampler_IsBufReady(const Sampler_Handle *h) {
    return h->buf_ready;
}

void Sampler_AckBufReady(Sampler_Handle *h) {
    h->buf_ready = 0;
}

static float GetChannelAvg(const Sampler_Handle *h, uint8_t ch) {
    uint8_t idx = h->buf_active ^ 1;
    float sum = 0.0f;
    for (uint8_t s = 0; s < SAMPLER_SAMPLES_PER_PERIOD; s++) {
        sum += h->buf[idx].cal[s][ch];
    }
    return sum / (float)SAMPLER_SAMPLES_PER_PERIOD;
}

static float GetChannelSample(const Sampler_Handle *h, uint8_t ch, uint8_t sample_idx) {
    if (sample_idx >= SAMPLER_SAMPLES_PER_PERIOD) {
        return 0.0f;
    }
    uint8_t idx = h->buf_active ^ 1;
    return h->buf[idx].cal[sample_idx][ch];
}

float Sampler_GetI_A_Avg(const Sampler_Handle *h) {
    return GetChannelAvg(h, SAMPLER_CH_I_A);
}

float Sampler_GetI_B_Avg(const Sampler_Handle *h) {
    return GetChannelAvg(h, SAMPLER_CH_I_B);
}

float Sampler_GetI_C_Avg(const Sampler_Handle *h) {
    return GetChannelAvg(h, SAMPLER_CH_I_C);
}

float Sampler_GetV_Bus_Avg(const Sampler_Handle *h) {
    return GetChannelAvg(h, SAMPLER_CH_V_BUS);
}

float Sampler_GetV_Out_Avg(const Sampler_Handle *h) {
    return GetChannelAvg(h, SAMPLER_CH_V_OUT);
}

float Sampler_GetTemp_Avg(const Sampler_Handle *h) {
    return GetChannelAvg(h, SAMPLER_CH_TEMP);
}

float Sampler_GetI_A_Sample(const Sampler_Handle *h, uint8_t sample_idx) {
    return GetChannelSample(h, SAMPLER_CH_I_A, sample_idx);
}

float Sampler_GetI_B_Sample(const Sampler_Handle *h, uint8_t sample_idx) {
    return GetChannelSample(h, SAMPLER_CH_I_B, sample_idx);
}

float Sampler_GetI_C_Sample(const Sampler_Handle *h, uint8_t sample_idx) {
    return GetChannelSample(h, SAMPLER_CH_I_C, sample_idx);
}

float Sampler_GetV_Bus_Sample(const Sampler_Handle *h, uint8_t sample_idx) {
    return GetChannelSample(h, SAMPLER_CH_V_BUS, sample_idx);
}

float Sampler_GetV_Out_Sample(const Sampler_Handle *h, uint8_t sample_idx) {
    return GetChannelSample(h, SAMPLER_CH_V_OUT, sample_idx);
}

float Sampler_GetTemp_Sample(const Sampler_Handle *h, uint8_t sample_idx) {
    return GetChannelSample(h, SAMPLER_CH_TEMP, sample_idx);
}

uint16_t Sampler_GetRaw(const Sampler_Handle *h, uint8_t sample_idx, uint8_t ch) {
    if (sample_idx >= SAMPLER_SAMPLES_PER_PERIOD) return 0;
    if (ch >= SAMPLER_CHANNEL_COUNT) return 0;
    uint8_t idx = h->buf_active ^ 1;
    return h->buf[idx].raw[sample_idx][ch];
}

uint32_t Sampler_GetPeriodCount(const Sampler_Handle *h) {
    return h->period_count;
}

uint32_t Sampler_GetErrorCount(const Sampler_Handle *h) {
    return h->error_count;
}

void Sampler_OnDmaBuf0(Sampler_Handle *h, uint32_t timestamp) {
    (void)timestamp;
    h->buf_active = 0;
    h->buf_ready = 1;
    h->period_count++;
}

void Sampler_OnDmaBuf1(Sampler_Handle *h, uint32_t timestamp) {
    (void)timestamp;
    h->buf_active = 1;
    h->buf_ready = 1;
    h->period_count++;
}

void Sampler_OnFault(Sampler_Handle *h, uint32_t timestamp, uint8_t fault_id) {
    (void)timestamp;
    h->fault_flags |= (1U << fault_id);
    h->error_count++;
}
