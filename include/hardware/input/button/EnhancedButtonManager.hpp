/**
 * @file EnhancedButtonManager.hpp
 * @brief 增强按键管理器 - 支持多模式按键功能映射
 * @version 2.0.0
 * 
 * 根据应用模式动态调整按键功能，支持：
 * - 内容阅读模式
 * - 主菜单模式  
 * - 文件列表模式
 * - 系统配置模式
 * - 子菜单模式
 */

#ifndef ENHANCED_BUTTON_MANAGER_HPP
#define ENHANCED_BUTTON_MANAGER_HPP

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "config/button_config.hpp"
#include "config/button_mapping_new.hpp"
#include <stdint.h>
#include <functional>

namespace hardware {
namespace input {

/**
 * @brief 增强按键事件枚举
 */
enum class EnhancedButtonEvent {
    NONE = 0,
    
    // 导航事件
    NAV_UP,              // 向上导航
    NAV_DOWN,            // 向下导航
    NAV_SELECT,          // 选择/确认
    NAV_BACK,            // 返回上级
    NAV_HOME,            // 返回主菜单
    
    // 阅读事件
    PAGE_UP,             // 上一页
    PAGE_DOWN,           // 下一页
    
    // 系统事件
    SCREEN_TOGGLE,       // 屏幕开关
    MENU_ENTER,          // 进入菜单
    BRIGHTNESS_ADJUST,   // 亮度调节确认
};

/**
 * @brief 按键状态结构
 */
struct ButtonState {
    bool pressed;           // 当前是否按下
    bool last_pressed;      // 上次是否按下
    uint32_t press_time;    // 按下时间戳
    uint32_t release_time;  // 释放时间戳
    bool long_press_handled; // 长按是否已处理
};

/**
 * @brief 增强按键管理器类
 */
class EnhancedButtonManager {
private:
    // 硬件引脚
    uint8_t up_pin_;      // GPIO8 - 上键
    uint8_t down_pin_;    // GPIO9 - 下键  
    uint8_t screen_pin_;  // GPIO14 - 屏幕按键
    
    // 按键状态
    ButtonState up_button_;
    ButtonState down_button_;
    ButtonState screen_button_;
    
    // 时间配置
    uint32_t debounce_time_ms_;
    uint32_t long_press_ms_;
    
    // 当前应用模式
    AppMode current_mode_;
    ButtonFunctionMapping current_mapping_;
    
    // 事件队列
    EnhancedButtonEvent pending_event_;
    
    // 回调函数
    std::function<void(EnhancedButtonEvent)> event_callback_;

public:
    /**
     * @brief 构造函数
     */
    EnhancedButtonManager();
    
    /**
     * @brief 析构函数
     */
    ~EnhancedButtonManager() = default;
    
    /**
     * @brief 初始化按键管理器
     * @return true 成功，false 失败
     */
    bool initialize();
    
    /**
     * @brief 设置应用模式
     * @param mode 应用模式
     */
    void set_app_mode(AppMode mode);
    
    /**
     * @brief 获取当前应用模式
     * @return 当前应用模式
     */
    AppMode get_app_mode() const { return current_mode_; }
    
    /**
     * @brief 设置事件回调函数
     * @param callback 回调函数
     */
    void set_event_callback(std::function<void(EnhancedButtonEvent)> callback);
    
    /**
     * @brief 更新按键状态（需要在主循环中调用）
     */
    void update();
    
    /**
     * @brief 获取下一个按键事件
     * @return 按键事件
     */
    EnhancedButtonEvent get_next_event();
    
    /**
     * @brief 检查是否有待处理的事件
     * @return true 有事件，false 无事件
     */
    bool has_event() const;
    
    /**
     * @brief 清除所有待处理事件
     */
    void clear_events();
    
    /**
     * @brief 设置长按时间阈值
     * @param ms 长按时间（毫秒）
     */
    void set_long_press_time(uint32_t ms) { long_press_ms_ = ms; }
    
    /**
     * @brief 设置防抖时间
     * @param ms 防抖时间（毫秒）
     */
    void set_debounce_time(uint32_t ms) { debounce_time_ms_ = ms; }
    
    /**
     * @brief 获取按键状态调试信息
     * @return 调试信息字符串
     */
    const char* get_debug_info() const;

private:
    /**
     * @brief 读取按键GPIO状态
     * @param pin GPIO引脚号
     * @return true 按键按下，false 按键释放
     */
    bool read_button_gpio(uint8_t pin);
    
    /**
     * @brief 更新单个按键状态
     * @param button 按键状态结构
     * @param pin GPIO引脚号
     * @param current_time 当前时间戳
     */
    void update_button_state(ButtonState& button, uint8_t pin, uint32_t current_time);
    
    /**
     * @brief 处理按键事件
     * @param button 按键状态结构
     * @param single_function 短按功能
     * @param long_function 长按功能
     * @param current_time 当前时间戳
     */
    void process_button_event(const ButtonState& button, 
                             ButtonFunction single_function,
                             ButtonFunction long_function,
                             uint32_t current_time);
    
    /**
     * @brief 将按键功能转换为事件
     * @param function 按键功能
     * @return 对应的按键事件
     */
    EnhancedButtonEvent function_to_event(ButtonFunction function);
    
    /**
     * @brief 触发事件
     * @param event 要触发的事件
     */
    void trigger_event(EnhancedButtonEvent event);
};

} // namespace input
} // namespace hardware

#endif // ENHANCED_BUTTON_MANAGER_HPP