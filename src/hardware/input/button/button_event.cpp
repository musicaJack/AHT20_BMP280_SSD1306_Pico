#include "hardware/input/button/button_event.hpp"
#include "config/button_mapping_new.hpp"
#include "pico/stdlib.h"
#include <stdio.h>

namespace button {

ButtonEventHandler::ButtonEventHandler(ButtonController* controller, uint32_t long_press_ms, uint32_t double_press_ms)
    : controller_(controller),
      // 使用传入的参数，而不是硬编码值
      long_press_ms_(long_press_ms),
      double_press_ms_(double_press_ms),
      key1_press_time_(0), key2_press_time_(0), key1_last_release_time_(0), key2_last_release_time_(0),
      key1_long_press_handled_(false), key2_long_press_handled_(false),
      last_key1_pressed_(false), last_key2_pressed_(false),
      key1_event_(ButtonLogicEvent::NONE), key2_event_(ButtonLogicEvent::NONE)
{}

void ButtonEventHandler::update() {
    controller_->update();
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // KEY1
    bool key1_pressed = controller_->is_key1_pressed();
    
    // 打印KEY1状态变化（仅在调试开启时）
    #if BUTTON_DEBUG_ENABLED && BUTTON_DEBUG_LEVEL >= 2
    if (key1_pressed != last_key1_pressed_) {
        if (key1_pressed) {
            printf("[KEY1] 按下 - 时间: %lu ms\n", now);
        } else {
            printf("[KEY1] 释放 - 时间: %lu ms, 按下持续时间: %lu ms\n", 
                   now, key1_press_time_ ? (now - key1_press_time_) : 0);
        }
    }
    #endif
    
    // 禁用单击延迟判定逻辑
    
    if (key1_pressed) {
        if (!last_key1_pressed_) {
            key1_press_time_ = now;
            key1_event_ = ButtonLogicEvent::NONE; // 重置事件
            #if BUTTON_DEBUG_ENABLED && BUTTON_DEBUG_LEVEL >= 2
            printf("[KEY1] 开始计时 - 按下时间: %lu ms\n", now);
            #endif
        }
        
        uint32_t press_duration = now - key1_press_time_;
        if (!key1_long_press_handled_ && (press_duration >= NEW_BUTTON_LONG_PRESS_MS)) {
            key1_event_ = ButtonLogicEvent::LONG_PRESS;
            key1_long_press_handled_ = true;
            #if BUTTON_DEBUG_ENABLED
            printf("[KEY1] 判定为长按 - 持续时间: %lu ms (阈值: %lu ms)\n", press_duration, NEW_BUTTON_LONG_PRESS_MS);
            #endif
        } else if (key1_long_press_handled_) {
            // 长按已触发，保持事件状态
            key1_event_ = ButtonLogicEvent::LONG_PRESS;
        }
    } else {
        if (last_key1_pressed_) { // 刚释放
            if (!key1_long_press_handled_) {
                // 简化逻辑：短按立即触发
                key1_event_ = ButtonLogicEvent::SHORT_PRESS;
                #if BUTTON_DEBUG_ENABLED
                printf("[KEY1] 判定为短按\n");
                #endif
            } else {
                // 长按释放，清除长按状态
                #if BUTTON_DEBUG_ENABLED && BUTTON_DEBUG_LEVEL >= 2
                printf("[KEY1] 长按释放，清除长按状态\n");
                #endif
            }
            key1_long_press_handled_ = false;
        }
        key1_press_time_ = 0;
    }
    
    // KEY2
    bool key2_pressed = controller_->is_key2_pressed();
    
    // 打印KEY2状态变化（仅在调试开启时）
    #if BUTTON_DEBUG_ENABLED && BUTTON_DEBUG_LEVEL >= 2
    if (key2_pressed != last_key2_pressed_) {
        if (key2_pressed) {
            printf("[KEY2] 按下 - 时间: %lu ms\n", now);
        } else {
            printf("[KEY2] 释放 - 时间: %lu ms, 按下持续时间: %lu ms\n", 
                   now, key2_press_time_ ? (now - key2_press_time_) : 0);
        }
    }
    #endif
    
    // 禁用KEY2单击延迟判定逻辑
    
    if (key2_pressed) {
        if (!last_key2_pressed_) {
            key2_press_time_ = now;
            key2_event_ = ButtonLogicEvent::NONE; // 重置事件
            #if BUTTON_DEBUG_ENABLED && BUTTON_DEBUG_LEVEL >= 2
            printf("[KEY2] 开始计时 - 按下时间: %lu ms\n", now);
            #endif
        }
        
        uint32_t press_duration = now - key2_press_time_;
        if (!key2_long_press_handled_ && (press_duration >= NEW_BUTTON_LONG_PRESS_MS)) {
            key2_event_ = ButtonLogicEvent::LONG_PRESS;
            key2_long_press_handled_ = true;
            #if BUTTON_DEBUG_ENABLED
            printf("[KEY2] 判定为长按 - 持续时间: %lu ms (阈值: %lu ms)\n", press_duration, NEW_BUTTON_LONG_PRESS_MS);
            #endif
        } else if (key2_long_press_handled_) {
            // 长按已触发，保持事件状态
            key2_event_ = ButtonLogicEvent::LONG_PRESS;
        }
    } else {
        if (last_key2_pressed_) { // 刚释放
            if (!key2_long_press_handled_) {
                // 简化逻辑：短按立即触发
                key2_event_ = ButtonLogicEvent::SHORT_PRESS;
                #if BUTTON_DEBUG_ENABLED
                printf("[KEY2] 判定为短按\n");
                #endif
            } else {
                // 长按释放，清除长按状态
                #if BUTTON_DEBUG_ENABLED && BUTTON_DEBUG_LEVEL >= 2
                printf("[KEY2] 长按释放，清除长按状态\n");
                #endif
            }
            key2_long_press_handled_ = false;
        }
        key2_press_time_ = 0;
    }
    
    // 禁用组合键逻辑
    
    last_key1_pressed_ = key1_pressed;
    last_key2_pressed_ = key2_pressed;
}

ButtonLogicEvent ButtonEventHandler::get_key1_event() const {
    return key1_event_;
}
ButtonLogicEvent ButtonEventHandler::get_key2_event() const {
    return key2_event_;
}
// 禁用组合键事件获取
// ButtonLogicEvent ButtonEventHandler::get_combo_event() const {
//     return combo_event_;
// }
void ButtonEventHandler::reset() {
    // 重置事件状态
    key1_event_ = ButtonLogicEvent::NONE;
    key2_event_ = ButtonLogicEvent::NONE;
    // combo_event_ = ButtonLogicEvent::NONE; // 禁用组合键
    
    // 重置时间记录
    key1_press_time_ = 0;
    key2_press_time_ = 0;
    key1_last_release_time_ = 0;
    key2_last_release_time_ = 0;
    
    // 重置长按状态
    key1_long_press_handled_ = false;
    key2_long_press_handled_ = false;
    
    // 重置上一帧状态
    last_key1_pressed_ = false;
    last_key2_pressed_ = false;
    
    #if BUTTON_DEBUG_ENABLED && BUTTON_DEBUG_LEVEL >= 2
    printf("[BUTTON_EVENT] 所有状态已重置\n");
    #endif
}

} // namespace button 