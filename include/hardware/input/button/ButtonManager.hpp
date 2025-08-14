/**
 * @file ButtonManager.hpp
 * @brief 新按键管理器 - 专门处理三个自定义按键
 * @version 1.0.0
 * 
 * 管理三个新增的自定义按键：
 * - GPIO14: 屏幕开关按键
 * - GPIO8:  翻页上按键 
 * - GPIO9:  翻页下按键
 */

#ifndef BUTTON_MANAGER_HPP
#define BUTTON_MANAGER_HPP

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "config/button_config.hpp"
#include <stdint.h>

namespace hardware {
namespace input {

/**
 * @brief 按键事件枚举
 */
enum class ButtonManagerEvent {
    NONE = 0,                 // 无事件
    SCREEN_POWER_TOGGLE = 1,  // GPIO14 - 屏幕开关切换
    PAGE_UP = 2,              // GPIO8  - 翻页上（短按）
    PAGE_DOWN = 3,            // GPIO9  - 翻页下（短按）
    PAGE_UP_LONG = 4,         // GPIO8  - 翻页上（长按）
    PAGE_DOWN_LONG = 5        // GPIO9  - 翻页下（长按）
};

/**
 * @brief 新按键管理器类
 * 
 * 独立管理三个新增按键，与现有按键系统并行工作
 */
class ButtonManager {
private:
    // 按键引脚
    uint8_t screen_power_pin_;    // GPIO14 - 屏幕开关
    uint8_t page_up_pin_;         // GPIO8  - 翻页上
    uint8_t page_down_pin_;       // GPIO9  - 翻页下
    
    // 按键状态（上一次读取的状态）
    bool last_screen_power_state_;
    bool last_page_up_state_;
    bool last_page_down_state_;
    
    // 防抖相关
    uint32_t last_screen_power_time_;
    uint32_t last_page_up_time_;
    uint32_t last_page_down_time_;
    uint32_t debounce_time_ms_;
    
    // 长按检测相关
    uint32_t long_press_time_ms_;
    uint32_t page_up_press_start_time_;
    uint32_t page_down_press_start_time_;
    bool page_up_long_press_handled_;
    bool page_down_long_press_handled_;
    
    // 事件队列（简单实现）
    ButtonManagerEvent pending_event_;

public:
    /**
     * @brief 构造函数
     */
    ButtonManager();
    
    /**
     * @brief 析构函数
     */
    ~ButtonManager() = default;
    
    /**
     * @brief 初始化按键管理器
     * @return true 成功，false 失败
     */
    bool initialize();
    
    /**
     * @brief 更新按键状态（需要在主循环中调用）
     */
    void update();
    
    /**
     * @brief 获取下一个按键事件
     * @return 按键事件
     */
    ButtonManagerEvent getNextEvent();
    
    /**
     * @brief 检查是否有待处理的事件
     * @return true 有事件，false 无事件
     */
    bool hasEvent() const;
    
    /**
     * @brief 获取按键状态调试信息
     * @return 调试信息字符串
     */
    const char* getDebugInfo() const;

private:
    /**
     * @brief 读取按键状态
     * @param pin GPIO引脚号
     * @return true 按键按下，false 按键释放
     */
    bool readButtonState(uint8_t pin);
    
    /**
     * @brief 检查按键是否刚刚按下（带防抖）
     * @param pin GPIO引脚号
     * @param current_state 当前状态
     * @param last_state 上次状态的引用
     * @param last_time 上次时间的引用
     * @return true 按键刚刚按下，false 未按下或防抖中
     */
    bool isButtonJustPressed(uint8_t pin, bool current_state, bool& last_state, uint32_t& last_time);
};

} // namespace input
} // namespace hardware

#endif // BUTTON_MANAGER_HPP