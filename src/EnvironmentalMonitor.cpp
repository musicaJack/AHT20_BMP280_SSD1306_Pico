#include "EnvironmentalMonitor.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>

namespace environmental_monitor {

EnvironmentalMonitor::EnvironmentalMonitor(ili9488::ILI9488Driver* display)
    : display_(display), data_initialized_(false) {
    // 初始化传感器数据
    current_data_ = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
}

void EnvironmentalMonitor::initialize_display() {
    if (!display_) {
        printf("[ENV_MONITOR] 显示驱动为空，无法初始化显示！\n");
        return;
    }
    
    printf("[ENV_MONITOR] 开始初始化显示界面...\n");
    
    // 设置显示模式为夜间模式（深色背景，浅色文字）
    printf("[ENV_MONITOR] 设置显示模式为夜间模式...\n");
    display_->setDisplayMode(ili9488::DisplayMode::Night);
    
    // 清屏并填充深色背景
    printf("[ENV_MONITOR] 清屏并填充黑色背景...\n");
    display_->fillScreenRGB666(ili9488_colors::rgb666::BLACK);
    
    // 绘制标题
    printf("[ENV_MONITOR] 绘制标题...\n");
    draw_title();
    
    // 绘制4个传感器数据卡片（移除第一个区块，改为中文显示）
    printf("[ENV_MONITOR] 绘制传感器数据卡片...\n");
    draw_sensor_card(get_card_y_position(0), "温度", "", 0.0f, "°C", "Normal");
    draw_sensor_card(get_card_y_position(1), "湿度", "", 0.0f, "%", "Normal");
    draw_sensor_card(get_card_y_position(2), "气压", "", 0.0f, "hPa", "Normal");
    draw_sensor_card(get_card_y_position(3), "海拔", "", 0.0f, "m", "Normal");
    
    // 刷新显示
    printf("[ENV_MONITOR] 调用display()刷新显示...\n");
    display_->display();
    
    printf("[ENV_MONITOR] 显示界面初始化完成\n");
}

void EnvironmentalMonitor::update_sensor_data(const SensorData& new_data) {
    if (!display_) {
        printf("[ENV_MONITOR] 显示驱动为空！\n");
        return;
    }
    
    printf("[ENV_MONITOR] 更新传感器数据: AHT20_T=%.1f°C, AHT20_H=%.1f%%, BMP280_T=%.1f°C, BMP280_P=%.4fhPa, 平均=%.2f°C\n",
           new_data.aht20_temperature, new_data.aht20_humidity, 
           new_data.bmp280_temperature, new_data.bmp280_pressure, new_data.average_temperature);
    
    // 更新温度（使用BMP280温度作为主要温度显示）
    if (!data_initialized_ || fabs(new_data.bmp280_temperature - current_data_.bmp280_temperature) > 0.1f) {
        printf("[ENV_MONITOR] 更新温度: %.1f°C\n", new_data.bmp280_temperature);
        update_temperature(new_data.bmp280_temperature);
    }
    
    // 更新湿度
    if (!data_initialized_ || fabs(new_data.aht20_humidity - current_data_.aht20_humidity) > 0.1f) {
        printf("[ENV_MONITOR] 更新湿度: %.1f%%\n", new_data.aht20_humidity);
        update_humidity(new_data.aht20_humidity);
    }
    
    // 更新气压
    if (!data_initialized_ || fabs(new_data.bmp280_pressure - current_data_.bmp280_pressure) > 0.01f) {
        printf("[ENV_MONITOR] 更新气压: %.4fhPa\n", new_data.bmp280_pressure);
        update_pressure(new_data.bmp280_pressure);
    }
    
    // 更新海拔
    if (!data_initialized_ || fabs(new_data.bmp280_altitude - current_data_.bmp280_altitude) > 0.1f) {
        printf("[ENV_MONITOR] 更新海拔: %.1fm\n", new_data.bmp280_altitude);
        update_altitude(new_data.bmp280_altitude);
    }
    
    // 保存当前数据
    current_data_ = new_data;
    data_initialized_ = true;
    
    // 刷新显示
    printf("[ENV_MONITOR] 调用display()刷新显示\n");
    display_->display();
}

void EnvironmentalMonitor::update_temperature(float temperature) {
    if (!display_) return;
    
    uint16_t card_y = get_card_y_position(0);
    refresh_value_area(card_y, current_data_.bmp280_temperature, temperature, "°C", 1);
    current_data_.bmp280_temperature = temperature;
}

void EnvironmentalMonitor::update_humidity(float humidity) {
    if (!display_) return;
    
    uint16_t card_y = get_card_y_position(1);
    refresh_value_area(card_y, current_data_.aht20_humidity, humidity, "%", 1);
    current_data_.aht20_humidity = humidity;
}

void EnvironmentalMonitor::update_pressure(float pressure) {
    if (!display_) return;
    
    uint16_t card_y = get_card_y_position(2);
    refresh_value_area(card_y, current_data_.bmp280_pressure, pressure, "hPa", 0);  // 气压使用整数显示
    current_data_.bmp280_pressure = pressure;
}

void EnvironmentalMonitor::update_altitude(float altitude) {
    if (!display_) return;
    
    uint16_t card_y = get_card_y_position(3);
    refresh_value_area(card_y, current_data_.bmp280_altitude, altitude, "m", 1);
    current_data_.bmp280_altitude = altitude;
}



void EnvironmentalMonitor::show_error(const std::string& error_msg) {
    if (!display_) return;
    
    // 在屏幕中央显示错误信息
    uint16_t error_y = 240; // 屏幕中央
    fill_rect(0, error_y - 20, 320, 40, ili9488_colors::rgb666::BLACK);
    display_->drawString(10, error_y, error_msg.c_str(), ili9488_colors::rgb666::RED);
    display_->display();
}

void EnvironmentalMonitor::clear_error() {
    if (!display_) return;
    
    // 重新绘制整个界面
    initialize_display();
}

void EnvironmentalMonitor::draw_title() {
    // 绘制标题
    display_->drawString(60, DisplayAreas::TITLE_Y, "ENVIRONMENTAL MONITOR", 
                        ili9488_colors::rgb666::LIGHT_BLUE);
    
    // 绘制分隔线
    for (uint16_t x = DisplayAreas::CARD_MARGIN_X; x < 320 - DisplayAreas::CARD_MARGIN_X; x++) {
        display_->drawPixelRGB666(x, DisplayAreas::TITLE_Y + 30, ili9488_colors::rgb666::LIGHT_BLUE);
    }
}

void EnvironmentalMonitor::draw_card_background(uint16_t y, uint16_t height) {
    // 绘制卡片背景（深灰色边框）
    fill_rect(DisplayAreas::CARD_MARGIN_X - 2, y - 2, 
              DisplayAreas::CARD_WIDTH + 4, height + 4, 
              ili9488_colors::rgb666::GRAY_30);
    
    // 绘制卡片内部（黑色背景）
    fill_rect(DisplayAreas::CARD_MARGIN_X, y, 
              DisplayAreas::CARD_WIDTH, height, 
              ili9488_colors::rgb666::BLACK);
}

void EnvironmentalMonitor::draw_sensor_card(uint16_t y, const std::string& sensor_name, 
                                           const std::string& measurement, float value, 
                                           const std::string& unit, const std::string& status) {
    // 绘制卡片背景
    draw_card_background(y, DisplayAreas::CARD_HEIGHT);
    
    // 计算中文标签的居中位置（假设每个中文字符宽度为16像素）
    uint16_t label_width = sensor_name.length() * 16;  // 中文字符宽度
    uint16_t label_x = DisplayAreas::CARD_MARGIN_X + (DisplayAreas::CARD_WIDTH - label_width) / 2;
    
    // 绘制传感器名称（居中显示）
    display_->drawString(label_x, y + 5, 
                        sensor_name.c_str(), ili9488_colors::rgb666::GRAY_70);
    
    // 绘制测量类型（小字体）- 只有当measurement不为空时才绘制
    if (!measurement.empty()) {
        display_->drawString(DisplayAreas::CARD_MARGIN_X + 10, y + 20, 
                            measurement.c_str(), ili9488_colors::rgb666::GRAY_70);
    }
    
    // 绘制数值和单位（大字体，单位跟在数值后面）
    std::string value_str = format_value(value, (unit == "hPa") ? 0 : 2);  // 气压显示整数，其他显示2位小数
    std::string value_with_unit = value_str + unit;  // 将数值和单位组合
    display_->drawString(DisplayAreas::CARD_MARGIN_X + DisplayAreas::VALUE_X, 
                        y + DisplayAreas::VALUE_Y_OFFSET, 
                        value_with_unit.c_str(), ili9488_colors::rgb666::LIGHT_BLUE);
    
    // 绘制状态
    display_->drawString(DisplayAreas::CARD_MARGIN_X + DisplayAreas::STATUS_X, 
                        y + DisplayAreas::VALUE_Y_OFFSET, 
                        status.c_str(), ili9488_colors::rgb666::GREEN);
}



void EnvironmentalMonitor::refresh_value_area(uint16_t card_y, float old_value, float new_value, 
                                             const std::string& unit, uint8_t precision) {
    // 清除数值区域（扩大清除区域以包含单位）
    fill_rect(DisplayAreas::CARD_MARGIN_X + DisplayAreas::VALUE_X, 
              card_y + DisplayAreas::VALUE_Y_OFFSET - 5, 
              120, 20, ili9488_colors::rgb666::BLACK);  // 扩大宽度以包含单位
    
    // 绘制新数值和单位
    std::string value_str = format_value(new_value, precision);
    std::string value_with_unit = value_str + unit;  // 将数值和单位组合
    display_->drawString(DisplayAreas::CARD_MARGIN_X + DisplayAreas::VALUE_X, 
                        card_y + DisplayAreas::VALUE_Y_OFFSET, 
                        value_with_unit.c_str(), ili9488_colors::rgb666::LIGHT_BLUE);
}

void EnvironmentalMonitor::refresh_status_area(uint16_t card_y, const std::string& old_status, 
                                              const std::string& new_status) {
    // 清除状态区域
    fill_rect(DisplayAreas::CARD_MARGIN_X + DisplayAreas::STATUS_X, 
              card_y + DisplayAreas::VALUE_Y_OFFSET - 5, 
              60, 20, ili9488_colors::rgb666::BLACK);
    
    // 绘制新状态
    uint32_t status_color = (new_status == "Normal") ? ili9488_colors::rgb666::GREEN : ili9488_colors::rgb666::RED;
    display_->drawString(DisplayAreas::CARD_MARGIN_X + DisplayAreas::STATUS_X, 
                        card_y + DisplayAreas::VALUE_Y_OFFSET, 
                        new_status.c_str(), status_color);
}

std::string EnvironmentalMonitor::format_value(float value, uint8_t precision) {
    char buffer[16];
    if (precision == 0) {
        // 简单截断到整数，不做4舍5入
        snprintf(buffer, sizeof(buffer), "%d", (int)value);
    } else if (precision == 1) {
        snprintf(buffer, sizeof(buffer), "%.1f", value);
    } else if (precision == 2) {
        snprintf(buffer, sizeof(buffer), "%.2f", value);
    } else if (precision == 4) {
        snprintf(buffer, sizeof(buffer), "%.4f", value);
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f", value);
    }
    return std::string(buffer);
}

uint16_t EnvironmentalMonitor::get_card_y_position(uint8_t card_index) {
    return DisplayAreas::CARD_START_Y + card_index * (DisplayAreas::CARD_HEIGHT + DisplayAreas::CARD_SPACING);
}

void EnvironmentalMonitor::fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
    for (uint16_t row = 0; row < height; row++) {
        for (uint16_t col = 0; col < width; col++) {
            display_->drawPixelRGB666(x + col, y + row, color);
        }
    }
}

} // namespace environmental_monitor
