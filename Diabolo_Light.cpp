#include <Arduino.h>
#include <avr/sleep.h>
#include <Diabolo_Light.h>

using namespace Diabolo_Light;

const unsigned int BUTTON_PIN = 2;
const unsigned int MOSFET_PIN = 0;

const static unsigned int DEBOUNCE_DELAY = 50; //ms
static unsigned long last_debounce_time = 0;

// HIGH means the button is pressed and LOW means the button is released
static int debounce_button_state;
static int button_state; 

static unsigned int num_modes;
static unsigned int current_mode; // 0 is the off mode, 1-num_modes inclusive are user defined modes

static void (*on_wake_up)();
static unsigned long wake_up_time;
static unsigned int hold_time;
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
    button_state = HIGH;
    has_just_woken_up = true;

    PCMSK &= ~(1 << PCINT2); // turns off PCINT2 as interrupt pin
    sleep_disable(); // clear sleep enable bit

    on_wake_up();
}

/*!
    @brief   Sets the button interrupt to run once every 5ms.
             Since button_history is 8 bits, the button will
             have to be stable for 40ms in order for
             button_state to be HIGH or LOW.
*/
void button_interrupt_setup()
{
    TCCR0A = 0x00; // Normal mode
    TCCR0A |= 1<<WGM01 // Clear timer on compare match mode

    TCCR0B = 0x00; // Clear TCCR0B
    TCCR0B |= (1<<CS02)|(1<<CS00); // Prescale with 1024

    OCR0A = 38 // Set counter to match once every 4.992ms (1024*(1+38))/8MHz = 4.992ms

    sei(); // Enable global interrupt
    TCNT0 = 0; // Clear the counter
    TIMSK |= (1<<OCIE0A); // Enable timer0 interrupt when counter matches OCR0A
}

/*!
    @brief   Configure the diabolo light to read button input and save power.
             Call this in the setup function.
    @param   num_modes  the number of modes the board should have
             not including the off mode
    @param   hold_time  the amount of time in milliseconds the user has to
             hold the button in order for the board to turn on. Defaults to
             500ms.
    @param   on_wake_up additional things the board should do when the button
             is pressed to wake up the board. Defaults to doing nothing.
*/
void Diabolo_Light::begin(const unsigned int num_modes, const unsigned int hold_time, void (*on_wake_up)()) {
    ::num_modes = num_modes;
    ::hold_time = hold_time;
    ::on_wake_up = on_wake_up;

    ADCSRA &= ~(1 << ADEN); // Disable ADC
    ACSR &= ~(1 << ACIE); // Disable analog comparator interrupt
    ACSR |= 1 << ACD; // Disable analog comparator

    pinMode(MOSFET_PIN, OUTPUT);
    digitalWrite(MOSFET_PIN, LOW); // Connect the LEDs

    pinMode(BUTTON_PIN, INPUT);
    debounce_button_state = digitalRead(BUTTON_PIN); // Set DBS to the reading so we can skip the debounce code on startup
    button_state = HIGH; // Set it to high so then the shut down command inside handle_button runs on startup if the button is low
    has_just_woken_up = false; // Set it to false because we want the board to sleep right away

    current_mode = 0; // Set the board to shut down mode
}

/*!
    @brief   Legacy function that used to read button input and
             change current_mode if necessary. Now this function
             does nothing.
*/
//void Diabolo_Light::handle_button() {}

/*!
    @brief   Read button input and change current_mode if necessary.
             Call this in the loop function. Make your loop function
             non-blocking or else current_mode will not update.
*/
void Diabolo_Light::handle_button() {
    // TODO: change this to a timer based interrupt that runs every 1ms
    // Read button state and shift a bit into the button history
    // If all ones, button state = HIGH,
    // If all zeros, button state = LOW,
    // Otherwise button state = UNSTABLE
    if (has_just_woken_up && awake_time() >= hold_time) {
        has_just_woken_up = false;
        current_mode = current_mode >= num_modes ? 0 : current_mode + 1;
        digitalWrite(MOSFET_PIN, LOW); // Connect the LEDs
    }

    int reading = digitalRead(BUTTON_PIN);
    if (reading != debounce_button_state) {
        last_debounce_time = millis();
        debounce_button_state = reading;
    }

    if ((millis() - last_debounce_time) > DEBOUNCE_DELAY && reading != button_state) {
        button_state = reading;

        if (button_state == HIGH) {
            current_mode = current_mode >= num_modes ? 0 : current_mode + 1;
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
