#define FASTLED_ALLOW_INTERRUPTS 0 // this is needed for ESP32 to prevent glitches caused by interrupts during LED data transmission
#include "DisplayGlobals.h"
#include <FastLED.h>
#include "SPIFFS.h"

// Définitions uniques des constantes et du buffer
const uint8_t pinData = 8; // Data pin for LED matrix (verify wiring). Try 13 if no response.
const int matrixLength = 27;
const int matrixHeight = 20;
const int nbLEDTotal = matrixLength * matrixHeight; // 540 LEDs
CRGB maMatrixLEDs[nbLEDTotal];

const int mouthLength = 9;
const int mouthHeight = 5;
const int mouthBeginsX = 9; // Starts at 9, ends at 17 (9 + mouthLength - 1)
const int mouthBeginsY = 10; // Starts at 10, ends at 14 (10 + mouthHeight - 1)
const int nbLEDMouth = mouthLength * mouthHeight;

void displayGlobals_init() {
    // Initialisation du module DisplayGlobals
    Serial.println("Initialisation du module DisplayGlobals");
    pinMode(pinData, OUTPUT);
    FastLED.addLeds<WS2812B, pinData, GRB>(maMatrixLEDs, nbLEDTotal);
    FastLED.setBrightness(3); // Increase brightness so test pattern is clearly visible
    
    // Fill a gradient test pattern on init to verify hardware before any animations
    for (int i = 0; i < nbLEDTotal; i++) {
        uint8_t hue = (i * 255) / nbLEDTotal;
        maMatrixLEDs[i] = CHSV(hue, 255, 255);
    }
    FastLED.show();

    // Lister les fichiers SPIFFS (CSV) pour debug
    size_t total = SPIFFS.totalBytes();
    size_t used  = SPIFFS.usedBytes();
    Serial.println("[DisplayGlobals] SPIFFS info:");
    Serial.print("  total: "); Serial.print(total); Serial.println(" bytes");
    Serial.print("  used : "); Serial.print(used);  Serial.println(" bytes");

    Serial.println("[DisplayGlobals] Listing SPIFFS root:");
    File root = SPIFFS.open("/");
    if (!root) {
        Serial.println("[DisplayGlobals] ERROR: SPIFFS.open('/') failed");
    } else if (!root.isDirectory()) {
        Serial.println("[DisplayGlobals] WARNING: '/' is not a directory on SPIFFS");
    } else {
        root.seek(0); // ensure we start at beginning for openNextFile
        int count = 0;
        while (true) {
            File f = root.openNextFile();
            if (!f) break;
            Serial.print(" - "); Serial.print(f.name()); Serial.print(" ("); Serial.print(f.size()); Serial.println(" bytes)");
            count++;
        }
        if (count == 0) {
            Serial.println("[DisplayGlobals] No files found in SPIFFS root. Verify uploadfs and partitions.");
        }
    }
}
