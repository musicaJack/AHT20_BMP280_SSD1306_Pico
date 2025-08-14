/**
 * @file ButtonSystemAdapter.cpp
 * @brief 按键系统适配器实现
 * @version 1.0.0
 */

#include "hardware/input/button/ButtonSystemAdapter.hpp"
#include <cstdio>

namespace hardware {
namespace input {

ButtonSystemAdapter::ButtonSystemAdapter(JoystickController* joystick_controller)
    : button_manager_(std::make_unique<EnhancedButtonManager>()),
      joystick_controller_(joystick_controller),
      current_app_mode_(AppMode::MAIN_MENU),
      last_joystick_direction_(JoystickDirection::NONE),
      last_joystick_button_(ButtonState::RELEASED),
      last_event_time_(0),
      repeat_delay_ms_(200) { // 默认200ms防连发
}

bool ButtonSystemAdapter::initialize() {
    printf("[ButtonSystemAdapter] 初始化按键系统适配器\n");
    
    // 初始化增强按键管理器
    if (!button_manager_->initialize()) {
        printf("[ButtonSystemAdapter] 增强按键管理器初始化失败\n");
        return false;
    }
    
    // 设置按键事件回调
    button_manager_->set_event_callback([this](EnhancedButtonEvent event) {
        handle_button_event(event);
    });
    
    // 设置初始应用模式
    button_manager_->set_app_mode(current_app_mode_);
    
    printf("[ButtonSystemAdapter] 初始化完成\n");
    printf("  摇杆控制器: %s\n", joystick_controller_ ? "已连接" : "未连接");
    printf("  防连发延迟: %lu ms\n", repeat_delay_ms_);
    
    return true;
}

void ButtonSystemAdapter::set_app_mode(AppMode mode) {
    if (current_app_mode_ != mode) {
        printf("[ButtonSystemAdapter] 切换应用模式: %s -> %s\n", 
               get_app_mode_name(current_app_mode_), get_app_mode_name(mode));
        
        current_app_mode_ = mode;
        button_manager_->set_app_mode(mode);
        
        // 重置状态，避免模式切换时的误操作
        last_joystick_direction_ = JoystickDirection::NONE;
        last_joystick_button_ = ButtonState::RELEASED;
        last_event_time_ = 0;
    }
}

void ButtonSystemAdapter::set_event_callback(std::function<void(UnifiedInputEvent)> callback) {
    event_callback_ = callback;
}

void ButtonSystemAdapter::update() {
    // 更新增强按键管理器
    button_manager_->update();
    
    // 更新摇杆状态（如果可用）
    if (joystick_controller_) {
        joystick_controller_->update();
        
        JoystickDirection current_direction = joystick_controller_->getCurrentDirection();
        ButtonState current_button = joystick_controller_->getButtonState();
        
        // 检查摇杆状态变化
        if (current_direction != last_joystick_direction_ || 
            current_button != last_joystick_button_) {
            
            handle_joystick_event(current_direction, current_button);
            
            last_joystick_direction_ = current_direction;
            last_joystick_button_ = current_button;
        }
    }
}

void ButtonSystemAdapter::handle_button_event(EnhancedButtonEvent button_event) {
    if (button_event == EnhancedButtonEvent::NONE) {
        return;
    }
    
    UnifiedInputEvent unified_event = button_event_to_unified(button_event);
    trigger_unified_event(unified_event);
}

void ButtonSystemAdapter::handle_joystick_event(JoystickDirection direction, ButtonState button_state) {
    // 处理摇杆方向
    if (direction != JoystickDirection::NONE && direction != last_joystick_direction_) {
        UnifiedInputEvent unified_event = joystick_event_to_unified(direction, ButtonState::RELEASED);
        trigger_unified_event(unified_event);
    }
    
    // 处理摇杆按钮
    if (button_state == ButtonState::PRESSED && last_joystick_button_ == ButtonState::RELEASED) {
        // 摇杆按钮刚按下
        UnifiedInputEvent unified_event = joystick_event_to_unified(JoystickDirection::NONE, button_state);
        trigger_unified_event(unified_event);
    }
}

UnifiedInputEvent ButtonSystemAdapter::button_event_to_unified(EnhancedButtonEvent button_event) {
    switch (button_event) {
        case EnhancedButtonEvent::NAV_UP:
            return UnifiedInputEvent::NAVIGATE_UP;
        case EnhancedButtonEvent::NAV_DOWN:
            return UnifiedInputEvent::NAVIGATE_DOWN;
        case EnhancedButtonEvent::NAV_SELECT:
            return UnifiedInputEvent::CONFIRM;
        case EnhancedButtonEvent::NAV_BACK:
            return UnifiedInputEvent::CANCEL;
        case EnhancedButtonEvent::NAV_HOME:
            return UnifiedInputEvent::HOME;
        case EnhancedButtonEvent::PAGE_UP:
            return UnifiedInputEvent::PAGE_PREVIOUS;
        case EnhancedButtonEvent::PAGE_DOWN:
            return UnifiedInputEvent::PAGE_NEXT;
        case EnhancedButtonEvent::SCREEN_TOGGLE:
            return UnifiedInputEvent::TOGGLE_SCREEN;
        case EnhancedButtonEvent::MENU_ENTER:
            return UnifiedInputEvent::ENTER_MENU;
        case EnhancedButtonEvent::BRIGHTNESS_ADJUST:
            return UnifiedInputEvent::ADJUST_BRIGHTNESS;
        default:
            return UnifiedInputEvent::NONE;
    }
}

UnifiedInputEvent ButtonSystemAdapter::joystick_event_to_unified(JoystickDirection direction, ButtonState button_state) {
    // 处理摇杆方向
    switch (direction) {
        case JoystickDirection::UP:
            return (current_app_mode_ == AppMode::CONTENT_READING) ? 
                   UnifiedInputEvent::PAGE_PREVIOUS : UnifiedInputEvent::NAVIGATE_UP;
        case JoystickDirection::DOWN:
            return (current_app_mode_ == AppMode::CONTENT_READING) ? 
                   UnifiedInputEvent::PAGE_NEXT : UnifiedInputEvent::NAVIGATE_DOWN;
        case JoystickDirection::LEFT:
            return UnifiedInputEvent::NAVIGATE_LEFT;
        case JoystickDirection::RIGHT:
            return UnifiedInputEvent::NAVIGATE_RIGHT;
        default:
            break;
    }
    
    // 处理摇杆按钮
    if (button_state == ButtonState::PRESSED) {
        switch (current_app_mode_) {
            case AppMode::CONTENT_READING:
                return UnifiedInputEvent::ENTER_MENU; // 在阅读模式下，摇杆按钮进入菜单
            case AppMode::MAIN_MENU:
            case AppMode::FILE_LIST:
            case AppMode::SYSTEM_CONFIG:
            case AppMode::SUB_MENU:
                return UnifiedInputEvent::CONFIRM; // 在菜单模式下，摇杆按钮确认选择
        }
    }
    
    return UnifiedInputEvent::NONE;
}

void ButtonSystemAdapter::trigger_unified_event(UnifiedInputEvent event) {
    if (event == UnifiedInputEvent::NONE) {
        return;
    }
    
    // 防连发检查
    if (!can_trigger_event()) {
        return;
    }
    
    // 更新最后事件时间
    last_event_time_ = to_ms_since_boot(get_absolute_time());
    
    #if BUTTON_DEBUG_ENABLED
    printf("[ButtonSystemAdapter] 触发统一事件: %s (模式: %s)\n", 
           get_unified_event_name(event), get_app_mode_name(current_app_mode_));
    #endif
    
    // 调用回调函数
    if (event_callback_) {
        event_callback_(event);
    }
}

bool ButtonSystemAdapter::can_trigger_event() {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    return (current_time - last_event_time_) >= repeat_delay_ms_;
}

// === 便利函数实现 ===

const char* get_unified_event_name(UnifiedInputEvent event) {
    switch (event) {
        case UnifiedInputEvent::NONE: return "NONE";
        case UnifiedInputEvent::NAVIGATE_UP: return "NAVIGATE_UP";
        case UnifiedInputEvent::NAVIGATE_DOWN: return "NAVIGATE_DOWN";
        case UnifiedInputEvent::NAVIGATE_LEFT: return "NAVIGATE_LEFT";
        case UnifiedInputEvent::NAVIGATE_RIGHT: return "NAVIGATE_RIGHT";
        case UnifiedInputEvent::CONFIRM: return "CONFIRM";
        case UnifiedInputEvent::CANCEL: return "CANCEL";
        case UnifiedInputEvent::HOME: return "HOME";
        case UnifiedInputEvent::PAGE_PREVIOUS: return "PAGE_PREVIOUS";
        case UnifiedInputEvent::PAGE_NEXT: return "PAGE_NEXT";
        case UnifiedInputEvent::ENTER_MENU: return "ENTER_MENU";
        case UnifiedInputEvent::TOGGLE_SCREEN: return "TOGGLE_SCREEN";
        case UnifiedInputEvent::ADJUST_BRIGHTNESS: return "ADJUST_BRIGHTNESS";
        default: return "UNKNOWN";
    }
}

const char* get_app_mode_name(AppMode mode) {
    switch (mode) {
        case AppMode::CONTENT_READING: return "CONTENT_READING";
        case AppMode::MAIN_MENU: return "MAIN_MENU";
        case AppMode::FILE_LIST: return "FILE_LIST";
        case AppMode::SYSTEM_CONFIG: return "SYSTEM_CONFIG";
        case AppMode::SUB_MENU: return "SUB_MENU";
        default: return "UNKNOWN";
    }
}

} // namespace input
} // namespace hardware