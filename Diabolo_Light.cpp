#include <Arduino.h>
#include <avr/sleep.h>
#include <Diabolo_Light.h>

using namespace Diabolo_Light;

const unsigned int BUTTON_PIN = 2;
const unsigned int MOSFET_PIN = 0;

const static unsigned int DEBOUNCE_DELAY = 50; //ms
static unsigned long last_debounce_time = 0;
static unsigned long press_start_time = 0;

// HIGH means the button is pressed and LOW means the button is released
static int debounce_button_state;
static int prev_button_state;
static int button_state; 
static int first_press;

static unsigned int num_modes;
static unsigned int current_mode; // 0 is the off mode, 1-num_modes inclusive are user defined modes

static void (*on_wake_up)();
static unsigned long wake_up_time;
static unsigned int begin_hold_time;
static unsigned int end_hold_time;
static bool has_just_woken_up; // If this is true, the user needs to hold the button for the mode to increment

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
    last_debounce_time = millis();
    wake_up_time = millis();
    button_state = LOW;
    has_just_woken_up = true;

    cli(); // disable interrupts
    PCMSK &= ~(1 << PCINT2); // turns of PCINT2 as interrupt pin
    sleep_disable(); // clear sleep enable bit

    on_wake_up();
}

/*!
    @brief   Configure the diabolo light to read button input and save power.
             Call this in the setup function.
    @param   num_modes  the number of modes the board should have
             not including the off mode
    @param   begin_hold_time  the amount of time in milliseconds the user has to
             hold the button in order for the board to turn on. Defaults to
             500ms.
    @param   end_hold_time  the amount of time in milliseconds the user has to
             hold the button in order for the board to turn off. Defaults to
             2000ms.
    @param   on_wake_up additional things the board should do when the button
             is pressed to wake up the board. Defaults to doing nothing.
*/
void Diabolo_Light::begin(const unsigned int num_modes, const unsigned int begin_hold_time, const unsigned int end_hold_time, void (*on_wake_up)()) {
    ::num_modes = num_modes;
    ::begin_hold_time = begin_hold_time;
    ::end_hold_time = end_hold_time;
    ::on_wake_up = on_wake_up;

    ADCSRA &= ~(1 << ADEN); // Disable ADC
    ACSR &= ~(1 << ACIE); // Disable analog comparator interrupt
    ACSR |= 1 << ACD; // Disable analog comparator

    pinMode(MOSFET_PIN, OUTPUT);
    digitalWrite(MOSFET_PIN, LOW); // Connect the LEDs

    pinMode(BUTTON_PIN, INPUT);
    debounce_button_state = digitalRead(BUTTON_PIN); // Set DBS to the reading so we can skip the debounce code on startup
    button_state = LOW; // Set it to high so then the shut down command inside handle_button runs on startup if the button is low
    has_just_woken_up = false; // Set it to false because we want the board to sleep right away
    first_press = true;

    current_mode = 0; // Set the board to shut down mode
}

/*!
    @brief   Read button input and change current_mode if necessary.
             Call this in the loop function. Make your loop function
             non-blocking or else current_mode will not update.
*/
void Diabolo_Light::handle_button() {
    if (has_just_woken_up && awake_time() >= begin_hold_time) {
        has_just_woken_up = false;
        first_press = true;
        current_mode = current_mode >= num_modes ? 0 : current_mode + 1;
        digitalWrite(MOSFET_PIN, LOW); // Connect the LEDs
    }

    int reading = digitalRead(BUTTON_PIN);
    if (reading != debounce_button_state) {
        last_debounce_time = millis();
        debounce_button_state = reading;
    }

    if ((millis() - last_debounce_time) > DEBOUNCE_DELAY && reading != button_state) {
        prev_button_state = button_state;
        button_state = reading;

        // Button pressed
        if (prev_button_state == LOW && button_state == HIGH) {
            press_start_time = millis();
        }

        // Button released
        if (prev_button_state == HIGH && button_state == LOW) {
            // Mode 1 is turned on during the first press down, so ignore the release that follows.
            if (first_press) {
                first_press = false;
            }
            else {
                current_mode = current_mode >= num_modes ? 0 : current_mode + 1;
            }
        }

        // If button is held for long enough, shut down on release.
        if ((millis() - press_start_time) > end_hold_time) {
            current_mode = 0;
            // TODO: shut down without requiring release. Not sure how to do that.
            // Calling shut_down() alone doesn't seem to work.
        }

        if (current_mode == 0 && button_state == LOW) {
            shut_down();
        }
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
*/
void Diabolo_Light::set_current_mode(const unsigned int new_mode) {
    current_mode = new_mode;
    
    if (current_mode == 0 && button_state == LOW) {
        shut_down();
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
