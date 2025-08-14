#pragma once

#include <cstdint>

namespace ili9488_colors {

namespace rgb666 {
    // 基础颜色 (RGB666格式)
    static constexpr uint32_t BLACK = 0x000000;
    static constexpr uint32_t WHITE = 0xFCFCFC;
    static constexpr uint32_t RED = 0xFC0000;
    static constexpr uint32_t GREEN = 0x00FC00;
    static constexpr uint32_t BLUE = 0x0000FC;
    static constexpr uint32_t YELLOW = 0xFCFC00;
    static constexpr uint32_t CYAN = 0x00FCFC;
    static constexpr uint32_t MAGENTA = 0xFC00FC;
    
    // 灰度色阶
    static constexpr uint32_t GRAY_10 = 0x191919;
    static constexpr uint32_t GRAY_20 = 0x333333;
    static constexpr uint32_t GRAY_30 = 0x4C4C4C;
    static constexpr uint32_t GRAY_40 = 0x666666;
    static constexpr uint32_t GRAY_50 = 0x7F7F7F;
    static constexpr uint32_t GRAY_60 = 0x999999;
    static constexpr uint32_t GRAY_70 = 0xB2B2B2;
    static constexpr uint32_t GRAY_80 = 0xCCCCCC;
    static constexpr uint32_t GRAY_90 = 0xE5E5E5;
    
    // 暖色调
    static constexpr uint32_t ORANGE = 0xFC8000;
    static constexpr uint32_t PINK = 0xFC80FC;
    static constexpr uint32_t BROWN = 0x804000;
    
    // 冷色调
    static constexpr uint32_t LIGHT_BLUE = 0x8080FC;
    static constexpr uint32_t DARK_BLUE = 0x000080;
    static constexpr uint32_t LIGHT_GREEN = 0x80FC80;
    static constexpr uint32_t DARK_GREEN = 0x008000;
    
    // 护眼模式颜色
    static constexpr uint32_t EYECARE_BROWN = 0xD2691E;  // 护眼模式1：褐色
    static constexpr uint32_t EYECARE_GREEN = 0x90EE90;  // 护眼模式2：浅绿色
    static constexpr uint32_t EYECARE_BLUE_BG = 0x1E3A8A;  // 护眼模式3：深蓝色背景
}

namespace rgb565 {
    // RGB565格式颜色 (用于兼容性)
    static constexpr uint16_t BLACK = 0x0000;
    static constexpr uint16_t WHITE = 0xFFFF;
    static constexpr uint16_t RED = 0xF800;
    static constexpr uint16_t GREEN = 0x07E0;
    static constexpr uint16_t BLUE = 0x001F;
    static constexpr uint16_t YELLOW = 0xFFE0;
    static constexpr uint16_t CYAN = 0x07FF;
    static constexpr uint16_t MAGENTA = 0xF81F;
}

// 颜色转换函数
inline uint16_t rgb666_to_rgb565(uint32_t rgb666) {
    uint8_t r = (rgb666 >> 16) & 0xFC;
    uint8_t g = (rgb666 >> 8) & 0xFC;
    uint8_t b = rgb666 & 0xFC;
    
    // 转换为RGB565
    uint16_t r5 = (r >> 3) & 0x1F;
    uint16_t g6 = (g >> 2) & 0x3F;
    uint16_t b5 = (b >> 3) & 0x1F;
    
    return (r5 << 11) | (g6 << 5) | b5;
}

inline uint32_t rgb565_to_rgb666(uint16_t rgb565) {
    uint8_t r5 = (rgb565 >> 11) & 0x1F;
    uint8_t g6 = (rgb565 >> 5) & 0x3F;
    uint8_t b5 = rgb565 & 0x1F;
    
    // 转换为RGB666
    uint8_t r = (r5 << 3) | (r5 >> 2);
    uint8_t g = (g6 << 2) | (g6 >> 4);
    uint8_t b = (b5 << 3) | (b5 >> 2);
    
    return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
}

} // namespace ili9488_colors 