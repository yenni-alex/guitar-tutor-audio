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


struct Note {

    uint8_t colorInt;
    uint8_t led;            // Numéro de la LED
    uint8_t corde;          // Numéro de la corde
    uint8_t caseFret;       // Numéro de la case    
    uint16_t freq;         // Fréquence de la note
    uint16_t threshold;    // Seuil de détection pour cette note //TODO DIVISER PAR 1000
    uint16_t colorDisplay;
    uint16_t currentX;      // TEST ANNIMATION
    CRGB colorLed; 

};

struct Chord {
    uint8_t noteCount; // Nombre de notes dans l'accord
    uint16_t time;          // Temps de décalage par rapport à l'accord précédent
    uint8_t heightOfHand;  // Hauteur de la main sur le manche
    Note notes[MAX_NOTES];
    
};

struct Song {
    uint8_t bpm;
    uint16_t chordCount; // Nombre d'accords dans la chanson
    Chord chords[MAX_CHORDS];
};

extern Song currentSong;
extern volatile int currentPlayingChordIndex;
extern volatile int oldPlayingChordIndex;
extern const int totalChords;

void initAudio();
bool checkNoteDetection(float frequencies[6], float thresholds[6]);


#endif