/**
 * @file EnhancedButtonManager.cpp
 * @brief 增强按键管理器实现
 * @version 2.0.0
 */

#include "hardware/input/button/EnhancedButtonManager.hpp"
#include <cstdio>
#include <cstring>

namespace hardware {
namespace input {

EnhancedButtonManager::EnhancedButtonManager()
    : up_pin_(BUTTON_KEY1_PIN),
      down_pin_(BUTTON_KEY2_PIN), 
      screen_pin_(BUTTON_SCREEN_POWER_PIN),
      debounce_time_ms_(BUTTON_DEBOUNCE_TIME),
      long_press_ms_(NEW_BUTTON_LONG_PRESS_MS), // 使用300ms
      current_mode_(AppMode::MAIN_MENU),
      pending_event_(EnhancedButtonEvent::NONE) {
    
    // 初始化按键状态
    memset(&up_button_, 0, sizeof(ButtonState));
    memset(&down_button_, 0, sizeof(ButtonState));
    memset(&screen_button_, 0, sizeof(ButtonState));
    
    // 设置默认映射
    current_mapping_ = get_button_mapping(current_mode_);
}

bool EnhancedButtonManager::initialize() {
    printf("[EnhancedButtonManager] 初始化增强按键管理器\n");
    
    // 初始化GPIO引脚
    gpio_init(up_pin_);
    gpio_init(down_pin_);
    gpio_init(screen_pin_);
    
    // 设置为输入模式，启用上拉电阻
    gpio_set_dir(up_pin_, GPIO_IN);
    gpio_set_dir(down_pin_, GPIO_IN);
    gpio_set_dir(screen_pin_, GPIO_IN);
    
    gpio_pull_up(up_pin_);
    gpio_pull_up(down_pin_);
    gpio_pull_up(screen_pin_);
    
    // 读取初始状态
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    up_button_.last_pressed = read_button_gpio(up_pin_);
    down_button_.last_pressed = read_button_gpio(down_pin_);
    screen_button_.last_pressed = read_button_gpio(screen_pin_);
    
    printf("[EnhancedButtonManager] 引脚配置:\n");
    printf("  上键 (GPIO%d): %s\n", up_pin_, up_button_.last_pressed ? "按下" : "释放");
    printf("  下键 (GPIO%d): %s\n", down_pin_, down_button_.last_pressed ? "按下" : "释放");
    printf("  屏幕键 (GPIO%d): %s\n", screen_pin_, screen_button_.last_pressed ? "按下" : "释放");
    printf("  长按阈值: %lu ms\n", long_press_ms_);
    printf("  防抖时间: %lu ms\n", debounce_time_ms_);
    printf("[EnhancedButtonManager] 初始化完成\n");
    
    return true;
}

void EnhancedButtonManager::set_app_mode(AppMode mode) {
    if (current_mode_ != mode) {
        printf("[EnhancedButtonManager] 切换应用模式: %d -> %d\n", 
               static_cast<int>(current_mode_), static_cast<int>(mode));
        
        current_mode_ = mode;
        current_mapping_ = get_button_mapping(mode);
        
        // 清除待处理事件，避免模式切换时的误操作
        clear_events();
        
        // 重置按键状态
        up_button_.long_press_handled = false;
        down_button_.long_press_handled = false;
        screen_button_.long_press_handled = false;
    }
}

void EnhancedButtonManager::set_event_callback(std::function<void(EnhancedButtonEvent)> callback) {
    event_callback_ = callback;
}

void EnhancedButtonManager::update() {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // 更新各按键状态
    update_button_state(up_button_, up_pin_, current_time);
    update_button_state(down_button_, down_pin_, current_time);
    update_button_state(screen_button_, screen_pin_, current_time);
    
    // 处理上键事件
    process_button_event(up_button_, 
                        current_mapping_.single_press_up,
                        current_mapping_.long_press_up,
                        current_time);
    
    // 处理下键事件
    process_button_event(down_button_, 
                        current_mapping_.single_press_down,
                        current_mapping_.long_press_down,
                        current_time);
    
    // 处理屏幕键事件（只支持短按）
    if (screen_button_.pressed && !screen_button_.last_pressed) {
        // 屏幕键按下
        trigger_event(function_to_event(current_mapping_.screen_press));
    }
}

void EnhancedButtonManager::update_button_state(ButtonState& button, uint8_t pin, uint32_t current_time) {
    bool current_pressed = read_button_gpio(pin);
    
    // 检测按键状态变化
    if (current_pressed && !button.last_pressed) {
        // 按键刚按下
        button.pressed = true;
        button.press_time = current_time;
        button.long_press_handled = false;
        
        #if BUTTON_DEBUG_ENABLED
        printf("[EnhancedButtonManager] 按键按下 (GPIO%d) - 时间: %lu ms\n", pin, current_time);
        #endif
        
    } else if (!current_pressed && button.last_pressed) {
        // 按键刚释放
        button.pressed = false;
        button.release_time = current_time;
        
        #if BUTTON_DEBUG_ENABLED
        uint32_t press_duration = current_time - button.press_time;
        printf("[EnhancedButtonManager] 按键释放 (GPIO%d) - 时间: %lu ms, 持续: %lu ms\n", 
               pin, current_time, press_duration);
        #endif
        
    } else if (current_pressed && button.last_pressed) {
        // 按键持续按下
        button.pressed = true;
    } else {
        // 按键持续释放
        button.pressed = false;
    }
    
    button.last_pressed = current_pressed;
}

void EnhancedButtonManager::process_button_event(const ButtonState& button, 
                                                ButtonFunction single_function,
                                                ButtonFunction long_function,
                                                uint32_t current_time) {
    
    if (button.pressed) {
        // 按键正在按下，检查是否达到长按时间
        uint32_t press_duration = current_time - button.press_time;
        
        if (!button.long_press_handled && press_duration >= long_press_ms_) {
            // 触发长按事件
            trigger_event(function_to_event(long_function));
            // 标记长按已处理，避免重复触发
            const_cast<ButtonState&>(button).long_press_handled = true;
            
            #if BUTTON_DEBUG_ENABLED
            printf("[EnhancedButtonManager] 长按触发 - 持续时间: %lu ms\n", press_duration);
            #endif
        }
        
    } else if (!button.pressed && button.last_pressed) {
        // 按键刚释放，检查是否为短按
        uint32_t press_duration = button.release_time - button.press_time;
        
        if (!button.long_press_handled && press_duration < long_press_ms_) {
            // 触发短按事件
            trigger_event(function_to_event(single_function));
            
            #if BUTTON_DEBUG_ENABLED
            printf("[EnhancedButtonManager] 短按触发 - 持续时间: %lu ms\n", press_duration);
            #endif
        }
    }
}

EnhancedButtonEvent EnhancedButtonManager::function_to_event(ButtonFunction function) {
    switch (function) {
        case ButtonFunction::NAV_UP:
            return EnhancedButtonEvent::NAV_UP;
        case ButtonFunction::NAV_DOWN:
            return EnhancedButtonEvent::NAV_DOWN;
        case ButtonFunction::NAV_SELECT:
            return EnhancedButtonEvent::NAV_SELECT;
        case ButtonFunction::NAV_BACK:
            return EnhancedButtonEvent::NAV_BACK;
        case ButtonFunction::NAV_HOME:
            return EnhancedButtonEvent::NAV_HOME;
        case ButtonFunction::PAGE_UP:
            return EnhancedButtonEvent::PAGE_UP;
        case ButtonFunction::PAGE_DOWN:
            return EnhancedButtonEvent::PAGE_DOWN;
        case ButtonFunction::SCREEN_TOGGLE:
            return EnhancedButtonEvent::SCREEN_TOGGLE;
        case ButtonFunction::MENU_ENTER:
            return EnhancedButtonEvent::MENU_ENTER;
        case ButtonFunction::BRIGHTNESS_ADJUST:
            return EnhancedButtonEvent::BRIGHTNESS_ADJUST;
        default:
            return EnhancedButtonEvent::NONE;
    }
}

void EnhancedButtonManager::trigger_event(EnhancedButtonEvent event) {
    if (event != EnhancedButtonEvent::NONE) {
        pending_event_ = event;
        
        // 如果设置了回调函数，立即调用
        if (event_callback_) {
            event_callback_(event);
        }
        
        #if BUTTON_DEBUG_ENABLED
        printf("[EnhancedButtonManager] 触发事件: %d (模式: %d)\n", 
               static_cast<int>(event), static_cast<int>(current_mode_));
        #endif
    }
}

EnhancedButtonEvent EnhancedButtonManager::get_next_event() {
    EnhancedButtonEvent event = pending_event_;
    pending_event_ = EnhancedButtonEvent::NONE;
    return event;
}

bool EnhancedButtonManager::has_event() const {
    return pending_event_ != EnhancedButtonEvent::NONE;
}

void EnhancedButtonManager::clear_events() {
    pending_event_ = EnhancedButtonEvent::NONE;
}

bool EnhancedButtonManager::read_button_gpio(uint8_t pin) {
    // 按键按下时为低电平（上拉电阻配置）
    return !gpio_get(pin);
}

const char* EnhancedButtonManager::get_debug_info() const {
    static char debug_buffer[256];
    snprintf(debug_buffer, sizeof(debug_buffer),
             "Mode: %d, Up: %s, Down: %s, Screen: %s, Event: %d",
             static_cast<int>(current_mode_),
             up_button_.pressed ? "按下" : "释放",
             down_button_.pressed ? "按下" : "释放", 
             screen_button_.pressed ? "按下" : "释放",
             static_cast<int>(pending_event_));
    return debug_buffer;
}

} // namespace input
} // namespace hardware