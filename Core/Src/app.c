#include "app.h"

#include "app_tasks.h"
#include "isr_dispatch.h"

#include "cmsis_os2.h"

static uint8_t s_app_started = 0;

void App_Init(void) {
  // 约定：RTOS 内核初始化可以在这里做，也可以由 CubeMX 生成的 main() 做
  // 为了兼容两种方式，我们只在“尚未初始化”时调用。CMSIS-RTOS2 没有标准查询接口，
  // 因此这里采取最常见工程结构：由 main() 调用 osKernelInitialize()，这里不重复调用。

  IsrDispatch_Init();
  App_TasksCreate();
}

void App_Start(void) {
  if (s_app_started) {
    return;
  }
  s_app_started = 1;

  // 同理：很多 CubeMX 工程会在 main() 中调用 osKernelStart()。
  // 如果你使用本仓库提供的最小 main.c，则这里会启动内核。
  (void)osKernelStart();
}

void App_IdleHook(void) {
  // 可选：放置低优先级后台任务或统计逻辑（不要阻塞）
}

