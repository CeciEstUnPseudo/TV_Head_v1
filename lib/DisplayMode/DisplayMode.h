#pragma once
#include <Arduino.h>

// L'équivalent du "setup()" d'Arduino pour l'initialisation de l'affichage
void displayMode_init(); 

// L'équivalent du "loop()" d'Arduino pour la mise à jour de l'affichage
void displayMode_loop();

// Spécifiques à ce module: 
void handleGyroUpdate();
void displayMode_mode(String mode);
void displayMode_modeVariation(String modeVariation);
void displayMode_handleRequest(String requestType, String name, int qteFrames);
void displayMode_setBrightness(int desiredBrightness, bool temporary);

// Variables globales externes utilisées ici
extern int currentBrightness;
extern String mode;
extern String modeVariation;
