#pragma once

#include <stdint.h>

// 统一的“ISR -> 任务”事件类型
typedef enum {
  APP_EVT_NONE = 0,

  // 控制相关
  APP_EVT_CTRL_TICK,        // 控制周期 tick（通常由定时器或 ADC DMA 完成触发）

  // ADC + DMA 事件
  // 双缓冲（DBM）推荐使用 BUF0/BUF1 READY：每次“完整一帧采样窗口”完成时触发一次事件
  APP_EVT_ADC_BUF0_READY,   // DMA M0 complete
  APP_EVT_ADC_BUF1_READY,   // DMA M1 complete

  // 兼容：若你仍使用 circular + half/full（非 DBM），可用这两个事件
  APP_EVT_ADC_HALF,         // ADC DMA half complete
  APP_EVT_ADC_FULL,         // ADC DMA full complete

  // 通讯/其它
  APP_EVT_UART_RX,          // 串口收到数据（仅示例）
  APP_EVT_FAULT_LATCHED,    // 故障锁存（过流/过压等）
} app_evt_type_t;

typedef struct {
  app_evt_type_t type;
  uint32_t ts;        // 时间戳（单位由实现决定）
  uint32_t arg0;
  uint32_t arg1;
} app_event_t;

