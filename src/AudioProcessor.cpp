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
                if(magnitudes[f] < thresholds[freq_indices[f]]) {
                    allNotesTrue = false;
                }
                Serial.print(magnitudes[f]);
                Serial.print(" (th: ");
                Serial.print(thresholds[freq_indices[f]]);
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



// Variables globales (à initialiser au début)
float rmsPeakRatio_Threshold = 0.4f; // Ratio RMS/Peak pour seuil
float relativeRMSChange_Threshold = 0.15f; // Changement relatif RMS
float score_Threshold = 0.5f; // Score combiné pour détection

float previousRMS = 0.0f;
float rmsBaseline = 0.0f;
bool baselineCalibrated = false;
unsigned long baselineStartTime = 0;
const unsigned long baselineDurationMs = 1000; // 1 seconde de calibration
float rmsBaselineSum = 0.0f;
int rmsBaselineCount = 0;

float smoothRMS(float currentRMS) {
    // Simple filtre passe-bas pour lisser
    static float smoothed = 0.0f;
    const float alpha = 0.1f;
    smoothed = alpha * currentRMS + (1 - alpha) * smoothed;
    return smoothed;
}

bool detectUniversalOnset(float rmsValue, float peakValue) {
    if (!baselineCalibrated) {
        // Phase de calibration
        if (millis() - baselineStartTime < baselineDurationMs) {
            rmsBaselineSum += rmsValue;
            rmsBaselineCount++;
            return false; // pas de détection pendant calibration
        } else {
            rmsBaseline = rmsBaselineSum / max(1, rmsBaselineCount);
            baselineCalibrated = true;
            Serial.print("Baseline RMS calibré : ");
            Serial.println(rmsBaseline);
            previousRMS = rmsValue;
            return false;
        }
    }

    // RMS lissé
    float rmsSmooth = smoothRMS(rmsValue);

    // Calcul des ratios
    float rmsPeakRatio = rmsSmooth / (peakValue + 1e-6f);
    float deltaRMS = rmsSmooth - previousRMS;
    previousRMS = rmsSmooth;
    float relativeRMSChange = deltaRMS / (rmsSmooth + 1e-6f);

    // Ajuster seuils en fonction baseline (ex: 3x le bruit de fond)
    float rmsThreshold = rmsBaseline * 3.0f;

    // Score combiné pondéré
    float score = 0.6f * rmsPeakRatio + 0.4f * relativeRMSChange;
    Serial.print("RMS: ");
    Serial.print(rmsSmooth);
    Serial.print(", Threshold: ");
    Serial.print(rmsThreshold);
    Serial.print(", RMS/Peak Ratio: ");
    Serial.print(rmsPeakRatio);
    Serial.print(", Threshold: ");
    Serial.print(rmsThreshold);
    Serial.print(", Relative RMS Change: ");
    Serial.print(relativeRMSChange);
    Serial.print(", Score: ");
    Serial.print(score);
    Serial.print(" - ");
    // Conditions de détection
    bool onsetDetected = false;
    if (rmsSmooth > rmsThreshold &&         // signal assez fort
        rmsPeakRatio > rmsPeakRatio_Threshold &&              // ratio RMS/Peak cohérent avec note tenue
        relativeRMSChange > relativeRMSChange_Threshold &&        // montée rapide
        score > score_Threshold) {                     // score global suffisant
        onsetDetected = true;
    }

    return onsetDetected;
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
    return magnitude / numSamples;  // Normalisation facultative
}


bool checkNoteDetection(float frequencies[6], float thresholds[6]) {
    int num_frequencies = 0;
    int freq_indices[6];
    for (int i = 0; i < 6; ++i) {
        if (frequencies[i] != 0.0f) {
            freq_indices[num_frequencies++] = i;
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
        float sample_rate = 44100.0;
        float magnitudes[6] = {0};
        float magnitudes_sub[6] = {0};
        float magnitudes_sup[6] = {0};

        for (int f = 0; f < num_frequencies; f++) {
            float targetFreq = frequencies[freq_indices[f]];
            magnitudes[f] = computeGoertzelMagnitude(input_buffer, FFT_SIZE, targetFreq, sample_rate);
            magnitudes_sub[f] = computeGoertzelMagnitude(input_buffer, FFT_SIZE, targetFreq / 2, sample_rate);
            magnitudes_sup[f] = computeGoertzelMagnitude(input_buffer, FFT_SIZE, targetFreq * 2, sample_rate);
        }

        bool allNotesTrue = true;
        float rms_ = rms.read();
        Serial.print("RMS: ");
        Serial.print(rms_);
        float peakToPeak = peak.readPeakToPeak() / 2.0f;
        Serial.print(", Peak-to-Peak: ");
        Serial.println(peakToPeak);

        if (detectUniversalOnset(rms_, peakToPeak)) {
            Serial.print("RMS: ");
            Serial.print(rms_);
            Serial.print(", Peak-to-Peak: ");
            Serial.println(peakToPeak);
        } else {
            Serial.println("Aucun onset détecté");
            return false; // Pas de détection, on sort
        }
/*
        if ((peak.readPeakToPeak() / 2 > 0.1) && (rms_ > 0.1)) {
            for (int f = 0; f < num_frequencies; f++) {
                if (magnitudes[f] < thresholds[freq_indices[f]]) {
                    allNotesTrue = false;
                }
                //Serial.print(magnitudes[f]);
                //Serial.print(" (th: ");
                //Serial.print(thresholds[freq_indices[f]]);
                //Serial.print(") - ");
                Serial.print("Freq: ");
                Serial.print(frequencies[freq_indices[f]]);
                Serial.print(" Hz, Mag: ");
                Serial.print(magnitudes[f]);
                Serial.print(", Sub: ");
                Serial.print(magnitudes_sub[f]);
                Serial.print(", Sup: ");
                Serial.print(magnitudes_sup[f]);
                Serial.print(" (th: ");
                Serial.print(thresholds[freq_indices[f]]);
                Serial.print(") - ");
            }

            if (allNotesTrue) {
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
*/
        ready_for_fft = false;
        return allNotesTrue;
    }

    return false;
}



