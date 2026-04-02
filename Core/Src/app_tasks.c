#include "app_tasks.h"

#include "app_events.h"

#include "cmsis_os2.h"

// ----------------------------
// RTOS objects
// ----------------------------

static osMessageQueueId_t s_evt_q = NULL;
static osEventFlagsId_t s_fault_flags = NULL;

// 事件队列深度：控制类事件（ADC half/full + tick）通常很频繁，不建议太小
// 注意：队列里放的是 app_event_t（固定大小），避免 ISR 动态分配
static const uint32_t kEvtQueueDepth = 64;

// ----------------------------
// Tasks
// ----------------------------

static void ControlTask(void *argument);
static void CommsTask(void *argument);
static void HousekeepingTask(void *argument);

void App_TasksCreate(void) {
  if (s_evt_q == NULL) {
    s_evt_q = osMessageQueueNew(kEvtQueueDepth, sizeof(app_event_t), NULL);
  }
  if (s_fault_flags == NULL) {
    s_fault_flags = osEventFlagsNew(NULL);
  }

  (void)osThreadNew(ControlTask, NULL, &(osThreadAttr_t){
                                         .name = "ctrl",
                                         .priority = osPriorityHigh,
                                         .stack_size = 2048,
                                     });

  (void)osThreadNew(CommsTask, NULL, &(osThreadAttr_t){
                                       .name = "comms",
                                       .priority = osPriorityNormal,
                                       .stack_size = 1536,
                                   });

  (void)osThreadNew(HousekeepingTask, NULL, &(osThreadAttr_t){
                                              .name = "hk",
                                              .priority = osPriorityLow,
                                              .stack_size = 1024,
                                          });
}

// ----------------------------
// Event posting helpers
// ----------------------------

static inline void PostEvtFromIsr(app_event_t *e) {
  if (s_evt_q == NULL) {
    return;
  }

  // CMSIS-RTOS2: 从 ISR 调用 osMessageQueuePut 是允许的（实现会走 ISR-safe 路径）
  // timeout 必须为 0
  (void)osMessageQueuePut(s_evt_q, e, 0U, 0U);
}

void App_PostControlTickFromIsr(uint32_t ts) {
  app_event_t e = {
      .type = APP_EVT_CTRL_TICK,
      .ts = ts,
      .arg0 = 0,
      .arg1 = 0,
  };
  PostEvtFromIsr(&e);
}

void App_PostAdcBufReadyFromIsr(uint32_t ts, uint32_t buf_addr, uint32_t buf_index) {
  app_event_t e = {
      .type = (buf_index == 0U) ? APP_EVT_ADC_BUF0_READY : APP_EVT_ADC_BUF1_READY,
      .ts = ts,
      .arg0 = buf_addr,
      .arg1 = buf_index,
  };
  PostEvtFromIsr(&e);
}

void App_PostAdcHalfFromIsr(uint32_t ts, uint32_t dma_addr) {
  app_event_t e = {
      .type = APP_EVT_ADC_HALF,
      .ts = ts,
      .arg0 = dma_addr, // 传递 DMA buffer 地址（或 index）
      .arg1 = 0,
  };
  PostEvtFromIsr(&e);
}

void App_PostAdcFullFromIsr(uint32_t ts, uint32_t dma_addr) {
  app_event_t e = {
      .type = APP_EVT_ADC_FULL,
      .ts = ts,
      .arg0 = dma_addr,
      .arg1 = 0,
  };
  PostEvtFromIsr(&e);
}

void App_FaultLatchFromIsr(uint32_t code, uint32_t detail) {
  (void)detail;
  if (s_fault_flags != NULL) {
    // 这里把 code 映射到 bit 位（示例：故障码低 5 位作为 bit 0..31）
    const uint32_t idx = code & 0x1FU;
    const uint32_t bit = 1U << idx;
    (void)osEventFlagsSet(s_fault_flags, bit);
  }

  app_event_t e = {
      .type = APP_EVT_FAULT_LATCHED,
      .ts = 0,
      .arg0 = code,
      .arg1 = detail,
  };
  PostEvtFromIsr(&e);
}

void App_FaultLatch(uint32_t code, uint32_t detail) {
  App_FaultLatchFromIsr(code, detail);
}

// ----------------------------
// Task implementations
// ----------------------------

static void ControlTask(void *argument) {
  (void)argument;

  // 控制任务职责建议：
  // - 从队列拿到 ADC half/full 事件 -> 读取采样 buffer -> 运行电流环/电压环/PLL/SVPWM 等
  // - 控制 tick 事件也可作为“软触发”，但强建议以 ADC DMA 事件作为同步源（抖动更小）

  app_event_t e;
  for (;;) {
    if (osMessageQueueGet(s_evt_q, &e, NULL, osWaitForever) != osOK) {
      continue;
    }

    switch (e.type) {
      case APP_EVT_ADC_BUF0_READY:
      case APP_EVT_ADC_BUF1_READY:
      case APP_EVT_ADC_HALF:
      case APP_EVT_ADC_FULL:
        // TODO: 在此处对 e.arg0 指向/表示的采样窗口做处理
        //       例如：Clarke/Park -> PI -> SVPWM -> 更新定时器比较值
        break;

      case APP_EVT_CTRL_TICK:
        // TODO: 若你用定时器触发控制环，可在此运行一次控制迭代
        break;

      case APP_EVT_FAULT_LATCHED:
        // TODO: 执行保护动作（例如关断 PWM、进入安全态、上报故障）
        break;

      default:
        break;
    }
  }
}

static void CommsTask(void *argument) {
  (void)argument;
  // 通讯任务职责建议：协议解析/参数下发/遥测上报（尽量不要影响控制任务时序）
  for (;;) {
    (void)osDelay(10);
  }
}

static void HousekeepingTask(void *argument) {
  (void)argument;
  // 后台任务职责建议：温度/母线电压监控、状态机、LED、日志缓冲刷新等
  for (;;) {
    (void)osDelay(100);
  }
}

