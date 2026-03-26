#include <stdint.h>

#include "app.h"
#include "cmsis_os2.h"

// 这是“最小可运行入口”示例。
// 实际用 STM32CubeMX 生成工程时，你应该把 App_Init() / App_Start() 融入 CubeMX 的 main.c：
// - 在 HAL 初始化 + 外设初始化后调用 App_Init()
// - 在创建完默认任务/或直接在末尾调用 App_Start()

int main(void) {
  // 在真实 STM32H7 工程里，这里会有：
  // - HAL_Init()
  // - SystemClock_Config()
  // - MX_GPIO_Init()/MX_DMA_Init()/MX_ADC*/MX_TIM*/...
  //
  // 本文件不包含 HAL 依赖，以便作为“框架入口模板”。

  (void)osKernelInitialize();
  App_Init();
  App_Start();

  // 正常情况下不会到达这里
  for (;;) {
    App_IdleHook();
  }
}

