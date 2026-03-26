#include "isr_dispatch.h"

#include "app_tasks.h"

#include "cmsis_os2.h"

// ISR 分片的原则：
// - ISR 快路径只做：读取必要状态/寄存器、时间戳、保存指针、投递轻量事件
// - 绝不在 ISR 中做：复杂数学（控制环/滤波）、printf、动态内存、长循环

void IsrDispatch_Init(void) {
  // 目前分发层无状态。若后续要做环形缓冲或双缓冲指针管理，可在此初始化。
}

void IsrDispatch_OnControlTick(uint32_t ts) {
  App_PostControlTickFromIsr(ts);
}

void IsrDispatch_OnAdcDmaBuf0(uint32_t ts, uint32_t buf_addr) {
  App_PostAdcBufReadyFromIsr(ts, buf_addr, 0U);
}

void IsrDispatch_OnAdcDmaBuf1(uint32_t ts, uint32_t buf_addr) {
  App_PostAdcBufReadyFromIsr(ts, buf_addr, 1U);
}

void IsrDispatch_OnAdcDmaHalf(uint32_t ts, uint32_t dma_addr) {
  App_PostAdcHalfFromIsr(ts, dma_addr);
}

void IsrDispatch_OnAdcDmaFull(uint32_t ts, uint32_t dma_addr) {
  App_PostAdcFullFromIsr(ts, dma_addr);
}

void IsrDispatch_OnFault(uint32_t ts, uint32_t code, uint32_t detail) {
  (void)ts;
  App_FaultLatchFromIsr(code, detail);
}

