#include "bsp_adc.h"

void BSP_Adc_Init(BSP_Adc *adc, const BSP_AdcConfig *cfg) {
    adc->config = cfg;
    adc->active_buf = 0;
    adc->buf_ready = 0;
    adc->sample_count = 0;
}

void BSP_AdcDma_Init(BSP_AdcDma *dma, const BSP_DmaConfig *cfg, BSP_Adc *adc) {
    dma->config = cfg;
    dma->adc = adc;
    dma->current_buf = 0;
}

void BSP_AdcDma_Start(BSP_AdcDma *dma) {
    (void)dma;
}

void BSP_AdcDma_Stop(BSP_AdcDma *dma) {
    (void)dma;
}

uint8_t BSP_Adc_IsBufReady(const BSP_Adc *adc) {
    return adc->buf_ready;
}

uint8_t BSP_Adc_GetActiveBuf(const BSP_Adc *adc) {
    return adc->active_buf;
}

void BSP_Adc_AckBuf(BSP_Adc *adc) {
    adc->buf_ready = 0;
}

uint32_t BSP_Adc_GetSampleCount(const BSP_Adc *adc) {
    return adc->sample_count;
}
