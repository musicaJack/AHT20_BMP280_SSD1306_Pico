cmake_minimum_required(VERSION 3.13)

# 设置Pico SDK路径
set(PICO_SDK_PATH "C:/Program Files/Raspberry Pi/Pico SDK v1.5.1/pico-sdk")

# 包含Pico SDK
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(aht20_bmp280_demo C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 初始化Pico SDK
pico_sdk_init()

# 添加可执行文件
add_executable(demo
    demo.cpp
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
