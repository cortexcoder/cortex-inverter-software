#include "bsp_io_map.h"
#include "bsp_hrpwm.h"

const BSP_PwmConfig bsp_pwm_default_config = {
    .ch = {
        [BSP_PWM_CH_A] = {
            .tim_instance = 1,
            .tim_channel = 1,
            .gpio = { BSP_GPIO_PORT_A, 7 },
            .af = 1,
        },
        [BSP_PWM_CH_B] = {
            .tim_instance = 1,
            .tim_channel = 2,
            .gpio = { BSP_GPIO_PORT_A, 8 },
            .af = 1,
        },
        [BSP_PWM_CH_C] = {
            .tim_instance = 1,
            .tim_channel = 3,
            .gpio = { BSP_GPIO_PORT_A, 9 },
            .af = 1,
        },
    },
};

const BSP_AdcConfig bsp_adc_default_config = {
    .ch = {
        [BSP_ADC_CH_I_A] = {
            .adc_instance = 1,
            .adc_channel = 1,
            .gpio = { BSP_GPIO_PORT_A, 0 },
            .sample_time = 6,
        },
        [BSP_ADC_CH_I_B] = {
            .adc_instance = 1,
            .adc_channel = 2,
            .gpio = { BSP_GPIO_PORT_A, 1 },
            .sample_time = 6,
        },
        [BSP_ADC_CH_I_C] = {
            .adc_instance = 1,
            .adc_channel = 3,
            .gpio = { BSP_GPIO_PORT_A, 2 },
            .sample_time = 6,
        },
        [BSP_ADC_CH_V_BUS] = {
            .adc_instance = 1,
            .adc_channel = 4,
            .gpio = { BSP_GPIO_PORT_A, 4 },
            .sample_time = 6,
        },
        [BSP_ADC_CH_V_OUT] = {
            .adc_instance = 1,
            .adc_channel = 5,
            .gpio = { BSP_GPIO_PORT_A, 5 },
            .sample_time = 6,
        },
        [BSP_ADC_CH_TEMP] = {
            .adc_instance = 1,
            .adc_channel = 16,
            .gpio = { 0, 0 },
            .sample_time = 6,
        },
    },
};

const BSP_DmaConfig bsp_dma_default_config = {
    .dma = {
        .dma_instance = 2,
        .dma_stream = 0,
        .dma_channel = 0,
    },
    .buf0 = (void*)0,
    .buf1 = (void*)0,
    .buf_size = BSP_HRPWM_SAMPLES_PER_PERIOD * BSP_ADC_CH_COUNT,
};
