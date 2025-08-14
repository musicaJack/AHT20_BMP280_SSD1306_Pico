/**
 * @file ButtonManager.cpp
 * @brief 新按键管理器实现
 */

#include "hardware/input/button/ButtonManager.hpp"
#include "config/button_mapping_new.hpp"
#include <stdio.h>

namespace hardware {
namespace input {

ButtonManager::ButtonManager() 
    : screen_power_pin_(BUTTON_SCREEN_POWER_PIN),
      page_up_pin_(BUTTON_PAGE_UP_PIN),
      page_down_pin_(BUTTON_PAGE_DOWN_PIN),
      last_screen_power_state_(false),
      last_page_up_state_(false),
      last_page_down_state_(false),
      last_screen_power_time_(0),
      last_page_up_time_(0),
      last_page_down_time_(0),
      debounce_time_ms_(BUTTON_DEBOUNCE_TIME),
      long_press_time_ms_(NEW_BUTTON_LONG_PRESS_MS), // 使用300ms长按时间
      page_up_press_start_time_(0),
      page_down_press_start_time_(0),
      page_up_long_press_handled_(false),
      page_down_long_press_handled_(false),
      pending_event_(ButtonManagerEvent::NONE) {
}

bool ButtonManager::initialize() {
    printf("[ButtonManager] 初始化按键管理器...\n");
    
    // 初始化GPIO引脚
    gpio_init(screen_power_pin_);
    gpio_init(page_up_pin_);
    gpio_init(page_down_pin_);
    
    // 设置为输入模式，启用内部上拉电阻
    gpio_set_dir(screen_power_pin_, GPIO_IN);
    gpio_set_dir(page_up_pin_, GPIO_IN);
    gpio_set_dir(page_down_pin_, GPIO_IN);
    
    gpio_pull_up(screen_power_pin_);
    gpio_pull_up(page_up_pin_);
    gpio_pull_up(page_down_pin_);
    
    // 读取初始状态
    last_screen_power_state_ = readButtonState(screen_power_pin_);
    last_page_up_state_ = readButtonState(page_up_pin_);
    last_page_down_state_ = readButtonState(page_down_pin_);
    
    printf("[ButtonManager] 按键配置:\n");
    printf("  屏幕开关按键: GPIO%d\n", screen_power_pin_);
    printf("  翻页上按键:   GPIO%d\n", page_up_pin_);
    printf("  翻页下按键:   GPIO%d\n", page_down_pin_);
    printf("  防抖时间:     %lu ms\n", debounce_time_ms_);
    
    printf("[ButtonManager] 初始状态:\n");
    printf("  屏幕开关: %s\n", last_screen_power_state_ ? "按下" : "释放");
    printf("  翻页上:   %s\n", last_page_up_state_ ? "按下" : "释放");
    printf("  翻页下:   %s\n", last_page_down_state_ ? "按下" : "释放");
    
    printf("[ButtonManager] 按键管理器初始化完成\n");
    return true;
}

void ButtonManager::update() {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // 如果已有待处理事件，不处理新事件（避免事件丢失）
    if (pending_event_ != ButtonManagerEvent::NONE) {
        return;
    }
    
    // 读取当前按键状态
    bool current_screen_power = readButtonState(screen_power_pin_);
    bool current_page_up = readButtonState(page_up_pin_);
    bool current_page_down = readButtonState(page_down_pin_);
    
    // 检查屏幕开关按键（只支持短按）
    if (isButtonJustPressed(screen_power_pin_, current_screen_power, 
                           last_screen_power_state_, last_screen_power_time_)) {
        printf("[ButtonManager] 屏幕开关按键按下\n");
        pending_event_ = ButtonManagerEvent::SCREEN_POWER_TOGGLE;
        return; // 一次只处理一个事件
    }
    
    // 处理翻页上按键状态变化
    if (current_page_up && !last_page_up_state_) {
        // 按键刚按下
        page_up_press_start_time_ = current_time;
        page_up_long_press_handled_ = false;
        printf("[ButtonManager] 翻页上按键按下，开始计时\n");
    } else if (current_page_up && last_page_up_state_) {
        // 按键持续按下，检查长按
        uint32_t press_duration = current_time - page_up_press_start_time_;
        if (!page_up_long_press_handled_ && press_duration >= long_press_time_ms_) {
            printf("[ButtonManager] 翻页上按键长按触发 (持续时间: %lu ms)\n", press_duration);
            pending_event_ = ButtonManagerEvent::PAGE_UP_LONG;
            page_up_long_press_handled_ = true;
            last_page_up_state_ = current_page_up; // 更新状态
            return; // 长按事件触发，立即返回
        }
    } else if (!current_page_up && last_page_up_state_) {
        // 按键刚释放
        uint32_t total_press_duration = current_time - page_up_press_start_time_;
        if (!page_up_long_press_handled_) {
            // 短按：释放时触发
            printf("[ButtonManager] 翻页上按键短按触发 (持续时间: %lu ms)\n", total_press_duration);
            pending_event_ = ButtonManagerEvent::PAGE_UP;
            last_page_up_state_ = current_page_up; // 更新状态
            return; // 短按事件触发，立即返回
        } else {
            // 长按已处理，释放时只记录日志
            printf("[ButtonManager] 翻页上按键长按释放 (总持续时间: %lu ms)\n", total_press_duration);
        }
    }
    
    // 处理翻页下按键状态变化
    if (current_page_down && !last_page_down_state_) {
        // 按键刚按下
        page_down_press_start_time_ = current_time;
        page_down_long_press_handled_ = false;
        printf("[ButtonManager] 翻页下按键按下，开始计时\n");
    } else if (current_page_down && last_page_down_state_) {
        // 按键持续按下，检查长按
        uint32_t press_duration = current_time - page_down_press_start_time_;
        if (!page_down_long_press_handled_ && press_duration >= long_press_time_ms_) {
            printf("[ButtonManager] 翻页下按键长按触发 (持续时间: %lu ms)\n", press_duration);
            pending_event_ = ButtonManagerEvent::PAGE_DOWN_LONG;
            page_down_long_press_handled_ = true;
            last_page_down_state_ = current_page_down; // 更新状态
            return; // 长按事件触发，立即返回
        }
    } else if (!current_page_down && last_page_down_state_) {
        // 按键刚释放
        uint32_t total_press_duration = current_time - page_down_press_start_time_;
        if (!page_down_long_press_handled_) {
            // 短按：释放时触发
            printf("[ButtonManager] 翻页下按键短按触发 (持续时间: %lu ms)\n", total_press_duration);
            pending_event_ = ButtonManagerEvent::PAGE_DOWN;
            last_page_down_state_ = current_page_down; // 更新状态
            return; // 短按事件触发，立即返回
        } else {
            // 长按已处理，释放时只记录日志
            printf("[ButtonManager] 翻页下按键长按释放 (总持续时间: %lu ms)\n", total_press_duration);
        }
    }
    
    // 更新按键状态（只在没有事件触发时更新）
    last_page_up_state_ = current_page_up;
    last_page_down_state_ = current_page_down;
}

ButtonManagerEvent ButtonManager::getNextEvent() {
    ButtonManagerEvent event = pending_event_;
    pending_event_ = ButtonManagerEvent::NONE; // 清除事件
    return event;
}

bool ButtonManager::hasEvent() const {
    return pending_event_ != ButtonManagerEvent::NONE;
}

const char* ButtonManager::getDebugInfo() const {
    static char debug_buffer[256];
    snprintf(debug_buffer, sizeof(debug_buffer),
             "Screen: %s, PageUp: %s, PageDown: %s, Event: %d, LongPressThreshold: %lu ms",
             last_screen_power_state_ ? "按下" : "释放",
             last_page_up_state_ ? "按下" : "释放",
             last_page_down_state_ ? "按下" : "释放",
             static_cast<int>(pending_event_),
             long_press_time_ms_);
    return debug_buffer;
}

bool ButtonManager::readButtonState(uint8_t pin) {
    // 按键按下时GPIO为低电平（因为使用了上拉电阻）
    return !gpio_get(pin);
}

bool ButtonManager::isButtonJustPressed(uint8_t pin, bool current_state, 
                                       bool& last_state, uint32_t& last_time) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // 检查状态变化：从释放变为按下
    if (current_state && !last_state) {
        // 检查防抖时间
        if (current_time - last_time >= debounce_time_ms_) {
            last_state = current_state;
            last_time = current_time;
            return true;
        }
    } else {
        // 更新状态（但不触发事件）
        last_state = current_state;
        if (!current_state) {
            // 按键释放时更新时间，为下次按下做准备
            last_time = current_time;
        }
    }
    
    return false;
}

} // namespace input
} // namespace hardware