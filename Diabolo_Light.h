#ifndef DIABOLO_LIGHT_H
#define DIABOLO_LIGHT_H

#include <Arduino.h>

namespace Diabolo_Light {
    typedef void (*func_ptr)();

    const uint8_t BUTTON_PIN = 2;
    const uint8_t LED_PIN = 1;
    const uint8_t MOSFET_PIN = 0;
    const uint8_t NUM_LEDS = 6;

    void begin(const unsigned int num_modes, const unsigned int hold_time = 500, func_ptr on_wake_up = [](){});
    void handle_button();
    unsigned int get_current_mode();
    void set_current_mode(const unsigned int new_mode);
    unsigned long get_wake_up_time();
    int get_button_state();
}

#endif
