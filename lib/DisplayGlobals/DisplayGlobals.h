#pragma once
#include <Arduino.h>
#include <FastLED.h>


void displayGlobals_init();
// Déclarations extern (une seule définition dans DisplayGlobals.cpp)
extern const uint8_t pinData;      // Data pin
extern const int matrixLength;     // Largeur
extern const int matrixHeight;     // Hauteur
extern const int nbLEDTotal;       // Total LEDs (matrixLength * matrixHeight)
extern CRGB maMatrixLEDs[];        // Buffer LED global

extern const int mouthLength;      // Largeur de la bouche
extern const int mouthHeight;      // Hauteur de la bouche
extern const int mouthBeginsX;
extern const int mouthBeginsY;
extern const int nbLEDMouth;       // Total LEDs de la bouche (mouthLength * mouthHeight)
extern CRGB mouthLEDs[];           // Buffer LED de la bouche