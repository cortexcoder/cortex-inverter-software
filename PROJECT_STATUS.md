# Project Status Memo

## Date
- 2026-03-26（初稿）
- 2026-03-29（续：整理 README、核对框架）
- 2026-04-02（续：工程路径与 VSCode / CMake 工具链备忘；与仓库现状对齐）
- 2026-04-05（续：解决 nano.specs + ARM DSP sin/cos 链接问题，建立 FastMath 替代方案）
- 2026-04-10（续：整合电流环控制器 ctrl_loop.c/h，整合滤波器模块，删除重复文件）

## Current Goal
- Build an inverter software framework based on STM32H743.
- Architecture uses PWM-triggered ADC sampling with DMA double-buffering.
- **补充（现状）**：除 Keil/CubeMX 路线外，本仓库已具备 **ARM GCC + CMake + VSCode** 的可选编译与编辑环境；二者并行，按场景选用。

## Git / Remote Snapshot
- Local repo: `F:\AI_workspace\user-data\inverter`
- Remote:
  - GitHub: `git@github.com:cortexcoder/cortex-inverter-software.git`
  - Gitee: `git@gitee.com:single-cool/cortex-inverter-software.git`

## 2026-04-10 更新：整合电流环控制器与滤波器模块

### 新增 ctrl_loop.c/h

整合了电流环相关功能到单一模块：
- **Clark/Park/InvPark变换** - 正序和负序
- **PI控制器** - 正序和负序d/q轴
- **IRC谐振控制器** - 可选谐波抑制
- **DCI直流分量控制** - 直流分量抑制
- **GVFF电网电压前馈** - 可选前馈控制
- **占空比输出** - SVM生成

执行时序设计（主中断中执行）：
1. `CtrlLoop_PreSample()` - 发波前读取电流，执行坐标变换
2. `CtrlLoop_Calc()` - 计算PI和IRC输出
3. `CtrlLoop_PostCalc()` - 发波后限幅和故障检测

所有内部函数实现为 `static inline` 以优化执行效率。

### 滤波器模块整合

删除了重复的滤波器文件：
- 删除 `Components/Filters/lpf.c/h`（与Math版本重复）
- 删除 `Components/Math/filters/fnf.c/h`（空文件）
- 删除 `Components/Math/filters/lpf1.c/h`（空文件）
- 删除 `Components/Math/filters/lpf2.c/h`（空文件）

保留：
- `Components/Math/filters/lpf.c/h` - LPF滤波器（MathLpf_* 前缀）
- `Components/Math/filters/biquad.c/h` - 双二阶滤波器（Biquad_* 前缀）
- `Components/Math/filters/notch.c/h` - 陷波滤波器（Notch_* 前缀）

### 编译状态
- **编译状态**：✅ 成功编译无错误（仅有 pedantic warnings）

---

## 历史备忘

### 2026-04-05 更新：解决 nano.specs + ARM DSP sin/cos 链接问题

### 问题描述
- ARM GCC nano.specs 使用 newlib-nano，要求 `__errno` 符号，但 nano.specs 不提供
- 调用 `sinf`/`cosf`（ARM DSP 库）链接失败：`undefined reference to '__errno'`
- CMSIS-DSP 预编译库（`libarm_cortex-m4lf.a` 等）在工具链中不存在

### 解决方案
建立 `Components/Math/common/fast_math.[h|c]`，使用 **Taylor 级数多项式逼近** 实现 sin/cos：
- **FastSin/FastCos**：6阶 sin 多项式 + 3阶 cos 多项式
- **FastSinCos**：一次计算同时返回 sin 和 cos

---

## 长期目标（CubeMX 集成）

1. 创建 STM32H743 CubeMX 项目（MDK-ARM / Keil）。
2. 配置 PWM timer（TIM1/TIM8）和 ADC 外部触发。
3. 配置 ADC + DMA 双缓冲流程（DBM startup through HAL API）。
4. 生成项目，然后集成框架文件 from `inverter/Core`。
5. Wire HAL callbacks/IRQ entry points to `IsrDispatch_*`.
6. Build in Keil and validate interrupt -> queue -> task scheduling path。
