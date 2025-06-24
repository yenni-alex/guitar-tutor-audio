#include "global.h"

extern "C" char* sbrk(int incr);
void printFreeMemory() {
    char stack_top;
    char* heap_top = sbrk(0);
    Serial.print("Heap: ");
    Serial.println((int)heap_top, HEX);
    Serial.print("Stack: ");
    Serial.println((int)&stack_top, HEX);
    Serial.print("Free memory estimate: ");
    Serial.println((int)&stack_top - (int)heap_top);
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