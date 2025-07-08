#include "AudioProcessor.h"
#include <TeensyThreads.h>
#include "DisplayManager.h" // Pour les couleurs ILI9341_T4_COLOR_*
#include "global.h"

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

void initAudio() {
    AudioMemory(80);
    audioShield.enable();
    audioShield.inputSelect(AUDIO_INPUT_MIC);
    audioShield.micGain(40);
    recorder.begin();
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
    //if (currentPlayingNoteIndex >= totalNotes) return false;
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
        for (int i = 0; i < FFT_SIZE; i++) {
            fft_buffer[2 * i]     = input_buffer[i];
            fft_buffer[2 * i + 1] = 0.0f;
        }
        arm_cfft_f32(&arm_cfft_sR_f32_len4096, fft_buffer, 0, 1);
        float sample_rate = 44100.0;
        float sum[num_frequencies];
        for (int i = 0; i < num_frequencies; i++) {
            sum[i] = 0.0f;
        }
        int N = 5;
        for (int f = 0; f < num_frequencies; f++) {
            float mags[N];
            int count = 0;
            for (int n = 1; n <= N; n++) {
                float harmonic = frequencies[freq_indices[f]] * n;
                int bin = round(harmonic / (sample_rate / FFT_SIZE));
                if (bin < FFT_SIZE / 2) {
                    float r = fft_buffer[2 * bin];
                    float i = fft_buffer[2 * bin + 1];
                    float mag = sqrtf(r * r + i * i) / (FFT_SIZE / 2.0f);
                    mags[count++] = mag;
                }
            }
            if (count > 0) {
                std::sort(mags, mags + count);
                if (count % 2 == 1) {
                    sum[f] = mags[count / 2];
                } else {
                    sum[f] = (mags[count / 2 - 1] + mags[count / 2]) / 2.0f;
                }
            } else {
                sum[f] = 0.0f;
            }
        }
        bool allNotesTrue = true;
        float rms_ = rms.read() * 100.0f; 
        float peak_to_peak = peak.readPeakToPeak() / 2.0f * 100.0f;
        float rms_diff = rms_ - last_rms;
        last_rms = rms_;
        unsigned long now = millis();
        bool onset = false;
        if((peak_to_peak > 10) && (rms_ > 5)){
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
        if(analyse_en_cours) {
            for (int f = 0; f < num_frequencies; f++) {
                sum[f] *= 100.0f; // Convertir en pourcentage
                thresholds[freq_indices[f]] *= 100.0f; // Convertir en pourcentage
                
                /* if(sum[f] < thresholds[freq_indices[f]]) {
                    allNotesTrue = false;
                } */
                if (sum[f] * sum[f] / (rms_ * peak_to_peak) * 100.0f < 0.6f) {
                    allNotesTrue = false;
                }
                Serial.print("rms  : ");
                Serial.print(rms_);
                Serial.print("  peak  : ");
                Serial.print(peak_to_peak);
                Serial.print("  * : ");
                Serial.println(sum[f] * sum[f] / (rms_ * peak_to_peak) * 100.0f);
                Serial.print(sum[f]);
                Serial.print(" (th: ");
                Serial.print(thresholds[freq_indices[f]]);
                Serial.print(") - ");
            }
            if(allNotesTrue){
                Serial.print("   TOUTES LES NOTES JUSTE");
                Serial.println();
                analyse_nb_juste++;
                

            } else {
                Serial.print("   FAUX");
                Serial.println();
                analyse_nb_faux++;
            }
            if (now - analyse_start_time >= analyse_duree_ms) {
                Serial.println("--- Fin période d'analyse ---");
                Serial.print("Nombre de notes justes : ");
                Serial.println(analyse_nb_juste);
                Serial.print("Nombre de notes fausses : ");
                Serial.println(analyse_nb_faux);
                if (analyse_nb_juste >= 1) {
                    Serial.println("==> NOTE JUSTE !");
                    currentSong.chords[currentPlayingChordIndex].isPlayed = true;
                    oldPlayingChordIndex = currentPlayingChordIndex;
                    currentPlayingChordIndex++;
                    borderColor = ILI9341_T4_COLOR_LIME;
                    borderColorUntil = millis() + 200;
                } else {
                    Serial.println("==> NOTE FAUSSE !");
                    borderColor = ILI9341_T4_COLOR_RED;
                    borderColorUntil = millis() + 200;
                }
                analyse_en_cours = false;
            }
        }
        ready_for_fft = false;
        return false;
    }
    return false;
}



