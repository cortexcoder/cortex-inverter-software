#pragma once

#include <stdint.h>

// 应用初始化：创建 RTOS 对象（队列/事件/任务）
void App_Init(void);

// 应用启动：启动内核（若需要）/开始调度（视工程结构而定）
void App_Start(void);

// 可选：给外部（例如 main loop）调用的心跳
void App_IdleHook(void);

