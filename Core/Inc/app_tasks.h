#pragma once

#include <stdint.h>

void App_TasksCreate(void);

void App_PostControlTickFromIsr(uint32_t ts);

void App_PostAdcBufReadyFromIsr(uint32_t ts, uint32_t buf_addr, uint32_t buf_index);

void App_PostAdcHalfFromIsr(uint32_t ts, uint32_t dma_addr);
void App_PostAdcFullFromIsr(uint32_t ts, uint32_t dma_addr);

void App_FaultLatchFromIsr(uint32_t code, uint32_t detail);
void App_FaultLatch(uint32_t code, uint32_t detail);

void Task1ms(void);
void Task5ms(void);
void Task10ms(void);
void Task50ms(void);
void Task1000ms(void);

