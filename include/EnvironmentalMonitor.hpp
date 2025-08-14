#pragma once

#include <string>
#include <cstdint>
#include "hardware/display/ili9488_driver.hpp"
#include "config/ili9488_colors.hpp"

namespace environmental_monitor {

// 传感器数据结构
struct SensorData {
    float aht20_temperature;    // AHT20温度 (°C)
    float aht20_humidity;       // AHT20湿度 (%)
    float bmp280_pressure;      // BMP280气压 (hPa)
    float bmp280_temperature;   // BMP280温度 (°C)
    float bmp280_altitude;      // BMP280海拔 (m)
    float average_temperature;  // 平均温度 (°C)
};

// 显示区域定义
struct DisplayAreas {
    // 标题区域
    static constexpr uint16_t TITLE_Y = 20;
    static constexpr uint16_t TITLE_HEIGHT = 40;
    
    // 数据卡片区域
    static constexpr uint16_t CARD_START_Y = 80;
    static constexpr uint16_t CARD_HEIGHT = 80;
    static constexpr uint16_t CARD_SPACING = 20;
    static constexpr uint16_t CARD_MARGIN_X = 20;
    static constexpr uint16_t CARD_WIDTH = 280; // 320 - 2 * 20
    
    // 平均温度区域
    static constexpr uint16_t AVERAGE_Y = 420;
    static constexpr uint16_t AVERAGE_HEIGHT = 40;
    
    // 数值显示区域（每个卡片内）
    static constexpr uint16_t VALUE_X = 20;
    static constexpr uint16_t VALUE_Y_OFFSET = 30;  // 从25调整为30，下调5个像素
    static constexpr uint16_t UNIT_X_OFFSET = 120;
    static constexpr uint16_t STATUS_X = 200;
};

class EnvironmentalMonitor {
public:
    EnvironmentalMonitor(ili9488::ILI9488Driver* display);
    ~EnvironmentalMonitor() = default;
    
    // 初始化显示界面
    void initialize_display();
    
    // 更新传感器数据（支持局部刷新）
    void update_sensor_data(const SensorData& new_data);
    
    // 单独更新某个传感器数据
    void update_temperature(float temperature);
    void update_humidity(float humidity);
    void update_pressure(float pressure);
    void update_altitude(float altitude);
    
    // 显示错误信息
    void show_error(const std::string& error_msg);
    
    // 清除错误显示
    void clear_error();

private:
    ili9488::ILI9488Driver* display_;
    SensorData current_data_;
    bool data_initialized_;
    
    // 绘制函数
    void draw_title();
    void draw_card_background(uint16_t y, uint16_t height);
    void draw_sensor_card(uint16_t y, const std::string& sensor_name, 
                         const std::string& measurement, float value, 
                         const std::string& unit, const std::string& status = "Normal");
    
    // 局部刷新函数
    void refresh_value_area(uint16_t card_y, float old_value, float new_value, 
                           const std::string& unit, uint8_t precision = 1);
    void refresh_status_area(uint16_t card_y, const std::string& old_status, 
                           const std::string& new_status);
    
    // 工具函数
    std::string format_value(float value, uint8_t precision = 1);
    uint16_t get_card_y_position(uint8_t card_index);
    void fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);
};

} // namespace environmental_monitor
