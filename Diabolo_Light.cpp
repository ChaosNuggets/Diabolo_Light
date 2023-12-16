#include <Arduino.h>
#include <avr/sleep.h>
#include <Diabolo_Light.h>

using namespace Diabolo_Light;

const static int DEBOUNCE_DELAY = 50; //ms
static unsigned long last_debounce_time = 0;

// HIGH means the button is pressed and LOW means the button is released
static int debounce_button_state;
static int button_state; 

static int num_modes;
static int current_mode; // 0 is the off mode, 1-num_modes inclusive are user defined modes

static func_ptr on_wake_up;

/*!
    @brief   Shut down the ATtiny85 and disconnect the LEDs to save power
*/
static void shut_down() {
    digitalWrite(MOSFET_PIN, LOW); // Disconnect the LEDs
    // TODO: delete the following line after you get new boards
    digitalWrite(LED_PIN, HIGH); // Make it so then ESD protection doesn't get in the way of reducing power
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
    digitalWrite(MOSFET_PIN, HIGH); // Connect the LEDs
    last_debounce_time = millis();
    current_mode = current_mode >= num_modes ? 0 : current_mode + 1;
    button_state = HIGH;

    cli(); // disable interrupts
    PCMSK &= ~(1 << PCINT2); // turns of PCINT2 as interrupt pin
    sleep_disable(); // clear sleep enable bit

    on_wake_up();
}

/*!
    @brief   Configure the diabolo light to read button input and save power.
             Call this in the setup function.
    @param   num_modes  a nonnegative number which is the number of modes
             the board should have not including the off mode
    @param   on_wake_up an optional parameter for additional things
             the board should do when it wakes up
*/
void Diabolo_Light::begin(const int num_modes, func_ptr on_wake_up) {
    ::num_modes = num_modes;
    ::on_wake_up = on_wake_up;

    ADCSRA &= ~(1 << ADEN); // Disable ADC
    ACSR &= ~(1 << ACIE); // Disable analog comparator interrupt
    ACSR |= 1 << ACD; // Disable analog comparator

    pinMode(MOSFET_PIN, OUTPUT);
    digitalWrite(MOSFET_PIN, HIGH); // TODO: change this to low when I get the new boards with pmosfets

    pinMode(BUTTON_PIN, INPUT);
    debounce_button_state = digitalRead(BUTTON_PIN); // Set DBS to the reading so we can skip the debounce code on startup
    button_state = HIGH; // Set it to high so then the shut down command inside handle_button runs on startup if the button is low

    current_mode = 0; // Set the board to shut down mode
}

/*!
    @brief   Read button input and change current_mode if necessary.
             Call this in the loop function. Make your loop function
             non-blocking or else current_mode will not update.
*/
void Diabolo_Light::handle_button() {
    int reading = digitalRead(Diabolo_Light::BUTTON_PIN);
    if (reading != debounce_button_state) {
        last_debounce_time = millis();
        debounce_button_state = reading;
    }

    if ((millis() - last_debounce_time) > DEBOUNCE_DELAY && reading != button_state) {
        button_state = reading;

        if (current_mode != 0 && button_state == HIGH) {
            current_mode = current_mode >= num_modes ? 0 : current_mode + 1;
        } 

        if (current_mode == 0 && button_state == LOW) {
            shut_down();
        }
    }
}

/*!
    @brief   Getter for current_mode. 0 is the off mode, and 1-num_modes inclusive are user defined modes.
    @return  Current mode
*/
int Diabolo_Light::get_current_mode() {
    return current_mode;
}

/*!
    @brief   Setter for current_mode. 0 is the off mode, and 1-num_modes inclusive are user defined modes.
*/
void Diabolo_Light::set_current_mode(const int new_mode) {
    current_mode = new_mode;
    
    if (current_mode == 0 && button_state == LOW) {
        shut_down();
    }
}
