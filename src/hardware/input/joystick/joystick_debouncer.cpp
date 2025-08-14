#include "hardware/input/joystick/joystick_debouncer.hpp"
#include <stdio.h>
#include <cstdlib>

JoystickDebouncer::JoystickDebouncer() {
    // 默认配置
    _config.threshold = 1800;
    _config.deadzone = 1000;
    _config.stable_count_required = 3;
    _config.button_debounce_ms = 200;
    _config.direction_ratio = 1.5f;
    
    _current_direction = JoystickDirection::NONE;
    _last_direction = JoystickDirection::NONE;
    _current_button_state = ButtonState::RELEASED;
    _last_button_state = ButtonState::RELEASED;
    _direction_stable_count = 0;
    _last_button_time = 0;
    _button_event_occurred = false;
    _direction_changed = false;
}

void JoystickDebouncer::update(Joystick* joystick) {
    if (!joystick) return;
    
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // 处理按钮防抖
    uint8_t button_value = joystick->get_button_value();
    ButtonState new_button_state = (button_value == 0) ? ButtonState::PRESSED : ButtonState::RELEASED;
    
    if (new_button_state != _current_button_state) {
        if (current_time - _last_button_time > _config.button_debounce_ms) {
            _last_button_state = _current_button_state;
            _current_button_state = new_button_state;
            _button_event_occurred = true;
            _last_button_time = current_time;
        }
    } else {
        _button_event_occurred = false;
    }
    
    // 处理方向防抖
    uint16_t x_adc, y_adc;
    joystick->get_joy_adc_16bits_value_xy(&x_adc, &y_adc);
    
    JoystickDirection new_direction = determine_direction(x_adc, y_adc);
    
    if (new_direction != _current_direction) {
        if (new_direction == _last_direction) {
            _direction_stable_count++;
            if (_direction_stable_count >= _config.stable_count_required) {
                _current_direction = new_direction;
                _direction_changed = true;
            }
        } else {
            _direction_stable_count = 1;
            _last_direction = new_direction;
        }
    } else {
        _direction_stable_count = 0;
        _direction_changed = false;
    }
}

bool JoystickDebouncer::has_button_event() const {
    return _button_event_occurred;
}

ButtonState JoystickDebouncer::get_button_state() const {
    return _current_button_state;
}

bool JoystickDebouncer::has_direction_changed() const {
    return _direction_changed;
}

JoystickDirection JoystickDebouncer::get_stable_direction() const {
    return _current_direction;
}

void JoystickDebouncer::set_debounce_config(const JoystickDebounceConfig& config) {
    _config = config;
}

JoystickDirection JoystickDebouncer::determine_direction(uint16_t x_adc, uint16_t y_adc) {
    // 计算中心点（假设12位ADC，中心在2048）
    const uint16_t center = 2048;
    const uint16_t threshold = _config.threshold;
    const uint16_t deadzone = _config.deadzone;
    
    // 计算偏移量
    int16_t x_offset = (int16_t)x_adc - center;
    int16_t y_offset = (int16_t)y_adc - center;
    
    // 检查是否在死区内
    if (abs(x_offset) < deadzone && abs(y_offset) < deadzone) {
        return JoystickDirection::NONE;
    }
    
    // 检查是否超过阈值
    if (abs(x_offset) < threshold && abs(y_offset) < threshold) {
        return JoystickDirection::NONE;
    }
    
    // 确定主要方向
    if (abs(x_offset) > abs(y_offset) * _config.direction_ratio) {
        // 水平方向为主
        return (x_offset > 0) ? JoystickDirection::RIGHT : JoystickDirection::LEFT;
    } else {
        // 垂直方向为主
        return (y_offset > 0) ? JoystickDirection::DOWN : JoystickDirection::UP;
    }
} 