#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <Arduino.h>
#include <Audio.h>
#include "Config.h"
#include <arm_math.h>
#include <arm_const_structs.h>  // Contient arm_cfft_sR_f32_len4096
#include <algorithm>  // pour std::sort
#include <Audio.h>
#include "global.h"

class Note {
public:
    float freq;         // Fréquence de la note
    float threshold;    // Seuil de détection pour cette note
    uint8_t colorInt;
    CRGB colorLed; 
    uint16_t colorDisplay;
    uint8_t led;            // Numéro de la LED
    uint8_t corde;          // Numéro de la corde
    uint8_t caseFret;       // Numéro de la case
    Note(float f = 0, float t = 0, uint8_t c = 255, uint8_t l = 255, uint8_t co = 255, uint8_t ca = 255)
        : freq(f), threshold(t), colorInt(c), led(l), corde(co), caseFret(ca) {
        colorLed = parseColorLed(colorInt);
        colorDisplay = parseColorDisplay(colorInt); 
        }
};

class Chord {
public:
    static const uint8_t MAX_NOTES = 6; // Un accord peut avoir jusqu'à 6 notes (une par corde)
    Note notes[MAX_NOTES];
    uint8_t noteCount; // Nombre de notes dans l'accord
    int heightOfHand;  // Hauteur de la main sur le manche
    int time;          // Temps de décalage par rapport à l'accord précédent
    Chord() : noteCount(0), heightOfHand(0), time(0) {}
};

class Song {
public:
    static const uint16_t MAX_CHORDS = 64; // Limite mémoire (modifiable)
    Chord chords[MAX_CHORDS];
    uint16_t chordCount; // Nombre d'accords dans la chanson
    Song() : chordCount(0) {}
};

extern Song currentSong;
extern volatile int currentPlayingChordIndex;
extern const int totalChords;

void initAudio();
bool checkNoteDetection(float frequencies[6], float thresholds[6]);


#endif