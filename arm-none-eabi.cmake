set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(TOOLCHAIN_PATH "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin")

set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-gcc")
set(CMAKE_AR "${TOOLCHAIN_PATH}/arm-none-eabi-ar")
set(CMAKE_RANLIB "${TOOLCHAIN_PATH}/arm-none-eabi-ranlib")
set(CMAKE_OBJCOPY "${TOOLCHAIN_PATH}/arm-none-eabi-objcopy")
set(CMAKE_SIZE "${TOOLCHAIN_PATH}/arm-none-eabi-size")

set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")
