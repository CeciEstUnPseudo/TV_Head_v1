#include <Arduino.h>

#include "LEDs.h"
#include "Gyro.h"
#include "DisplayMode.h"
#include "DisplayDraw.h"
#include "DisplayGlobals.h"
#include "VoiceRecognition.h"
#include "VoiceCommands.h"
#include "ServeurSetup.h"
#include "ServeurRequetes.h"

#include "WiFi.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include <FastLED.h>
#include <SD.h>
#include <sd_defines.h>
#include <sd_diskio.h>
#include "nvs_flash.h"
#include "esp_partition.h"
#include "esp_crc.h"

// Buffer statique pour audit partition modèle (audit partition modele est le seul endroit où on lit cette partition)
static uint8_t g_model_part_buf[1024];

void startupServeur();
void startupLEDs();

// Variables globales utilisées un peu partout:

// LED matrix globals sont maintenant définis dans DisplayGlobals.cpp
// On conserve seulement les variables locales supplémentaires
bool matrixFilLDirection = true; // Sens de remplissage si nécessaire

// Externes
extern void animationTick();
extern void gyroSetup();
extern void gyroDetection();
extern void voiceRecognition_loop();

// Hotspot telephone
const char *ssid = "TV-Head";      // Nom
const char *password = "12345678"; // mdp
// INSTANCE DU SERVEUR ASYNCHRONE (Port 80)
AsyncWebServer monServeur(80);

/**
 * @file main.c
 * @brief Main pour TV head - L'équivalent du app_main ou setup pour tout
 *
 * Kickstart les modules + boucle principale
 *
 **/

extern "C" void app_main()
{
    // Initialisation complète du core Arduino (assure NVS, netif, événements) avant utilisation API Arduino
    initArduino();
    Serial.begin(115200);
    Serial.println("Initialisation du module Arduino App");

    Serial.println("Initialisation des modules");

    // NVS déjà initialisé par initArduino(); on garde un check optionnel si besoin
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Montage du système de fichiers pour dessins CSV (SPIFFS) avant l'initialisation de l'affichage (mode + draw)
    if (!SPIFFS.begin(true))
    {
        Serial.println("[FS] SPIFFS mount failed");
    }
    else
    {
        Serial.println("[FS] SPIFFS mounted");
    }

    // FastLED setup avec extern const pinData & nbLEDTotal
    displayGlobals_init();

    // Dessiner avec LEDs
    displayMode_init();

    // Gyro - Equivalent de gyroSetup
    gyro_init();

    // Voix
    // Initialisation de la reconnaissance vocale
    const esp_partition_t *model_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "model");
    if (model_part)
    {
        Serial.printf("[ModelPartition] Taille: %u bytes Offset: 0x%08X\n", model_part->size, model_part->address);
        uint32_t to_read = model_part->size < (64 * 1024) ? model_part->size : (64 * 1024); // Limite à 64KB pour CRC échantillon
        // Utiliser un chunk réduit pour limiter l'utilisation mémoire par itération
        const uint32_t chunk = sizeof(g_model_part_buf); // 1024
        uint32_t crc = 0;
        uint32_t pos = 0;
        while (pos < to_read)
        {
            uint32_t rd = (to_read - pos) < chunk ? (to_read - pos) : chunk;
            if (esp_partition_read(model_part, pos, g_model_part_buf, rd) == ESP_OK)
            {
                crc = esp_crc32_le(crc, g_model_part_buf, rd);
            }
            else
            {
                Serial.println("[ModelPartition] Lecture échouée");
                break;
            }
            pos += rd;
        }
        Serial.printf("[ModelPartition] CRC32 (premiers %u bytes): 0x%08X\n", to_read, crc);
        if (esp_partition_read(model_part, 0, g_model_part_buf, 32) == ESP_OK)
        {
            Serial.print("[ModelPartition] Premiers 32 octets: ");
            for (int i = 0; i < 32; ++i)
            {
                Serial.printf("%02X", g_model_part_buf[i]);
            }
            Serial.println();
        }
    }
    else
    {
        Serial.println("[ModelPartition] Partition 'model' introuvable");
    }

    voiceRecognition_init();

    // Serveur (mobile) (disabled for now)
    //serveurSetup_init();

    //serveurRequetes_init();

    Serial.println("Démarrage de la boucle principale");
    while (1)
    {
        // Boucle non bloquante: chaque sous-système gère sa propre loop et son timing interne
        animationTick();          // Animation: appelée à chaque tour (peut être optimisée avec un gating interne si nécessaire)
        gyro_loop();              // Gyro: contient maintenant un contrôle de fréquence interne (voir Gyro.cpp)
        displayMode_loop();       // DisplayMode: déjà non bloquant
        voiceCommands_loop();  // voiceCommands: déjà non bloquant
        voiceRecognition_loop(); // voiceRecognition: déjà non bloquant
        // Ajout d'un très léger yield pour éviter le déclenchement du task watchdog (IDLE0 doit tourner)
        // Sans cela FastLED + boucle serrée peuvent empêcher IDLE de feed le WDT.
        vTaskDelay(1); // 1 tick (~1ms) coopératif
    }
}
