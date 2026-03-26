#pragma once

#include <stdint.h>

// 这份头文件用于“粘贴/集成”到 STM32H743 + CubeMX + HAL 工程中：
// - PWM 触发 ADC（外部触发）
// - DMA 双缓冲（DBM，M0/M1）
//
// 注意：本仓库不自带 HAL/CMSIS 设备头文件；在 CubeMX 工程中使用时才会编译。

#ifdef USE_HAL_DRIVER
#include "stm32h7xx_hal.h"

// 给 ADC 绑定 DMA DBM 回调，并启动双缓冲传输（模板）
HAL_StatusTypeDef H743_AdcDmaDbm_Start(ADC_HandleTypeDef *hadc,
                                      DMA_HandleTypeDef *hdma,
                                      uint32_t adc_dr_addr,
                                      uint32_t buf0_addr,
                                      uint32_t buf1_addr,
                                      uint32_t sample_count);
#endif

