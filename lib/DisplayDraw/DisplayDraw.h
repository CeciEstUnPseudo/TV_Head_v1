#pragma once
#include <Arduino.h>
#include <FastLED.h>

// L'équivalent du "setup()" d'Arduino pour l'initialisation de l'affichage
void displayDraw_init(); 

// L'équivalent du "loop()" d'Arduino pour la mise à jour de l'affichage
void displayDraw_loop();

// Spécifiques à ce module: 
void dessin(String nomDessin);
void eteindre_matrix();
void imageFixe(String nomDessin);
void animation(String nomAnimation, int qteFrames);
void gif(String nomAnimation, int qteFrames);
void animTransitionVers(String nomAnimTransition, int qteFrames, String nomImage);
void startAnimation(String nomAnimation, int qteFrames, unsigned long intervalMillis, bool gifInfini);
void stopAnimation();
void animationTick();
void dessinBouche(String nomDessin, int x1, int y1, int x2, int y2);



