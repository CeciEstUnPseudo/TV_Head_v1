/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


/**
 * Moi qui m'expliquer via online ressources comment tout ça marche:
 * 3 piliers: AFE (Audio Front End), WakeNet (Mot de réveil) et MultiNet (Reconnaissance de commandes)
 * 
 * (S'intègre au backend BSP pour l'initialisation de la carte et de l'I2S via bsp_board_init, bsp_get_feed_data et bsp_get_feed_channel)
 * 
 * AFE : Prétraitement audio, suppression de l'écho acoustique (AEC), amélioration du signal (SE) et détection du mot de réveil (WakeNet) qui va activer la reconnaissance des commandes vocales (MultiNet) pour un temps limité de 6 secondes (configurable via timeoutAfterWakeWordInSeconds)
 * 
 * WakeNet : Modèle de détection du mot de réveil, intégré dans l'AFE. Lorsqu'il détecte le mot de réveil, il active MultiNet pour la reconnaissance des commandes vocales 
 * 
 * 
 * MultiNet : Modèle de reconnaissance des commandes vocales, qui reconnaît les commandes prédéfinies après l'activation par WakeNet. Commandes = phrases courtes comme "allumer la lumière", "éteindre la lumière", etc. en alphabet phonétique international (API). Chaque commande a un ID unique qui est renvoyé lors de la reconnaissance. Donc on peut mapper chaque ID à une action spécifique dans l'application (jusqu'à 200 commandes).
 * 
 * 
 * 
 * 
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_board_init.h"
#include "speech_commands_action.h"
#include "model_path.h"
#include "esp_process_sdkconfig.h"
#include "esp_mn_speech_commands.h"

int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;
static volatile int task_flag = 0;
srmodel_list_t *models = NULL;
const int timeoutAfterWakeWordInSeconds = 3;

/**
 * 
 * @brief Function to do something based on recognized command ID
    * @param command_id Recognized command ID
 */

void commandes_vocales_action(int id_commande)
{
    switch (id_commande) {
        // 0-10: Changer le mode du visage
        case 0:
            printf("ID 0: Mode Gyro \n"); // Contrôles via gyro (head tilt)
            // Mode du visage -> Gyro
            break;
        case 1:
            printf("ID 1: Mode Manuel \n"); // Contrôles via voix
            // Mode du visage -> Manuel
            break;
        case 2:
            printf("ID 2: Mode Telephone \n"); // Contrôles via téléphone
            // Mode du visage -> Telephone
            break;
        case 3:
            printf("ID 3: Mode Horloge \n"); // Affichera l'heure actuelle
            // Mode du visage -> Horloge
            break;
        case 4: 
            printf("ID 4: Mode Température \n"); // Affichera la température actuelle
            // Mode du visage -> Température
            break;
        case 5: 
            printf("ID 5: Mode Texte \n"); // Affichera du texte qui défilera
            // Mode du visage -> Texte
            break;


        // 11-30: Variations Gyro
        case 11:
            printf("ID 11: Variation Gyro Normal \n");
            // Variation Gyro -> Mode "Normal"
            break;
        case 12:
            printf("ID 12: Variation Gyro Gremlin \n");
            // Variation Gyro -> Mode "Gremlin"
            break;
        case 13:
            printf("ID 13: Variation Gyro Coeurs \n");
            // Variation Gyro -> Mode "Coeurs"
            break;
        case 14:
            printf("ID 14: Variation Gyro Fâché \n");
            // Variation Gyro -> Mode "Fâché"
            break;
        case 15:
            printf("ID 15: Variation Gyro Triste \n");
            // Variation Gyro -> Mode "Triste"
            break;
        case 16:
            printf("ID 16: Variation Gyro Heureux \n");
            // Variation Gyro -> Mode "Heureux"
            break;
        case 17:
            printf("ID 17: Variation Gyro Surpris \n");
            // Variation Gyro -> Mode "Surpris"
            break;
        case 18:
            printf("ID 18: Variation Gyro Fatigué \n");
            // Variation Gyro -> Mode "Fatigué"
            break;


        // 31-199 : Modes de visages "manuel" (non gyro, visage restera dans ce state jusqu'à nouvelle commande vocale)
        case 31:
            printf("ID 31: Fixe Éteint \n");
            // Visage Manuel  -> Fixe "Éteint"
            break;
        case 32:
            printf("ID 32: Animation Loading \n");
            // Visage Manuel  -> Animation "Loading"
            break;
        case 33:
            printf("ID 33: Animation Shutdown \n");
            // Visage Manuel  -> Transition "Shutdown" vers Fixe "Éteint"
            break;
        case 34:
            printf("ID 34: Fixe Papillon \n");
            // Visage Manuel -> Fixe "Papillon"
        break;
        case 35:
            printf("ID 35: Fixe Cœur \n");
            // Visage Manuel -> Fixe "Cœur"
            break;
        case 36:
            printf("ID 36: Fixe 10/10 \n");
            // Visage Manuel -> Fixe "10/10"
            break;
        case 37:
            printf("ID 37: Fixe Thumbs-up \n");
            // Visage Manuel -> Fixe "Thumbs-up"
            break;
        case 38:
            printf("ID 38: Fixe Canard \n");
            // Visage Manuel -> Fixe "Canard"
            break;

        
        case 199:
        printf("ID 199 Nevermind / Forget about it\n");
        break;
        default:
            printf("Unknown command ID: %d\n", id_commande); // should not happen since we make sure its a recognized command aka an existing ID, but just in case
            break;
    }
}
/**
 * 
 * @brief Task to feed audio data to the AFE (Audio Front End)
 * This task continuously reads audio data from the I2S interface and feeds it to the AFE to process
 * 
 * @param arg Pointer to esp_afe_sr_data_t structure
 * 
 * 
 * 
 */
void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    assert(nch <= feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (task_flag) {
        esp_get_feed_data(false, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

        afe_handle->feed(afe_data, i2s_buff);
    }
    if (i2s_buff) {
        free(i2s_buff);
        i2s_buff = NULL;
    }
    vTaskDelete(NULL);
}

/**
 * 
 * @brief Task to detect wake words and speech commands
 * This task continuously fetches processed audio data from the AFE and uses MultiNet to detect speech commands after a wake word is detected
 * 
 * So basically takes data from AFE (which has it from feed_Task), checks for wake word detection, and if detected, uses MultiNet to recognize commands
 * 
 * @param arg Pointer to esp_afe_sr_data_t structure
 * 
 */
void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg; // get AFE data from argument
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data); // get AFE fetch chunk size
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_ENGLISH); // get MultiNet model name for English

    if (mn_name == NULL) { 
        printf("ERROR: No MultiNet model found (mn_name is NULL)\n"); // exit the task or handle gracefully
        vTaskDelete(NULL); 
        return; 
    }

    printf("Using MultiNet model: %s\n", mn_name); // print the MultiNet model name being used
    esp_mn_iface_t *multinet = esp_mn_handle_from_name(mn_name); // get MultiNet interface from model name
    model_iface_data_t *model_data = multinet->create(mn_name, (timeoutAfterWakeWordInSeconds*1000)); // create MultiNet model data with timeout so it stops listening after that time until woken up again

    // Ici on s'occupe de convertir la chaine phonétique en commandes 

    /**
     * // Commande: 
     * cd 'C:\Espressif\projects\esp-skainet\components\esp-sr\tool'; python multinet_g2p.py -t
     * 
     * // CHAINE NORMALE DE TEXTE
     * "mode gyro, gyro; mode manual, manual; mode phone, phone; mode clock, clock; mode text, text; placeholder6;placeholder7;placeholder8;placeholder9;placeholder10;gyro normal; gyro gremlin, gyro goblin; gyro heart; gyro angry; gyro sad; gyro happy; gyro surprised; gyro tired;placeholder19;placeholder20;placeholder21;placeholder22;placeholder23;placeholder24;placeholder25;placeholder26;placeholder27;placeholder28;placeholder29;placeholder30;fixed off, off, close, closed; animation loading, anim loading, loading; animation shutdown, anim shutdown, shutdown; fixed butterfly, butterfly; fixed heart, heart; fixed ten out of ten, ten out of ten; fixed thumbs-up, thumbs-up; fixed duck, duck;"
     * 
     * 
     * // CHAINE PHONÉTIQUE
     * MbD qiRb,qiRb;MbD MaNYocL,MaNYocL;MbD FbN,FbN;MbD KLnK,KLnK;MbD TfKST,TfKST;PLaKcSheLDfSgZ;PLaShbLDeSfRN;PLdSmheLTgD;PLaSmheLDNYnR;PLdscLDnNT;qiRb NeRMcL;qiRb GRfMLcN,qiRb GnBLgN;qiRb hnRT;qiRb alGRm;qiRb SaD;qiRb haPm;qiRb SkPRiZD;qiRb TikD;PaLcShnDcNfLTmN;PLdscFTeLTkm;PLdscLeTheRNm;PaLcShuThbLDmc;PLdShnLThkNm;PLdSKeLhkTfR;PLdShnLThfFSTk;PaLcKcFfpkcLDZ;PLdShnLTcFSfND;PLdSpWeLThiTk;PLdscLDeNTk;PLdscFeLTkhm;FgKST eF,eF,KLbS,KLbZD;aNcMdscN LbDgl,aNgM LbDgl,LbDgl;aNcMdscN scTDtN,aNgM scTDtN,scTDtN;FgKST BcTkFLi,BcTkFLi;FgKST hnRT,hnRT;FgKST TfN tT cV TfN,TfN tT cV TfN;FgKST vcMScP,vcMScP;FgKST DcK,DcK;
     * 
     * On change le stuff dans commands_en.txt (esp-sr - model - multinet_model - fst - commands_en.txt)
     * 
     * On on va dans menuconfig i guess cause commands_en est frustrée et veut pas
     * 
     *  
     * Exemple: mode gyro (MbD qiRb) et gyro (qiRb) sont les deux pour voice command ID 0
     * Les IDs sont séparés par des ; et les options par ID par des ,
     * 

     https://docs.espressif.com/projects/esp-sr/en/latest/esp32/speech_command_recognition/README.html
     */




   

    int mu_chunksize = multinet->get_samp_chunksize(model_data); // get MultiNet sample chunk size
    esp_mn_commands_update_from_sdkconfig(multinet, model_data); // Add speech commands from sdkconfig to MultiNet model so it knows what to listen for
    assert(mu_chunksize == afe_chunksize); // ensure chunk sizes match between AFE and MultiNet
    //print active speech commands
    multinet->print_active_speech_commands(model_data); // print the active speech commands for debugging

    printf("------------ Détection Allumé - On attend le WakeWord ------------\n"); // indicate listening state to check for wake word
    while (task_flag) { // main loop while task is running to keep checking for wake word and commands
        afe_fetch_result_t* res = afe_handle->fetch(afe_data);  // fetch processed audio data from AFE
        if (!res || res->ret_value == ESP_FAIL) { // check for fetch errors
            printf("fetch error!\n");
            break;
        }

        if (res->wakeup_state == WAKENET_DETECTED) { // check if wake word is detected
            printf("WAKEWORD DETECTED\n");
            detect_flag = 1; // set flag to indicate wake word detected and ready for command recognition
	    multinet->clean(model_data); // reset multinet when wake word is detected to clear previous state
        } else if (res->wakeup_state == WAKENET_CHANNEL_VERIFIED) { // check if wake word channel is verified

            detect_flag = 1; // set flag to indicate wake word detected and ready for command recognition
            printf("AFE_FETCH_CHANNEL_VERIFIED, channel index: %d\n", res->trigger_channel_id);
            // afe_handle->disable_wakenet(afe_data);
            // afe_handle->disable_aec(afe_data);
        }

        if (detect_flag == 1) { // if wake word detected, proceed to speech command recognition
            esp_mn_state_t mn_state = multinet->detect(model_data, res->data);

            if (mn_state == ESP_MN_STATE_DETECTING) { // still detecting, continue fetching more data
                continue;
            }

            if (mn_state == ESP_MN_STATE_DETECTED) { // Commande reconnue après le mot de réveil
                esp_mn_results_t *mn_result = multinet->get_results(model_data); // Les hypotheses

                // Imprimer toutes les hypothèses (pour dev), mais n'exécuter qu'une seule action (Top-1)
                for (int i = 0; i < mn_result->num; i++) {
                    printf("TOP %d, command_id: %d, phrase_id: %d, string: %s, prob: %f\n",
                        i+1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->string, mn_result->prob[i]);
                }

                // Faire uniquement la meilleure hypothèse (Top-1)
                if (mn_result->num > 0) {
                    commandes_vocales_action(mn_result->command_id[0]);

                    // Remettre wakenet en mode actif pour écouter la prochaine commande (wake word -> commande ofc as always)
                    afe_handle->enable_wakenet(afe_data);
                    detect_flag = 0;
                }

            }

            if (mn_state == ESP_MN_STATE_TIMEOUT) { // timeout waiting for command after wake word (happens after set timeout)
                esp_mn_results_t *mn_result = multinet->get_results(model_data); // get results (likely empty)
                printf("timeout, string:%s\n", mn_result->string); // print timeout message
                afe_handle->enable_wakenet(afe_data);
                detect_flag = 0;
                printf("\n------------ Temps pour commande écoulé - Retour à la détection - On attend le WakeWord ------------\n"); // indicate back to listening state (waiting for wake word)
                continue;
            }
        }
    }
    if (model_data) { // cleanup MultiNet model data on exit
        multinet->destroy(model_data); // destroy MultiNet model data
        model_data = NULL; // set pointer to NULL
    }
    printf("Task exit\n"); // indicate task exit
    vTaskDelete(NULL);
}

void app_main()
{
    models = esp_srmodel_init("model"); // partition label defined in partitions.csv
    ESP_ERROR_CHECK(esp_board_init(AUDIO_HAL_16K_SAMPLES, 1, 16));

    
    // ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));
#if defined CONFIG_ESP32_KORVO_V1_1_BOARD
    led_init();
#endif

#if CONFIG_IDF_TARGET_ESP32
    printf("This demo only support ESP32S3\n");
    return;
#else
    // Use new AFE v2 API: initialize afe_config via afe_config_init
    // Input format: "M" = microphone, "R" = playback reference, "N" = unused
    // For the devkit (1 mic + 1 ref) use "MR"
    afe_config_t *afe_config = afe_config_init("MR", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    afe_config_print(afe_config);
    // set wake model name discovered from models
    afe_config->wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
#if defined CONFIG_ESP32_S3_BOX_BOARD || defined CONFIG_ESP32_S3_EYE_BOARD || CONFIG_ESP32_S3_DEVKIT_C
    afe_config->aec_init = false;
    #if defined CONFIG_ESP32_S3_EYE_BOARD || CONFIG_ESP32_S3_DEVKIT_C
        afe_config->pcm_config.total_ch_num = 2;
        afe_config->pcm_config.mic_num = 1;
        afe_config->pcm_config.ref_num = 1;
    #endif
#endif
    // create afe handle from config and then create afe data instance
    afe_handle = esp_afe_handle_from_config(afe_config);
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);
#endif

    task_flag = 1;
    xTaskCreatePinnedToCore(&detect_Task, "detect", 8 * 1024, (void*)afe_data, 5, NULL, 1);
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
#if defined  CONFIG_ESP32_S3_KORVO_1_V4_0_BOARD
    xTaskCreatePinnedToCore(&led_Task, "led", 2 * 1024, NULL, 5, NULL, 0);
#endif
    /* playback task creation removed: disable automatic sound playback on detections */

    // // You can call afe_handle->destroy to destroy AFE.
    // task_flag = 0;

    // printf("destroy\n");
    // afe_handle->destroy(afe_data);
    // afe_data = NULL;
    // printf("successful\n");
}
