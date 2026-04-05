#pragma once

#include <stdint.h>

// ============================================
// GPIO引脚配置 - 用户根据实际原理图填写
// ============================================

#define BSP_GPIO_PORT_A 0
#define BSP_GPIO_PORT_B 1
#define BSP_GPIO_PORT_C 2

typedef struct {
    uint8_t port;
    uint8_t pin;
} BSP_GpioPin;

typedef struct {
    uint8_t tim_instance;
    uint8_t tim_channel;
    BSP_GpioPin gpio;
    uint8_t af;
} BSP_PwmChConfig;

typedef struct {
    uint8_t adc_instance;
    uint8_t adc_channel;
    BSP_GpioPin gpio;
    uint8_t sample_time;
} BSP_AdcChConfig;

typedef struct {
    uint8_t dma_instance;
    uint8_t dma_stream;
    uint8_t dma_channel;
} BSP_DmaBaseConfig;

// ============================================
// PWM通道配置 - 3相逆变器
// ============================================

#define BSP_PWM_CH_A   0
#define BSP_PWM_CH_B   1
#define BSP_PWM_CH_C   2
#define BSP_PWM_CH_COUNT 3

typedef struct {
    BSP_PwmChConfig ch[BSP_PWM_CH_COUNT];
} BSP_PwmConfig;

// ============================================
// ADC通道配置 - 6个采样通道
// ============================================

#define BSP_ADC_CH_I_A     0
#define BSP_ADC_CH_I_B     1
#define BSP_ADC_CH_I_C     2
#define BSP_ADC_CH_V_BUS   3
#define BSP_ADC_CH_V_OUT   4
#define BSP_ADC_CH_TEMP    5
#define BSP_ADC_CH_COUNT   6

typedef struct {
    BSP_AdcChConfig ch[BSP_ADC_CH_COUNT];
} BSP_AdcConfig;

// ============================================
// DMA配置
// ============================================

typedef struct {
    BSP_DmaBaseConfig dma;
    void *buf0;
    void *buf1;
    uint16_t buf_size;
} BSP_DmaConfig;

// ============================================
// 默认空配置 (无实际IO映射)
// ============================================

extern const BSP_PwmConfig  bsp_pwm_default_config;
extern const BSP_AdcConfig  bsp_adc_default_config;
extern const BSP_DmaConfig  bsp_dma_default_config;
