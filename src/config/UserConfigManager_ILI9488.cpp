#include "config/UserConfigManager_ILI9488.hpp"
#include "pico/stdlib.h"
#include <sstream>
#include <algorithm>
// 声明全局LCD指针
#include "hardware/display/ili9488_driver.hpp"
extern ili9488::ILI9488Driver* g_lcd_driver;

namespace config {

// 全局单例实例
static UserConfigManager_ILI9488* g_instance = nullptr;

UserConfigManager_ILI9488::UserConfigManager_ILI9488()
    : sd_manager_(nullptr), display_mode_(ili9488::DisplayMode::Night), initialized_(false), config_loaded_(false), 
      brightness_level_(DEFAULT_BRIGHTNESS_LEVEL), bookmark_sync_interval_(0), joystick_led_enabled_(true) {}

UserConfigManager_ILI9488& UserConfigManager_ILI9488::get_instance() {
    if (!g_instance) {
        g_instance = new UserConfigManager_ILI9488();
    }
    return *g_instance;
}

bool UserConfigManager_ILI9488::initialize(micro_sd_text_reader::MicroSDManager* sd_manager) {
    if (initialized_) {
        printf("[USER_CONFIG_ILI9488] 已经初始化过了\n");
        return true;
    }
    
    sd_manager_ = sd_manager;
    printf("[USER_CONFIG_ILI9488] 初始化ILI9488全局显示模式管理器\n");
    
    // 尝试加载现有配置
    if (load_config()) {
        std::string mode_str;
        switch (display_mode_) {
            case ili9488::DisplayMode::Day:
                mode_str = "日间模式";
                break;
            case ili9488::DisplayMode::Night:
                mode_str = "夜间模式";
                break;
            case ili9488::DisplayMode::EyeCare1:
                mode_str = "护眼模式1";
                break;
            case ili9488::DisplayMode::EyeCare2:
                mode_str = "护眼模式2";
                break;
            case ili9488::DisplayMode::EyeCare3:
                mode_str = "护眼模式3";
                break;
            default:
                mode_str = "夜间模式";
                break;
        }
        printf("[USER_CONFIG_ILI9488] 成功加载用户配置，显示模式: %s, 亮度等级: %d\n", 
               mode_str.c_str(), brightness_level_);
    } else {
        printf("[USER_CONFIG_ILI9488] 未找到配置文件，使用默认配置（夜间模式，亮度等级7）\n");
        // 创建默认配置文件
        save_config();
    }

    initialized_ = true;
    return true;
}

bool UserConfigManager_ILI9488::toggle_display_mode() {
    ili9488::DisplayMode new_mode;
    switch (display_mode_) {
        case ili9488::DisplayMode::Day:
            new_mode = ili9488::DisplayMode::Night;
            break;
        case ili9488::DisplayMode::Night:
            new_mode = ili9488::DisplayMode::EyeCare1;
            break;
        case ili9488::DisplayMode::EyeCare1:
            new_mode = ili9488::DisplayMode::EyeCare2;
            break;
        case ili9488::DisplayMode::EyeCare2:
            new_mode = ili9488::DisplayMode::EyeCare3;
            break;
        case ili9488::DisplayMode::EyeCare3:
            new_mode = ili9488::DisplayMode::Day;
            break;
        default:
            new_mode = ili9488::DisplayMode::Night;
            break;
    }
    
    return set_display_mode(new_mode);
}

bool UserConfigManager_ILI9488::set_display_mode(ili9488::DisplayMode mode) {
    std::string mode_str;
    switch (mode) {
        case ili9488::DisplayMode::Day:
            mode_str = "日间模式";
            break;
        case ili9488::DisplayMode::Night:
            mode_str = "夜间模式";
            break;
        case ili9488::DisplayMode::EyeCare1:
            mode_str = "护眼模式1";
            break;
        case ili9488::DisplayMode::EyeCare2:
            mode_str = "护眼模式2";
            break;
        case ili9488::DisplayMode::EyeCare3:
            mode_str = "护眼模式3";
            break;
        default:
            mode_str = "夜间模式";
            break;
    }
    
    std::string current_mode_str;
    switch (display_mode_) {
        case ili9488::DisplayMode::Day:
            current_mode_str = "日间模式";
            break;
        case ili9488::DisplayMode::Night:
            current_mode_str = "夜间模式";
            break;
        case ili9488::DisplayMode::EyeCare1:
            current_mode_str = "护眼模式1";
            break;
        case ili9488::DisplayMode::EyeCare2:
            current_mode_str = "护眼模式2";
            break;
        case ili9488::DisplayMode::EyeCare3:
            current_mode_str = "护眼模式3";
            break;
        default:
            current_mode_str = "夜间模式";
            break;
    }
    
    printf("[USER_CONFIG_ILI9488] 尝试设置显示模式: %s (当前: %s)\n", 
           mode_str.c_str(), current_mode_str.c_str());
    
    if (display_mode_ != mode) {
        display_mode_ = mode;
        printf("[USER_CONFIG_ILI9488] 全局显示模式已更新: %s\n", mode_str.c_str());
        
        // 立即保存配置到文件
        bool save_result = save_config();
        printf("[USER_CONFIG_ILI9488] 配置保存结果: %s\n", save_result ? "成功" : "失败");
        return save_result;
    } else {
        printf("[USER_CONFIG_ILI9488] 显示模式未改变，无需保存\n");
        return true; // 模式未改变，无需保存
    }
}

void UserConfigManager_ILI9488::apply_display_mode(ili9488::ILI9488Driver* display) {
    if (display && display->is_initialized()) {
        display->setDisplayMode(display_mode_);
        std::string mode_str;
        switch (display_mode_) {
            case ili9488::DisplayMode::Day:
                mode_str = "日间模式";
                break;
            case ili9488::DisplayMode::Night:
                mode_str = "夜间模式";
                break;
            case ili9488::DisplayMode::EyeCare1:
                mode_str = "护眼模式1";
                break;
            case ili9488::DisplayMode::EyeCare2:
                mode_str = "护眼模式2";
                break;
            case ili9488::DisplayMode::EyeCare3:
                mode_str = "护眼模式3";
                break;
            default:
                mode_str = "夜间模式";
                break;
        }
        printf("[USER_CONFIG_ILI9488] 应用全局显示模式到ILI9488显示屏: %s\n", mode_str.c_str());
    }
}

bool UserConfigManager_ILI9488::load_config() {
    if (!sd_manager_ || !sd_manager_->is_ready()) {
        printf("[USER_CONFIG_ILI9488] SD卡未就绪，无法加载配置\n");
        return false;
    }
    
    std::string config_path = get_config_path();
    printf("[USER_CONFIG_ILI9488] 尝试加载配置文件: %s\n", config_path.c_str());
    
    // 检查文件是否存在
    if (!sd_manager_->file_exists(config_path)) {
        printf("[USER_CONFIG_ILI9488] 配置文件不存在，将创建默认配置\n");
        return false;
    }
    
    // 打开文件读取
    auto file_handle = sd_manager_->open_file(config_path, "r");
    if (!file_handle) {
        printf("[USER_CONFIG_ILI9488] 无法打开配置文件\n");
        return false;
    }
    
    // 读取文件内容
    std::string content;
    while (true) {
        auto result = file_handle->read(256); // 读取256字节
        if (!result.is_ok() || result->empty()) break;
        content.append(result->begin(), result->end());
    }
    file_handle->close();
    
    if (parse_config_content(content)) {
        config_loaded_ = true;
        printf("[USER_CONFIG_ILI9488] 配置文件解析成功\n");
        printf("[USER_CONFIG_ILI9488] 解析后的亮度等级: %d\n", brightness_level_);
        return true;
    } else {
        printf("[USER_CONFIG_ILI9488] 配置文件格式错误，使用默认配置\n");
        return false;
    }
}

bool UserConfigManager_ILI9488::save_config() {
    if (!sd_manager_ || !sd_manager_->is_ready()) {
        printf("[USER_CONFIG_ILI9488] SD卡未就绪，无法保存配置\n");
        return false;
    }
    
    std::string config_path = get_config_path();
    std::string content = generate_config_content();
    
    printf("[USER_CONFIG_ILI9488] 保存配置文件: %s\n", config_path.c_str());
    printf("[USER_CONFIG_ILI9488] 配置内容:\n%s\n", content.c_str());
    
    // 打开文件写入
    auto file_handle = sd_manager_->open_file(config_path, "w");
    if (!file_handle) {
        printf("[USER_CONFIG_ILI9488] 无法创建配置文件\n");
        return false;
    }
    
    // 写入内容
    auto result = file_handle->write(content);
    file_handle->close();
    
    if (result.is_ok() && *result == content.length()) {
        printf("[USER_CONFIG_ILI9488] 配置文件保存成功，写入 %zu 字节\n", *result);
        
        // 验证文件是否真的写入了
        if (sd_manager_->file_exists(config_path)) {
            printf("[USER_CONFIG_ILI9488] 配置文件验证成功，文件确实存在\n");
        } else {
            printf("[USER_CONFIG_ILI9488] 警告：配置文件验证失败，文件不存在\n");
        }
        
        return true;
    } else {
        printf("[USER_CONFIG_ILI9488] 配置文件保存失败，写入字节数: %zu / %zu\n", 
               result.is_ok() ? *result : 0, content.length());
        return false;
    }
}

bool UserConfigManager_ILI9488::parse_config_content(const std::string& content) {
    printf("[USER_CONFIG_ILI9488] 开始解析配置文件内容，长度: %zu\n", content.length());
    std::istringstream iss(content);
    std::string line;
    std::string current_section;
    menu_hide_files_.clear();
    menu_hide_dirs_.clear();
    int line_number = 0;
    
    while (std::getline(iss, line)) {
        line_number++;
        // 去除行首行尾空白字符
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        // 跳过空行和注释行
        if (line.empty() || line[0] == '#') {
            continue;
        }
        // 解析section
        if (line.front() == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            printf("[USER_CONFIG_ILI9488] 第%d行: 进入section [%s]\n", line_number, current_section.c_str());
            continue;
        }
        // 查找等号分隔符
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            printf("[USER_CONFIG_ILI9488] 第%d行: 解析配置项 key='%s', value='%s', current_section='%s'\n", 
                   line_number, key.c_str(), value.c_str(), current_section.c_str());
            
            // 解析显示模式（全局配置，不在任何section中）
            if (key == "display_mode" && current_section.empty()) {
                printf("[USER_CONFIG_ILI9488] 第%d行: 解析显示模式 key='%s', value='%s'\n", line_number, key.c_str(), value.c_str());
                if (value == "day" || value == "Day" || value == "DAY") {
                    display_mode_ = ili9488::DisplayMode::Day;
                } else if (value == "night" || value == "Night" || value == "NIGHT") {
                    display_mode_ = ili9488::DisplayMode::Night;
                } else if (value == "eyecare1" || value == "EyeCare1" || value == "EYECARE1") {
                    display_mode_ = ili9488::DisplayMode::EyeCare1;
                } else if (value == "eyecare2" || value == "EyeCare2" || value == "EYECARE2") {
                    display_mode_ = ili9488::DisplayMode::EyeCare2;
                } else if (value == "eyecare3" || value == "EyeCare3" || value == "EYECARE3") {
                    display_mode_ = ili9488::DisplayMode::EyeCare3;
                } else {
                    printf("[USER_CONFIG_ILI9488] 无效的显示模式值: %s，使用默认值\n", value.c_str());
                    display_mode_ = ili9488::DisplayMode::Night;
                }
                printf("[USER_CONFIG_ILI9488] 解析显示模式: %s\n", value.c_str());
            }
            // 解析亮度（无论在哪个section都解析）
            else if (key == "brightness") {
                printf("[USER_CONFIG_ILI9488] 第%d行: 解析亮度 key='%s', value='%s'\n", line_number, key.c_str(), value.c_str());
                bool is_valid = true;
                for (char c : value) {
                    if (c < '0' || c > '9') {
                        is_valid = false;
                        break;
                    }
                }
                if (is_valid) {
                    int v = std::stoi(value);
                    printf("[USER_CONFIG_ILI9488] 亮度值解析为: %d\n", v);
                    if (v >= 1 && v <= 12) {
                        brightness_level_ = v;
                        printf("[USER_CONFIG_ILI9488] 亮度等级设置为: %d\n", brightness_level_);
                        if (g_lcd_driver) {
                            int pwm = brightness_to_pwm(brightness_level_);
                            g_lcd_driver->setBacklightBrightness(pwm);
                            printf("[USER_CONFIG_ILI9488] 已全局应用亮度: %d (PWM=%d)\n", brightness_level_, pwm);
                        }
                    } else {
                        printf("[USER_CONFIG_ILI9488] 亮度值超出范围(1-12): %d，保持默认值: %d\n", v, brightness_level_);
                    }
                } else {
                    printf("[USER_CONFIG_ILI9488] 亮度值格式无效: %s，保持默认值: %d\n", value.c_str(), brightness_level_);
                }
            }
            // 解析菜单过滤
            else if (current_section == "menu_filter") {
                if (key == "hide_files") {
                    std::istringstream ss(value);
                    std::string token;
                    while (std::getline(ss, token, ',')) {
                        token.erase(0, token.find_first_not_of(" \t"));
                        token.erase(token.find_last_not_of(" \t") + 1);
                        if (!token.empty()) menu_hide_files_.insert(token);
                    }
                } else if (key == "hide_dirs") {
                    std::istringstream ss(value);
                    std::string token;
                    while (std::getline(ss, token, ',')) {
                        token.erase(0, token.find_first_not_of(" \t"));
                        token.erase(token.find_last_not_of(" \t") + 1);
                        if (!token.empty()) menu_hide_dirs_.insert(token);
                    }
                }
            }
            // 解析书签配置
            else if (current_section == "Bookmark") {
                if (key == "sync_interval") {
                    printf("[USER_CONFIG_ILI9488] 第%d行: 解析书签同步间隔 key='%s', value='%s'\n", line_number, key.c_str(), value.c_str());
                    bool is_valid = true;
                    for (char c : value) {
                        if (c < '0' || c > '9') {
                            is_valid = false;
                            break;
                        }
                    }
                    if (is_valid) {
                        int v = std::stoi(value);
                        printf("[USER_CONFIG_ILI9488] 书签同步间隔解析为: %d 分钟\n", v);
                        if (v == 0) {
                            bookmark_sync_interval_ = 0;
                            printf("[USER_CONFIG_ILI9488] 书签同步间隔设置为: 0 (关闭定时器)\n");
                        } else if (v >= 1 && v <= 60) {
                            bookmark_sync_interval_ = v;
                            printf("[USER_CONFIG_ILI9488] 书签同步间隔设置为: %d 分钟\n", bookmark_sync_interval_);
                        } else {
                            printf("[USER_CONFIG_ILI9488] 书签同步间隔超出范围(0-60): %d，使用默认值: 0\n", v);
                            bookmark_sync_interval_ = 0;
                        }
                    } else {
                        printf("[USER_CONFIG_ILI9488] 书签同步间隔格式无效: %s，使用默认值: 0\n", value.c_str());
                        bookmark_sync_interval_ = 0;
                    }
                }
            }
            // 解析Joystick LED配置（优先处理，全局配置）
            if (key == "joystick_led") {
                printf("[USER_CONFIG_ILI9488] 第%d行: 解析Joystick LED开关 key='%s', value='%s'\n", line_number, key.c_str(), value.c_str());
                if (value == "1") {
                    joystick_led_enabled_ = true;
                    printf("[USER_CONFIG_ILI9488] Joystick LED设置为: 开启 (1)\n");
                } else if (value == "0") {
                    joystick_led_enabled_ = false;
                    printf("[USER_CONFIG_ILI9488] Joystick LED设置为: 关闭 (0)\n");
                } else {
                    printf("[USER_CONFIG_ILI9488] 无效的Joystick LED开关值: %s，使用默认值: 1 (开启)\n", value.c_str());
                    joystick_led_enabled_ = true;
                }
            }
            // 解析显示模式（全局配置，不在任何section中）
            else if (key == "display_mode" && current_section.empty()) {
                printf("[USER_CONFIG_ILI9488] 第%d行: 解析显示模式 key='%s', value='%s'\n", line_number, key.c_str(), value.c_str());
                if (value == "day" || value == "Day" || value == "DAY") {
                    display_mode_ = ili9488::DisplayMode::Day;
                } else if (value == "night" || value == "Night" || value == "NIGHT") {
                    display_mode_ = ili9488::DisplayMode::Night;
                } else if (value == "eyecare1" || value == "EyeCare1" || value == "EYECARE1") {
                    display_mode_ = ili9488::DisplayMode::EyeCare1;
                } else if (value == "eyecare2" || value == "EyeCare2" || value == "EYECARE2") {
                    display_mode_ = ili9488::DisplayMode::EyeCare2;
                } else if (value == "eyecare3" || value == "EyeCare3" || value == "EYECARE3") {
                    display_mode_ = ili9488::DisplayMode::EyeCare3;
                } else {
                    printf("[USER_CONFIG_ILI9488] 无效的显示模式值: %s，使用默认值\n", value.c_str());
                    display_mode_ = ili9488::DisplayMode::Night;
                }
                printf("[USER_CONFIG_ILI9488] 解析显示模式: %s\n", value.c_str());
            }
            // 解析亮度（无论在哪个section都解析）
            else if (key == "brightness") {
                printf("[USER_CONFIG_ILI9488] 第%d行: 解析亮度 key='%s', value='%s'\n", line_number, key.c_str(), value.c_str());
                bool is_valid = true;
                for (char c : value) {
                    if (c < '0' || c > '9') {
                        is_valid = false;
                        break;
                    }
                }
                if (is_valid) {
                    int v = std::stoi(value);
                    printf("[USER_CONFIG_ILI9488] 亮度值解析为: %d\n", v);
                    if (v >= 1 && v <= 12) {
                        brightness_level_ = v;
                        printf("[USER_CONFIG_ILI9488] 亮度等级设置为: %d\n", brightness_level_);
                        if (g_lcd_driver) {
                            int pwm = brightness_to_pwm(brightness_level_);
                            g_lcd_driver->setBacklightBrightness(pwm);
                            printf("[USER_CONFIG_ILI9488] 已全局应用亮度: %d (PWM=%d)\n", brightness_level_, pwm);
                        }
                    } else {
                        printf("[USER_CONFIG_ILI9488] 亮度值超出范围(1-12): %d，保持默认值: %d\n", v, brightness_level_);
                    }
                } else {
                    printf("[USER_CONFIG_ILI9488] 亮度值格式无效: %s，保持默认值: %d\n", value.c_str(), brightness_level_);
                }
            }
            // 解析菜单过滤
            else if (current_section == "menu_filter") {
                if (key == "hide_files") {
                    std::istringstream ss(value);
                    std::string token;
                    while (std::getline(ss, token, ',')) {
                        token.erase(0, token.find_first_not_of(" \t"));
                        token.erase(token.find_last_not_of(" \t") + 1);
                        if (!token.empty()) menu_hide_files_.insert(token);
                    }
                } else if (key == "hide_dirs") {
                    std::istringstream ss(value);
                    std::string token;
                    while (std::getline(ss, token, ',')) {
                        token.erase(0, token.find_first_not_of(" \t"));
                        token.erase(token.find_last_not_of(" \t") + 1);
                        if (!token.empty()) menu_hide_dirs_.insert(token);
                    }
                }
            }
            // 解析书签配置
            else if (current_section == "Bookmark") {
                if (key == "sync_interval") {
                    printf("[USER_CONFIG_ILI9488] 第%d行: 解析书签同步间隔 key='%s', value='%s'\n", line_number, key.c_str(), value.c_str());
                    bool is_valid = true;
                    for (char c : value) {
                        if (c < '0' || c > '9') {
                            is_valid = false;
                            break;
                        }
                    }
                    if (is_valid) {
                        int v = std::stoi(value);
                        printf("[USER_CONFIG_ILI9488] 书签同步间隔解析为: %d 分钟\n", v);
                        if (v == 0) {
                            bookmark_sync_interval_ = 0;
                            printf("[USER_CONFIG_ILI9488] 书签同步间隔设置为: 0 (关闭定时器)\n");
                        } else if (v >= 1 && v <= 60) {
                            bookmark_sync_interval_ = v;
                            printf("[USER_CONFIG_ILI9488] 书签同步间隔设置为: %d 分钟\n", bookmark_sync_interval_);
                        } else {
                            printf("[USER_CONFIG_ILI9488] 书签同步间隔超出范围(0-60): %d，使用默认值: 0\n", v);
                            bookmark_sync_interval_ = 0;
                        }
                    } else {
                        printf("[USER_CONFIG_ILI9488] 书签同步间隔格式无效: %s，使用默认值: 0\n", value.c_str());
                        bookmark_sync_interval_ = 0;
                    }
                }
            }
        }
    }
    return true;
}

std::string UserConfigManager_ILI9488::generate_config_content() const {
    std::ostringstream oss;
    oss << "# ========================================\n";
    oss << "# 用户配置文件 - MicroSD文本阅读器\n";
    oss << "# ========================================\n";
    oss << "# 配置文件版本: 1.1\n";
    oss << "# 创建时间: " << __DATE__ << " " << __TIME__ << "\n";
    oss << "# ========================================\n\n";
    
    // Joystick LED设置放在最前面
    oss << "# Joystick LED设置\n";
    oss << "# 可选值: 1=开启LED, 0=关闭LED\n";
    oss << "joystick_led=" << (joystick_led_enabled_ ? "1" : "0") << "\n\n";
    
    oss << "# 显示模式设置\n";
    oss << "# 可选值: day=日间模式(白底黑字), night=夜间模式(黑底白字), eyecare1=护眼模式1(黑底褐色字), eyecare2=护眼模式2(黑底绿色字), eyecare3=护眼模式3(蓝底白字)\n";
    std::string mode_str;
    switch (display_mode_) {
        case ili9488::DisplayMode::Day:
            mode_str = "day";
            break;
        case ili9488::DisplayMode::Night:
            mode_str = "night";
            break;
        case ili9488::DisplayMode::EyeCare1:
            mode_str = "eyecare1";
            break;
        case ili9488::DisplayMode::EyeCare2:
            mode_str = "eyecare2";
            break;
        case ili9488::DisplayMode::EyeCare3:
            mode_str = "eyecare3";
            break;
        default:
            mode_str = "night";
            break;
    }
    oss << "display_mode=" << mode_str << "\n\n";
    
    oss << "# 菜单过滤设置\n";
    oss << "# 在菜单中隐藏的文件和目录\n";
    oss << "[menu_filter]\n";
    oss << "hide_files=.DS_Store,Thumbs.db,desktop.ini,user_config.ini\n";
    oss << "hide_dirs=.pidx,System Volume Information\n\n";

    oss << "# 亮度设置\n";
    oss << "# 可选值: 1-12 (1为最暗，12为最亮)\n";
    oss << "brightness=" << brightness_level_ << "\n\n";
    
    oss << "# 书签设置\n";
    oss << "[Bookmark]\n";
    oss << "# 书签同步间隔: 分钟 (0=关闭定时器, 1-60=启用定时器)\n";
    oss << "sync_interval=" << bookmark_sync_interval_ << "\n\n";
    
    oss << "# 其他配置项预留\n";
    oss << "# font_size=16\n";
    oss << "# line_spacing=1.2\n";
    oss << "# auto_bookmark=true\n";
    
    return oss.str();
}

ili9488::DisplayMode UserConfigManager_ILI9488::get_display_mode() const {
    return display_mode_;
}

const std::unordered_set<std::string>& UserConfigManager_ILI9488::get_menu_hide_files() const {
    return menu_hide_files_;
}

const std::unordered_set<std::string>& UserConfigManager_ILI9488::get_menu_hide_dirs() const {
    return menu_hide_dirs_;
}

int UserConfigManager_ILI9488::get_brightness_level() const { return brightness_level_; }

void UserConfigManager_ILI9488::set_brightness_level(int level) {
    // 限制亮度等级在1-12范围内
    if (level < 1) level = 1;
    if (level > 12) level = 12;
    
    brightness_level_ = level;
    
    // 立即应用到显示屏
    if (g_lcd_driver) {
        int pwm_value = brightness_to_pwm(level);
        g_lcd_driver->setBacklightBrightness(pwm_value);
        printf("[USER_CONFIG_ILI9488] 设置亮度等级: %d, PWM值: %d\n", level, pwm_value);
    }
    
    // 保存到配置文件
    save_config();
}

// 亮度等级转PWM
int UserConfigManager_ILI9488::brightness_to_pwm(int level) const {
    // 12级亮度映射到PWM值 (4-255)
    // 1级最暗，12级最亮
    static const int pwm_values[] = {4, 8, 16, 24, 32, 48, 64, 96, 128, 160, 192, 255};
    if (level >= 1 && level <= 12) {
        return pwm_values[level - 1];
    }
    return 64; // 默认中等亮度
}

int UserConfigManager_ILI9488::get_bookmark_sync_interval() const {
    return bookmark_sync_interval_;
}

void UserConfigManager_ILI9488::set_bookmark_sync_interval(int minutes) {
    if (minutes < 0) minutes = 0;
    if (minutes > 60) minutes = 60;
    
    bookmark_sync_interval_ = minutes;
    if (minutes == 0) {
        printf("[USER_CONFIG_ILI9488] 设置书签同步间隔: 0 (关闭定时器)\n");
    } else {
        printf("[USER_CONFIG_ILI9488] 设置书签同步间隔: %d 分钟\n", minutes);
    }
    
    // 保存到配置文件
    save_config();
}

bool UserConfigManager_ILI9488::get_joystick_led_enabled() const {
    printf("[USER_CONFIG_ILI9488] 获取Joystick LED状态: %s\n", joystick_led_enabled_ ? "开启" : "关闭");
    return joystick_led_enabled_;
}

void UserConfigManager_ILI9488::set_joystick_led_enabled(bool enabled) {
    joystick_led_enabled_ = enabled;
    
    if (enabled) {
        printf("[USER_CONFIG_ILI9488] Joystick LED设置为: 开启\n");
    } else {
        printf("[USER_CONFIG_ILI9488] Joystick LED设置为: 关闭\n");
    }
    
    // 保存到配置文件
    save_config();
}

} // namespace config 