cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(TOOLCHAIN_PATH "Z:/bin")
set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-gcc.exe")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-g++.exe")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-gcc.exe")
set(CMAKE_AR "${TOOLCHAIN_PATH}/arm-none-eabi-ar.exe")
set(CMAKE_RANLIB "${TOOLCHAIN_PATH}/arm-none-eabi-ranlib.exe")
set(CMAKE_OBJCOPY "${TOOLCHAIN_PATH}/arm-none-eabi-objcopy.exe")
set(CMAKE_SIZE "${TOOLCHAIN_PATH}/arm-none-eabi-size.exe")

set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

project(inverter C ASM CXX)

set(MCU_FAMILY STM32H7xx)
set(MCU_MODEL STM32H743xx)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ASM_STANDARD "")

add_compile_options(
    -mcpu=cortex-m7
    -mthumb
    -mfpu=fpv5-d16
    -mfloat-abi=hard
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -ffunction-sections
    -fdata-sections
)

add_link_options(
    -mcpu=cortex-m7
    -mthumb
    -mfpu=fpv5-d16
    -mfloat-abi=hard
    -specs=nano.specs
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
)

file(GLOB_RECURSE HAL_DRIVER_SOURCES 
    "Drivers/STM32H7xx_HAL_Driver/Src/*.c"
)

list(FILTER HAL_DRIVER_SOURCES EXCLUDE REGEX ".*timebase.*template.*")

file(GLOB_RECURSE SOURCES 
    "Core/Src/*.c"
    "Core/Src/*.s"
    "Components/**/*.c"
)
list(APPEND SOURCES ${HAL_DRIVER_SOURCES})

include_directories(
    "Core/Inc"
    "Drivers/STM32H7xx_HAL_Driver/Inc"
    "Drivers/CMSIS/Device/ST/STM32H7xx/Include"
    "Drivers/CMSIS/Include"
    "Drivers/CMSIS/DSP/Include"
    "Drivers/CMSIS/RTOS2/Include"
    "Components/Common"
    "Components/Transforms"
    "Components/Controllers"
    "Components/Filters"
    "Components/InverterFoc"
)

add_definitions(
    -DSTM32H743xx
    -DUSE_HAL_DRIVER
    -DARM_MATH_CM7
    -DARM_MATH_MATRIX_CHECK
    -DARM_MATH_ROUNDING
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_options(${PROJECT_NAME} PRIVATE
    -T${CMAKE_SOURCE_DIR}/Core/Src/STM32H743BITx_FLASH.ld
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "Building ${PROJECT_NAME}.hex and ${PROJECT_NAME}.bin"
)
