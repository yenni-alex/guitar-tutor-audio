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
        float magnitudes[6] = {0};
        for (int f = 0; f < num_frequencies; f++) {
            magnitudes[f] = computeGoertzelMagnitude(input_buffer, FFT_SIZE, frequencies[freq_indices[f]], sample_rate);
        }
        bool allNotesTrue = true;
        float rms_ = rms.read();
        if((peak.readPeakToPeak()/2 > 0.1) && (rms_ > 0.1)){
            for (int f = 0; f < num_frequencies; f++) {
                //if(magnitudes[f] < thresholds[freq_indices[f]]) {
                float threshold = ((frequencies[f] - 70.0f) / 1200.0f * 3.0f) + 2.0f;
                if(magnitudes[f] < threshold ) { // TODO juste pour tester, à remplacer par thresholds[freq_indices[f]]
                    allNotesTrue = false;
                }
                Serial.print(magnitudes[f]);
                Serial.print(" (th: ");
                Serial.print(threshold);
                Serial.print(") - ");
            }
            if(allNotesTrue){
                Serial.print("   TOUTES LES NOTES JUSTE");
                Serial.println();
                currentSong.chords[currentPlayingChordIndex].isPlayed = true;
                oldPlayingChordIndex = currentPlayingChordIndex;
                currentPlayingChordIndex++;
            } else {
                Serial.print("   FAUX");
                Serial.println();
            }
        }
        ready_for_fft = false;
        return allNotesTrue;
    }
    return false;
}






