#include <Arduino.h>
#include "DisplayManager.h"
#include "Config.h"
#include "AudioProcessor.h"
#include "LedController.h"
#include "xml.h"
#include "Icons.h"
#include "global.h"
#include <TeensyThreads.h>

LedController ledController;

void UpdateDisplayThread() {      //TODO modifier drawtab set rot drawnote... leds bloque quand restart... goertzel...
  while (true) {
    clearDisplay(ILI9341_T4_COLOR_WHITE);
    drawTabulation();
    drawIcon(W - 50, 0, play_icon, 48, 48); // play
    drawIcon(W - 100, 0, pause_icon, 48, 48); // pause
    drawIcon(W - 150, 0, stop_icon, 48, 48); // stop
    drawIcon(W - 200, 0, restart_icon, 48, 48); // rewind
    drawIcon(0, H - 50, settings_icon, 48, 48); // next
    if (currentPlayingChordIndex < currentSong.chordCount) {
      Chord& chord = currentSong.chords[currentPlayingChordIndex];
      drawNote(chord.notes->corde, chord.notes->caseFret, true, chord.notes->colorDisplay);

    }
    checkTouch();
    updateDisplay();
    //threads.delay(20);
    threads.delay(100);
  }
}

void UpdateAudioThread() {
    while (true) {
        if (currentPlayingChordIndex < currentSong.chordCount) {
            Chord& chord = currentSong.chords[currentPlayingChordIndex];
            float freqs[Chord::MAX_NOTES] = {0};
            float ths[Chord::MAX_NOTES] = {0};
            for (uint8_t i = 0; i < chord.noteCount; ++i) {
                freqs[i] = chord.notes[i].freq;
                ths[i] = chord.notes[i].threshold;
            }
            checkNoteDetection(freqs, ths);
        }
        threads.delay(20);
    }
}

void updateLedsThread() {
    int oldChordIndex = -1;
    while (true) {
        if (currentPlayingChordIndex != oldChordIndex) {
            ledController.clear();
            Chord& chord = currentSong.chords[currentPlayingChordIndex];
            for (uint8_t i = 0; i < chord.noteCount; ++i) {
                ledController.setLed(chord.notes[i].led, chord.notes[i].colorLed);
            }
            for (uint8_t i = NUM_LEDS - 1; i >= chord.heightOfHand; --i) {
                ledController.setLed(i, CRGB::Yellow); // ALLUME les LEDs au-dessus de la hauteur de la main
            }
            ledController.show();
            oldChordIndex = currentPlayingChordIndex;
        }
        threads.delay(40);
    }
}

void setup() {
  Serial.begin(115200);
  initDisplay();
  initAudio();
  ledController.begin();
  clearDisplay(ILI9341_T4_COLOR_WHITE);
  delay(5000);
  Serial.println("Setup started.");
  printFreeMemory(); // Affiche la mémoire libre au démarrage
  SD.begin(BUILTIN_SDCARD);
  Serial.println("SD card initialized");
  if (!SD.exists("/test_gladiator.xml")) {
    Serial.println("Fichier test_gladiator.xml introuvable");
    return;
  }
  loadSongFromXML("/test_gladiator.xml");
  Serial.println("Song loaded from XML.");
  printFreeMemory(); // Affiche la mémoire libre après l'initialisation de la SD
  Serial.print("Accords lus: ");
  Serial.println(currentSong.chordCount);
  for (uint16_t i = 0; i < currentSong.chordCount; ++i) {
    Serial.print("Chord "); Serial.print(i); Serial.print(" notes: "); Serial.println(currentSong.chords[i].noteCount);
    for (uint8_t j = 0; j < currentSong.chords[i].noteCount; ++j) {
      Serial.print("  Note "); Serial.print(j);
      Serial.print(" freq: "); Serial.print(currentSong.chords[i].notes[j].freq);
      Serial.print(" led: "); Serial.print(currentSong.chords[i].notes[j].led);
      Serial.print(" color: "); Serial.print(currentSong.chords[i].notes[j].colorInt);
      Serial.print(" corde: "); Serial.print(currentSong.chords[i].notes[j].corde);
      Serial.print(" caseFret: "); Serial.print(currentSong.chords[i].notes[j].caseFret);
      Serial.print(" colorLed: "); Serial.print(currentSong.chords[i].notes[j].colorLed.r);
      Serial.print(", "); Serial.print(currentSong.chords[i].notes[j].colorLed.g);
      Serial.print(", "); Serial.print(currentSong.chords[i].notes[j].colorLed.b);
      Serial.print(" colorDisplay: "); Serial.print(currentSong.chords[i].notes[j].colorDisplay, HEX);
      Serial.println();
    }
  }
  // Affiche le premier accord dès le début
  ledController.clear();
  if (currentSong.chordCount > 0) {
    Chord& chord = currentSong.chords[0];
    for (uint8_t i = 0; i < chord.noteCount; ++i) {
      ledController.setLed(chord.notes[i].led, chord.notes[i].colorLed);
    }
    ledController.show();
  }
  Serial.println("leds initialized.");
  printFreeMemory(); // Affiche la mémoire libre après l'initialisation de l'
  threads.addThread(UpdateDisplayThread);
  threads.addThread(UpdateAudioThread);
  threads.addThread(updateLedsThread);
  Serial.println("Threads started.");
  printFreeMemory(); // Affiche la mémoire libre après le démarrage des threads
}

void loop() {
  delay(1000);
}

