#include "hardware/display/ili9488_driver.hpp"
#include "config/ili9488_colors.hpp"
// #include "config/UserConfigManager_ILI9488.hpp"  // 注释掉，避免依赖MicroSDManager
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <cstring>
#include <cstdio>
#include "fonts/hybrid_font_renderer.hpp"
#include "fonts/st73xx_font.hpp"

namespace ili9488 {

// ILI9488寄存器定义
#define ILI9488_CMD_NOP 0x00
#define ILI9488_CMD_SWRESET 0x01
#define ILI9488_CMD_RDDID 0x04
#define ILI9488_CMD_RDDST 0x09
#define ILI9488_CMD_SLPIN 0x10
#define ILI9488_CMD_SLPOUT 0x11
#define ILI9488_CMD_PTLON 0x12
#define ILI9488_CMD_NORON 0x13
#define ILI9488_CMD_INVOFF 0x20
#define ILI9488_CMD_INVON 0x21
#define ILI9488_CMD_DISPOFF 0x28
#define ILI9488_CMD_DISPON 0x29
#define ILI9488_CMD_CASET 0x2A
#define ILI9488_CMD_RASET 0x2B
#define ILI9488_CMD_RAMWR 0x2C
#define ILI9488_CMD_RAMRD 0x2E
#define ILI9488_CMD_PTLAR 0x30
#define ILI9488_CMD_VSCRDEF 0x33
#define ILI9488_CMD_MADCTL 0x36
#define ILI9488_CMD_VSCRSADD 0x37
#define ILI9488_CMD_PIXFMT 0x3A
#define ILI9488_CMD_FRMCTR1 0xB1
#define ILI9488_CMD_FRMCTR2 0xB2
#define ILI9488_CMD_FRMCTR3 0xB3
#define ILI9488_CMD_INVCTR 0xB4
#define ILI9488_CMD_DFUNCTR 0xB6
#define ILI9488_CMD_PWCTR1 0xC0
#define ILI9488_CMD_PWCTR2 0xC1
#define ILI9488_CMD_PWCTR3 0xC2
#define ILI9488_CMD_PWCTR4 0xC3
#define ILI9488_CMD_PWCTR5 0xC4
#define ILI9488_CMD_VMCTR1 0xC5
#define ILI9488_CMD_RDID1 0xDA
#define ILI9488_CMD_RDID2 0xDB
#define ILI9488_CMD_RDID3 0xDC
#define ILI9488_CMD_RDID4 0xDD
#define ILI9488_CMD_GMCTRP1 0xE0
#define ILI9488_CMD_GMCTRN1 0xE1
#define ILI9488_CMD_PWCTR6 0xFC

// MADCTL位定义
#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH 0x04

ILI9488Driver::ILI9488Driver(spi_inst_t* spi_inst, uint8_t dc_pin, uint8_t rst_pin, uint8_t cs_pin,
                             uint8_t sck_pin, uint8_t mosi_pin, uint8_t bl_pin, uint32_t spi_speed_hz)
    : dc_pin_(dc_pin), rst_pin_(rst_pin), cs_pin_(cs_pin), sck_pin_(sck_pin),
      mosi_pin_(mosi_pin), bl_pin_(bl_pin), spi_speed_hz_(spi_speed_hz) {
    // 移除大缓冲区分配，改用直接写入模式
}

ILI9488Driver::~ILI9488Driver() {
    // 移除大缓冲区释放，改用直接写入模式
}

bool ILI9488Driver::initialize() {
    printf("  [ILI9488] 开始硬件初始化...\n");
    
    // 初始化GPIO
    printf("  [ILI9488] 初始化GPIO引脚...\n");
    gpio_init(dc_pin_);
    gpio_init(rst_pin_);
    gpio_init(cs_pin_);
    gpio_init(bl_pin_);
    
    gpio_set_dir(dc_pin_, GPIO_OUT);
    gpio_set_dir(rst_pin_, GPIO_OUT);
    gpio_set_dir(cs_pin_, GPIO_OUT);
    gpio_set_dir(bl_pin_, GPIO_OUT);
    
    // 设置初始状态
    gpio_put(cs_pin_, 1);    // CS high (inactive)
    gpio_put(dc_pin_, 1);    // DC high (data mode)
    gpio_put(rst_pin_, 1);   // Reset high (inactive)
    
    // 初始化SPI
    printf("  [ILI9488] 初始化SPI，速度: %lu Hz\n", (unsigned long)spi_speed_hz_);
    spi_init(spi0, spi_speed_hz_);
    gpio_set_function(sck_pin_, GPIO_FUNC_SPI);
    gpio_set_function(mosi_pin_, GPIO_FUNC_SPI);
    
    // 配置背光PWM
    printf("  [ILI9488] 配置背光PWM: 引脚=%d\n", bl_pin_);
    gpio_set_function(bl_pin_, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(bl_pin_);
    uint channel = pwm_gpio_to_channel(bl_pin_);
    
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_config_set_wrap(&config, 255);
    pwm_init(slice_num, &config, true);
    
    pwm_set_chan_level(slice_num, channel, 255);
    printf("  [ILI9488] 背光PWM配置完成: slice=%d, channel=%d\n", slice_num, channel);
    
    // 硬件复位
    printf("  [ILI9488] 执行硬件复位...\n");
    gpio_put(rst_pin_, 1);
    sleep_ms(10);
    gpio_put(rst_pin_, 0);
    sleep_ms(10);
    gpio_put(rst_pin_, 1);
    sleep_ms(150);
    printf("  [ILI9488] 硬件复位完成\n");
    
    // 初始化ILI9488
    initILI9488();
    
    // 设置旋转为180度
    // printf("  [ILI9488] 设置旋转为180度...\n");
    // setRotation(Rotation::Portrait_180);
    
    // 设置显示模式为黑底白字
    printf("  [ILI9488] 设置显示模式为黑底白字...\n");
    updateDisplayMode();
    
    printf("  [ILI9488] 硬件初始化完成\n");
    initialized_ = true;
    return true;
}

void ILI9488Driver::initILI9488() {
    printf("  [ILI9488] 开始初始化序列...\n");
    
    // 软件复位
    printf("  [ILI9488] 软件复位...\n");
    writeCommand(ILI9488_CMD_SWRESET);
    sleep_ms(200);
    
    // 退出睡眠模式
    printf("  [ILI9488] 退出睡眠模式...\n");
    writeCommand(ILI9488_CMD_SLPOUT);
    sleep_ms(200);
    
    // 内存访问控制
    printf("  [ILI9488] 设置内存访问控制...\n");
    writeCommand(ILI9488_CMD_MADCTL);
    writeData(0x48);
    
    // 像素格式 (18位 RGB666)
    printf("  [ILI9488] 设置像素格式...\n");
    writeCommand(ILI9488_CMD_PIXFMT);
    writeData(0x66);
    
    // VCOM控制
    printf("  [ILI9488] 设置VCOM控制...\n");
    writeCommand(0xC5);
    writeData(0x00);
    writeData(0x36);
    writeData(0x80);
    
    // 电源控制
    printf("  [ILI9488] 设置电源控制...\n");
    writeCommand(0xC2);
    writeData(0xA7);
    
    // 正伽马校正
    printf("  [ILI9488] 设置正伽马校正...\n");
    writeCommand(0xE0);
    const uint8_t gamma_pos[] = {
        0xF0, 0x01, 0x06, 0x0F, 0x12, 0x1D, 0x36, 0x54,
        0x44, 0x0C, 0x18, 0x16, 0x13, 0x15
    };
    for (uint8_t data : gamma_pos) {
        writeData(data);
    }
    
    // 负伽马校正
    printf("  [ILI9488] 设置负伽马校正...\n");
    writeCommand(0xE1);
    const uint8_t gamma_neg[] = {
        0xF0, 0x01, 0x05, 0x0A, 0x0B, 0x07, 0x32, 0x44,
        0x44, 0x0C, 0x18, 0x17, 0x13, 0x16
    };
    for (uint8_t data : gamma_neg) {
        writeData(data);
    }
    
    // 显示反转
    printf("  [ILI9488] 设置显示反转...\n");
    writeCommand(ILI9488_CMD_INVON);
    
    // 开启显示
    printf("  [ILI9488] 开启显示...\n");
    writeCommand(ILI9488_CMD_DISPON);
    sleep_ms(50);
    
    printf("  [ILI9488] 初始化序列完成\n");
}

void ILI9488Driver::setRotation(Rotation r) {
    rotation_ = r;
    uint8_t madctl = 0;
    
    switch (r) {
        case Rotation::Portrait_0:
            madctl = MADCTL_MX | MADCTL_MY;
            break;
        case Rotation::Landscape_90:
            madctl = MADCTL_MY | MADCTL_MV;
            break;
        case Rotation::Portrait_180:
            madctl = 0;
            break;
        case Rotation::Landscape_270:
            madctl = MADCTL_MX | MADCTL_MV;
            break;
    }
    
    // 根据当前镜像设置调整MADCTL
    if (mirror_x_) {
        madctl |= MADCTL_MX;
    }
    if (mirror_y_) {
        madctl |= MADCTL_MY;
    }
    
    writeCommand(ILI9488_CMD_MADCTL);
    writeData(madctl);
}

void ILI9488Driver::clear() {
    fillScreenRGB666(COLOR_BLACK); // 只允许黑色清屏，禁止白色清屏
}

void ILI9488Driver::display() {
    // 直接写入模式，不需要缓冲区刷新
    // 所有绘制操作都是直接写入到屏幕的
}

void ILI9488Driver::drawPixelRGB666(uint16_t x, uint16_t y, uint32_t color666) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    // 设置像素窗口
    writeCommand(ILI9488_CMD_CASET);
    writeData(x >> 8);
    writeData(x & 0xFF);
    writeData(x >> 8);
    writeData(x & 0xFF);
    
    writeCommand(ILI9488_CMD_RASET);
    writeData(y >> 8);
    writeData(y & 0xFF);
    writeData(y >> 8);
    writeData(y & 0xFF);
    
    // 写入像素数据
    writeCommand(ILI9488_CMD_RAMWR);
    
    uint8_t r = (color666 >> 16) & 0xFC;
    uint8_t g = (color666 >> 8) & 0xFC;
    uint8_t b = color666 & 0xFC;
    
    writeData(r);
    writeData(g);
    writeData(b);
}

void ILI9488Driver::fillScreenRGB666(uint32_t color666) {
    uint8_t r = (color666 >> 16) & 0xFC;
    uint8_t g = (color666 >> 8) & 0xFC;
    uint8_t b = color666 & 0xFC;
    
    // 设置整个屏幕区域
    writeCommand(ILI9488_CMD_CASET);
    writeData(0x00);
    writeData(0x00);
    writeData((LCD_WIDTH - 1) >> 8);
    writeData((LCD_WIDTH - 1) & 0xFF);
    
    writeCommand(ILI9488_CMD_RASET);
    writeData(0x00);
    writeData(0x00);
    writeData((LCD_HEIGHT - 1) >> 8);
    writeData((LCD_HEIGHT - 1) & 0xFF);
    
    writeCommand(ILI9488_CMD_RAMWR);
    
    // 使用更大的缓冲区进行批量写入
    constexpr size_t BATCH_SIZE = 1024;
    uint8_t batch_buffer[BATCH_SIZE * 3];
    
    // 预填充批次缓冲区
    for (size_t i = 0; i < BATCH_SIZE; i++) {
        batch_buffer[i * 3] = r;
        batch_buffer[i * 3 + 1] = g;
        batch_buffer[i * 3 + 2] = b;
    }
    
    // 批量写入
    uint32_t total_pixels = LCD_WIDTH * LCD_HEIGHT;
    uint32_t remaining = total_pixels;
    
    while (remaining > 0) {
        size_t batch_count = std::min(remaining, static_cast<uint32_t>(BATCH_SIZE));
        writeData(batch_buffer, batch_count * 3);
        remaining -= batch_count;
    }
}

void ILI9488Driver::drawString(uint16_t x, uint16_t y, std::string_view str, uint32_t color, uint32_t bg_color) {
    // 使用混合字体系统
    static hybrid_font::FontManager<ILI9488Driver>* font_manager = nullptr;
    
    // 延迟初始化字体管理器
    if (!font_manager) {
        font_manager = new hybrid_font::FontManager<ILI9488Driver>();
        if (!font_manager->initialize()) {
            printf("[ILI9488] 字体管理器初始化失败，回退到简单字体\n");
            // 回退到简单实现
            uint16_t current_x = x;
            for (char c : str) {
                drawChar(current_x, y, c, (color != bg_color));
                current_x += 8;
            }
            return;
        }
        printf("[ILI9488] 字体管理器初始化成功\n");
    }
    
    // 使用字体管理器绘制字符串
    bool text_color = (color != bg_color);
    font_manager->draw_string(*this, x, y, std::string(str), text_color);
}

void ILI9488Driver::drawChar(uint16_t x, uint16_t y, char c, bool color) {
    // 使用ST7305 8x16字体
    const uint8_t* char_data = font::get_char_data(c);
    
    // 绘制8x16字符
    for (int row = 0; row < font::FONT_HEIGHT; row++) {
        uint8_t line_data = char_data[row];
        for (int col = 0; col < font::FONT_WIDTH; col++) {
            if (line_data & (0x80 >> col)) {
                drawPixel(x + col, y + row, color);
            }
        }
    }
}

void ILI9488Driver::setAddress() {
    // 设置当前像素地址
    writeCommand(ILI9488_CMD_CASET);
    writeData(0x00);
    writeData(0x00);
    writeData((LCD_WIDTH - 1) >> 8);
    writeData((LCD_WIDTH - 1) & 0xFF);
    
    writeCommand(ILI9488_CMD_RASET);
    writeData(0x00);
    writeData(0x00);
    writeData((LCD_HEIGHT - 1) >> 8);
    writeData((LCD_HEIGHT - 1) & 0xFF);
}

void ILI9488Driver::writeCommand(uint8_t cmd) {
    gpio_put(cs_pin_, 0);
    gpio_put(dc_pin_, 0); // 命令模式
    spi_write_blocking(spi0, &cmd, 1);
    gpio_put(cs_pin_, 1);
}

void ILI9488Driver::writeData(uint8_t data) {
    gpio_put(cs_pin_, 0);
    gpio_put(dc_pin_, 1); // 数据模式
    spi_write_blocking(spi0, &data, 1);
    gpio_put(cs_pin_, 1);
}

void ILI9488Driver::writeData(const uint8_t* data, size_t len) {
    gpio_put(cs_pin_, 0);
    gpio_put(dc_pin_, 1); // 数据模式
    spi_write_blocking(spi0, data, len);
    gpio_put(cs_pin_, 1);
}

// 其他接口的简单实现
void ILI9488Driver::displayOn(bool enabled) {
    writeCommand(enabled ? ILI9488_CMD_DISPON : ILI9488_CMD_DISPOFF);
}

void ILI9488Driver::displaySleep(bool enabled) {
    writeCommand(enabled ? ILI9488_CMD_SLPIN : ILI9488_CMD_SLPOUT);
}

void ILI9488Driver::displayInversion(bool enabled) {
    writeCommand(enabled ? ILI9488_CMD_INVON : ILI9488_CMD_INVOFF);
}

// 镜像控制实现
void ILI9488Driver::setMirrorX(bool enable) {
    mirror_x_ = enable;
    setRotation(rotation_); // 重新应用旋转设置
}

void ILI9488Driver::setMirrorY(bool enable) {
    mirror_y_ = enable;
    setRotation(rotation_); // 重新应用旋转设置
}

bool ILI9488Driver::getMirrorX() const {
    return mirror_x_;
}

bool ILI9488Driver::getMirrorY() const {
    return mirror_y_;
}

void ILI9488Driver::lowPowerMode() {
    lpm_mode_ = true;
    hpm_mode_ = false;
}

void ILI9488Driver::highPowerMode() {
    hpm_mode_ = true;
    lpm_mode_ = false;
}

void ILI9488Driver::clearDisplay() {
    fillScreenRGB666(COLOR_BLACK); // 只允许黑色清屏，禁止白色清屏
}

Rotation ILI9488Driver::getRotation() const {
    return rotation_;
}

void ILI9488Driver::display_on(bool enabled) {
    displayOn(enabled);
}

void ILI9488Driver::display_sleep(bool enabled) {
    displaySleep(enabled);
}

void ILI9488Driver::display_Inversion(bool enabled) {
    displayInversion(enabled);
}

void ILI9488Driver::Low_Power_Mode() {
    lowPowerMode();
}

void ILI9488Driver::High_Power_Mode() {
    highPowerMode();
}

void ILI9488Driver::plotPixelRaw(uint16_t x, uint16_t y, bool color) {
    drawPixelRGB666(x, y, color ? COLOR_BLACK : COLOR_WHITE);
}

void ILI9488Driver::plotPixelGrayRaw(uint16_t x, uint16_t y, uint8_t gray_level) {
    uint32_t color = (gray_level << 16) | (gray_level << 8) | gray_level;
    drawPixelRGB666(x, y, color);
}

uint8_t ILI9488Driver::getCurrentFontWidth() const {
    return 8; // 默认字体宽度
}

void ILI9488Driver::setDisplayMode(DisplayMode mode) {
    display_mode_ = mode;
    updateDisplayMode();
}

DisplayMode ILI9488Driver::getDisplayMode() const {
    return display_mode_;
}

bool ILI9488Driver::is_initialized() const {
    return initialized_;
}

void ILI9488Driver::updateDisplayMode() {
    if (display_mode_ == DisplayMode::Night) {
        displayInversion(true);
    } else if (display_mode_ == DisplayMode::Day) {
        displayInversion(false);
    } else if (display_mode_ == DisplayMode::EyeCare1 || display_mode_ == DisplayMode::EyeCare2) {
        // 护眼模式1和2：黑底，但使用特定颜色
        displayInversion(true);
    } else if (display_mode_ == DisplayMode::EyeCare3) {
        // 护眼模式3：蓝底白字，不使用反转
        displayInversion(false);
    }
}

// 其他未实现的接口
void ILI9488Driver::drawPixel(uint16_t x, uint16_t y, bool color) {
    // 根据显示模式决定颜色映射
    if (display_mode_ == DisplayMode::Night) {
        // 黑底白字模式：color=true画白色，color=false画黑色
        drawPixelRGB666(x, y, color ? COLOR_WHITE : COLOR_BLACK);
    } else if (display_mode_ == DisplayMode::Day) {
        // 白底黑字模式：color=true画黑色，color=false画白色
        drawPixelRGB666(x, y, color ? COLOR_BLACK : COLOR_WHITE);
    } else if (display_mode_ == DisplayMode::EyeCare1) {
        // 护眼模式1：黑底褐色字
        drawPixelRGB666(x, y, color ? ili9488_colors::rgb666::EYECARE_BROWN : COLOR_BLACK);
    } else if (display_mode_ == DisplayMode::EyeCare2) {
        // 护眼模式2：黑底绿色字
        drawPixelRGB666(x, y, color ? ili9488_colors::rgb666::EYECARE_GREEN : COLOR_BLACK);
    } else if (display_mode_ == DisplayMode::EyeCare3) {
        // 护眼模式3：蓝底白字
        drawPixelRGB666(x, y, color ? COLOR_WHITE : ili9488_colors::rgb666::EYECARE_BLUE_BG);
    }
}

void ILI9488Driver::fill(uint8_t data) {
    // 简单实现
    uint32_t color = (data << 16) | (data << 8) | data;
    fillScreenRGB666(color);
}

void ILI9488Driver::drawString(uint16_t x, uint16_t y, std::string_view str, bool color) {
    drawString(x, y, str, color ? COLOR_WHITE : COLOR_BLACK, color ? COLOR_BLACK : COLOR_WHITE);
}

void ILI9488Driver::drawString(uint16_t x, uint16_t y, const char* str, bool color) {
    drawString(x, y, std::string_view(str), color);
}

uint16_t ILI9488Driver::getStringWidth(std::string_view str) const {
    return str.length() * 8; // 简单估算
}

void ILI9488Driver::fillAreaRGB666(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color666) {
    if (x0 > x1 || y0 > y1) return;
    
    // 设置绘制窗口
    writeCommand(ILI9488_CMD_CASET);
    writeData(x0 >> 8);
    writeData(x0 & 0xFF);
    writeData(x1 >> 8);
    writeData(x1 & 0xFF);
    
    writeCommand(ILI9488_CMD_RASET);
    writeData(y0 >> 8);
    writeData(y0 & 0xFF);
    writeData(y1 >> 8);
    writeData(y1 & 0xFF);
    
    // 开始写入数据
    writeCommand(ILI9488_CMD_RAMWR);
    
    // 准备RGB666数据
    uint8_t r = (color666 >> 16) & 0xFC;
    uint8_t g = (color666 >> 8) & 0xFC;
    uint8_t b = color666 & 0xFC;
    
    // 批量写入数据以提高效率
    uint32_t pixel_count = (x1 - x0 + 1) * (y1 - y0 + 1);
    
    // 使用更大的缓冲区进行批量写入
    constexpr size_t BATCH_SIZE = 1024;
    uint8_t batch_buffer[BATCH_SIZE * 3];
    
    // 预填充批次缓冲区
    for (size_t i = 0; i < BATCH_SIZE; i++) {
        batch_buffer[i * 3] = r;
        batch_buffer[i * 3 + 1] = g;
        batch_buffer[i * 3 + 2] = b;
    }
    
    // 批量写入
    uint32_t remaining = pixel_count;
    while (remaining > 0) {
        size_t batch_count = std::min(remaining, static_cast<uint32_t>(BATCH_SIZE));
        writeData(batch_buffer, batch_count * 3);
        remaining -= batch_count;
    }
}

void ILI9488Driver::setBacklight(bool enable) {
    if (bl_pin_ != 255) {
        gpio_set_function(bl_pin_, GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(bl_pin_);
        uint channel = pwm_gpio_to_channel(bl_pin_);
        pwm_config config = pwm_get_default_config();
        pwm_config_set_clkdiv(&config, 4.0f);
        pwm_config_set_wrap(&config, 255);
        pwm_init(slice_num, &config, true);
        pwm_set_chan_level(slice_num, channel, enable ? 255 : 0);
    } else {
        gpio_put(bl_pin_, enable ? 1 : 0);
    }
}

void ILI9488Driver::setBacklightBrightness(uint8_t brightness) {
    if (bl_pin_ != 255) {
        gpio_set_function(bl_pin_, GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(bl_pin_);
        uint channel = pwm_gpio_to_channel(bl_pin_);
        pwm_config config = pwm_get_default_config();
        pwm_config_set_clkdiv(&config, 4.0f);
        pwm_config_set_wrap(&config, 255);
        pwm_init(slice_num, &config, true);
        pwm_set_chan_level(slice_num, channel, brightness);
    } else {
        gpio_put(bl_pin_, brightness > 0 ? 1 : 0);
    }
}

uint32_t ILI9488Driver::getBackgroundColor() const {
    switch (display_mode_) {
        case DisplayMode::Day:
            return COLOR_WHITE; // 白底
        case DisplayMode::Night:
            return COLOR_BLACK; // 黑底
        case DisplayMode::EyeCare1:
            return COLOR_BLACK; // 黑底
        case DisplayMode::EyeCare2:
            return COLOR_BLACK; // 黑底
        case DisplayMode::EyeCare3:
            return ili9488_colors::rgb666::EYECARE_BLUE_BG; // 护眼模式3：蓝底白字
        default:
            return COLOR_BLACK; // 默认黑底
    }
}

// 屏幕开关控制实现
void ILI9488Driver::toggleScreenPower() {
    setScreenPowerState(!screen_power_on_);
}

void ILI9488Driver::setScreenPowerState(bool on) {
    if (screen_power_on_ == on) {
        return; // 状态没有变化，直接返回
    }
    
    screen_power_on_ = on;
    
    if (on) {
        // 开启屏幕
        printf("[ILI9488] 开启屏幕显示\n");
        displaySleep(false);  // 退出睡眠模式
        // 使用默认亮度设置
        setBacklight(true);  // 使用默认值
        printf("[ILI9488] 使用默认亮度设置\n");
        printf("[ILI9488] 屏幕已开启\n");
    } else {
        // 关闭屏幕
        printf("[ILI9488] 关闭屏幕显示\n");
        setBacklight(false);  // 关闭背光
        displaySleep(true);   // 进入睡眠模式
        printf("[ILI9488] 屏幕已关闭\n");
    }
}

bool ILI9488Driver::getScreenPowerState() const {
    return screen_power_on_;
}

} // namespace ili9488 