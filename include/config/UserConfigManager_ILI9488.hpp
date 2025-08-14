#pragma once

#include "hardware/display/ili9488_driver.hpp"
#include "micro_sd_text_reader/MicroSDManager.hpp"
#include <unordered_set>

namespace config {

#define DEFAULT_BRIGHTNESS_LEVEL 4

// ILI9488版本的用户配置管理器
class UserConfigManager_ILI9488 {
public:
    static UserConfigManager_ILI9488& get_instance();
    
    // 初始化配置管理器
    bool initialize(micro_sd_text_reader::MicroSDManager* sd_manager);
    
    // 应用显示模式到ILI9488显示屏
    void apply_display_mode(ili9488::ILI9488Driver* display);
    
    // 获取显示模式
    ili9488::DisplayMode get_display_mode() const;
    
    // 设置显示模式
    bool set_display_mode(ili9488::DisplayMode mode);
    
    // 切换显示模式
    bool toggle_display_mode();
    
    // 保存配置到SD卡
    bool save_config();
    
    // 从SD卡加载配置
    bool load_config();
    
    // 获取配置文件路径
    std::string get_config_path() const { return "/user_config.ini"; }
    
    // 检查是否已初始化
    bool is_initialized() const { return initialized_; }
    
    // 获取菜单过滤文件/目录名集合
    const std::unordered_set<std::string>& get_menu_hide_files() const;
    const std::unordered_set<std::string>& get_menu_hide_dirs() const;

    int get_brightness_level() const;
    void set_brightness_level(int level);
    int brightness_to_pwm(int level) const;
    
    // 书签同步间隔配置
    int get_bookmark_sync_interval() const;
    void set_bookmark_sync_interval(int minutes);
    
    // Joystick LED控制配置
    bool get_joystick_led_enabled() const;
    void set_joystick_led_enabled(bool enabled);

private:
    UserConfigManager_ILI9488();
    ~UserConfigManager_ILI9488();
    UserConfigManager_ILI9488(const UserConfigManager_ILI9488&) = delete;
    UserConfigManager_ILI9488& operator=(const UserConfigManager_ILI9488&) = delete;
    
    // 解析配置文件内容
    bool parse_config_content(const std::string& content);
    
    // 生成配置文件内容
    std::string generate_config_content() const;
    
    micro_sd_text_reader::MicroSDManager* sd_manager_;
    ili9488::DisplayMode display_mode_;
    bool initialized_;
    bool config_loaded_;
    
    // 菜单过滤配置
    std::unordered_set<std::string> menu_hide_files_;
    std::unordered_set<std::string> menu_hide_dirs_;

    int brightness_level_;
    
    // 书签同步间隔（分钟）
    int bookmark_sync_interval_;
    
    // Joystick LED开关
    bool joystick_led_enabled_;
};

} // namespace config 