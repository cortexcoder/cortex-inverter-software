# VSCode 编译环境设置指南

## 快速开始

### 1. 下载 STM32CubeH7 驱动库

从 ST 官网下载最新版本的 STM32CubeH7：
```
https://www.st.com/en/embedded-software/stm32cubeh7.html
```

下载并解压，找到以下目录：
- `Drivers/STM32H7xx_HAL_Driver/`
- `Drivers/CMSIS/`

将这两个文件夹复制到 `F:/AI_workspace/inverter/Drivers/` 目录下。

### 2. 安装 ARM 工具链

#### Windows 方法 1：使用 MSYS2
```bash
# 在 MSYS2 中安装
pacman -S mingw-w64-x86_64-gcc-arm-none-eabi

# 添加到系统 PATH
# C:\msys64\mingw64\bin
```

#### Windows 方法 2：从 ARM 官网下载
```
https://developer.arm.com/downloads/-/gnu-rm
```

下载 `gcc-arm-none-eabi-*-win32.zip`，解压到任意位置，添加 `bin` 目录到系统 PATH。

验证安装：
```bash
arm-none-eabi-gcc --version
```

### 3. 安装 CMake

#### Windows 方法 1：使用 Chocolatey
```bash
choco install cmake
```

#### Windows 方法 2：下载安装包
```
https://cmake.org/download/
```

下载并安装，确保添加到 PATH。

验证安装：
```bash
cmake --version
```

### 4. 安装 VSCode 扩展

在 VSCode 中安装以下扩展：
1. **C/C++** (Microsoft)
2. **CMake Tools** (Microsoft) - 可选

### 5. 配置 IntelliSense

1. 打开 VSCode
2. 按 `Ctrl+Shift+P`
3. 输入 `C/C++: Select a Configuration...`
4. 选择 `STM32H743`

### 6. 编译项目

#### 方法 1：使用 VSCode 任务
- 按 `Ctrl+Shift+B`
- 选择 `CMake: Build`

#### 方法 2：使用命令行
```bash
cd F:/AI_workspace/inverter
cmake -S . -B build -G "Unix Makefiles"
cmake --build build --config Release
```

## 项目文件说明

### 核心文件
- `CMakeLists.txt` - CMake 构建配置
- `.vscode/tasks.json` - 编译任务
- `.vscode/c_cpp_properties.json` - IntelliSense 配置

### 系统文件
- `Core/Src/main.c` - 主入口 + 480MHz 系统时钟配置
- `Core/Src/system_stm32h7xx.c` - 系统初始化
- `Core/Src/startup_stm32h743xx.s` - 启动文件（汇编）
- `Core/Src/STM32H743BITx_FLASH.ld` - 链接脚本

### HAL 配置
- `Core/Inc/stm32h7xx_hal_conf.h` - HAL 库配置（启用的模块）

## 编译输出

编译成功后，在 `build/` 目录生成：
- `inverter.elf` - ELF 格式
- `inverter.hex` - Intel HEX（用于烧录）
- `inverter.bin` - 二进制文件
- `inverter.map` - 内存映射

## 系统时钟配置

当前配置（8MHz HSE → 480MHz）：
```c
HSE: 8 MHz
PLL1M: 2
PLL1N: 240
PLL1P: 2
PLL1Q: 4
PLL1R: 2

SYSCLK: 480 MHz
HCLK: 240 MHz (/2)
APB: 120 MHz (/4)
```

## 故障排除

### 问题 1：找不到 arm-none-eabi-gcc
**解决方案**：确保工具链在 PATH 中，重启 VSCode

### 问题 2：CMake 找不到编译器
**解决方案**：
```bash
# 指定工具链前缀
cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -S . -B build
```

### 问题 3：HAL 驱动库缺失
**解决方案**：按照步骤 1 下载并复制驱动库

### 问题 4：IntelliSense 无法找到头文件
**解决方案**：
- 检查 `c_cpp_properties.json` 中的 `includePath`
- 按 `Ctrl+Shift+P` → `C/C++: Reset IntelliSense Database`
- 重启 VSCode

## 下一步

1. **配置外设**：
   在 `Core/Src/main.c` 中添加外设初始化：
   ```c
   MX_GPIO_Init();
   MX_TIM1_Init();
   MX_ADC1_Init();
   MX_DMA_Init();
   ```

2. **集成现有框架**：
   - 框架代码已在 `Core/Src/app.c` 等
   - 在 `main.c` 的 `App_Init()` 之前调用外设初始化

3. **烧录程序**：
   ```bash
   # 使用 ST-Link
   st-flash write inverter.bin 0x08000000
   ```

## 参考资源

- [STM32H7 数据手册](https://www.st.com/resource/en/datasheet/stm32h743bi.pdf)
- [STM32H7 HAL 库文档](https://www.st.com/resource/en/user_manual/um2400-stm32cubeh7-mcubfmw-package-description-stmicroelectronics.pdf)
- [CMake 文档](https://cmake.org/documentation/)
- [ARM 工具链文档](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)
