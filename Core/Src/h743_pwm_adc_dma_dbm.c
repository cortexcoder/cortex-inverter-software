#include "h743_pwm_adc_dma_dbm.h"

#ifdef USE_HAL_DRIVER

#include "isr_dispatch.h"

// 你可以把时间戳替换为 DWT->CYCCNT 或 TIM 计数器读数
static inline uint32_t FastTimestamp(void) { return 0U; }

// DMA M0 完成：buf0 ready
static void Dma_XferCpltCallback(DMA_HandleTypeDef *hdma) {
  (void)hdma;
  const uint32_t ts = FastTimestamp();
  // 当前回调对应 MultiBuffer 的 Memory0 完成
  // buf 地址建议通过你自己的静态变量保存；这里用 HAL 提供的 Memory 0 地址
  const uint32_t buf0 = (uint32_t)hdma->Instance->M0AR;
  IsrDispatch_OnAdcDmaBuf0(ts, buf0);
}

// DMA M1 完成：buf1 ready
static void Dma_XferM1CpltCallback(DMA_HandleTypeDef *hdma) {
  (void)hdma;
  const uint32_t ts = FastTimestamp();
  const uint32_t buf1 = (uint32_t)hdma->Instance->M1AR;
  IsrDispatch_OnAdcDmaBuf1(ts, buf1);
}

// 可选：错误回调 -> 锁存故障
static void Dma_ErrorCallback(DMA_HandleTypeDef *hdma) {
  (void)hdma;
  const uint32_t ts = FastTimestamp();
  IsrDispatch_OnFault(ts, 1U, 0U);
}

HAL_StatusTypeDef H743_AdcDmaDbm_Start(ADC_HandleTypeDef *hadc,
                                      DMA_HandleTypeDef *hdma,
                                      uint32_t adc_dr_addr,
                                      uint32_t buf0_addr,
                                      uint32_t buf1_addr,
                                      uint32_t sample_count) {
  if ((hadc == NULL) || (hdma == NULL)) {
    return HAL_ERROR;
  }

  // 绑定回调（DBM 专用：M0 / M1 完成）
  hdma->XferCpltCallback = Dma_XferCpltCallback;
  hdma->XferM1CpltCallback = Dma_XferM1CpltCallback;
  hdma->XferErrorCallback = Dma_ErrorCallback;

  // 启动 DMA 双缓冲（DBM）
  // adc_dr_addr 通常是 (uint32_t)&ADC1->DR
  // buf0/buf1 是两个等长 buffer
  HAL_StatusTypeDef st = HAL_DMAEx_MultiBufferStart_IT(hdma,
                                                      adc_dr_addr,
                                                      buf0_addr,
                                                      buf1_addr,
                                                      sample_count);
  if (st != HAL_OK) {
    return st;
  }

  // 让 ADC 产生 DMA 请求并开始转换：
  // - ADC 的外部触发（来自 PWM 定时器）应由 CubeMX 配置
  // - 这里不调用 HAL_ADC_Start_DMA()，因为我们已经用 MultiBufferStart_IT 启动了 DMA
  // - 对 H7 而言，通常需要设置 ADC 的 DMA 使能位并启动 ADC
  // 由于不同 CubeMX 版本/配置差异，这部分建议你在 CubeMX 生成代码基础上最小修改：
  //   - 确保 hadc->Instance 的 DMA 相关配置已就绪
  //   - 然后调用 HAL_ADC_Start(hadc) 或 HAL_ADCEx_MultiModeStart_DMA 等（按你的工程）
  (void)hadc;

  return HAL_OK;
}

#endif

