#ifndef DIABOLO_LIGHT_H
#define DIABOLO_LIGHT_H

namespace Diabolo_Light {
    typedef void (*func_ptr)();

    const int BUTTON_PIN = 2;
    const int LED_PIN = 1;
    const int MOSFET_PIN = 0;
    const int NUM_LEDS = 6;

    void begin(const int num_modes, const int hold_time = 500, func_ptr on_wake_up = [](){});
    void handle_button();
    int get_current_mode();
    void set_current_mode(const int new_mode);
    unsigned long get_wake_up_time();
}

#endif
