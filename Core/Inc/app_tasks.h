#pragma once

#include <stdint.h>

// 任务创建（由 App_Init 调用）
void App_TasksCreate(void);

// 给任务层投递“控制 tick”等事件（通常由 ISR dispatch 调用）
void App_PostControlTickFromIsr(uint32_t ts);

// ADC + DMA 双缓冲（DBM）推荐接口：buf_index=0/1
void App_PostAdcBufReadyFromIsr(uint32_t ts, uint32_t buf_addr, uint32_t buf_index);

void App_PostAdcHalfFromIsr(uint32_t ts, uint32_t dma_addr);
void App_PostAdcFullFromIsr(uint32_t ts, uint32_t dma_addr);

// 故障接口（任务/ISR 都可能触发）
void App_FaultLatchFromIsr(uint32_t code, uint32_t detail);
void App_FaultLatch(uint32_t code, uint32_t detail);

