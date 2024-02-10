# Diabolo Light Library

## Introduction

The Diabolo Light library provides a set of functions for managing button input and power consumption in an ATtiny85 microcontroller-based diabolo light. The library allows the diabolo light to enter a low-power sleep mode to conserve energy and supports multiple user-defined modes.

## Installation

1. In PlatformIO Home, navigate to Libraries and search "Diabolo Light".
2. Click on the Diabolo Light library and press the "Add to Project" button.
3. Select your project and click "Add".
4. Use the library by including the Diabolo_Light.h file.

```cpp
#include <Diabolo_Light.h>
```

## Constants

- `const unsigned int Diabolo_Light::LED_PIN`: The ATtiny85 pin that the LEDs are attached to.
- `const unsigned int Diabolo_Light::NUM_LEDS`: The number of leds that are on the board
- `const unsigned int Diabolo_Light::LED_TYPE`: The type of neopixel the diabolo lights use.

Use these constants to initialize your [Adafruit_Neopixel](https://github.com/adafruit/Adafruit_NeoPixel) object.

```cpp
#include <Diabolo_Light.h>

using namespace Diabolo_Light;

Adafruit_Neopixel pixels(LED_PIN, NUM_LEDS, LED_TYPE);
```

## Functions

### `void Diabolo_Light::begin(const unsigned int num_modes, const unsigned long time_to_turn_on = 500, void (*on_wake_up)() = [](){}, const unsigned long time_to_turn_off = 2000)`

Configures the diabolo light to read button input and save power. This function should be called in the setup function.

- **Parameters:**
  - `num_modes`: The number of user-defined modes (excluding the off mode).
  - `time_to_turn_on`: How long the user has to hold the button for in order for the board to turn on. Defaults to 500ms.
  - `on_wake_up`: An optional callback function to be executed when the board wakes up from sleep mode. Defaults to doing nothing.
  - `time_to_turn_off`: How long the user has to hold the button for in order for the board to turn off. Defaults to 2000ms.

### `void Diabolo_Light::handle_button()`

Reads button input and changes the `current_mode` if necessary. Also shuts down the board if `current_mode` == 0. This function should be called in the loop function. Ensure your loop function is non-blocking to allow `current_mode` to update correctly.

### `unsigned int Diabolo_Light::get_current_mode()`

Returns the current operating mode.

- **Returns:**
  - `unsigned int`: The current mode (0 for off mode, 1-num_modes for user-defined modes).

### `void Diabolo_Light::set_current_mode(const unsigned int new_mode)`

Sets the current operating mode.

- **Parameters:**
  - `new_mode`: The desired mode to set (`0` for off mode, `1-num_modes` for user-defined modes).

### `unsigned long Diabolo_Light::awake_time()`

Returns the amount of time the board has been awake in milliseconds.

- **Returns:**
  - `unsigned long`: The difference between the time at which the button was pressed to wake up the board and the current time in milliseconds.

### `int Diabolo_Light::get_button_state()`

Returns the button state. The button state is HIGH if the button is pressed and LOW if the button is released. There is a 50ms delay between the time the button is pressed/released and the time button state updates to account for button debounce.

- **Returns:**
  - `int`: The button state

## Examples

There are multiple examples in the [examples folder](https://github.com/ChaosNuggets/Diabolo_Light/tree/main/examples).
