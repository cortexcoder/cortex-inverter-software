#include <stdint.h>

#include "isr_dispatch.h"

// 说明：
// - 真实 STM32H7 + HAL 工程里，CubeMX 会生成自己的 `stm32h7xx_it.c`。
// - 你可以把本文件里的“调用 IsrDispatch_*”片段合并到 CubeMX 生成的 ISR 中，
//   或在 HAL 回调里调用（例如 HAL_ADC_ConvHalfCpltCallback）。
//
// 这里不包含具体芯片头文件和 HAL 头文件，避免脱离工程无法编译。

static inline uint32_t ReadFastTimestamp(void) {
  // TODO: 建议用 DWT->CYCCNT 或定时器计数器作为时间戳来源
  return 0;
}

// 示例：控制 tick 中断（比如 TIMx Update IRQ）
void TIMx_IRQHandler(void) {
  const uint32_t ts = ReadFastTimestamp();

  // TODO: 清除 TIM 中断标志（在真实工程里使用 HAL 或直接寄存器操作）
  IsrDispatch_OnControlTick(ts);
}

// 示例：ADC DMA half/full 完成中断入口（具体 IRQ 名称按你配置的 DMA Stream 而定）
void DMAx_Streamy_IRQHandler(void) {
  const uint32_t ts = ReadFastTimestamp();

  // TODO: 判断 half/full 标志、清除标志、取得 buffer 地址
  // 以下仅演示“分片下放”的调用方式
  // IsrDispatch_OnAdcDmaHalf(ts, (uint32_t)buffer_addr);
  // IsrDispatch_OnAdcDmaFull(ts, (uint32_t)buffer_addr);

  (void)ts;
}

