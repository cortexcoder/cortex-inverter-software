#pragma once

#include <stdint.h>
#include "gfl_types.h"

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

/* GFL 环路接口 */
void GFL_Task_1ms(void);
void GFL_SetPowerRef(float P_ref, float Q_ref);
Gfl_Mode GFL_GetMode(void);
Gfl_FaultType GFL_GetFault(void);
void GFL_ClearFault(void);
float GFL_GetDutyA(void);
float GFL_GetDutyB(void);
float GFL_GetDutyC(void);
bool GFL_IsPllLocked(void);
float GFL_GetFreq(void);
float GFL_GetGridVoltage(void);

