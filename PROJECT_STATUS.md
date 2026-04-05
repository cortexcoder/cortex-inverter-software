# Project Status Memo

## Date
- 2026-03-26（初稿）
- 2026-03-29（续：整理 README、核对框架）
- 2026-04-02（续：工程路径与 VSCode / CMake 工具链备忘；与仓库现状对齐）
- 2026-04-05（续：解决 nano.specs + ARM DSP sin/cos 链接问题，建立 FastMath 替代方案）

## Current Goal
- Build an inverter software framework based on STM32H743.
- Architecture uses PWM-triggered ADC sampling with DMA double-buffering.
- **补充（现状）**：除 Keil/CubeMX 路线外，本仓库已具备 **ARM GCC + CMake + VSCode** 的可选编译与编辑环境；二者并行，按场景选用。

## Git / Remote Snapshot
- Local repo initialized: `F:\AI_workspace\inverter`
- Remote:
  - `git@gitee.com:single-cool/cortex-inverter-software.git`
- SSH authentication to Gitee is working.

## Environment Notes
- Git identity configured:
  - `user.name = cortex`
  - `user.email = bigwheel2019@163.com`
- SSH authentication to Gitee is working.
- Repository may require explicit SSH identity in some terminals:
  - `git config core.sshCommand "ssh -i C:/Users/shanliang/.ssh/id_ed25519 -o IdentitiesOnly=yes"`

## 2026-04-05 更新：解决 nano.specs + ARM DSP sin/cos 链接问题

### 问题描述
- ARM GCC nano.specs 使用 newlib-nano，要求 `__errno` 符号，但 nano.specs 不提供
- 调用 `sinf`/`cosf`（ARM DSP 库）链接失败：`undefined reference to '__errno'`
- CMSIS-DSP 预编译库（`libarm_cortex-m4lf.a` 等）在工具链中不存在

### 解决方案
建立 `Components/Math/common/fast_math.[h|c]`，使用 **Taylor 级数多项式逼近** 实现 sin/cos：

- **FastSin/FastCos**：6阶 sin 多项式 + 3阶 cos 多项式
- **FastSinCos**：一次计算同时返回 sin 和 cos
- 所有 Math 组件（transforms, filters, biquad, notch, math_utils）已切换到 FastMath
- `svpwm.c` 中 `fmodf`/`sqrtf` 保留标准 math.h（不影响 ARM DSP 依赖）

### 已修复问题
1. **重复 LPF 定义** - `Components/Filters/lpf.c` 和 `Components/Math/filters/lpf.c` 都有 `Lpf_*` 函数，导致链接多重定义错误。已将 Math 版本重命名为 `MathLpf_*`。

2. **CMakeLists.txt 工具链路径** - 原路径 `Z:/bin` 不存在，已更正为实际路径 `C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin`。

3. **CMakeLists.txt 包含目录** - 添加了 BSP、Math components 的 include 目录。

### 当前状态
- **编译状态**：✅ 成功编译无错误（仅有一些 pedantic warnings）
- **组件初始化**：`App_TasksCreate()` 中已添加：
  - `Transforms_Init`
  - `Math_ClarkeFactors_Init` / `Math_ParkFactors_Init`
  - `PiCtrl_Init`（s_pi_d, s_pi_q）
  - `Biquad_LPF` + `Biquad_Init`
  - `Notch_Config` + `Notch_Init`
  - `Inv_Init`

### 待完成任务
1. 硬件 IO 映射填充（BSP 层）- 用户自行根据硬件填充 GPIO/外设配置
2. 集成测试 - 在目标硬件上验证
3. 可能需要添加 Cortex-M4F 浮点 ABI 优化

---

## 历史备忘

### 2026-04-02 更新：工程路径与 VSCode / CMake（与现状对齐）

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

---

## Next Session Plan

### 短期（软件框架完善）
1. 填充 BSP 层 GPIO/外设映射（用户根据实际硬件填充）
2. 完成 `App_TasksCreate()` 中剩余依赖硬件的初始化
3. 在目标硬件上进行集成测试

### 长期（CubeMX 集成）
1. 创建 STM32H743 CubeMX 项目（MDK-ARM / Keil）。
2. 配置 PWM timer（TIM1/TIM8）和 ADC 外部触发。
3. 配置 ADC + DMA 双缓冲流程（DBM startup through HAL API）。
4. 生成项目，然后集成框架文件 from `inverter/Core`。
5. Wire HAL callbacks/IRQ entry points to `IsrDispatch_*`。
6. Build in Keil and validate interrupt -> queue -> task scheduling path。
