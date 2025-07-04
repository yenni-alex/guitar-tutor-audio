#include "AudioProcessor.h"
#include <TeensyThreads.h>
#include "DisplayManager.h" // Pour les couleurs ILI9341_T4_COLOR_*

AudioInputI2S            i2s1;
AudioRecordQueue         recorder;
AudioAnalyzeRMS          rms;
AudioAnalyzePeak         peak;
AudioControlSGTL5000     audioShield;

AudioConnection          patchCord2(i2s1, 0, recorder, 0);
AudioConnection          patchCord3(i2s1, 0, rms, 0);
AudioConnection          patchCord4(i2s1, 0, peak, 0);

DMAMEM float32_t input_buffer[FFT_SIZE];
DMAMEM float32_t fft_buffer[FFT_SIZE * 2];
int sample_index = 0;
bool ready_for_fft = false;

DMAMEM Song currentSong;
volatile int currentPlayingChordIndex = 0;
volatile int oldPlayingChordIndex = -1;
const int totalChords = MAX_CHORDS;

// --- Détection d'attaque améliorée ---
float last_rms = 0.0f;
unsigned long last_onset_time = 0;
const unsigned long onset_debounce_ms = 150; // Anti-rebond sur la détection d'attaque
const float rms_jump_threshold = 3.0f; // Seuil de variation rapide du RMS (à ajuster)

// --- Période d'analyse après détection d'attaque ---
bool analyse_en_cours = false;
unsigned long analyse_start_time = 0;
const unsigned long analyse_duree_ms = 100; // Durée de la période d'analyse (modifiable)
int analyse_nb_juste = 0;
int analyse_nb_faux = 0;

// Facteur de discrimination pour la justesse (modifiable)
const float magnitude_factor = 1.5f; // La magnitude à la fréquence cible doit être au moins 1.5x supérieure à celle des notes voisines

void initAudio() {
    AudioMemory(80);
    audioShield.enable();
    audioShield.inputSelect(AUDIO_INPUT_MIC);
    audioShield.micGain(40);
    audioShield.volume(1);
    audioShield.inputLevel(1.0f); // Niveau d'entrée à 1.0 (100%)
    recorder.begin();
}

float computeGoertzelMagnitude(float* samples, int numSamples, float targetFreq, float sampleRate) {
    float k = 0.5f + ((numSamples * targetFreq) / sampleRate);
    int bin = (int)k;
    float omega = (2.0f * PI * bin) / numSamples;
    float coeff = 2.0f * cosf(omega);

    float q0 = 0.0f, q1 = 0.0f, q2 = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        q0 = coeff * q1 - q2 + samples[i];
        q2 = q1;
        q1 = q0;
    }

    float magnitude = sqrtf(q1 * q1 + q2 * q2 - q1 * q2 * coeff);
    return magnitude / numSamples * 100;  // Normalisation facultative
}

// Nouvelle fonction utilitaire pour la détection comparative
bool isNoteJusteComparative(float* samples, int numSamples, float freq_cible, float freq_below, float freq_above, float sampleRate, float factor) {
    float mag_cible = computeGoertzelMagnitude(samples, numSamples, freq_cible, sampleRate);
    float mag_below = computeGoertzelMagnitude(samples, numSamples, freq_below, sampleRate);
    float mag_above = computeGoertzelMagnitude(samples, numSamples, freq_above, sampleRate);
    Serial.print("Mag cible: "); Serial.print(mag_cible);
    Serial.print(" | below: "); Serial.print(mag_below);
    Serial.print(" | above: "); Serial.println(mag_above);
    return (mag_cible > mag_below * factor) && (mag_cible > mag_above * factor);
}

bool checkNoteDetection(float frequencies[6], float thresholds[6]) {
    int num_frequencies = 0;
    int freq_indices[6];
    for (int i = 0; i < 6; ++i) {
        if (frequencies[i] != 0.0f) {
            freq_indices[num_frequencies] = i;
            num_frequencies++;
        }
    }
    if (!ready_for_fft) {
        while (recorder.available() > 0 && sample_index < FFT_SIZE) {
            int16_t *data = recorder.readBuffer();
            for (int i = 0; i < BLOCK_SAMPLES && sample_index < FFT_SIZE; i++) {
                input_buffer[sample_index++] = (float32_t)data[i] / 32768.0f;
            }
            recorder.freeBuffer();
        }
        if (sample_index >= FFT_SIZE) {
            ready_for_fft = true;
            sample_index = 0;
        }
    }
    if (ready_for_fft) {
        float sample_rate = 44100.0f;
        bool allNotesTrue = true;
        float rms_ = rms.read() * 100.0f; // Convertir en pourcentage
        float peak_to_peak = peak.readPeakToPeak() / 2.0f * 100.0f; // Diviser par 2 pour obtenir la valeur réelle
        float rms_diff = rms_ - last_rms;
        last_rms = rms_;
        unsigned long now = millis();
        bool onset = false;
        if ((peak_to_peak > 10) && (rms_ > 5)) {
            // Détection d'attaque : variation rapide du RMS
            if (rms_diff > rms_jump_threshold && (now - last_onset_time > onset_debounce_ms) && !analyse_en_cours) {
                onset = true;
                last_onset_time = now;
            }
            // Démarrage de la période d'analyse après détection d'attaque
            if (onset) {
                analyse_en_cours = true;
                analyse_start_time = now;
                analyse_nb_juste = 0;
                analyse_nb_faux = 0;
                Serial.println("--- Début période d'analyse ---");
            }
        }
        // Si période d'analyse en cours
        if (analyse_en_cours) {
            // On analyse la justesse à chaque passage
            for (int f = 0; f < num_frequencies; f++) {
                // Fréquence cible et ses voisins à un demi-ton
                float freq_cible = frequencies[freq_indices[f]];
                float freq_below = freq_cible - 20; // resolution frequentielle de 10.76 Hz
                float freq_above = freq_cible + 20;
                bool juste = isNoteJusteComparative(input_buffer, FFT_SIZE, freq_cible, freq_below, freq_above, sample_rate, magnitude_factor);
                if (!juste) allNotesTrue = false;
            }
            if (allNotesTrue) {
                analyse_nb_juste++;
                Serial.println("Juste (analyse)");
            } else {
                analyse_nb_faux++;
                Serial.println("Faux (analyse)");
            }
            // Si la période d'analyse est terminée
            if (now - analyse_start_time > analyse_duree_ms) {
                Serial.print("--- Fin période d'analyse. Résultat: ");
                Serial.print(analyse_nb_juste);
                Serial.print(" justes, ");
                Serial.print(analyse_nb_faux);
                Serial.println(" faux ---");
                // Critère : au moins une analyse juste
                if (analyse_nb_juste >= 1) {
                    Serial.println("==> NOTE JUSTE !");
                    currentSong.chords[currentPlayingChordIndex].isPlayed = true;
                    oldPlayingChordIndex = currentPlayingChordIndex;
                    currentPlayingChordIndex++;
                } else {
                    Serial.println("==> NOTE FAUSSE !");
                }
                analyse_en_cours = false;
            }
        }
        ready_for_fft = false;
        // On ne retourne true que si la note a été validée à la fin de la période d'analyse
        return false;
    }
    return false;
}






