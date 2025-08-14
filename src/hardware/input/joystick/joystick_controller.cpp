#include "hardware/input/joystick/joystick_controller.hpp"
#include "config/UserConfigManager_ILI9488.hpp"
#include <cmath>

namespace hardware {
namespace input {

JoystickController::JoystickController(Joystick& joystick)
    : joystick_(joystick)
    , threshold_(1800)
    , stable_count_required_(3)
    , previous_direction_(0)
    , stable_count_(0)
    , last_reported_direction_(0)
    , last_button_pressed_(false)
    , direction_callback_(nullptr)
    , button_callback_(nullptr)
    , led_enabled_(true) {
}

bool JoystickController::initialize() {
    // 初始化状态变量
    previous_direction_ = 0;
    stable_count_ = 0;
    last_reported_direction_ = 0;
    last_button_pressed_ = false;
    
    // 注意：LED设置将在软件初始化阶段从用户配置管理器获取
    // 这里暂时使用默认值，避免在硬件初始化阶段读取未初始化的配置
    led_enabled_ = true; // 默认开启，将在软件初始化阶段重新设置
    
    printf("[JOYSTICK_CONTROLLER] 硬件初始化完成，LED设置将在软件初始化阶段应用\n");
    
    return true;
}

void JoystickController::update() {
    // 获取ADC偏移值
    int16_t offset_x = joystick_.get_joy_adc_12bits_offset_value_x();
    int16_t offset_y = joystick_.get_joy_adc_12bits_offset_value_y();
    
    // 判断当前方向
    JoystickDirection current_direction = determineDirection(offset_x, offset_y);
    
    // 稳定性检查
    if (static_cast<int>(current_direction) == previous_direction_) {
        stable_count_++;
    } else {
        stable_count_ = 0;
        previous_direction_ = static_cast<int>(current_direction);
    }
    
    // 处理方向变化
    if (stable_count_ >= stable_count_required_ && 
        current_direction != JoystickDirection::NONE && 
        static_cast<int>(current_direction) != last_reported_direction_) {
        
        handleDirectionChange(current_direction);
        last_reported_direction_ = static_cast<int>(current_direction);
    } else if (current_direction == JoystickDirection::NONE && 
               last_reported_direction_ != 0) {
        // 回到中心位置时静默处理
        updateLEDColor(0x000000); // 关闭LED
        last_reported_direction_ = 0;
    } else if (current_direction == JoystickDirection::NONE) {
        updateLEDColor(0x000000); // 关闭LED
    }
    
    // 处理按钮事件
    uint8_t button_value = joystick_.get_button_value();
    handleButtonEvent(button_value);
}

void JoystickController::setDirectionCallback(DirectionCallback callback) {
    direction_callback_ = callback;
}

void JoystickController::setButtonCallback(ButtonCallback callback) {
    button_callback_ = callback;
}

JoystickDirection JoystickController::getCurrentDirection() const {
    return static_cast<JoystickDirection>(last_reported_direction_);
}

ButtonState JoystickController::getButtonState() const {
    return last_button_pressed_ ? ButtonState::PRESSED : ButtonState::RELEASED;
}

void JoystickController::setLEDColor(uint32_t color) {
    // 注意：此方法会绕过LED开关控制，直接设置LED颜色
    // 建议使用updateLEDColor方法来确保遵守LED开关设置
    joystick_.set_rgb_color(color);
}

void JoystickController::setLEDEnabled(bool enabled) {
    led_enabled_ = enabled;
    
    if (!enabled) {
        // 如果禁用LED，立即关闭
        joystick_.set_rgb_color(0x000000);
    }
    
    printf("[JOYSTICK_CONTROLLER] LED %s (led_enabled_ = %s)\n", enabled ? "启用" : "禁用", enabled ? "true" : "false");
}

bool JoystickController::isLEDEnabled() const {
    return led_enabled_;
}

void JoystickController::updateLEDColor(uint32_t color) {
    static uint32_t last_color = 0xFFFFFFFF; // 初始化为一个不可能的颜色值
    
    if (led_enabled_) {
        // 只有当颜色发生变化时才打印日志
        if (color != last_color) {
            printf("[JOYSTICK_CONTROLLER] LED启用，设置颜色: 0x%06X\n", color);
            last_color = color;
        }
        joystick_.set_rgb_color(color);
    } else {
        // 如果LED被禁用，确保LED关闭
        if (last_color != 0x000000) {
            printf("[JOYSTICK_CONTROLLER] LED禁用，强制关闭LED\n");
            last_color = 0x000000;
        }
        joystick_.set_rgb_color(0x000000);
    }
}

void JoystickController::setThreshold(uint16_t threshold) {
    threshold_ = threshold;
}

void JoystickController::setStableCount(uint8_t count) {
    stable_count_required_ = count;
}

Joystick& JoystickController::getJoystick() {
    return joystick_;
}

JoystickDirection JoystickController::determineDirection(int16_t offset_x, int16_t offset_y) {
    int16_t abs_x = abs(offset_x);
    int16_t abs_y = abs(offset_y);
    
    // 检查是否超过阈值
    if (abs_x < threshold_ && abs_y < threshold_) {
        return JoystickDirection::NONE;
    }
    
    // 检查死区（阈值的80%）
    if (abs_x < threshold_ * 0.8f && abs_y < threshold_ * 0.8f) {
        return JoystickDirection::NONE;
    }
    
    // 判断主要方向
    if (abs_y > abs_x * 1.5f) {
        return (offset_y < 0) ? JoystickDirection::UP : JoystickDirection::DOWN;
    }
    
    if (abs_x > abs_y * 1.5f) {
        return (offset_x < 0) ? JoystickDirection::LEFT : JoystickDirection::RIGHT;
    }
    
    return JoystickDirection::NONE;
}

void JoystickController::handleDirectionChange(JoystickDirection new_direction) {
    // 设置LED颜色（蓝色表示方向变化）
    updateLEDColor(0x0000FF);
    
    // 输出调试信息
    switch (new_direction) {
        case JoystickDirection::UP:
            printf("[JOYSTICK] Direction: UP\n");
            break;
        case JoystickDirection::DOWN:
            printf("[JOYSTICK] Direction: DOWN\n");
            break;
        case JoystickDirection::LEFT:
            printf("[JOYSTICK] Direction: LEFT\n");
            break;
        case JoystickDirection::RIGHT:
            printf("[JOYSTICK] Direction: RIGHT\n");
            break;
        default:
            break;
    }
    
    // 调用回调函数
    if (direction_callback_) {
        direction_callback_(new_direction);
    }
}

void JoystickController::handleButtonEvent(uint8_t button_value) {
    bool current_button_pressed = (button_value == 0);
    
    if (current_button_pressed && !last_button_pressed_) {
        // 按钮按下
        printf("[JOYSTICK] Button pressed\n");
        updateLEDColor(0xFF0000); // 红色LED
        
        if (button_callback_) {
            button_callback_(ButtonState::PRESSED);
        }
        
        last_button_pressed_ = true;
    } else if (!current_button_pressed && last_button_pressed_) {
        // 按钮释放
        printf("[JOYSTICK] Button released\n");
        updateLEDColor(0x000000); // 关闭LED
        
        if (button_callback_) {
            button_callback_(ButtonState::RELEASED);
        }
        
        last_button_pressed_ = false;
    }
}

} // namespace input
} // namespace hardware 