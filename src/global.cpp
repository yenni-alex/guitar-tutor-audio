#include "global.h"
// variables pour compteur
bool isPlaying = false;
uint32_t baseTime = 0;      // moment où on a commencé à jouer
uint32_t pausedTime = 0;    // moment où on a mis en pause

extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern char *__brkval;

int freeram() {
  return (char *)&_heap_end - __brkval;
}

void printFreeMemory() {
    int freeMem = freeram();
    Serial.print("Free memory: ");
    Serial.println(freeMem);
    //Serial.print("Heap start: ");
    //Serial.println((unsigned long)&_heap_start, HEX);
    //Serial.print("Heap end: ");
    //Serial.println((unsigned long)&_heap_end, HEX);
    //Serial.print("Brkval: ");
    //Serial.println((unsigned long)__brkval, HEX);
}

CRGB parseColorLed(int colorInt) {
    // Si c'est un code hexadécimal (commence par un chiffre ou A-F)
    CRGB color;
    switch (colorInt) 
    {
    case 0:
        color.r = 0; color.g = 0; color.b = 0; // Noir
        break;
    case 1:
        color.r = 255; color.g = 0; color.b = 0; // Rouge
        break;
    case 2:
        color.r = 0; color.g = 255; color.b = 0; // Vert
        break;
    case 3:
        color.r = 0; color.g = 0; color.b = 255; // Bleu
        break;
    case 4:
        color.r = 255; color.g = 255; color.b = 0; // Jaune
        break; 
    case 5:
        color.r = 255; color.g = 0; color.b = 255; // Magenta
        break;
    case 6:
        color.r = 0; color.g = 255; color.b = 255; // Cyan
        break;
    
    default:
        color.r = 0; color.g = 0; color.b = 0; // Noir par défaut
        break;
    }
    return color;
}

uint16_t parseColorDisplay(int colorInt) {
    // Si c'est un code hexadécimal (commence par un chiffre ou A-F)
    uint16_t color;
    switch (colorInt) 
    {
    case 0:
        color = ILI9341_T4_COLOR_BLACK; // Noir
        break;
    case 1:
        color = ILI9341_T4_COLOR_RED; // Rouge
        break;
    case 2:
        color = ILI9341_T4_COLOR_GREEN; // Vert
        break;
    case 3:
        color = ILI9341_T4_COLOR_BLUE; // Bleu
        break;
    case 4:
        color = ILI9341_T4_COLOR_YELLOW; // Jaune
        break; 
    case 5:
        color = ILI9341_T4_COLOR_MAJENTA; // Magenta
        break;
    case 6:
        color = ILI9341_T4_COLOR_CYAN; // Cyan
        break;
    
    default:
        color = ILI9341_T4_COLOR_BLACK; // Noir par défaut
        break;
    }
    return color;
}

volatile uint32_t borderColorUntil = 0;
volatile uint16_t borderColor = 0;