#pragma once
#include <Arduino.h>

#ifdef __cplusplus // Pour permettre l'utilisation en C++ et C
extern "C" {
#endif

#include "esp_afe_sr_iface.h"
// L'équivalent du "setup()" d'Arduino pour l'initialisation de l'affichage
void voiceRecognition_init(); 
void voiceRecognition_loop();

#ifdef __cplusplus
}
#endif

extern volatile bool WearerTalking;
