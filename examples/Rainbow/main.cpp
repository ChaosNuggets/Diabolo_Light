// Video of this program: https://youtu.be/cGEZ_opQ9x8

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Diabolo_Light.h>

using namespace Diabolo_Light;

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, LED_TYPE);
uint16_t hue = 0;

void setup() {
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

    // Initialize the diabolo light with 1 on mode (there's a default off mode built into the library),
    // require the board to be held for 500ms in order for it to turn on,
    // and make the board set the hue to 0 whenever the board wakes up
    begin(1, 500, [](){ hue = 0; });
}

void loop() {
    // Read button input and determine whether the mode should be changed.
    // Also determine whether or not the board should shut off.
    handle_button();

    // If the current mode is the first mode
    if (get_current_mode() == 1) {
        // Do the rainbow effect
        for (unsigned int i = 0; i < NUM_LEDS; i++) {
            pixels.setPixelColor(i, pixels.ColorHSV(hue, 255, 255));
        }
        pixels.show();
        hue += 5;
    }
}
