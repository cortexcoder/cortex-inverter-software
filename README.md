# STM32H7 Inverter Framework (ARM GCC + CMake)

仓库名：**cortex-inverter-software**（远程：`git@gitee.com:single-cool/cortex-inverter-software.git`）

这是一个面向 STM32H7 逆变器控制软件的**最小可落地框架骨架**。

## 编译状态

✅ **编译成功** - ARM GCC + CMake + VSCode 环境

```powershell
# 清理并重新配置
Remove-Item -Recurse -Force build
cmake -G "MinGW Makefiles" -B build

# 编译
cmake --build build
```

## 目录结构

```
Components/
├── Common/           # 通用工具 (FastMath)
├── Transforms/       # 坐标变换 (Clarke, Park, SVPWM)
├── Controllers/      # 控制器
│   ├── ctrl_loop.c/h # 电流环 (Clark/Park/PI/IRC/DCI/GVFF)
│   └── pi_ctrl.h    # 通用PI控制器
├── Filters/          # 滤波器 (LPF, Biquad, Notch) - 已废弃，使用 Math/filters
├── Math/             # 数学组件
│   ├── common/       # FastMath (Taylor级数 sin/cos)
│   ├── filters/      # Biquad, Notch, LPF 滤波器
│   └── transforms/   # Clarke, Park 因子
├── InverterSampler/  # 采样器 (6通道 + 校准)
├── InverterFoc/      # FOC参考实现
└── BSP/              # 底层外设抽象

Core/
├── Inc/              # 框架头文件
└── Src/              # 框架实现
```

---

## 电流环控制器 (ctrl_loop.c/h)

`ctrl_loop.c/h` 集成了完整的电流环实现，包括：

### 子模块
| 模块 | 功能 |
|------|------|
| **Clark变换** | 三相ABC → αβ 两相静止坐标系 |
| **Park变换** | αβ → dq 旋转坐标系 |
| **PI控制器** | d轴和q轴电流闭环控制 |
| **IRC (Inverse Race Counter)** | 反电动势补偿 |
| **DCI (Decoupling Current Inject)** | 电流解耦 |
| **GVFF (Generalized Voltage Feed Forward)** | 电压前馈 |

### 代码执行顺序

**发波前 (Pre-PWM):**
1. `CtrlLoop_CalcClark()` - Clark变换
2. `CtrlLoop_CalcPark()` - Park变换
3. `CtrlLoop_CalcCurrent()` - 计算d/q轴电流

**发波后 (Post-PWM):**
4. `CtrlLoop_CalcVoltage()` - 计算d/q轴电压
5. `CtrlLoop_CalcPI()` - PI控制输出
6. `CtrlLoop_CalcDecoupling()` - 电流解耦 (DCI)
7. `CtrlLoop_CalcFeedForward()` - 前馈 (GVFF)
8. `CtrlLoop_CalcIRC()` - 反电动势补偿 (IRC)
9. `CtrlLoop_CalcParkInv()` - 反Park变换 (αβ)
10. `CtrlLoop_CalcSvm()` - 空间矢量调制

### 使用示例

```c
#include "ctrl_loop.h"

static CtrlLoop_Handle ctrl;

void ControlISR(void) {
    // 发波前 - 采样后立即执行
    float i_alpha, i_beta;
    float i_d, i_q;
    float v_d, v_q;
    
    CtrlLoop_Init(&ctrl);
    
    // 获取采样电流
    float i_a = BSP_Adc_ReadChannel(BSP_ADC_CH_I_A);
    float i_b = BSP_Adc_ReadChannel(BSP_ADC_CH_I_B);
    float i_c = BSP_Adc_ReadChannel(BSP_ADC_CH_I_C);
    
    // Clark变换
    CtrlLoop_CalcClark(&ctrl, i_a, i_b, i_c, &i_alpha, &i_beta);
    
    // 获取角度
    float angle = CtrlLoop_GetAngle(&ctrl);
    
    // Park变换
    CtrlLoop_CalcPark(&ctrl, i_alpha, i_beta, angle, &i_d, &i_q);
    
    // ========== 发波 ==========
    BSP_HRPWM_Start();
    
    // 发波后 - 计算电压
    CtrlLoop_CalcVoltage(&ctrl, v_d_ref, v_q_ref, &v_d, &v_q);
    CtrlLoop_CalcPI(&ctrl, i_d, i_q, v_d, v_q);
    CtrlLoop_CalcDecoupling(&ctrl, i_q, &v_d);
    CtrlLoop_CalcFeedForward(&ctrl, v_d, v_q);
    CtrlLoop_CalcIRC(&ctrl, omega, &v_d, &v_q);
    
    // 反Park变换
    CtrlLoop_CalcParkInv(&ctrl, v_d, v_q, angle, &v_alpha, &v_beta);
    
    // SVM输出
    CtrlLoop_CalcSvm(&ctrl, v_alpha, v_beta);
}
```

---

## 滤波器模块 (Math/filters)

位于 `Components/Math/filters/`，提供以下滤波器：

| 文件 | 类型 | 用途 |
|------|------|------|
| `biquad.c/h` | 二阶滤波器 | 通用二阶IIR滤波器 |
| `notch.c/h` | 陷波滤波器 | 抑制特定频率 (如100Hz干扰) |
| `lpf.c/h` | 低通滤波器 | 基础低通滤波 |

### 类型定义

```c
// 二阶滤波器
typedef struct {
    float b0, b1, b2;  // 分子系数
    float a1, a2;      // 分母系数 (a0=1)
    float x1, x2;      // 输入历史
    float y1, y2;      // 输出历史
} BiquadFilter_Handle;

typedef struct {
    float fc;          // 截止频率 (Hz)
    float zeta;        // 阻尼系数
    float dt;          // 采样周期 (s)
} BiquadFilter_Config;

// 低通滤波器
typedef struct {
    float fc;          // 截止频率 (Hz)
    float dt;          // 采样周期 (s)
    float y1;          // 上一次输出
} MathLpf_Handle;

typedef struct {
    float fc;
    float dt;
} MathLpf_Config;
```

### 使用示例

```c
#include "biquad.h"
#include "lpf.h"

// 配置陷波滤波器 (50Hz notch)
BiquadFilter_Config notch_cfg = {
    .fc = 50.0f,
    .zeta = 0.5f,
    .dt = 1.0f / 24000.0f
};

// 配置低通滤波器
MathLpf_Config lpf_cfg = {
    .fc = 500.0f,
    .dt = 1.0f / 24000.0f
};

// 初始化
BiquadFilter_Handle notch;
BiquadFilter_Init(&notch, &notch_cfg);

MathLpf_Handle lpf;
MathLpf_Init(&lpf, &lpf_cfg);

// 使用
float filtered = BiquadFilter_Apply(&notch, raw_signal);
float smoothed = MathLpf_Apply(&lpf, raw_signal);
```

Core/
├── Inc/              # 框架头文件
└── Src/              # 框架实现
```

---

## FastMath 实现（解决 nano.specs 链接问题）

由于 ARM GCC nano.specs 使用 newlib-nano，要求 `__errno` 符号但未提供，导致使用 `sinf`/`cosf` 时链接失败。

本项目使用 **Taylor 级数多项式逼近** 实现高效的 sin/cos：

- `FastSin(x)` - 6阶多项式逼近
- `FastCos(x)` - 3阶多项式逼近  
- `FastSinCos(x, *sin, *cos)` - 一次调用同时返回 sin 和 cos

位于 `Components/Math/common/fast_math.[h|c]`

---

## 关键配置参数

### 开关与采样频率 (bsp_hrpwm.h)
```c
#define BSP_HRPWM_SWITCH_FREQ_HZ       24000U    // 开关频率 24kHz
#define BSP_HRPWM_SAMPLES_PER_PERIOD   8U        // 每周期8点采样
```

### 采样参数 (inverter_sampler.h)
```c
#define SAMPLER_SHUNT_RESISTOR   (0.001f)    // 0.001Ω采样电阻
#define SAMPLER_OPAMP_GAIN       (10.0f)    // 运放增益
#define SAMPLER_VDIV_R1          (100.0f)    // 分压电阻R1
#define SAMPLER_VDIV_R2          (100.0f)   // 分压电阻R2
```

---

## ADC采样配置方法

ADC采样配置在 `Components/BSP/bsp_io_map.c` 文件中。

### 1. ADC通道配置

每个ADC通道需要配置：
- `adc_instance`: ADC实例 (1, 2, 3)
- `adc_channel`: ADC通道号
- `gpio`: GPIO端口和引脚
- `sample_time`: 采样时间 (0-15)

```c
const BSP_AdcConfig bsp_adc_default_config = {
    .ch = {
        // 三相电流采样 (需要差分放大)
        [BSP_ADC_CH_I_A] = {
            .adc_instance = 1,
            .adc_channel = 1,        // CH1
            .gpio = { BSP_GPIO_PORT_A, 0, 15 },  // PA0
            .sample_time = 6,         // 47.5 cycles
        },
        [BSP_ADC_CH_I_B] = {
            .adc_instance = 1,
            .adc_channel = 2,        // CH2
            .gpio = { BSP_GPIO_PORT_A, 1, 15 },  // PA1
            .sample_time = 6,
        },
        [BSP_ADC_CH_I_C] = {
            .adc_instance = 1,
            .adc_channel = 3,        // CH3
            .gpio = { BSP_GPIO_PORT_A, 2, 15 },  // PA2
            .sample_time = 6,
        },

        // 母线电压 (电阻分压)
        [BSP_ADC_CH_V_BUS] = {
            .adc_instance = 1,
            .adc_channel = 4,        // CH4
            .gpio = { BSP_GPIO_PORT_PA4, 4, 15 },  // PA4
            .sample_time = 6,
        },

        // 输出电压 (电阻分压)
        [BSP_ADC_CH_V_OUT] = {
            .adc_instance = 1,
            .adc_channel = 5,        // CH5
            .gpio = { BSP_GPIO_PORT_PA5, 5, 15 },  // PA5
            .sample_time = 6,
        },

        // 温度采样 (内部温度传感器或外部NTC)
        [BSP_ADC_CH_TEMP] = {
            .adc_instance = 1,
            .adc_channel = 16,       // CH16 (内部温度)
            .gpio = { 0, 0, 0 },     // 无外部GPIO
            .sample_time = 6,
        },
    },
};
```

### 2. DMA配置

DMA用于ADC双缓冲传输：

```c
const BSP_DmaConfig bsp_dma_default_config = {
    .dma = {
        .dma_instance = 2,
        .dma_stream = 0,
        .dma_channel = 0,
    },
    .buf0 = (void*)0,      // 实际会在运行时分配
    .buf1 = (void*)0,
    .buf_size = BSP_HRPWM_SAMPLES_PER_PERIOD * 6,  // 8点 × 6通道 = 48
};
```

### 3. 使用示例

```c
#include "bsp_adc.h"

static BSP_Adc s_adc;
static BSP_AdcDma s_adc_dma;

void App_Init(void) {
    // 初始化ADC
    BSP_Adc_Init(&s_adc, &bsp_adc_default_config);
    BSP_AdcDma_Init(&s_adc_dma, &bsp_dma_default_config, &s_adc);
}

void ControlISR(void) {
    // 检查采样缓冲是否就绪
    if (BSP_Adc_IsBufReady(&s_adc)) {
        uint8_t buf = BSP_Adc_GetActiveBuf(&s_adc);
        // 处理采样数据...
        BSP_Adc_AckBuf(&s_adc);
    }
}
```

---

## 框架入口

- `Core/Src/main.c`：最小启动入口
- `Core/Src/app.c`：应用入口，创建队列/事件/任务
- `Core/Src/app_tasks.c`：任务实现
- `Core/Src/isr_dispatch.c`：ISR 分片分发

## 任务周期

| 任务 | 周期 | 用途 |
|------|------|------|
| Task1ms | 1ms | 快速控制环 |
| Task5ms | 5ms | **状态机+故障检测** |
| Task10ms | 10ms | 慢速控制 |
| Task50ms | 50ms | 通讯 |
| Task1000ms | 1000ms | 系统监控 |

## 状态机

状态定义在 `Components/InverterSampler/inverter_state.h`

```
INIT → IDLE → START → RUN → SHUTDOWN
                     ↓
              (故障/停止命令)
                     ↓
                   SHUTDOWN → RUN (恢复)
                              → START (重启动)
```

---

## VSCode / CMake 工具链

- **CMake**：`CMakeLists.txt`（STM32H743 / ARM GCC 等）。
- **VSCode 任务**：`.vscode/tasks.json`（CMake Configure / Build / Clean / Clean Build）。
- **IntelliSense**：`.vscode/c_cpp_properties.json`（配置名 `STM32H743`，含 HAL/CMSIS 等 include 与宏）。
- **文档**：`SUMMARY.md`、`SETUP.md`、`BUILD_GUIDE.md`（VSCode 编译与环境说明）。
- **脚本**：`build.bat`、`check_env.bat`（一键编译与环境检查）。

### 调试
可用 ST-Link GDB + **Cortex-Debug** 等扩展。
