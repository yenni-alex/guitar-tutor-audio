#include "xml.h"

void parseColor(const char* hex, CRGB& color) {
    unsigned long val = strtoul(hex, nullptr, 16);
    color.r = (val >> 16) & 0xFF;
    color.g = (val >> 8) & 0xFF;
    color.b = val & 0xFF;
}

static bool extractAttr(const char* line, const char* key, char* out, size_t outlen) {
    const char* p = strstr(line, key);
    if (!p) return false;
    p += strlen(key);
    while (*p == ' ' || *p == '=') ++p;
    if (*p == '"') ++p;
    size_t i = 0;
    while (*p && *p != '"' && i < outlen-1) out[i++] = *p++;
    out[i] = 0;
    return i > 0;
}

void loadSongFromXML(const char* filename) {
    File file = SD.open(filename);
    if (!file) {
        Serial.println("Erreur ouverture fichier XML");
        return;
    }
    Serial.println("Fichier XML ouvert avec succès");
    currentSong.chordCount = 0;
    Chord* chord = nullptr;
    char line[128];
    while (file.available()) {
        file.readBytesUntil('\n', line, sizeof(line) - 1);
        line[sizeof(line) - 1] = 0;
        // Début d'un accord
        if (strstr(line, "<chord")) {
            Serial.println("lecture d'un accord");
            if (currentSong.chordCount < MAX_CHORDS) {
                chord = &currentSong.chords[currentSong.chordCount++];
                chord->noteCount = 0;
                char buf[16];
                chord->time = 0; chord->heightOfHandLed = 0; chord->heightOfHandFret = 0;
                if (extractAttr(line, "time", buf, sizeof(buf))) chord->time = atoi(buf);
                if (extractAttr(line, "heightOfHandLeds", buf, sizeof(buf))) chord->heightOfHandLed = atoi(buf);
                if (extractAttr(line, "heightOfHandFret", buf, sizeof(buf))) chord->heightOfHandFret = atoi(buf);
            }
        }
        // Note
        else if (strstr(line, "<note") && chord && chord->noteCount < MAX_NOTES) {
            Serial.println("lecture d'une note");
            float freq = 0, threshold = 0;
            uint8_t led = 255, corde = 255, caseFret = 255, color = 255;
            
            char buf[16];
            if (extractAttr(line, "freq", buf, sizeof(buf))) freq = atof(buf);
            if (extractAttr(line, "threshold", buf, sizeof(buf))) threshold = atof(buf);
            if (extractAttr(line, "color", buf, sizeof(buf))) color = atoi(buf);
            if (extractAttr(line, "led", buf, sizeof(buf))) led = atoi(buf);
            if (extractAttr(line, "corde", buf, sizeof(buf))) corde = atoi(buf);
            if (extractAttr(line, "caseFret", buf, sizeof(buf))) caseFret = atoi(buf);
            
            
            chord->notes[chord->noteCount++] = {
                .colorInt = color,
                .led = led,
                .corde = corde,
                .caseFret = caseFret,
                .freq = (uint16_t)(freq * 10), 
                .threshold = (uint16_t)(threshold * 1000),
                .colorDisplay = parseColorDisplay(color),
                .currentX = 0,
                .colorLed = parseColorLed(color)
            };
        }
    }
    Serial.print("Nombre d'accords lus : ");
    Serial.println(currentSong.chordCount);
    file.close();
}