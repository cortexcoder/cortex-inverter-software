cmake_minimum_required(VERSION 3.20)
project(inverter STM32 ASM C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ASM_STANDARD "")

set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

set(MCU_FAMILY STM32H7xx)
set(MCU_MODEL STM32H743xx)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(TOOLCHAIN arm-none-eabi-)
set(CMAKE_C_COMPILER ${TOOLCHAIN}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN}gcc)
set(CMAKE_OBJCOPY ${TOOLCHAIN}objcopy)
set(CMAKE_SIZE ${TOOLCHAIN}size)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

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

file(GLOB_RECURSE SOURCES 
    "Core/Src/*.c"
    "Core/Src/*.s"
    "Drivers/STM32H7xx_HAL_Driver/Src/*.c"
    "Drivers/CMSIS/Device/ST/STM32H7xx/Source/Templates/system_stm32h7xx.c"
)

include_directories(
    "Core/Inc"
    "Drivers/STM32H7xx_HAL_Driver/Inc"
    "Drivers/CMSIS/Device/ST/STM32H7xx/Include"
    "Drivers/CMSIS/Include"
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
