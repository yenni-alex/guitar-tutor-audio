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
        float rms_ = rms.read();
        if((peak.readPeakToPeak()/2 > 0.1) && (rms_ > 0.1)){
            for (int f = 0; f < num_frequencies; f++) {
                if(sum[f] < thresholds[freq_indices[f]]) {
                    allNotesTrue = false;
                }
                Serial.print(sum[f]);
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



