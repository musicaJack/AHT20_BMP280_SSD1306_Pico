/**
 * @file ButtonSystemAdapter.hpp
 * @brief 按键系统适配器 - 将新按键系统集成到现有控制器
 * @version 1.0.0
 * 
 * 提供统一的接口，让现有的MenuController和TextReaderController
 * 可以无缝使用新的按键功能映射系统
 */

#ifndef BUTTON_SYSTEM_ADAPTER_HPP
#define BUTTON_SYSTEM_ADAPTER_HPP

#include "hardware/input/button/EnhancedButtonManager.hpp"
#include "hardware/input/joystick/joystick.hpp"
#include <functional>
#include <memory>

namespace hardware {
namespace input {

/**
 * @brief 统一的输入事件枚举
 */
enum class UnifiedInputEvent {
    NONE = 0,
    
    // 导航事件
    NAVIGATE_UP,         // 向上导航
    NAVIGATE_DOWN,       // 向下导航
    NAVIGATE_LEFT,       // 向左导航
    NAVIGATE_RIGHT,      // 向右导航
    CONFIRM,            // 确认选择
    CANCEL,             // 取消/返回
    HOME,               // 返回主页
    
    // 阅读事件
    PAGE_PREVIOUS,      // 上一页
    PAGE_NEXT,          // 下一页
    ENTER_MENU,         // 进入菜单
    
    // 系统事件
    TOGGLE_SCREEN,      // 屏幕开关
    ADJUST_BRIGHTNESS,  // 亮度调节
};

/**
 * @brief 按键系统适配器类
 * 
 * 统一管理摇杆和物理按键输入，根据应用模式
 * 提供合适的事件映射
 */
class ButtonSystemAdapter {
private:
    std::unique_ptr<EnhancedButtonManager> button_manager_;
    JoystickController* joystick_controller_;
    
    // 事件回调
    std::function<void(UnifiedInputEvent)> event_callback_;
    
    // 当前应用模式
    AppMode current_app_mode_;
    
    // 上次摇杆状态（用于检测状态变化）
    JoystickDirection last_joystick_direction_;
    ButtonState last_joystick_button_;
    
    // 防连发计时
    uint32_t last_event_time_;
    uint32_t repeat_delay_ms_;

public:
    /**
     * @brief 构造函数
     * @param joystick_controller 摇杆控制器指针
     */
    explicit ButtonSystemAdapter(JoystickController* joystick_controller = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~ButtonSystemAdapter() = default;
    
    /**
     * @brief 初始化适配器
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
    AppMode get_app_mode() const { return current_app_mode_; }
    
    /**
     * @brief 设置事件回调函数
     * @param callback 回调函数
     */
    void set_event_callback(std::function<void(UnifiedInputEvent)> callback);
    
    /**
     * @brief 更新输入状态（需要在主循环中调用）
     */
    void update();
    
    /**
     * @brief 设置防连发延迟时间
     * @param ms 延迟时间（毫秒）
     */
    void set_repeat_delay(uint32_t ms) { repeat_delay_ms_ = ms; }
    
    /**
     * @brief 获取增强按键管理器
     * @return 按键管理器指针
     */
    EnhancedButtonManager* get_button_manager() { return button_manager_.get(); }
    
    /**
     * @brief 获取摇杆控制器
     * @return 摇杆控制器指针
     */
    JoystickController* get_joystick_controller() { return joystick_controller_; }

private:
    /**
     * @brief 处理增强按键事件
     * @param button_event 按键事件
     */
    void handle_button_event(EnhancedButtonEvent button_event);
    
    /**
     * @brief 处理摇杆事件
     * @param direction 摇杆方向
     * @param button_state 摇杆按钮状态
     */
    void handle_joystick_event(JoystickDirection direction, ButtonState button_state);
    
    /**
     * @brief 将增强按键事件转换为统一输入事件
     * @param button_event 按键事件
     * @return 统一输入事件
     */
    UnifiedInputEvent button_event_to_unified(EnhancedButtonEvent button_event);
    
    /**
     * @brief 将摇杆事件转换为统一输入事件
     * @param direction 摇杆方向
     * @param button_state 摇杆按钮状态
     * @return 统一输入事件
     */
    UnifiedInputEvent joystick_event_to_unified(JoystickDirection direction, ButtonState button_state);
    
    /**
     * @brief 触发统一输入事件
     * @param event 要触发的事件
     */
    void trigger_unified_event(UnifiedInputEvent event);
    
    /**
     * @brief 检查是否可以触发事件（防连发）
     * @return true 可以触发，false 需要等待
     */
    bool can_trigger_event();
};

// === 便利函数 ===

/**
 * @brief 获取统一输入事件的字符串描述
 * @param event 输入事件
 * @return 事件描述字符串
 */
const char* get_unified_event_name(UnifiedInputEvent event);

/**
 * @brief 获取应用模式的字符串描述
 * @param mode 应用模式
 * @return 模式描述字符串
 */
const char* get_app_mode_name(AppMode mode);

} // namespace input
} // namespace hardware

#endif // BUTTON_SYSTEM_ADAPTER_HPP