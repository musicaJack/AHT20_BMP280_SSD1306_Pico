cmake_minimum_required(VERSION 3.13)


# Pull in Raspberry Pi Pico SDK (must be defined before project)
# Adjust the path if your SDK is installed elsewhere
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Force the PICO_BOARD to pico_w BEFORE pico_sdk_init()
set(PICO_BOARD pico_w CACHE STRING "Target board" FORCE)

project(MicroSDTextReader C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 初始化Pico SDK
pico_sdk_init()

# 添加可执行文件
add_executable(demo
    examples/demo.cpp
)

# 启用USB输出，禁用UART输出
pico_enable_stdio_usb(demo 1)
pico_enable_stdio_uart(demo 0)

# 创建map/bin/hex/uf2文件
pico_add_extra_outputs(demo)

# 链接必要的库
target_link_libraries(demo
    pico_stdlib
    hardware_i2c
    hardware_gpio
    pico_platform
)

# 设置编译选项
target_compile_options(demo PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-function
)
