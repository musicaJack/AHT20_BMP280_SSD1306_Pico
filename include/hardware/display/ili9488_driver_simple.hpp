#pragma once

#include <cstdint>
#include <cstring>
#include <string_view>
#include "pico/stdlib.h"
#include "hardware/spi.h"

namespace ili9488 {

// 显示模式
enum class DisplayMode {
    Day,           // 白底黑字
    Night,         // 黑底白字（默认）
    EyeCare1,      // 护眼模式1：黑底褐色字
    EyeCare2,      // 护眼模式2：黑底绿色字
    EyeCare3       // 护眼模式3：蓝底白字
};

// 旋转枚举
enum class Rotation {
    Portrait_0 = 0,     // 0°
    Landscape_90 = 1,   // 90°
    Portrait_180 = 2,   // 180°
    Landscape_270 = 3   // 270°
};

class ILI9488DriverSimple {
public:
    // 颜色定义 (RGB666格式)
    static constexpr uint32_t COLOR_WHITE = 0xFCFCFC;
    static constexpr uint32_t COLOR_BLACK = 0x000000;
    static constexpr uint32_t COLOR_RED = 0xFC0000;
    static constexpr uint32_t COLOR_GREEN = 0x00FC00;
    static constexpr uint32_t COLOR_BLUE = 0x0000FC;
    static constexpr uint32_t COLOR_YELLOW = 0xFCFC00;
    static constexpr uint32_t COLOR_CYAN = 0x00FCFC;
    static constexpr uint32_t COLOR_MAGENTA = 0xFC00FC;
    static constexpr uint32_t COLOR_GRAY_50 = 0x7C7C7C;

    // 显示参数
    static constexpr uint16_t LCD_WIDTH = 320;
    static constexpr uint16_t LCD_HEIGHT = 480;

    // 构造函数
    ILI9488DriverSimple(spi_inst_t* spi_inst, uint8_t dc_pin, uint8_t rst_pin, uint8_t cs_pin, 
                        uint8_t sck_pin, uint8_t mosi_pin, uint8_t bl_pin, uint32_t spi_speed_hz = 40000000);
    ~ILI9488DriverSimple();

    // 初始化函数
    bool initialize();
    void clear();
    void display(); // 空函数，直接写入模式不需要刷新

    // 绘图函数 - 直接写入模式
    void drawPixelRGB666(uint16_t x, uint16_t y, uint32_t color666);
    void fillScreenRGB666(uint32_t color666);
    void fillAreaRGB666(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color666);

    // 文本显示函数 - 简化版本
    void drawString(uint16_t x, uint16_t y, std::string_view str, uint32_t color, uint32_t bg_color);
    void drawString(uint16_t x, uint16_t y, const char* str, uint32_t color, uint32_t bg_color);

    // 显示控制
    void setRotation(Rotation r);
    Rotation getRotation() const;
    void displayOn(bool enabled);
    void displaySleep(bool enabled);
    
    // 初始化状态检查
    bool is_initialized() const;

private:
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void writeData(const uint8_t* data, size_t len);
    void setAddress(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void initILI9488();

    const uint dc_pin_;
    const uint rst_pin_;
    const uint cs_pin_;
    const uint sck_pin_;
    const uint mosi_pin_;
    const uint bl_pin_;
    const uint32_t spi_speed_hz_;
    spi_inst_t* spi_inst_;

    Rotation rotation_ = Rotation::Portrait_0;
    bool initialized_ = false;

    // 简单的字体数据 - 8x16像素
    static constexpr uint8_t FONT_WIDTH = 8;
    static constexpr uint8_t FONT_HEIGHT = 16;
};

} // namespace ili9488 