#pragma once

#include <stdint.h>
#include "bsp_io_map.h"
#include "bsp_hrpwm.h"

typedef struct BSP_Adc_s {
    const BSP_AdcConfig *config;
    volatile uint8_t active_buf;
    volatile uint8_t buf_ready;
    uint32_t sample_count;
} BSP_Adc;

typedef struct BSP_AdcDma_s {
    const BSP_DmaConfig *config;
    BSP_Adc *adc;
    volatile uint8_t current_buf;
} BSP_AdcDma;

void BSP_Adc_Init(BSP_Adc *adc, const BSP_AdcConfig *cfg);

void BSP_AdcDma_Init(BSP_AdcDma *dma, const BSP_DmaConfig *cfg, BSP_Adc *adc);

void BSP_AdcDma_Start(BSP_AdcDma *dma);

void BSP_AdcDma_Stop(BSP_AdcDma *dma);

uint8_t BSP_Adc_IsBufReady(const BSP_Adc *adc);

uint8_t BSP_Adc_GetActiveBuf(const BSP_Adc *adc);

void BSP_Adc_AckBuf(BSP_Adc *adc);

uint32_t BSP_Adc_GetSampleCount(const BSP_Adc *adc);
