@echo off
REM VSCode 编译环境检查脚本

echo ========================================
echo VSCode 编译环境检查
echo ========================================
echo.

echo [1/4] 检查 ARM 工具链...
arm-none-eabi-gcc --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 找不到 arm-none-eabi-gcc
    echo 请安装 ARM 工具链并添加到 PATH
    echo 下载地址: https://developer.arm.com/downloads/-/gnu-rm
    echo.
    goto :error
) else (
    echo [OK] ARM 工具链已安装
    arm-none-eabi-gcc --version | findstr "arm-none-eabi-gcc"
)

echo.
echo [2/4] 检查 CMake...
cmake --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 找不到 CMake
    echo 请安装 CMake 并添加到 PATH
    echo 下载地址: https://cmake.org/download/
    echo.
    goto :error
) else (
    echo [OK] CMake 已安装
    cmake --version | findstr "cmake version"
)

echo.
echo [3/4] 检查 HAL 驱动库...
if not exist "Drivers\STM32H7xx_HAL_Driver" (
    echo [错误] 找不到 HAL 驱动库
    echo 请从 ST 官网下载 STM32CubeH7 并复制到 Drivers 目录
    echo 下载地址: https://www.st.com/en/embedded-software/stm32cubeh7.html
    echo.
    goto :error
) else (
    echo [OK] HAL 驱动库已存在
)

if not exist "Drivers\CMSIS" (
    echo [错误] 找不到 CMSIS
    echo 请从 STM32CubeH7 中复制 CMSIS 目录到 Drivers
    echo.
    goto :error
) else (
    echo [OK] CMSIS 已存在
)

echo.
echo [4/4] 检查 VSCode 配置...
if not exist ".vscode\tasks.json" (
    echo [警告] VSCode tasks.json 不存在
) else (
    echo [OK] VSCode tasks.json 已存在
)

if not exist ".vscode\c_cpp_properties.json" (
    echo [警告] VSCode c_cpp_properties.json 不存在
) else (
    echo [OK] VSCode c_cpp_properties.json 已存在
)

echo.
echo ========================================
echo 环境检查完成！
echo ========================================
echo.
echo 下一步：
echo 1. 在 VSCode 中打开项目
echo 2. 按 Ctrl+Shift+B 开始编译
echo.
echo 或者使用命令行：
echo   cmake -S . -B build -G "Unix Makefiles"
echo   cmake --build build --config Release
echo.
goto :end

:error
echo ========================================
echo 环境检查失败！
echo ========================================
echo 请根据上述错误信息安装缺失的工具
echo 详见 SETUP.md 文件
exit /b 1

:end
exit /b 0
