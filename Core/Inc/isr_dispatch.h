#pragma once

#include <stdint.h>

// ISR 分片层：只做快路径，并把事件/数据指针下放到任务
void IsrDispatch_Init(void);

// 这些函数应该在中断上下文调用（例如 DMA/定时器/ADC 中断或 HAL 回调）
void IsrDispatch_OnControlTick(uint32_t ts);

// ADC + DMA 双缓冲（DBM）：DMA M0/M1 complete 时调用
void IsrDispatch_OnAdcDmaBuf0(uint32_t ts, uint32_t buf_addr);
void IsrDispatch_OnAdcDmaBuf1(uint32_t ts, uint32_t buf_addr);

void IsrDispatch_OnAdcDmaHalf(uint32_t ts, uint32_t dma_addr);
void IsrDispatch_OnAdcDmaFull(uint32_t ts, uint32_t dma_addr);

// 故障锁存（中断可调用）
void IsrDispatch_OnFault(uint32_t ts, uint32_t code, uint32_t detail);

