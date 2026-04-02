@echo off
REM 一键编译脚本

echo ========================================
echo 开始编译 inverter 项目
echo ========================================
echo.

echo [1/2] 配置项目...
cmake -S . -B build -G "Unix Makefiles"
if %errorlevel% neq 0 (
    echo [错误] 配置失败
    pause
    exit /b 1
)

echo.
echo [2/2] 编译项目...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo [错误] 编译失败
    pause
    exit /b 1
)

echo.
echo ========================================
echo 编译成功！
echo ========================================
echo.
echo 输出文件：
dir build\inverter.*
echo.
echo 详细信息：详见 SETUP.md
pause
