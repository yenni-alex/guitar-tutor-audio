#ifndef CONFIG_H
#define CONFIG_H

// DISPLAY
#define PIN_SCK       27      // mandatory 
#define PIN_MISO      255     // mandatory (if the display has no MISO line, set this to 255 but then VSync will be disabled)
#define PIN_MOSI      26      // mandatory
#define PIN_DC        38      // mandatory, can be any pin but using pin 0 (or 38 on T4.1) provides greater performance
#define PIN_CS        36      
#define PIN_RESET     37      
#define PIN_BACKLIGHT 255   // optional, set this only if the screen LED pin is connected directly to the Teensy. 
#define PIN_TOUCH_IRQ 255   // optional. set this only if the touchscreen is connected on the same SPI bus
#define PIN_TOUCH_CS  255   // optional. set this only if the touchscreen is connected on the same spi bus

#define YP 39  // must be an analog pin, use "An" notation!
#define XM 40  // must be an analog pin, use "An" notation!
#define YM 34   // can be a digital pin
#define XP 33   // can be a digital pin
#define TS_MINX 130 // touch screen calibration values
#define TS_MAXX 900 
#define TS_MINY 100
#define TS_MAXY 900

#define H 240    // screen dimension
#define W 320
#define TOP_BORDER 40    // set borders
#define BOTTOM_BORDER 60
#define LEFT_BORDER 15
#define RIGHT_BORDER 100
#define CORDS_ECART ((H-TOP_BORDER-BOTTOM_BORDER)/5)
#define FRET_ECART ((W-RIGHT_BORDER-LEFT_BORDER)/4)

// LEDS
#define NUM_LEDS      52    // Nombre total de LEDs
#define DATA_PIN_LEDS      31    
#define CLOCK_PIN_LEDS     32     

// AUDIO
#define MARGE         5
#define NUMBER_OF_HARMONICS 5
#define SAMPLE_RATE 44100.0
#define FFT_SIZE      4096
#define BLOCK_SAMPLES 128
#define SPI_SPEED_DISPLAY     30000000  // 30MHz SPI. Can do much better with short wires

#define MAX_NOTES 6
#define MAX_CHORDS 64

#endif