// Video of this program: https://youtu.be/phi5yQ2QbCU

#include <Arduino.h>
#include <Diabolo_Light.h>
#include <Adafruit_Neopixel.h>

using namespace Diabolo_Light;

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, LED_TYPE);
uint16_t hue = 0;

typedef uint32_t led_color;
const led_color BLUE = pixels.Color(0, 0, 255);
const led_color WHITE = pixels.Color(255/3, 255/3, 255/3);
const led_color OFF = pixels.Color(0, 0, 0);

void setup() {
    pixels.begin();
    begin(4, 500, [](){ hue = 0; });
}

/*!
    @brief   Set half the LEDs to color1 and the other
             half to color2 and show them.
    @param   color1  the first color (use the pixels.Color()
             function to generate this)
    @param   color2  the second color
*/
void set_all_to_colors(led_color color1, led_color color2) {
    for (unsigned int i = 0; i < NUM_LEDS; i += 2) {
        pixels.setPixelColor(i, color1);
    }
    for (unsigned int i = 1; i < NUM_LEDS; i += 2) {
        pixels.setPixelColor(i, color2);
    }
    pixels.show();
}

void loop() {
    handle_button();

    switch (get_current_mode()) {
        case 1:
            set_all_to_colors(BLUE, WHITE);
            break;
        case 2:
            set_all_to_colors(BLUE, OFF);
            break;
        case 3:
            set_all_to_colors(OFF, WHITE);
            break;
        case 4: { // Why does adding these brackets fix anything
                const led_color rainbow_color = pixels.ColorHSV(hue, 255, 255/2);
                set_all_to_colors(rainbow_color, rainbow_color);
                hue += 5;
            }
            break;
        default:
            pixels.clear();
            pixels.show();
            break;
    }
}
