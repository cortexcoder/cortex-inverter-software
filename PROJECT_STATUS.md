# Project Status Memo

## Date
- 2026-03-26（初稿）
- 2026-03-29（续：整理 README、核对框架）
- 2026-04-02（续：工程路径与 VSCode / CMake 工具链备忘；与仓库现状对齐）

## Current Goal
- Build an inverter software framework based on STM32H743.
- Architecture uses PWM-triggered ADC sampling with DMA double-buffering.
- **补充（现状）**：除 Keil/CubeMX 路线外，本仓库已具备 **ARM GCC + CMake + VSCode** 的可选编译与编辑环境；二者并行，按场景选用。

## What Was Completed Today
- Created initial framework at `F:\cursor_workspace\inverter`.
- Added RTOS task skeleton (CMSIS-RTOS2 style):
  - control task
  - comms task
  - housekeeping task
- Added ISR splitting pipeline:
  - ISR fast path only
  - event dispatch to queue/flags
  - heavy control logic deferred to tasks
- Added ADC DMA events including DBM-style buffer-ready events:
  - `APP_EVT_ADC_BUF0_READY`
  - `APP_EVT_ADC_BUF1_READY`
- Added STM32H743-oriented HAL template files for PWM + ADC + DMA DBM integration.

## Git / Remote Snapshot
- Local repo initialized: `F:\cursor_workspace\inverter`
- Baseline commit:
  - `802afe8`
  - message: `feat: STM32H743 inverter framework skeleton`
- Baseline tag:
  - `v0.1-framework-base`
- Remote:
  - `git@gitee.com:single-cool/cortex-inverter-software.git`
- Remote `master` has been updated to current baseline.

## Environment Notes
- Git identity configured:
  - `user.name = cortex`
  - `user.email = bigwheel2019@163.com`
- SSH authentication to Gitee is working.
- Repository may require explicit SSH identity in some terminals:
  - `git config core.sshCommand "ssh -i C:/Users/shanliang/.ssh/id_ed25519 -o IdentitiesOnly=yes"`

## Next Session Plan (CubeMX)
1. Create STM32H743 CubeMX project (MDK-ARM / Keil).
2. Configure PWM timer (TIM1/TIM8) and ADC external trigger from PWM timing event.
3. Configure ADC + DMA for double-buffer flow (DBM startup through HAL API).
4. Generate project, then integrate framework files from `inverter/Core`.
5. Wire HAL callbacks/IRQ entry points to `IsrDispatch_*`.
6. Build in Keil and validate interrupt -> queue -> task scheduling path.

## 2026-03-29 续作备忘
- 已阅读本文件；`inverter/README.md` 曾含 Git 合并冲突，已合并为单一说明（保留框架文档 + 仓库/远程一行）。
- `Core/Src/app_tasks.c` 中故障 EventFlags 位映射由 `0x17` 更正为 `0x1F`（与注释一致，避免非连续掩码）。
- 仍待你在本机用 **STM32CubeMX** 生成 H743 工程并完成上表 Next Session 步骤；本仓库继续作为可与 CubeMX 输出合并的 `Core` 框架来源。

---

## 2026-04-02 更新：工程路径与 VSCode / CMake（与现状对齐）

### 路径说明（历史与当前）
- 上文中 **「What Was Completed Today」「Git / Remote Snapshot」** 所记本地路径 `F:\cursor_workspace\inverter` 为 **当时初稿时的记录**，予以保留。
- **当前** 工程目录为 **`F:\AI_workspace\inverter`**（本仓库根目录）。
- 工作区根目录 `F:\AI_workspace\PROJECT_STATUS.md` 与本文件 **内容同步**，便于在上一级目录打开工作区时也能看到同一份备忘。

### VSCode / CMake 工具链（已纳入仓库）
- **CMake**：`CMakeLists.txt`（STM32H743 / ARM GCC 等）。
- **VSCode 任务**：`.vscode/tasks.json`（CMake Configure / Build / Clean / Clean Build；Configure 使用 `Unix Makefiles` generator）。
- **IntelliSense**：`.vscode/c_cpp_properties.json`（配置名 `STM32H743`，含 HAL/CMSIS 等 include 与宏）。
- **文档**：`SUMMARY.md`、`SETUP.md`、`BUILD_GUIDE.md`（VSCode 编译与环境说明）。
- **脚本**：`build.bat`、`check_env.bat`（一键编译与环境检查）。
- **HAL 与 Cube 包**：仍须按 `SETUP.md` 将 **STM32CubeH7** 的 `Drivers/STM32H7xx_HAL_Driver`、`Drivers/CMSIS` 拷入 `Drivers/` 后再完整编译（与历史备忘一致）。

### 尚未在仓库中默认提供的项（可选）
- **调试**：可用 ST-Link GDB + **Cortex-Debug** 等扩展；当前仓库 **未** 提交 `launch.json`（可按需自行添加）。

### 与「Next Session Plan」的关系
- **Keil（CubeMX 生成）** 路线仍为官方备忘中的主流程之一。
- **VSCode + CMake + ARM GCC** 为并行可选路线，用于在本机无 Keil 时编译与浏览；详见 `SETUP.md` / `BUILD_GUIDE.md`。
