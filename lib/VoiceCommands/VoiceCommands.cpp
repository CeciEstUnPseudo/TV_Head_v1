#include "VoiceCommands.h"
#include <Arduino.h>
#include <stdio.h>
// FreeRTOS software timer for non-blocking scheduling
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

extern void displayMode_handleRequest(String requestType, String name, int qteFrames);
extern void displayMode_mode(String mode);
extern void displayMode_modeVariation(String modeVariation);
extern void displayMode_setBrightness(int brightness, bool temporary);
extern int currentBrightness;
extern String mode;

// Optional non-blocking loop-based timer scaffolding
static bool flashBangPending = false;
static bool flashlightPending = false;
static unsigned long timerDeadline = 0;

void voiceCommands_loop()
{
    if (flashBangPending)
    {
        unsigned long now = millis();
        if ((long)(now - timerDeadline) >= 0)
        {
            displayMode_setBrightness(currentBrightness, false);
            flashBangPending = false;
        }
    }

    if (flashlightPending)
    {
        unsigned long now = millis();
        if ((long)(now - timerDeadline) >= 0)
        {
            displayMode_setBrightness(currentBrightness, false);
            flashlightPending = false;
        }
    }
}

void commandes_vocales_action(int id_commande)
{
    printf("Voice Command Received: ID %d\n", id_commande);
    displayMode_setBrightness(currentBrightness, false);
    switch (id_commande)
    {
    // 0-10: Changer le mode du visage
    case 0:
        printf("ID 0: Mode Gyro\n");
        displayMode_mode("gyro");
        break;

    case 1:
        printf("ID 1: Mode Manual\n");
        displayMode_mode("manual");
        break;

    case 2:
        printf("ID 2: Mode Telephone\n");
        displayMode_mode("phone");
        break;

    case 3:
        printf("ID 3: Mode Horloge\n");
        displayMode_mode("clock");
        break;

    case 4:
        printf("ID 4: Mode Température\n");
        displayMode_mode("temperature");
        break;

    case 5:
        printf("ID 5: Mode Texte\n");
        displayMode_mode("text");
        break;

    // 11-30: Variations Gyro
    case 11:
        printf("ID 11: Variation Gyro Normal\n");
        displayMode_modeVariation("normal");
        break;

    case 12:
        printf("ID 12: Variation Gyro Gremlin\n");
        displayMode_modeVariation("gremlin");
        break;

    case 13:
        printf("ID 13: Variation Gyro Hearts\n");
        displayMode_modeVariation("heart");
        break;

    case 14:
        printf("ID 14: Variation Gyro Angry\n");
        displayMode_modeVariation("angry");
        break;

    case 15:
        printf("ID 15: Variation Gyro Sad\n");
        displayMode_modeVariation("sad");
        break;

    case 16:
        printf("ID 16: Variation Gyro Happy\n");
        displayMode_modeVariation("happy");
        break;

    case 17:
        printf("ID 17: Variation Gyro Surprised\n");
        displayMode_modeVariation("surprised");
        break;

    case 18:
        printf("ID 18: Variation Gyro Tired\n");
        displayMode_modeVariation("tired");
        break;
    case 19:
        printf("ID 19: Variation Gyro Punctuation\n");
        displayMode_modeVariation("punctuation");
        break;
    case 20:
        printf("ID 20: Variation Gyro Cute\n");
        displayMode_modeVariation("cute");
        break;

    // 31-193 : Modes de visages "manuel" (non gyro, visage restera dans ce state jusqu'à nouvelle commande vocale)
    case 31:
        printf("ID 31: Fixe Éteint\n");
        displayMode_handleRequest("imageFixe", "Vide", 0);
        break;

    case 32:
        printf("ID 32: Animation Loading\n");
        displayMode_handleRequest("gif", "Gif_Chargement", 12);
        break;

    case 33:
        printf("ID 33: Animation Shutdown\n");
        break;

    case 34:
        printf("ID 34: Fixe Papillon\n");
        displayMode_handleRequest("imageFixe", "Papillon", 0);
        break;

    case 35:
        printf("ID 35: Fixe Cœur\n");
        displayMode_handleRequest("imageFixe", "Heart", 0);
        break;

    case 36:
        printf("ID 36: Fixe 10/10\n");
        displayMode_handleRequest("imageFixe", "TenTen", 0);
        break;

    case 37:
        printf("ID 37: Fixe Thumbs-up\n");
        displayMode_handleRequest("imageFixe", "ThumbsUp", 0);
        break;

    case 38:
        printf("ID 38: Fixe Canard\n");
        displayMode_handleRequest("imageFixe", "Canard", 0);
        break;

    case 39:
        printf("ID 39: Fixe Cheese\n");
        displayMode_handleRequest("imageFixe", "Cheese", 0);
        break;

    case 40:
        printf("ID 40: Fixe Cake\n");
        displayMode_handleRequest("imageFixe", "Cake", 0);
        break;

    case 41:
        printf("ID 41: Fixe Christmas Tree\n");
        displayMode_handleRequest("imageFixe", "ChristmasTree", 0);
        break;

    case 42:
        printf("ID 42: Fixe Christmas Gift\n");
        displayMode_handleRequest("imageFixe", "ChristmasGift", 0);
        break;

    case 43:
        printf("ID 43: Fixe Flashbang\n");

        // Set brightness to max for flashbang effect
        displayMode_handleRequest("imageFixe", "Flashbang", 0);
        displayMode_setBrightness(200, true);
        // Remet la brightness à 3 après 300ms sans bloquer les autres (donc pas un delay, comme on fait dans le .cpp du DisplayMode)
        flashBangPending = true;
        timerDeadline = millis() + 300UL;
        break;

    case 44:
        printf("ID 44: Fixe Flashlight\n");
        displayMode_handleRequest("imageFixe", "Flashlight", 0);
        displayMode_setBrightness(100, true);
        if (mode == "gyro") {
            flashlightPending = true;
            timerDeadline = millis() + 3000UL;
        }
        break;

    case 45:
        printf("ID 45: Fixe Cuh Pupal\n");
        displayMode_handleRequest("imageFixe", "CuhPupal", 0);
        break;

    // 194-199 : Commandes utilitaires
    case 194:
        printf("ID 194: Brightness NORMAL\n");
        displayMode_setBrightness(3, false);
        break;

    case 195:
        printf("ID 195: Brightness down\n");
        displayMode_setBrightness(currentBrightness - 2, false);
        break;

    case 196:
        printf("ID 196: Brightness up\n");
        displayMode_setBrightness(currentBrightness + 2, false);
        break;

    case 197:
        printf("ID 197: Brightness MINIMUM\n");
        displayMode_setBrightness(1, false);
        break;

    case 198:
        printf("ID 198: Brightness MAXIMUM\n");
        displayMode_setBrightness(200, false);
        break;
    // Nevermind (annuler la commande vocale)
    case 199:
        printf("ID 199 Nevermind / Forget about it\n");
        break;

    // Fail-safe au cas où un ID inconnu est reçu
    default:
        printf("Unknown command ID: %d\n", id_commande);
        break;
    }
}
