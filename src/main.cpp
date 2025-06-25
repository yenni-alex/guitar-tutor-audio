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

void UpdateDisplayThread() {      // TODO goertzel... annimation jouer avec taille stack thread voir consommation ram goertzel vs fFT
  Serial.println("UpdateDisplayThread started.");
  printFreeMemory(); // Affiche la mémoire libre au démarrage du thread
  clearDisplay(ILI9341_T4_COLOR_WHITE);
  drawIcon(0, H - 50, play_icon, 48, 48); // play
  drawIcon(50, H - 50, pause_icon, 48, 48); // pause
  drawIcon(100, H - 50, stop_icon, 48, 48); // stop
  drawIcon(150, H - 50, restart_icon, 48, 48); // rewind
  drawIcon(W - 50, 0, settings_icon, 48, 48); // next
  drawTabulation();

  int oldPlayingChordIndexDisplay = -1;
  while (true) {
    
    uint32_t currentTime = isPlaying ? millis() - baseTime : pausedTime - baseTime;
    //Serial.print("Current time: ");
    //Serial.println(currentTime);
    // Parcourir les 3 prochains accords (max)

    if(currentPlayingChordIndex < currentSong.chordCount) {

      Chord& chord = currentSong.chords[currentPlayingChordIndex];

      for(uint8_t i = 0; i < chord.noteCount; ++i) {
        Note& note = chord.notes[i];

        // Dessiner la note sur le manche
        drawNote(note.corde, note.caseFret, true, note.colorDisplay);
      }
    }
    if(currentPlayingChordIndex != oldPlayingChordIndexDisplay) {
      clearTabRegion();
      drawTabulation();
      oldPlayingChordIndexDisplay = currentPlayingChordIndex;
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
            float freqs[MAX_NOTES] = {0};
            float ths[MAX_NOTES] = {0};
            for (uint8_t i = 0; i < chord.noteCount; ++i) {
                freqs[i] = chord.notes[i].freq / 10.0; // Convertir en Hz
                ths[i] = chord.notes[i].threshold / 1000.0; 
            }
            checkNoteDetection(freqs, ths);
        }
        threads.delay(20);
    }
}

void updateLedsThread() {
    //int oldChordIndex = -1;
    while (true) {
      Serial.print("currentPlayingChordIndex: ");
      Serial.println(currentPlayingChordIndex);
      Serial.print("oldPlayingChordIndex: ");
      Serial.println(oldPlayingChordIndex);
        if (currentPlayingChordIndex != oldPlayingChordIndex) {
          
            ledController.clear();
            Serial.println("etape 1");
            Chord& chord = currentSong.chords[currentPlayingChordIndex];
            Serial.println("etape 2");
            for (uint8_t i = 0; i < chord.noteCount; ++i) {
                ledController.setLed(chord.notes[i].led, chord.notes[i].colorLed);
                Serial.println("etape 3");
            }
            for (int8_t i = NUM_LEDS - 1; i >= chord.heightOfHand; --i) { /// YA un beug ici. tout s allume quand j ai fini mais au moins ca marche et c est joli
                ledController.setLed(i, CRGB::Yellow); // ALLUME les LEDs au-dessus de la hauteur de la main
                Serial.println("etape 4");
            }
            ledController.show();
            Serial.println("etape 5");
            //old = currentPlayingChordIndex;
          
        }
        threads.delay(100);
    }
}


void setup() {
  Serial.begin(115200);
  initDisplay();
  initAudio();
  ledController.begin();
  clearDisplay(ILI9341_T4_COLOR_WHITE);
  //delay(5000);
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
  threads.addThread(UpdateDisplayThread, 0, Threads::DEFAULT_STACK0_SIZE); // Utilise la taille de pile par défaut
  threads.addThread(UpdateAudioThread, 0, Threads::DEFAULT_STACK0_SIZE); // Utilise la taille de pile par défaut
  threads.addThread(updateLedsThread, 0, Threads::DEFAULT_STACK0_SIZE); // Utilise la taille de pile par défaut
  // ajout d un thrad avec memoire statique
  Serial.println("Threads started.");
  printFreeMemory(); // Affiche la mémoire libre après le démarrage des threads
}

void loop() {
  delay(1000);
}

