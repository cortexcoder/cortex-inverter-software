# STM32H7 Inverter Framework (Keil / CMSIS-RTOS2)

这是一个面向 STM32H7 逆变器控制软件的**最小可落地框架骨架**，重点解决：

- **中断**：预留典型 ISR 入口（Timer / DMA / ADC 等）
- **中断分片（ISR Splitting）**：ISR 里只做“快路径”（采样/时间戳/极少量处理），然后通过 **MessageQueue / EventFlags** 把重活下放到任务
- **任务**：基于 **CMSIS-RTOS2**（Keil RTX5 或 FreeRTOS 的 CMSIS-RTOS2 适配层均可）组织控制环、通讯、保护等任务

## 目录结构

- `Core/Inc`：框架头文件
- `Core/Src`：框架实现

## 你需要做什么（建议流程：CubeMX -> Keil）

1. 用 STM32CubeMX 新建 STM32H7 工程，Toolchain 选择 **MDK-ARM(Keil)**。
2. 在 CubeMX 里启用 RTOS：
   - 推荐：**CMSIS-RTOS2 (RTX5)**（Keil 自带，最省事）
   - 或者：FreeRTOS + CMSIS-RTOS2 wrapper（也能用，但工程配置更复杂）
3. 生成工程后，把本仓库的 `Core/Inc`、`Core/Src` 下文件加入 Keil 工程（可直接覆盖同名文件，或仅添加新文件）。
4. Keil 工程选项里确保包含路径至少有：
   - `.\Core\Inc`
   - CubeMX 生成的 HAL/CMSIS include 路径（CubeMX 会配置）

## 编译注意事项（非常重要）

- 本框架的任务 API 使用 `cmsis_os2.h`（CMSIS-RTOS2）。因此你必须确保工程里存在并启用 RTOS：
  - RTX5：Keil Pack 中提供，CubeMX 生成后通常直接可编译。
  - FreeRTOS：需要 CMSIS-RTOS2 适配层（CubeMX 可选）。
- 本框架**不强依赖 HAL**：与外设强绑定的回调/IRQ 入口都通过宏隔离：
  - 你在 CubeMX 开启 `USE_HAL_DRIVER` 时，可在 `stm32h7xx_it.c` 或 HAL 回调里调用框架的 ISR hook。
- 逆变器的关键外设（PWM/ADC/DMA/比较器/过流保护等）请由 CubeMX 配置生成；本框架只提供软件结构。

## 框架入口

- `Core/Src/main.c`：最小启动入口（示例，实际以 CubeMX 生成的 `main.c` 为准）
- `Core/Src/app.c`：应用入口，创建队列/事件/任务
- `Core/Src/app_tasks.c`：任务实现（控制环/通讯/监控）
- `Core/Src/isr_dispatch.c`：ISR 分片分发（ISR -> queue/flags）

