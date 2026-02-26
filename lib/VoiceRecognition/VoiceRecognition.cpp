#include "VoiceRecognition.h"
#include "VoiceCommands.h" // include for commandes_vocales_action
#include "DisplayDraw.h" // include for dessinBouche
#include "DisplayGlobals.h" // include for mouth region coordinates
#include "DisplayMode.h" // include for mode and modeVariation
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
extern "C"
{
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_board_init.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
}
#include "freertos/task.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include <stdio.h>
#include <stdlib.h>
#include <esp_process_sdkconfig.h>

// Variables globales externes utilisées ici

// Variables spécifiques à ce module utilisées ici
static const esp_afe_sr_iface_t *afe_handle = NULL;
static volatile int task_flag = 0;
static volatile int detect_flag = 0;
srmodel_list_t *models = NULL;
const int timeoutAfterWakeWordInSeconds = 3;
volatile bool WearerTalking = false;

// Loudness partagé entre feed_Task et detect_Task
// feed_Task l'écrit, detect_Task le lit pour décider si la bouche s'anime
static volatile int32_t currentAudioLoudness = 0;

// Seuil minimal pour filtrer le bruit de fond avant d'envoyer à l'AFE (commandes vocales)
// Assez bas pour capter un chuchotement
const int16_t MIC_SENSITIVITY = 200;

// Seuil pour animer la bouche — doit être plus élevé que MIC_SENSITIVITY
// pour que les chuchotements ne déclenchent pas l'animation
const int16_t TALKING_THRESHOLD = 2000;

static void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *)arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    int16_t *i2s_buff = (int16_t *)malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    const int sample_rate = 16000; // Aligné avec la config AFE
    TickType_t last_wake = xTaskGetTickCount();
    TickType_t feed_period_ticks = pdMS_TO_TICKS((audio_chunksize * 1000) / sample_rate);

    while (task_flag)
    {
        esp_get_feed_data(false, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

        // Calculer le volume moyen du chunk
        int32_t loudness = 0;
        for (int i = 0; i < audio_chunksize; i++) {
            loudness += abs(i2s_buff[i]);
        }
        loudness /= audio_chunksize;

        // Partager le volume avec detect_Task pour la décision d'animation de bouche
        currentAudioLoudness = loudness;

        // Filtrer le bruit de fond avant d'envoyer à l'AFE
        // MIC_SENSITIVITY est bas pour permettre les chuchotements
        if (loudness < MIC_SENSITIVITY) {
            memset(i2s_buff, 0, audio_chunksize * sizeof(int16_t) * feed_channel);
        }

        afe_handle->feed(afe_data, i2s_buff);
        vTaskDelayUntil(&last_wake, feed_period_ticks);
    }
    free(i2s_buff);
    vTaskDelete(NULL);
}

static void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *)arg;

    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_ENGLISH);
    if (!mn_name)
    {
        printf("ERROR: No MultiNet model found\n");
        vTaskDelete(NULL);
        return;
    }

    esp_mn_iface_t *multinet = esp_mn_handle_from_name(mn_name);
    model_iface_data_t *model_data = multinet->create(mn_name, timeoutAfterWakeWordInSeconds * 1000);
    int mu_chunksize = multinet->get_samp_chunksize(model_data);
    esp_mn_commands_update_from_sdkconfig(multinet, model_data);
    multinet->print_active_speech_commands(model_data);

    printf("WAITING FOR WAKE WORD\n");

    static TickType_t lastSpeechTick = 0;
    const TickType_t silenceTimeoutTicks = pdMS_TO_TICKS(400);

    while (task_flag)
    {
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if (!res) {
            WearerTalking = false;
            continue;
        }

        // WearerTalking = vrai seulement si l'AFE détecte une voix ET que le volume
        // est assez fort pour être une vraie voix (pas un chuchotement - so on peut chuchoter pour changer visage n such mais pas pour animer la bouche)
        if (res->vad_state == AFE_VAD_SPEECH && currentAudioLoudness > TALKING_THRESHOLD) {
            lastSpeechTick = xTaskGetTickCount();
            WearerTalking = true;
        } else {
            if ((xTaskGetTickCount() - lastSpeechTick) > silenceTimeoutTicks) {
                WearerTalking = false;
            }
        }

        // Check pour wakeword (commandes) — fonctionne peu importe WearerTalking
        if (res->wakeup_state == WAKENET_DETECTED || res->wakeup_state == WAKENET_CHANNEL_VERIFIED)
        {
            detect_flag = 1;
            multinet->clean(model_data);
            printf("WAKEWORD DETECTED\n");
        }

        if (detect_flag)
        {
            esp_mn_state_t mn_state = multinet->detect(model_data, res->data);
            if (mn_state == ESP_MN_STATE_DETECTED)
            {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                if (mn_result->num > 0)
                {
                    commandes_vocales_action(mn_result->command_id[0]);
                    afe_handle->enable_wakenet(afe_data);
                    detect_flag = 0;
                }
            }
            else if (mn_state == ESP_MN_STATE_TIMEOUT)
            {
                printf("Time's Up: Command Cancelled\n");
                afe_handle->enable_wakenet(afe_data);
                detect_flag = 0;
            }
        }
        // Pas de delay ici: fetch cadence naturellement la boucle (pour reagir le plus vite possible au wakeword + commandes)
    }

    multinet->destroy(model_data);
    vTaskDelete(NULL);
}

void voiceRecognition_init()
{
    Serial.println("Initialisation du module VoiceRecognition");
    models = esp_srmodel_init("model");
    if (!models)
    {
        Serial.println("[VoiceRecognition] Aucun modèle chargé depuis la partition 'model'. Abandon.");
        return;
    }

    char *wakenet_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    char *multinet_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_ENGLISH);
    bool has_wakenet = (wakenet_name != NULL);
    bool has_multinet = (multinet_name != NULL);
    Serial.printf("[VoiceRecognition] WakeNet: %s\n", has_wakenet ? wakenet_name : "ABSENT");
    Serial.printf("[VoiceRecognition] MultiNet EN: %s\n", has_multinet ? multinet_name : "ABSENT");
    if (!has_wakenet)
        Serial.println("[VoiceRecognition] WakeNet manquant -> pas de wake word.");
    if (!has_multinet)
        Serial.println("[VoiceRecognition] MultiNet manquant -> pas de commandes vocales.");
    ESP_ERROR_CHECK(esp_board_init(AUDIO_HAL_16K_SAMPLES, 1, 16));

    if (!has_wakenet || !has_multinet)
    {
        Serial.println("[VoiceRecognition] Modèles requis incomplets. Tâches non lancées. Consultez README_models_voice.txt.");
        return;
    }

    afe_config_t *afe_config = afe_config_init("MR", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);

    afe_config->aec_init = false;       // Pas d'annulation d'écho
    afe_config->se_init = true;         // Suppression de bruit active
    afe_config->vad_init = true;
    afe_config->vad_mode = VAD_MODE_3;  // 0–4, plus élevé = VAD moins sensible

    afe_handle = esp_afe_handle_from_config(afe_config);
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);

    task_flag = 1;
    xTaskCreatePinnedToCore(&detect_Task, "detect", 8 * 1024, afe_data, 7, NULL, 1);
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, afe_data, 5, NULL, 0);
    Serial.println("[VoiceRecognition] Tâches feed/detect lancées.");
}

void voiceRecognition_loop() {
    static unsigned long lastMouthUpdateMs = 0;
    static int mouthIndex = 0;
    static bool wasWearerTalking = false;
    const unsigned long mouthUpdateIntervalMs = 400;

    bool isAllowedFace = (modeVariation == "normal" || modeVariation == "gremlin" ||
                          modeVariation == "smile" || modeVariation == "happy" ||
                          modeVariation == "heart" || modeVariation == "angry" ||
                          modeVariation == "cute");
    bool shouldAnimateMouth = WearerTalking && mode == "gyro" && isAllowedFace;

    if (shouldAnimateMouth && !wasWearerTalking) {
        handleGyroUpdate();
    }

    if (shouldAnimateMouth) {
        unsigned long currentMs = millis();
        if (currentMs - lastMouthUpdateMs >= mouthUpdateIntervalMs) {
            lastMouthUpdateMs = currentMs;
            dessinBouche("Mouth_Speaking" + String(mouthIndex), mouthBeginsX, mouthBeginsY, mouthBeginsX + mouthLength, mouthBeginsY + mouthHeight);
            mouthIndex = (mouthIndex + 1) % 4;
        }
    } else if (wasWearerTalking) {
        handleGyroUpdate();
    }

    wasWearerTalking = shouldAnimateMouth;
}