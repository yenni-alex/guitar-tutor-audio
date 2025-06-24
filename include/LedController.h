#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <FastLED.h>
#include "Config.h"

class LedController {
public:
    CRGB leds[NUM_LEDS];

    LedController();
    void begin();
    void setLed(int index, CRGB color);
    void clear();
    void show();
};

//extern LedController ledController;

#endif