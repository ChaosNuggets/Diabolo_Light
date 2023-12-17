# Diabolo Light Library Documentation

## Introduction

The Diabolo Light library provides a set of functions for managing button input and power consumption in an ATtiny85 microcontroller-based diabolo light. The library allows the diabolo light to enter a low-power sleep mode to conserve energy and supports multiple user-defined modes.

## Installation

Ensure that you have the Arduino IDE installed and set up for ATtiny85 development. Include the Diabolo Light library in your project by placing the library files in the Arduino `libraries` folder.

```cpp
#include <Arduino.h>
#include <avr/sleep.h>
#include <Diabolo_Light.h>
```

## Constants

- `DEBOUNCE_DELAY`: The debounce delay in milliseconds used for filtering button input.

## Variables

- `last_debounce_time`: Holds the timestamp of the last button debounce.
- `debounce_button_state`: Stores the debounced state of the button.
- `button_state`: Represents the current state of the button (HIGH for pressed, LOW for released).
- `num_modes`: The number of user-defined modes (excluding the off mode).
- `current_mode`: The current operating mode (0 for off mode, 1-num_modes for user-defined modes).
- `on_wake_up`: A function pointer to an optional callback executed when the board wakes up from sleep mode.

## Functions

### `void Diabolo_Light::begin(const int num_modes, func_ptr on_wake_up)`

Configures the diabolo light to read button input and save power. This function should be called in the setup function.

- **Parameters:**
  - `num_modes`: The number of user-defined modes (excluding the off mode).
  - `on_wake_up`: An optional callback function to be executed when the board wakes up from sleep mode.

### `void Diabolo_Light::handle_button()`

Reads button input and changes the `current_mode` if necessary. This function should be called in the loop function. Ensure your loop function is non-blocking to allow `current_mode` to update correctly.

### `int Diabolo_Light::get_current_mode()`

Returns the current operating mode.

- **Returns:**
  - `int`: The current mode (0 for off mode, 1-num_modes for user-defined modes).

### `void Diabolo_Light::set_current_mode(const int new_mode)`

Sets the current operating mode.

- **Parameters:**
  - `new_mode`: The desired mode to set (`0` for off mode, `1-num_modes` for user-defined modes).