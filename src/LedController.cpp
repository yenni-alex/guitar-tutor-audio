#include "LedController.h"

//LedController ledController;

LedController::LedController() {
    // Rien à faire ici
}

void LedController::begin() {
    //FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR, DATA_RATE_MHZ(1)>(leds, NUM_LEDS);
    FastLED.addLeds<APA102, DATA_PIN_LEDS, CLOCK_PIN_LEDS, BGR>(leds, NUM_LEDS);
    FastLED.setBrightness(20);
    clear();
    show();
}

void LedController::setLed(int index, CRGB color, uint8_t brightness) {
    if (index >= 0 && index < NUM_LEDS) {
        leds[index] = color;
        leds[index].nscale8(brightness);
    }
}


void LedController::clear() {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void LedController::show() {
    FastLED.show();
}
