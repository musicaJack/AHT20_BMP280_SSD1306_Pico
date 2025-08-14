#ifndef _JOYSTICK_DEBOUNCER_H_
#define _JOYSTICK_DEBOUNCER_H_

#include <pico/stdlib.h>
#include "hardware/input/joystick/joystick.hpp"

// 注意：JoystickDebounceConfig, JoystickDirection, ButtonState 已在 joystick.hpp 中定义

/**
 * @brief Joystick debouncer class
 */
class JoystickDebouncer {
public:
    /**
     * @brief Constructor
     */
    JoystickDebouncer();

    /**
     * @brief Update joystick state
     * @param joystick joystick instance
     */
    void update(Joystick* joystick);

    /**
     * @brief Check if button event occurred
     * @return true if button event occurred
     */
    bool has_button_event() const;

    /**
     * @brief Get button state
     * @return ButtonState
     */
    ButtonState get_button_state() const;

    /**
     * @brief Check if direction changed
     * @return true if direction changed
     */
    bool has_direction_changed() const;

    /**
     * @brief Get stable direction
     * @return JoystickDirection
     */
    JoystickDirection get_stable_direction() const;

    /**
     * @brief Set debounce configuration
     * @param config debounce configuration
     */
    void set_debounce_config(const JoystickDebounceConfig& config);

private:
    JoystickDebounceConfig _config;
    JoystickDirection _current_direction;
    JoystickDirection _last_direction;
    ButtonState _current_button_state;
    ButtonState _last_button_state;
    uint8_t _direction_stable_count;
    uint32_t _last_button_time;
    bool _button_event_occurred;
    bool _direction_changed;

    /**
     * @brief Determine direction from ADC values
     * @param x_adc x-axis ADC value
     * @param y_adc y-axis ADC value
     * @return JoystickDirection
     */
    JoystickDirection determine_direction(uint16_t x_adc, uint16_t y_adc);
};

#endif 