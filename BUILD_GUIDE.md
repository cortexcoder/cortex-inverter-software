# VSCode 编译配置指南

## 前置要求

1. **安装 ARM 工具链**：
   ```bash
   # Windows: 使用 MSYS2 或从 ARM 官网下载
   # 下载地址: https://developer.arm.com/downloads/-/gnu-rm
   # 安装后确保 arm-none-eabi-gcc 在 PATH 中
   ```

2. **安装 CMake**：
   ```bash
   # Windows: 下载安装或使用 Chocolatey
   choco install cmake
   ```

3. **安装 VSCode 扩展**：
   - C/C++ (Microsoft)
   - CMake Tools (可选，但推荐)

## 目录结构

```
F:/AI_workspace/inverter/
├── .vscode/
│   ├── tasks.json              # 编译任务配置
│   └── c_cpp_properties.json   # IntelliSense 配置
├── Core/
│   ├── Inc/                    # 头文件
│   │   ├── main.h
│   │   ├── stm32h7xx_hal_conf.h
│   │   └── ...
│   └── Src/                    # 源文件
│       ├── main.c              # 主入口（480MHz 系统时钟）
│       ├── system_stm32h7xx.c  # 系统初始化
│       ├── startup_stm32h743xx.s  # 启动文件
│       ├── STM32H743BITx_FLASH.ld  # 链接脚本
│       └── ...
├── Drivers/                    # HAL 驱动和 CMSIS
│   ├── STM32H7xx_HAL_Driver/
│   └── CMSIS/
├── CMakeLists.txt              # CMake 构建配置
└── README.md
```

## 编译步骤

### 方法 1：使用 VSCode 任务（推荐）

1. **打开项目**：
   ```bash
   code F:/AI_workspace/inverter
   ```

2. **配置 IntelliSense**：
   - 按 `Ctrl+Shift+P`
   - 输入 `C/C++: Select a Configuration...`
   - 选择 `STM32H743`

3. **编译项目**：
   - 按 `Ctrl+Shift+B` 或 `F5` 选择 `CMake: Build`
   - 或者按 `Ctrl+Shift+P`，输入 `Tasks: Run Task`，选择 `CMake: Build`

### 方法 2：命令行编译

```bash
cd F:/AI_workspace/inverter

# 配置项目
cmake -S . -B build -G "Unix Makefiles"

# 编译
cmake --build build --config Release

# 清理
cmake --build build --target clean
```

## 编译输出

编译成功后会在 `build/` 目录生成：

- `inverter.elf` - ELF 可执行文件
- `inverter.hex` - Intel HEX 格式（用于烧录）
- `inverter.bin` - 二进制文件
- `inverter.map` - 内存映射文件

## 系统时钟配置

当前配置：
- **HSE**: 8 MHz
- **SYSCLK**: 480 MHz
- **HCLK**: 240 MHz
- **APB1/2/3/4**: 120 MHz

时钟配置在 `Core/Src/main.c::SystemClock_Config()` 函数中。

## 常见问题

### 1. 找不到 arm-none-eabi-gcc

**解决方法**：
```bash
# 检查是否在 PATH 中
arm-none-eabi-gcc --version

# 如果不存在，需要添加到 PATH
# Windows: 将工具链安装路径添加到系统环境变量 PATH
# 例如: C:\Program Files\arm-none-eabi\bin
```

### 2. CMake 找不到编译器

**解决方法**：
在 CMakeLists.txt 中指定工具链前缀：
```cmake
set(TOOLCHAIN arm-none-eabi-)
set(CMAKE_C_COMPILER ${TOOLCHAIN}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN}gcc)
```

### 3. IntelliSense 无法找到头文件

**解决方法**：
- 确保在 `c_cpp_properties.json` 中正确配置了 `includePath`
- 按 `Ctrl+Shift+P`，输入 `C/C++: Reset IntelliSense Database`
- 重启 VSCode

## 下一步

1. **配置外设**：
   - 在 `Core/Src/main.c` 中添加 `MX_GPIO_Init()`, `MX_TIM1_Init()`, `MX_ADC1_Init()` 等
   - 参考项目进度文档 `PROJECT_STATUS.md`

2. **集成框架代码**：
   - 已有的框架代码位于 `Core/Src/app.c`, `Core/Src/app_tasks.c` 等
   - 在 `main.c` 的 `App_Init()` 之前添加外设初始化

3. **烧录程序**：
   - 使用 ST-Link 工具烧录 `inverter.hex` 或 `inverter.bin`
   - 命令：`st-flash write inverter.bin 0x08000000`

## 参考资料

- [STM32H7 参考手册](https://www.st.com/resource/en/reference_manual/rm0433-stm32h742-stm32h743-stm32h753-and-stm32h750-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32H7 数据手册](https://www.st.com/resource/en/datasheet/stm32h743bi.pdf)
- [HAL 库文档](https://www.st.com/resource/en/user_manual/um2400-stm32cubeh7-mcubfmw-package-description-stmicroelectronics.pdf)
