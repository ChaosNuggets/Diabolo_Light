#include <Arduino.h>
#include <avr/sleep.h>
#include <Diabolo_Light.h>

using namespace Diabolo_Light;

const unsigned int BUTTON_PIN = 2;
const unsigned int MOSFET_PIN = 0;

// HIGH means the button is pressed, LOW means the button is released, and
// UNSTABLE means the button hasn't been held down / let go for long enough
static volatile int prev_button_state;
static volatile int button_state;

// A series of bits, 1 means the reading was HIGH and 0 means the reading was LOW. Rightmost bit is the most recent.
static volatile uint8_t button_history;

static unsigned int num_modes;
static unsigned int current_mode; // 0 is the off mode, 1-num_modes inclusive are user defined modes

static void (*on_wake_up)();
static unsigned long wake_up_time;
static unsigned long holding_start_time; // The time at which the user starts holding the button

// "Constants" initialized in begin() that specify the time the user has to hold the button to turn the lights on/off.
static unsigned int time_to_turn_on;
static unsigned int time_to_turn_off;

static bool has_just_woken_up; // If this is true, the user needs to hold the button for the mode to increment to 1

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
static void button_interrupt_setup()
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
    
    button_interrupt_setup();

    pinMode(BUTTON_PIN, INPUT);
    prev_button_state = UNSTABLE;
    button_state = UNSTABLE; // Set it to UNSTABLE because we don't know whether the button is currently pressed or not
    has_just_woken_up = false; // Set it to false because we want the board to sleep right away

    pinMode(MOSFET_PIN, OUTPUT);
    set_current_mode(0); // Set the board to shut down
}

/*!
    @brief   Legacy function that used to read button input and change
             current_mode if necessary. Now this function does nothing. It's
             just here so code using previous versions of this library doesn't break.
*/
void Diabolo_Light::handle_button() {}

/*!
    @brief   Read button input and change current_mode if necessary.
             Runs once every 5ms.
*/
ISR(TIMER0_COMPA_vect) {
    // Read button state and shift a bit into the button history
    button_history = (button_history<<1) | digitalRead(BUTTON_PIN);

    // If all ones, button state = HIGH,
    // If all zeros, button state = LOW,
    // Otherwise button state = UNSTABLE
    if (~button_history == 0x0) {
        button_state = HIGH;
    } else if (button_history == 0x0) {
        button_state = LOW;
    } else {
        button_state = UNSTABLE;
        // Reset holding_start_time so it basically acts like the button has been held for 0 time
        holding_start_time = millis(); 
    }

    // Connect the LEDs if the user has held down the button for long enough
    if (has_just_woken_up && button_state == HIGH && millis() - holding_start_time >= time_to_turn_on) {
        has_just_woken_up = false;
        set_current_mode(current_mode >= num_modes ? 0 : current_mode + 1);
        digitalWrite(MOSFET_PIN, LOW); // Connect the LEDs
    }
    
    // Ty Victor Lin for this ideaa (shut down on long press)
    if (button_state == HIGH && millis() - holding_start_time >= time_to_turn_off) {
        set_current_mode(0);
    }

    // Runs on button state change except for when button_state is unstable.
    // We don't want prev_button_state to be UNSTABLE because if button_state transitions from HIGH->UNSTABLE->HIGH, we don't want to increment current_mode.
    if (prev_button_state != button_state && button_state != UNSTABLE) {
        prev_button_state = button_state;

        if (button_state == HIGH) {
            set_current_mode(current_mode >= num_modes ? 0 : current_mode + 1);
            holding_start_time = millis();
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
