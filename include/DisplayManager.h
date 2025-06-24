#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <stdint.h>
#include <ILI9341_T4.h>
#include <TouchScreen.h>
#include "Config.h"
#include "AudioProcessor.h"

struct IconButton {
  int x, y;         // position coin haut-gauche
  int size;         // taille (carré pour simplifier)
  void (*onClick)(); // fonction appelée quand on clique dessus

  IconButton(int x_, int y_, int size_, void (*onClick_)()) 
    : x(x_), y(y_), size(size_), onClick(onClick_) {}
};

void checkTouch();

void initDisplay();
void clearDisplay(uint16_t color = 0);
void drawIcon(int x, int y, const uint16_t* icon, int w, int h);
void drawLine(int x0, int y0, int x1, int y1, int thickness, uint16_t color);
void drawRectangle(int x, int y, int width, int height, int thickness, uint16_t color);
void drawRectangleCentered(int x, int y, int width, int height, int thickness, uint16_t color);
void drawCircle(int x, int y, int radius, uint16_t color, bool fill, int thickness);
void drawNote(int corde, int fret, bool fill, uint16_t color);
void drawTabulation();
void updateDisplay();
    


#endif
