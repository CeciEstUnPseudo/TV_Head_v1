#pragma once
#include <FastLED.h>
#include <Arduino.h>


// Spécifiques à ce module: 
CRGB stringToCRGB(String couleur);
void setLEDColor(int y, int x, CRGB couleurVoulue);
CRGB getLEDColor(int y, int x);
int getLED(int y, int x);
