#pragma once
#include <Arduino.h>

// L'équivalent du "setup()" d'Arduino pour l'initialisation de l'affichage
void voiceCommands_init(); 

// L'équivalent du "loop()" d'Arduino pour la mise à jour de l'affichage
void voiceCommands_loop();

// Spécifiques à ce module: 
#ifdef __cplusplus // Pour permettre l'utilisation en C++ et C
extern "C" {
#endif

void commandes_vocales_action(int id_commande);

#ifdef __cplusplus
}
#endif