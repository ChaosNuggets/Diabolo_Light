#include <Arduino.h>
#include <avr/sleep.h>
#include "Diabolo_Light.h"

using namespace Diabolo_Light;

const unsigned int BUTTON_PIN = 2;
const unsigned int MOSFET_PIN = 0;

// All time variables in this file have units of ms
const static unsigned int DEBOUNCE_DELAY = 50; // the time the button has to be stable before button_state updates
static unsigned long last_debounce_time; // the most recent time that the button was unstable
static int prev_reading; // the previous result of digitalRead(BUTTON_PIN)

// HIGH means the button is pressed, and LOW means the button is released
static volatile int button_state;

static unsigned int num_modes;
static unsigned int current_mode; // 0 is the off mode, 1-num_modes inclusive are user defined modes

static void (*on_wake_up)();
static volatile unsigned long wake_up_time;
static volatile unsigned long holding_start_time; // The time at which the user starts holding the button

// "Constants" initialized in begin() that specify the time the user has to hold the button to turn the lights on/off.
static unsigned long time_to_turn_on;
static unsigned long time_to_turn_off;

// If this is true, the user needs to hold the button for time_to_turn_on ms for the mode to increment to 1
static volatile bool has_just_woken_up;

/*!
    @brief   Shut down the ATtiny85 and disconnect the LEDs to save power
*/
static void shut_down() {
    digitalWrite(MOSFET_PIN, HIGH); // Disconnect the LEDs
    GIMSK |= 1 << PCIE; // enable pin change interrupt
    PCMSK |= 1 << PCINT2; // turns on PCINT2 as interrupt pin
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable(); //set sleep enable bit
    sei(); // enable interrupts
    sleep_mode(); // put the microcontroller to sleep
}

/*!
    @brief   This function runs when the board is woken up from sleep mode.
             It does some button stuff and some power stuff to turn the
             board back on.
*/
ISR(PCINT0_vect) {
    wake_up_time = millis();
    holding_start_time = millis();
    has_just_woken_up = true;
    // Set button_state to HIGH because specific actions only occur when button_state changes, and we want to
    // do "nothing" if button_state changes to high, and shut down the board if button_state changes to low.
    // Yes we want to connect the LEDs when the button is held for enough time, but that's handled in a
    // separate block of code.
    button_state = HIGH;

    cli(); // disable interrupts
    PCMSK &= ~(1 << PCINT2); // turns off PCINT2 as interrupt pin
    sleep_disable(); // clear sleep enable bit

    on_wake_up();
}

/*!
    @brief   Configure the diabolo light to read button input and save power.
             Call this in the setup function.
    @param   num_modes  the number of modes the board should have
             not including the off mode
    @param   time_to_turn_on  the amount of time in milliseconds the user has to
             hold the button in order for the board to turn on. Defaults to
             500ms.
    @param   on_wake_up  additional things the board should do when the button
             is pressed to wake up the board. Defaults to doing nothing.
    @param   time_to_turn_off  the amount of time in milliseconds the user has to
             hold the button in order for the board to turn off. Defaults to
             2000ms.
*/
void Diabolo_Light::begin(const unsigned int num_modes, const unsigned long time_to_turn_on, void (*on_wake_up)(), const unsigned long time_to_turn_off) {
    ::num_modes = num_modes;
    ::time_to_turn_on = time_to_turn_on;
    ::on_wake_up = on_wake_up;
    ::time_to_turn_off = time_to_turn_off;

    ADCSRA &= ~(1 << ADEN); // Disable ADC
    ACSR &= ~(1 << ACIE); // Disable analog comparator interrupt
    ACSR |= 1 << ACD; // Disable analog comparator

    pinMode(BUTTON_PIN, INPUT);
    // Set button_state to HIGH because actions only occur when button_state changes, and we
    // want to do nothing if button_state is high, and shut down the board if button_state is low.
    button_state = HIGH;
    last_debounce_time = millis(); // Set this to the current time to allow the button to debounce if it is currently unstable.
    has_just_woken_up = false; // Set it to false because we want the board to sleep right away

    pinMode(MOSFET_PIN, OUTPUT);
    set_current_mode(0); // Set the board to shut down
}

/*!
    @brief   Read button input and change current_mode if necessary.
             Call this at the start of your loop function. Make sure
             your code is non blocking or else current_mode won't update.
*/
void Diabolo_Light::handle_button() {
    const int reading = digitalRead(BUTTON_PIN);
    // If the button is unstable, reset the timer for how long the button has been stable for
    if (reading != prev_reading) {
        prev_reading = reading;
        last_debounce_time = millis();
    }

    // Runs on button state change
    if ((millis() - last_debounce_time) > DEBOUNCE_DELAY && reading != button_state) {
        button_state = reading;

        if (button_state == HIGH) {
            holding_start_time = millis();
            set_current_mode(current_mode >= num_modes ? 0 : current_mode + 1);
        }

        if (current_mode == 0 && button_state == LOW) {
            shut_down();
        }
    }

    // Connect the LEDs if the user has held down the button for long enough
    if (has_just_woken_up && button_state == HIGH && millis() - holding_start_time >= time_to_turn_on) {
        has_just_woken_up = false;
        set_current_mode(current_mode >= num_modes ? 0 : current_mode + 1);
        digitalWrite(MOSFET_PIN, LOW); // Connect the LEDs
    }
    
    // Ty Victor Lin for this idea (shut down on long press)
    if (button_state == HIGH && millis() - holding_start_time >= time_to_turn_off) {
        set_current_mode(0);
    }
}

/*!
    @brief   Getter for current_mode. 0 is the off mode,
             and 1-num_modes inclusive are user defined modes.
    @return  Current mode
*/
unsigned int Diabolo_Light::get_current_mode() {
    return current_mode;
}

/*!
    @brief   Setter for current_mode. 0 is the off mode,
             and 1-num_modes inclusive are user defined modes.
    @param   new_mode  the number to set current_mode to
*/
void Diabolo_Light::set_current_mode(const unsigned int new_mode) {
    current_mode = new_mode;
    
    if (current_mode == 0) {
        digitalWrite(MOSFET_PIN, HIGH); // Disconnect the LEDs
        if (button_state == LOW) {
            shut_down();
        }
    }
}

/*!
    @brief   Returns the amount of time the board has been awake in milliseconds.
    @return  The difference between the time at which the button was pressed
             to wake up the board and the current time in milliseconds
*/
unsigned long Diabolo_Light::awake_time() {
    return millis() - wake_up_time;
}

/*!
    @brief   Getter for button_state, which is HIGH if the button
             is pressed and LOW if the button is released. This
             updates after a 50ms delay to account for button debounce.
    @return  button_state
*/
int Diabolo_Light::get_button_state() {
    return button_state;
}

