#pragma once

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/display/ili9488_driver.hpp"
#include "hardware/input/joystick/joystick.hpp"

// ILI9488 显示屏配置
#define ILI9488_SPI_INST spi0
#define ILI9488_DC_PIN 20
#define ILI9488_RST_PIN 15
#define ILI9488_CS_PIN 17
#define ILI9488_SCK_PIN 18
#define ILI9488_MOSI_PIN 19
#define ILI9488_BL_PIN 16
#define ILI9488_SPI_SPEED_HZ 40000000

// 摇杆配置
#define JOYSTICK_I2C_INST i2c1
#define JOYSTICK_SDA_PIN 6
#define JOYSTICK_SCL_PIN 7
#define JOYSTICK_I2C_ADDRESS 0x63
#define JOYSTICK_I2C_SPEED_HZ 100000

// 时间配置
#define JOYSTICK_LONG_PRESS_MS 1000
#define ILI9488_READING_MODE_DOUBLE_PRESS_MS 300

// 配置宏函数
#define ILI9488_GET_SPI_CONFIG() \
    ili9488::ILI9488Driver(ILI9488_SPI_INST, ILI9488_DC_PIN, ILI9488_RST_PIN, \
                          ILI9488_CS_PIN, ILI9488_SCK_PIN, ILI9488_MOSI_PIN, \
                          ILI9488_BL_PIN, ILI9488_SPI_SPEED_HZ)

#define JOYSTICK_GET_I2C_CONFIG() joystick::JoystickConfig() 