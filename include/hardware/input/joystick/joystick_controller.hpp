#pragma once

#include "joystick.hpp"
#include "pico/stdlib.h"
#include <functional>

namespace hardware {
namespace input {

// 摇杆方向枚举
enum class JoystickDirection {
    NONE = 0,
    UP = 1,
    DOWN = 2,
    LEFT = 3,
    RIGHT = 4
};

// 按钮状态枚举
enum class ButtonState {
    RELEASED,
    PRESSED
};

// 摇杆事件回调函数类型
using DirectionCallback = std::function<void(JoystickDirection)>;
using ButtonCallback = std::function<void(ButtonState)>;

/**
 * @brief 摇杆控制器类
 * @description 封装摇杆的处理逻辑，提供简单的事件驱动接口
 */
class JoystickController {
public:
    /**
     * @brief 构造函数
     * @param joystick 摇杆实例的引用
     */
    JoystickController(Joystick& joystick);
    
    /**
     * @brief 析构函数
     */
    ~JoystickController() = default;
    
    /**
     * @brief 初始化摇杆控制器
     * @return true 成功，false 失败
     */
    bool initialize();
    
    /**
     * @brief 更新摇杆状态（需要在主循环中调用）
     */
    void update();
    
    /**
     * @brief 设置方向变化回调函数
     * @param callback 回调函数
     */
    void setDirectionCallback(DirectionCallback callback);
    
    /**
     * @brief 设置按钮事件回调函数
     * @param callback 回调函数
     */
    void setButtonCallback(ButtonCallback callback);
    
    /**
     * @brief 获取当前稳定方向
     * @return 当前方向
     */
    JoystickDirection getCurrentDirection() const;
    
    /**
     * @brief 获取当前按钮状态
     * @return 按钮状态
     */
    ButtonState getButtonState() const;
    
    /**
     * @brief 设置LED颜色
     * @param color RGB颜色值
     */
    void setLEDColor(uint32_t color);
    
    // LED控制相关方法
    void setLEDEnabled(bool enabled);
    bool isLEDEnabled() const;
    void updateLEDColor(uint32_t color); // 根据LED开关状态更新颜色
    
    /**
     * @brief 设置方向阈值
     * @param threshold 阈值（默认1800）
     */
    void setThreshold(uint16_t threshold);
    
    /**
     * @brief 设置稳定计数要求
     * @param count 稳定计数（默认3）
     */
    void setStableCount(uint8_t count);
    
    /**
     * @brief 获取底层摇杆对象引用
     * @return 摇杆对象引用
     */
    Joystick& getJoystick();

private:
    Joystick& joystick_;
    
    // 配置参数
    uint16_t threshold_;
    uint8_t stable_count_required_;
    
    // 状态变量
    int previous_direction_;
    uint8_t stable_count_;
    int last_reported_direction_;
    bool last_button_pressed_;
    
    // 回调函数
    DirectionCallback direction_callback_;
    ButtonCallback button_callback_;
    
    // LED控制相关
    bool led_enabled_;
    
    /**
     * @brief 判断摇杆方向
     * @param offset_x X轴偏移值
     * @param offset_y Y轴偏移值
     * @return 方向枚举
     */
    JoystickDirection determineDirection(int16_t offset_x, int16_t offset_y);
    
    /**
     * @brief 处理方向变化
     * @param new_direction 新方向
     */
    void handleDirectionChange(JoystickDirection new_direction);
    
    /**
     * @brief 处理按钮事件
     * @param button_value 按钮值
     */
    void handleButtonEvent(uint8_t button_value);
};

} // namespace input
} // namespace hardware 