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

class ILI9488Driver {
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

    // 显示参数
    static constexpr uint16_t LCD_WIDTH = 320;
    static constexpr uint16_t LCD_HEIGHT = 480;
    // 移除大缓冲区定义，改用直接写入模式
    // static constexpr uint32_t DISPLAY_BUFFER_LENGTH = LCD_WIDTH * LCD_HEIGHT * 3; // RGB666 = 3 bytes per pixel

    // 构造函数
    ILI9488Driver(spi_inst_t* spi_inst, uint8_t dc_pin, uint8_t rst_pin, uint8_t cs_pin, 
                  uint8_t sck_pin, uint8_t mosi_pin, uint8_t bl_pin, uint32_t spi_speed_hz = 40000000);
    ~ILI9488Driver();

    // 初始化函数
    bool initialize();
    void clear();
    void display();

    // 绘图函数
    void drawPixel(uint16_t x, uint16_t y, bool color);
    void drawPixelRGB666(uint16_t x, uint16_t y, uint32_t color666);
    void fill(uint8_t data);

    // 文本显示函数
    void drawChar(uint16_t x, uint16_t y, char c, bool color);
    void drawString(uint16_t x, uint16_t y, std::string_view str, bool color);
    void drawString(uint16_t x, uint16_t y, const char* str, bool color);
    void drawString(uint16_t x, uint16_t y, std::string_view str, uint32_t color, uint32_t bg_color);
    uint16_t getStringWidth(std::string_view str) const;

    // 区域填充函数
    void fillAreaRGB666(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color666);
    void fillScreenRGB666(uint32_t color666);

    // 显示控制
    void displayOn(bool enabled);
    void displaySleep(bool enabled);
    void displayInversion(bool enabled);
    void lowPowerMode();
    void highPowerMode();
    
    // 镜像控制
    void setMirrorX(bool enable);
    void setMirrorY(bool enable);
    bool getMirrorX() const;
    bool getMirrorY() const;

    // 新增接口
    void clearDisplay();
    void setRotation(Rotation r);
    Rotation getRotation() const;
    void display_on(bool enabled);
    void display_sleep(bool enabled);
    void display_Inversion(bool enabled);
    void Low_Power_Mode();
    void High_Power_Mode();

    void plotPixelRaw(uint16_t x, uint16_t y, bool color);
    void plotPixelGrayRaw(uint16_t x, uint16_t y, uint8_t gray_level);

    uint8_t getCurrentFontWidth() const;

    // 显示模式控制
    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const;
    uint32_t getBackgroundColor() const; // 新增：获取当前显示模式的背景色
    
    // 初始化状态检查
    bool is_initialized() const;

    void setBacklight(bool enable);
    void setBacklightBrightness(uint8_t brightness = 255);
    
    // 屏幕开关控制
    void toggleScreenPower();           // 切换屏幕开关状态
    void setScreenPowerState(bool on);  // 设置屏幕开关状态
    bool getScreenPowerState() const;   // 获取屏幕开关状态

private:
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void writeData(const uint8_t* data, size_t len);
    void writePoint(uint16_t x, uint16_t y, bool enabled);
    void writePointGray(uint16_t x, uint16_t y, uint8_t color);

    const uint dc_pin_;
    const uint rst_pin_;
    const uint cs_pin_;
    const uint sck_pin_;
    const uint mosi_pin_;
    const uint bl_pin_;
    const uint32_t spi_speed_hz_;
    // 移除大缓冲区，改用直接写入模式
    // uint8_t* display_buffer_;

    bool hpm_mode_ = false;
    bool lpm_mode_ = false;

    Rotation rotation_ = Rotation::Portrait_0;
    
    // 镜像控制
    bool mirror_x_ = true;  // 默认开启X轴镜像，解决你的问题
    bool mirror_y_ = false;

    DisplayMode display_mode_ = DisplayMode::Night;  // 默认黑底白字
    
    bool initialized_ = false;  // 初始化状态标志
    bool screen_power_on_ = true;  // 屏幕开关状态，默认开启

    // 私有辅助函数
    void setAddress();
    void initILI9488();
    void updateDisplayMode();
};

} // namespace ili9488 