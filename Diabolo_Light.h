#ifndef DIABOLO_LIGHT_H
#define DIABOLO_LIGHT_H

namespace Diabolo_Light {
    const int BUTTON_PIN = 2;
    const int LED_PIN = 1;
    const int MOSFET_PIN = 0;
    const int NUM_LEDS = 6;
    typedef void (*func_ptr)();

    void begin(const int num_modes, func_ptr on_wake_up = [](){});
    void handle_button();
    int get_current_mode();
    void set_current_mode(const int new_mode);
}

#endif