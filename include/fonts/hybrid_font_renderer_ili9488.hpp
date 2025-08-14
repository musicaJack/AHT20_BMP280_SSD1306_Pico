// #pragma once
// 
// #include "fonts/hybrid_font_renderer.hpp"
// #include "hardware/display/ili9488_driver.hpp"
// #include "config/ili9488_colors.hpp"
// 
// namespace hybrid_font {
// 
// // ILI9488专用的字体管理器（已废弃，强制走模板通用版本）
// /*
// template<>
// class FontManager<ili9488::ILI9488Driver> {
// public:
//     FontManager() = default;
//     ~FontManager() = default;
// 
//     bool initialize() {
//         // 简单初始化，实际应该加载字体数据
//         return true;
//     }
// 
//     // 绘制字符串到ILI9488显示屏
//     void draw_string(ili9488::ILI9488Driver& display, uint16_t x, uint16_t y, 
//                     const std::string& text, uint32_t color, uint32_t bg_color) {
//         display.drawString(x, y, text, color, bg_color);
//     }
// 
//     void draw_string(ili9488::ILI9488Driver& display, uint16_t x, uint16_t y, 
//                     const std::string& text, bool color) {
//         uint32_t text_color = color ? ili9488_colors::rgb666::BLACK : ili9488_colors::rgb666::WHITE;
//         uint32_t bg_color = color ? ili9488_colors::rgb666::WHITE : ili9488_colors::rgb666::BLACK;
//         display.drawString(x, y, text, text_color, bg_color);
//     }
// 
//     uint16_t get_string_width(const std::string& text) const {
//         return text.length() * 8; // 简单估算，每个字符8像素宽
//     }
// 
//     uint16_t get_font_height() const {
//         return 16; // 假设字体高度为16像素
//     }
// 
//     std::vector<std::string> wrap_text(const std::string& text, uint16_t max_width) {
//         // ...
//     }
// };
// */
// 
// } // namespace hybrid_font 