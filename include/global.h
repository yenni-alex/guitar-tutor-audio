#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <ILI9341_t4.h>
#include <FastLED.h>
// variables pour compteur
extern bool isPlaying;
extern uint32_t baseTime;      
extern uint32_t pausedTime;    

extern "C" char* sbrk(int incr);
CRGB parseColorLed(int colorInt);
uint16_t parseColorDisplay(int colorInt);

void printFreeMemory();

#endif