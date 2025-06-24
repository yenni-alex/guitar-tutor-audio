#include "DisplayManager.h"


ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

DMAMEM ILI9341_T4::DiffBuffStatic<6000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<6000> diff2;

DMAMEM uint16_t internal_fb[H * W]; 
DMAMEM uint16_t fb[H * W]; 

void onRestart() {
  Serial.println("Restart clicked");
  currentPlayingChordIndex = 0; // Réinitialiser l'index du chord en cours
  oldPlayingChordIndex = -1; // Réinitialiser l'ancien index du chord
  // ici ton code restart
}

IconButton restart(150, H - 50, 48, onRestart); // Bouton de redémarrage

void mapTouchToScreen(int raw_x, int raw_y, int &screen_x, int &screen_y) {

    screen_x = map(raw_y, TS_MINY, TS_MAXY, W, 0);
    screen_y = map(raw_x, TS_MINX, TS_MAXX, 0, H);
    // Limiter les débordements éventuels
    screen_x = constrain(screen_x, 0, W - 1);
    screen_y = constrain(screen_y, 0, H - 1);
}

void checkTouch() {
    TSPoint p = ts.getPoint();
    if (p.z > 100) { // Adjust threshold as needed
        int x, y;
        // Map touch coordinates to screen coordinates
        mapTouchToScreen(p.x, p.y, x, y);
        // Handle touch event
        Serial.printf("Touched at: (%d, %d)\n", x, y);
        if(x >= restart.x && x <= restart.x + restart.size &&
           y >= restart.y && y <= restart.y + restart.size) {
            restart.onClick(); // Call the restart function if the button is touched
        }
        // Add your touch handling logic here
        
    }
}

void initDisplay() {
    //tft.output(&Serial);                // output debug infos to serial port.  
    while (!tft.begin(SPI_SPEED_DISPLAY));      // init the display
    tft.setRotation(3);                 // start in portrait mode 240x320
    tft.setFramebuffer(internal_fb);    // set the internal framebuffer (enables double buffering)
    tft.setDiffBuffers(&diff1, &diff2); // set 2 diff buffers -> enables diffenrential updates. 
    tft.setRefreshRate(90);  // start with a screen refresh rate around 40hz
    
}
void clearDisplay(uint16_t color) {
    for (int i = 0; i < H * W; i++) fb[i] = color;
}

void drawIcon(int x, int y, const uint16_t* icon, int w, int h) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int px = x + i;
            int py = y + j;
            if (px >= 0 && px < W && py >= 0 && py < H) {
                fb[py * W + px] = icon[j * w + i];
            }
        }
    }
}

void drawLine(int x0, int y0, int x1, int y1, int thickness, uint16_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;

    for (int i = -thickness / 2; i <= thickness / 2; i++) {
        int x = x0, y = y0;
        int errTemp = err;
        while (true) {
            int drawX = x;
            int drawY = y + i;

            // Vérifie que le pixel est dans les limites de l'écran
            if (drawX >= 0 && drawX < W && drawY >= 0 && drawY < H) {
                fb[drawY * W + drawX] = color;
            }

            if (x == x1 && y == y1) break;
            e2 = errTemp;
            if (e2 > -dx) { errTemp -= dy; x += sx; }
            if (e2 < dy) { errTemp += dx; y += sy; }
        }
    }
}


void drawRectangle(int x, int y, int width, int height, int thickness, uint16_t color) {
    for (int i = 0; i < thickness; i++) {
        drawLine(x, y + i, x + width - 1, y + i, 1, color);
        drawLine(x, y + height - 1 - i, x + width - 1, y + height - 1 - i, 1, color);
        drawLine(x + i, y, x + i, y + height - 1, 1, color);
        drawLine(x + width - 1 - i, y, x + width - 1 - i, y + height - 1, 1, color);
    }
}

void drawRectangleCentered(int x, int y, int width, int height, int thickness, uint16_t color) {
    
    // Calcul du coin supérieur gauche à partir du centre
    int startX = x - width / 2;
    int startY = y - height / 2;

    for (int i = 0; i < thickness; i++) {
        // Lignes horizontales
        drawLine(startX, startY + i, startX + width - 1, startY + i, 1, color);
        drawLine(startX, startY + height - 1 - i, startX + width - 1, startY + height - 1 - i, 1, color);
        // Lignes verticales
        drawLine(startX + i, startY, startX + i, startY + height - 1, 1, color);
        drawLine(startX + width - 1 - i, startY, startX + width - 1 - i, startY + height - 1, 1, color);
    }
}

void drawCircle(int x, int y, int radius, uint16_t color, bool fill, int thickness) {
    if (fill) {
        for (int i = -radius; i <= radius; i++) {
            for (int j = -radius; j <= radius; j++) {
                if (i * i + j * j <= radius * radius) {
                    int px = x + i;
                    int py = y + j;
                    if (px >= 0 && px < W && py >= 0 && py < H) {
                        fb[py * W + px] = color;
                    }
                }
            }
        }
    }
    else {
        for (int t = 0; t < thickness; t++) {
            int r = radius - t;
            if (r < 0) break;

            int dx = r - 1;
            int dy = 0;
            int err = 1 - dx;

            while (dx >= dy) {
                auto setPixel = [&](int px, int py) {
                    if (px >= 0 && px < W && py >= 0 && py < H) {
                        fb[py * W + px] = color;
                    }
                };

                setPixel(x + dx, y + dy);
                setPixel(x - dx, y + dy);
                setPixel(x + dx, y - dy);
                setPixel(x - dx, y - dy);
                setPixel(x + dy, y + dx);
                setPixel(x - dy, y + dx);
                setPixel(x + dy, y - dx);
                setPixel(x - dy, y - dx);

                dy++;
                if (err <= 0) {
                    err += 2 * dy + 1;
                }
                else {
                    dx--;
                    err += 2 * (dy - dx) + 1;
                }
            }
        }
    }
}
void drawNote(int corde, int fret, bool fill, uint16_t color) {
    int x, y;
    y = BOTTOM_BORDER + (corde - 1) * CORDS_ECART;
    if (fret == 0) {
        x = LEFT_BORDER;
    }
    else if (fret == 1) {
        x = LEFT_BORDER + 0.5 * FRET_ECART;
    }
    else {
        x = LEFT_BORDER + 0.5 * FRET_ECART + (fret - 1) * FRET_ECART;
    }

    drawCircle(x, y, 10,color, fill, 1);
}

void drawTabulation() {

    drawRectangle(LEFT_BORDER, BOTTOM_BORDER, W -(RIGHT_BORDER + LEFT_BORDER), H -(TOP_BORDER + BOTTOM_BORDER), 3, ILI9341_T4_COLOR_BLACK);
    
    // cords lines
    drawLine(LEFT_BORDER, BOTTOM_BORDER + CORDS_ECART, W - RIGHT_BORDER - 1, BOTTOM_BORDER + CORDS_ECART, 3, ILI9341_T4_COLOR_BLACK);
    drawLine(LEFT_BORDER, BOTTOM_BORDER + 2 * CORDS_ECART, W - RIGHT_BORDER - 1, BOTTOM_BORDER + 2 * CORDS_ECART, 3, ILI9341_T4_COLOR_BLACK);
    drawLine(LEFT_BORDER, BOTTOM_BORDER + 3 * CORDS_ECART, W - RIGHT_BORDER - 1, BOTTOM_BORDER + 3 * CORDS_ECART, 3, ILI9341_T4_COLOR_BLACK);
    drawLine(LEFT_BORDER, BOTTOM_BORDER + 4 * CORDS_ECART, W - RIGHT_BORDER - 1, BOTTOM_BORDER + 4 * CORDS_ECART, 3, ILI9341_T4_COLOR_BLACK);

    // frets lines
    drawLine(LEFT_BORDER + FRET_ECART, BOTTOM_BORDER, LEFT_BORDER + FRET_ECART, H - TOP_BORDER - 1, 1, ILI9341_T4_COLOR_BLACK);
    drawLine(LEFT_BORDER + 2 * FRET_ECART, BOTTOM_BORDER, LEFT_BORDER + 2 * FRET_ECART, H - TOP_BORDER - 1, 1, ILI9341_T4_COLOR_BLACK);
    drawLine(LEFT_BORDER + 3 * FRET_ECART, BOTTOM_BORDER, LEFT_BORDER + 3 * FRET_ECART, H - TOP_BORDER - 1, 1, ILI9341_T4_COLOR_BLACK);
}

void updateDisplay() {
    tft.update(fb);
}



