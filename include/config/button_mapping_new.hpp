/**
 * @file button_mapping_new.hpp
 * @brief 新按键功能映射配置
 * @version 2.0.0
 * 
 * 根据用户需求重新设计的按键功能映射：
 * - 单击上键 (GPIO8): 向上导航
 * - 单击下键 (GPIO9): 向下导航  
 * - 长按上键 (GPIO8): 进入菜单/返回上级
 * - 长按下键 (GPIO9): 返回主菜单
 * - 屏幕按键 (GPIO14): 屏幕开关
 */

#ifndef BUTTON_MAPPING_NEW_HPP
#define BUTTON_MAPPING_NEW_HPP

#include "config/button_config.hpp"

// === 新按键功能映射定义 ===

// 长按时间调整为300ms（根据用户需求）
#define NEW_BUTTON_LONG_PRESS_MS    600

// === 按键功能枚举 ===
enum class ButtonFunction {
    NONE = 0,
    
    // 导航功能
    NAV_UP,              // 向上导航
    NAV_DOWN,            // 向下导航
    NAV_SELECT,          // 选择/确认
    NAV_BACK,            // 返回上级
    NAV_HOME,            // 返回主菜单
    
    // 阅读功能  
    PAGE_UP,             // 上一页
    PAGE_DOWN,           // 下一页
    
    // 系统功能
    SCREEN_TOGGLE,       // 屏幕开关
    MENU_ENTER,          // 进入菜单
    BRIGHTNESS_ADJUST,   // 亮度调节确认
};

// === 各模式下的按键功能映射 ===

namespace ButtonMapping {

    // 内容模式（文本阅读）
    struct ContentMode {
        // 单击上键 (GPIO8) -> 向上导航/上一页
        static constexpr ButtonFunction SINGLE_UP = ButtonFunction::PAGE_UP;
        
        // 单击下键 (GPIO9) -> 向下导航/下一页  
        static constexpr ButtonFunction SINGLE_DOWN = ButtonFunction::PAGE_DOWN;
        
        // 长按上键 (GPIO8) -> 进入菜单
        static constexpr ButtonFunction LONG_UP = ButtonFunction::MENU_ENTER;
        
        // 长按下键 (GPIO9) -> 返回主菜单
        static constexpr ButtonFunction LONG_DOWN = ButtonFunction::NAV_HOME;
        
        // 屏幕按键 (GPIO14) -> 屏幕开关
        static constexpr ButtonFunction SCREEN_KEY = ButtonFunction::SCREEN_TOGGLE;
    };

    // 主菜单模式
    struct MainMenuMode {
        // 单击上键 (GPIO8) -> 向上导航
        static constexpr ButtonFunction SINGLE_UP = ButtonFunction::NAV_UP;
        
        // 单击下键 (GPIO9) -> 向下导航
        static constexpr ButtonFunction SINGLE_DOWN = ButtonFunction::NAV_DOWN;
        
        // 长按上键 (GPIO8) -> 进入选择
        static constexpr ButtonFunction LONG_UP = ButtonFunction::NAV_SELECT;
        
        // 长按下键 (GPIO9) -> 返回上级菜单
        static constexpr ButtonFunction LONG_DOWN = ButtonFunction::NAV_BACK;
        
        // 屏幕按键 (GPIO14) -> 屏幕开关
        static constexpr ButtonFunction SCREEN_KEY = ButtonFunction::SCREEN_TOGGLE;
    };

    // 文件列表模式
    struct FileListMode {
        // 单击上键 (GPIO8) -> 向上导航
        static constexpr ButtonFunction SINGLE_UP = ButtonFunction::NAV_UP;
        
        // 单击下键 (GPIO9) -> 向下导航
        static constexpr ButtonFunction SINGLE_DOWN = ButtonFunction::NAV_DOWN;
        
        // 长按上键 (GPIO8) -> 选择文件
        static constexpr ButtonFunction LONG_UP = ButtonFunction::NAV_SELECT;
        
        // 长按下键 (GPIO9) -> 返回主菜单
        static constexpr ButtonFunction LONG_DOWN = ButtonFunction::NAV_HOME;
        
        // 屏幕按键 (GPIO14) -> 屏幕开关
        static constexpr ButtonFunction SCREEN_KEY = ButtonFunction::SCREEN_TOGGLE;
    };

    // 系统配置模式
    struct SystemConfigMode {
        // 单击上键 (GPIO8) -> 向上导航
        static constexpr ButtonFunction SINGLE_UP = ButtonFunction::NAV_UP;
        
        // 单击下键 (GPIO9) -> 向下导航
        static constexpr ButtonFunction SINGLE_DOWN = ButtonFunction::NAV_DOWN;
        
        // 长按上键 (GPIO8) -> 进入配置项
        static constexpr ButtonFunction LONG_UP = ButtonFunction::NAV_SELECT;
        
        // 长按下键 (GPIO9) -> 返回主菜单
        static constexpr ButtonFunction LONG_DOWN = ButtonFunction::NAV_HOME;
        
        // 屏幕按键 (GPIO14) -> 屏幕开关
        static constexpr ButtonFunction SCREEN_KEY = ButtonFunction::SCREEN_TOGGLE;
    };

    // 子菜单模式（亮度调节等）
    struct SubMenuMode {
        // 单击上键 (GPIO8) -> 向上导航
        static constexpr ButtonFunction SINGLE_UP = ButtonFunction::NAV_UP;
        
        // 单击下键 (GPIO9) -> 向下导航
        static constexpr ButtonFunction SINGLE_DOWN = ButtonFunction::NAV_DOWN;
        
        // 长按上键 (GPIO8) -> 确认设置
        static constexpr ButtonFunction LONG_UP = ButtonFunction::BRIGHTNESS_ADJUST;
        
        // 长按下键 (GPIO9) -> 返回上级菜单
        static constexpr ButtonFunction LONG_DOWN = ButtonFunction::NAV_BACK;
        
        // 屏幕按键 (GPIO14) -> 屏幕开关
        static constexpr ButtonFunction SCREEN_KEY = ButtonFunction::SCREEN_TOGGLE;
    };
}

// === 功能映射表 ===
struct ButtonFunctionMapping {
    ButtonFunction single_press_up;    // 单击上键功能
    ButtonFunction single_press_down;  // 单击下键功能
    ButtonFunction long_press_up;      // 长按上键功能
    ButtonFunction long_press_down;    // 长按下键功能
    ButtonFunction screen_press;       // 屏幕按键功能
};

// === 应用模式枚举 ===
enum class AppMode {
    CONTENT_READING,    // 内容模式（文本阅读）
    MAIN_MENU,         // 主菜单
    FILE_LIST,         // 文件列表
    SYSTEM_CONFIG,     // 系统配置
    SUB_MENU          // 子菜单
};

// === 获取当前模式的按键映射 ===
inline ButtonFunctionMapping get_button_mapping(AppMode mode) {
    switch (mode) {
        case AppMode::CONTENT_READING:
            return {
                ButtonMapping::ContentMode::SINGLE_UP,
                ButtonMapping::ContentMode::SINGLE_DOWN,
                ButtonMapping::ContentMode::LONG_UP,
                ButtonMapping::ContentMode::LONG_DOWN,
                ButtonMapping::ContentMode::SCREEN_KEY
            };
            
        case AppMode::MAIN_MENU:
            return {
                ButtonMapping::MainMenuMode::SINGLE_UP,
                ButtonMapping::MainMenuMode::SINGLE_DOWN,
                ButtonMapping::MainMenuMode::LONG_UP,
                ButtonMapping::MainMenuMode::LONG_DOWN,
                ButtonMapping::MainMenuMode::SCREEN_KEY
            };
            
        case AppMode::FILE_LIST:
            return {
                ButtonMapping::FileListMode::SINGLE_UP,
                ButtonMapping::FileListMode::SINGLE_DOWN,
                ButtonMapping::FileListMode::LONG_UP,
                ButtonMapping::FileListMode::LONG_DOWN,
                ButtonMapping::FileListMode::SCREEN_KEY
            };
            
        case AppMode::SYSTEM_CONFIG:
            return {
                ButtonMapping::SystemConfigMode::SINGLE_UP,
                ButtonMapping::SystemConfigMode::SINGLE_DOWN,
                ButtonMapping::SystemConfigMode::LONG_UP,
                ButtonMapping::SystemConfigMode::LONG_DOWN,
                ButtonMapping::SystemConfigMode::SCREEN_KEY
            };
            
        case AppMode::SUB_MENU:
            return {
                ButtonMapping::SubMenuMode::SINGLE_UP,
                ButtonMapping::SubMenuMode::SINGLE_DOWN,
                ButtonMapping::SubMenuMode::LONG_UP,
                ButtonMapping::SubMenuMode::LONG_DOWN,
                ButtonMapping::SubMenuMode::SCREEN_KEY
            };
            
        default:
            return {ButtonFunction::NONE, ButtonFunction::NONE, ButtonFunction::NONE, ButtonFunction::NONE, ButtonFunction::NONE};
    }
}

#endif // BUTTON_MAPPING_NEW_HPP