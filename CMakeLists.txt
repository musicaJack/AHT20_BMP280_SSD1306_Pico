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

# 添加pico_fatfs库
add_subdirectory(lib/pico_fatfs)

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

# 添加环境监测器可执行文件
add_executable(environmental_monitor
    examples/environmental_monitor_demo.cpp
    src/EnvironmentalMonitor.cpp
    src/hardware/display/ili9488_driver.cpp
    src/fonts/hybrid_font_system.cpp
    src/fonts/flash_font_cache.cpp
    src/fonts/st73xx_font.cpp
)

# 启用USB输出，禁用UART输出
pico_enable_stdio_usb(environmental_monitor 1)
pico_enable_stdio_uart(environmental_monitor 0)

# 创建map/bin/hex/uf2文件
pico_add_extra_outputs(environmental_monitor)

# 链接必要的库
target_link_libraries(environmental_monitor
    pico_stdlib
    hardware_i2c
    hardware_gpio
    hardware_spi
    hardware_pwm
    pico_platform
    pico_fatfs
)

# 设置编译选项
target_compile_options(environmental_monitor PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-function
)

# 添加包含目录
target_include_directories(environmental_monitor PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/MicroSDTextReader_ILI9488/include
)
