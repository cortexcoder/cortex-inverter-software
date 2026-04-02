/**
  ******************************************************************************
  * @file    stm32h7xx_hal_conf.h
  * @brief   HAL configuration file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef __STM32H7xx_HAL_CONF_H
#define __STM32H7xx_HAL_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* ########################## Module Selection ############################## */
#define HAL_MODULE_ENABLED  

#define HAL_ADC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_HSEM_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_RNG_MODULE_ENABLED
#define HAL_RTC_MODULE_ENABLED
#define HAL_SAI_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_USART_MODULE_ENABLED
#define HAL_WWDG_MODULE_ENABLED

#define HAL_ETH_MODULE_DISABLED
#define HAL_DCMI_MODULE_DISABLED
#define HAL_DFSDM_MODULE_DISABLED
#define HAL_DSI_MODULE_DISABLED
#define HAL_FDCAN_MODULE_DISABLED
#define HAL_FMAC_MODULE_DISABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_HASH_MODULE_DISABLED
#define HAL_HCD_MODULE_DISABLED
#define HAL_HRTIM_MODULE_DISABLED
#define HAL_IRDA_MODULE_DISABLED
#define HAL_IWDG_MODULE_DISABLED
#define HAL_JPEG_MODULE_DISABLED
#define HAL_LPTIM_MODULE_DISABLED
#define HAL_LTDC_MODULE_DISABLED
#define HAL_MMC_MODULE_DISABLED
#define HAL_NAND_MODULE_DISABLED
#define HAL_NOR_MODULE_DISABLED
#define HAL_OPAMP_MODULE_DISABLED
#define HAL_PCD_MODULE_DISABLED
#define HAL_QSPI_MODULE_DISABLED
#define HAL_RAMECC_MODULE_DISABLED
#define HAL_SMBUS_MODULE_DISABLED
#define HAL_SPDIFRX_MODULE_DISABLED
#define HAL_SRAM_MODULE_DISABLED
#define HAL_SWPMI_MODULE_DISABLED
#define HAL_TIM_MODULE_ENABLED

#define HAL_EXTI_MODULE_ENABLED

#define HAL_GPIO_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_MDMA_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_SMBUS_MODULE_DISABLED
#define HAL_I2S_MODULE_DISABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_USART_MODULE_ENABLED
#define HAL_IRDA_MODULE_DISABLED
#define HAL_SMARTCARD_MODULE_DISABLED
#define HAL_WWDG_MODULE_DISABLED
#define HAL_PCD_MODULE_DISABLED
#define HAL_HCD_MODULE_DISABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_DAC_MODULE_DISABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_RTC_MODULE_ENABLED

#define HAL_COMP_MODULE_DISABLED
#define HAL_CORDIC_MODULE_DISABLED
#define HAL_CRS_MODULE_DISABLED
#define HAL_CRYP_MODULE_DISABLED
#define HAL_DAC_MODULE_DISABLED
#define HAL_DMA2D_MODULE_DISABLED
#define HAL_DTS_MODULE_DISABLED
#define HAL_FIREWALL_MODULE_DISABLED
#define HAL_GFXMMU_MODULE_DISABLED
#define HAL_GTZC_MODULE_DISABLED
#define HAL_HASH_MODULE_DISABLED
#define HAL_HSEM_MODULE_ENABLED
#define HAL_ICACHE_MODULE_DISABLED
#define HAL_JPEG_MODULE_DISABLED
#define HAL_LPTIM_MODULE_DISABLED
#define HAL_MDF_MODULE_DISABLED
#define HAL_MMC_MODULE_DISABLED
#define HAL_NAND_MODULE_DISABLED
#define HAL_NOR_MODULE_DISABLED
#define HAL_OSPI_MODULE_DISABLED
#define HAL_OTFDEC_MODULE_DISABLED
#define HAL_PSSI_MODULE_DISABLED
#define HAL_RAMCFG_MODULE_DISABLED
#define HAL_RNG_MODULE_ENABLED
#define HAL_SAI_MODULE_ENABLED
#define HAL_SD_MODULE_DISABLED
#define HAL_SDRAM_MODULE_DISABLED
#define HAL_SMARTCARD_MODULE_DISABLED
#define HAL_SMBUS_MODULE_DISABLED
#define HAL_SPDIFRX_MODULE_DISABLED
#define HAL_SRAM_MODULE_DISABLED
#define HAL_SWPMI_MODULE_DISABLED
#define HAL_UCPD_MODULE_DISABLED
#define HAL_WWDG_MODULE_DISABLED

#define HAL_XSPI_MODULE_DISABLED


/* ########################## Oscillator Values adaptation ####################*/
#if !defined  (HSE_VALUE)
#define HSE_VALUE    8000000UL  /*!< Value of the External oscillator in Hz */
#endif

#if !defined  (HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT    100UL   /*!< Time out for HSE start up, in ms */
#endif

#if !defined  (HSI_VALUE)
#define HSI_VALUE    64000000UL /*!< Value of the Internal oscillator in Hz*/
#endif

#if !defined  (CSI_VALUE)
#define CSI_VALUE    4000000UL  /*!< Value of the Internal oscillator in Hz*/
#endif

#if !defined  (HSI48_VALUE)
#define HSI48_VALUE    48000000UL /*!< Value of the HSI48 Internal oscillator in Hz*/
#endif

#if !defined  (LSI_VALUE) 
#define LSI_VALUE  32000UL       /*!< Value of LSI in Hz*/
#endif

#if !defined  (LSE_VALUE)
#define LSE_VALUE  32768UL    /*!< Value of LSE in Hz*/
#endif

#if !defined  (EXTERNAL_SAI1_CLOCK_VALUE)
  #define EXTERNAL_SAI1_CLOCK_VALUE    48000UL /*!< Value of the SAI1 External clock source in Hz*/
#endif 

#if !defined  (EXTERNAL_SAI2_CLOCK_VALUE)
  #define EXTERNAL_SAI2_CLOCK_VALUE   48000UL /*!< Value of the SAI2 External clock source in Hz*/
#endif

/* Tip: To avoid modifying this file each time you need to use different HSE,
   ===  you can define the HSE value in your toolchain compiler preprocessor. */

/* ########################### System Configuration ######################### */
#define  VDD_VALUE                    3300UL /*!< Value of VDD in mv */
#define  TICK_INT_PRIORITY            0x0FUL /*!< tick interrupt priority */
#define  USE_RTOS                     0U
#define  PREFETCH_ENABLE              1U
#define  INSTRUCTION_CACHE_ENABLE     1U
#define  DATA_CACHE_ENABLE            1U

/* ########################## Assert Selection ############################## */
#define  USE_FULL_ASSERT    0U

/* Includes ------------------------------------------------------------------*/
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32h7xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32h7xx_hal_gpio.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32h7xx_hal_dma.h"
#endif

#ifdef HAL_MDMA_MODULE_ENABLED
  #include "stm32h7xx_hal_mdma.h"
#endif

#ifdef HAL_EXTI_MODULE_ENABLED
  #include "stm32h7xx_hal_exti.h"
#endif

#ifdef HAL_ADC_MODULE_ENABLED
  #include "stm32h7xx_hal_adc.h"
#endif

#ifdef HAL_FDCAN_MODULE_ENABLED
  #include "stm32h7xx_hal_fdcan.h"
#endif

#ifdef HAL_CEC_MODULE_ENABLED
  #include "stm32h7xx_hal_cec.h"
#endif

#ifdef HAL_COMP_MODULE_ENABLED
  #include "stm32h7xx_hal_comp.h"
#endif

#ifdef HAL_CORDIC_MODULE_ENABLED
  #include "stm32h7xx_hal_cordic.h"
#endif

#ifdef HAL_CRC_MODULE_ENABLED
  #include "stm32h7xx_hal_crc.h"
#endif

#ifdef HAL_CRYP_MODULE_ENABLED
  #include "stm32h7xx_hal_cryp.h"
#endif

#ifdef HAL_DAC_MODULE_ENABLED
  #include "stm32h7xx_hal_dac.h"
#endif

#ifdef HAL_DCMI_MODULE_ENABLED
  #include "stm32h7xx_hal_dcmi.h"
#endif

#ifdef HAL_DFSDM_MODULE_ENABLED
  #include "stm32h7xx_hal_dfsdm.h"
#endif

#ifdef HAL_DMA2D_MODULE_ENABLED
  #include "stm32h7xx_hal_dma2d.h"
#endif

#ifdef HAL_DSI_MODULE_ENABLED
  #include "stm32h7xx_hal_dsi.h"
#endif

#ifdef HAL_ETH_MODULE_ENABLED
  #include "stm32h7xx_hal_eth.h"
#endif

#ifdef HAL_ETH_LEGACY_MODULE_ENABLED
  #include "stm32h7xx_hal_eth_legacy.h"
#endif

#ifdef HAL_FIREWALL_MODULE_ENABLED
  #include "stm32h7xx_hal_firewall.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32h7xx_hal_flash.h"
#endif

#ifdef HAL_GFXMMU_MODULE_ENABLED
  #include "stm32h7xx_hal_gfxmmu.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32h7xx_hal_gpio.h"
#endif

#ifdef HAL_HASH_MODULE_ENABLED
  #include "stm32h7xx_hal_hash.h"
#endif

#ifdef HAL_HCD_MODULE_ENABLED
  #include "stm32h7xx_hal_hcd.h"
#endif

#ifdef HAL_HRTIM_MODULE_ENABLED
  #include "stm32h7xx_hal_hrtim.h"
#endif

#ifdef HAL_HSEM_MODULE_ENABLED
  #include "stm32h7xx_hal_hsem.h"
#endif

#ifdef HAL_I2C_MODULE_ENABLED
 #include "stm32h7xx_hal_i2c.h"
#endif

#ifdef HAL_I2S_MODULE_ENABLED
 #include "stm32h7xx_hal_i2s.h"
#endif

#ifdef HAL_IRDA_MODULE_ENABLED
 #include "stm32h7xx_hal_irda.h"
#endif

#ifdef HAL_IWDG_MODULE_ENABLED
 #include "stm32h7xx_hal_iwdg.h"
#endif

#ifdef HAL_JPEG_MODULE_ENABLED
 #include "stm32h7xx_hal_jpeg.h"
#endif

#ifdef HAL_LPTIM_MODULE_ENABLED
#include "stm32h7xx_hal_lptim.h"
#endif

#ifdef HAL_LTDC_MODULE_ENABLED
#include "stm32h7xx_hal_ltdc.h"
#endif

#ifdef HAL_MDF_MODULE_ENABLED
#include "stm32h7xx_hal_mdf.h"
#endif

#ifdef HAL_MMC_MODULE_ENABLED
 #include "stm32h7xx_hal_mmc.h"
#endif

#ifdef HAL_NAND_MODULE_ENABLED
  #include "stm32h7xx_hal_nand.h"
#endif

#ifdef HAL_NOR_MODULE_ENABLED
  #include "stm32h7xx_hal_nor.h"
#endif

#ifdef HAL_OPAMP_MODULE_ENABLED
#include "stm32h7xx_hal_opamp.h"
#endif

#ifdef HAL_OSPI_MODULE_ENABLED
#include "stm32h7xx_hal_ospi.h"
#endif

#ifdef HAL_OTFDEC_MODULE_ENABLED
#include "stm32h7xx_hal_otfdec.h"
#endif

#ifdef HAL_PCD_MODULE_ENABLED
 #include "stm32h7xx_hal_pcd.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
 #include "stm32h7xx_hal_pwr.h"
#endif

#ifdef HAL_PSSI_MODULE_ENABLED
 #include "stm32h7xx_hal_pssi.h"
#endif

#ifdef HAL_RAMCFG_MODULE_ENABLED
 #include "stm32h7xx_hal_ramcfg.h"
#endif

#ifdef HAL_RNG_MODULE_ENABLED
 #include "stm32h7xx_hal_rng.h"
#endif

#ifdef HAL_RTC_MODULE_ENABLED
 #include "stm32h7xx_hal_rtc.h"
#endif

#ifdef HAL_SAI_MODULE_ENABLED
 #include "stm32h7xx_hal_sai.h"
#endif

#ifdef HAL_SD_MODULE_ENABLED
 #include "stm32h7xx_hal_sd.h"
#endif

#ifdef HAL_SDRAM_MODULE_ENABLED
 #include "stm32h7xx_hal_sdram.h"
#endif

#ifdef HAL_SMARTCARD_MODULE_ENABLED
 #include "stm32h7xx_hal_smartcard.h"
#endif

#ifdef HAL_SMBUS_MODULE_ENABLED
 #include "stm32h7xx_hal_smbus.h"
#endif

#ifdef HAL_SPDIFRX_MODULE_ENABLED
 #include "stm32h7xx_hal_spdifrx.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
 #include "stm32h7xx_hal_spi.h"
#endif

#ifdef HAL_SRAM_MODULE_ENABLED
 #include "stm32h7xx_hal_sram.h"
#endif

#ifdef HAL_SWPMI_MODULE_ENABLED
 #include "stm32h7xx_hal_swpmi.h"
#endif

#ifdef HAL_TIM_MODULE_ENABLED
 #include "stm32h7xx_hal_tim.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
 #include "stm32h7xx_hal_uart.h"
#endif

#ifdef HAL_USART_MODULE_ENABLED
 #include "stm32h7xx_hal_usart.h"
#endif

#ifdef HAL_WWDG_MODULE_ENABLED
 #include "stm32h7xx_hal_wwdg.h"
#endif

#ifdef HAL_XSPI_MODULE_ENABLED
 #include "stm32h7xx_hal_xspi.h"
#endif

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t *file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32H7xx_HAL_CONF_H */
