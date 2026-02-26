#include "DisplayMode.h"
#include <Arduino.h>
#include <map>
#include <FastLED.h>

// Variables specifiques
String mode = "gyro";            // Valeur par défaut au démarrage
String modeVariation = "normal"; // Valeur par défaut au démarrage
String gyroTilt = "normal";      // Valeur par défaut au démarrage

static bool pendingGyroReturn = false;         // Indique si un retour au mode gyro est appelé
static unsigned long gyroReturnDeadlineMs = 0; // Timestamp en ms pour le retour au mode gyro
int currentBrightness = 3; // Valeur par défaut au démarrage

// Variables/Fonctions externes
extern void imageFixe(String nomDessin);
extern void animation(String nomAnimation, int qteFrames);
extern void animTransitionVers(String nomAnimTransition, int qteFrames, String nomImage);
extern void gif(String nomAnimation, int qteFrames);
void handleGyroUpdate();

// Changement des "Parametres" du display (Mode, variation)
void displayMode_init()
{
    // Initialisation du module DisplayMode
    Serial.println("Initialisation du module DisplayMode");
}

void displayMode_mode(String mode)
{
    // Changer le mode
    Serial.print("Changement du mode d'affichage vers: ");
    Serial.println(mode);
    ::mode = mode;
    if (::mode == "gyro")
    {
        // Si on est en mode gyro, on doit appliquer le dessin/animation correspondant tout de suite
        Serial.println("Mode actif est 'gyro', on fait le dessin/animation correspondant");
        // Appeler le dispatch pour appliquer le changement
        handleGyroUpdate();
    }
}

void displayMode_modeVariation(String modeVariation)
{
    // Changer la modeVariation du mode d'affichage
    // Pour l'instant, QUE utile au mode gyro car les autres n'ont pas de variations, donc pas besoin de check si le mode est actif ou si le mode actif supporte cette variation
    Serial.print("Changement de la modeVariation vers: ");
    Serial.println(modeVariation);
    ::modeVariation = modeVariation;

    if (::mode == "gyro")
    {
        // Si on est en mode gyro, on doit appliquer le changement de variation tout de suite
        Serial.println("Mode actif est 'gyro', on fait le dessin/animation correspondant au nouveau modeVariation");
        // Appeler le dispatch pour appliquer le changement
        handleGyroUpdate();
    }
}

// Ici, on catch les requetes de Serveur/Les requetes qui sont directement imageFixe, animation, et gif, et on les active:
// SOIT directement (si mode manuel),
// SOIT on les active "temporairement" (3s pour imageFixe, 1 fois animation, on affiche pas si gif, on retourne tjrs au mode gyro) (si mode gyro)
void displayMode_handleRequest(String requestType, String name, int qteFrames = 0)
{
    Serial.print("DisplayMode a reçu une requete de type ");
    Serial.print(requestType);
    Serial.print(" avec le nom ");
    Serial.println(name);

    // If Current Mode is manual
    if (::mode == "manual")
    {
        // Mode manuel: on applique directement la requete
        Serial.println("Mode actif est 'manual', on applique directement la requete");
        if (requestType == "imageFixe")
        {
            imageFixe(name);
        }
        else if (requestType == "animation")
        {
            animation(name, qteFrames);
        }
        else if (requestType == "gif")
        {
            gif(name, qteFrames);
        }
        else
        {
            Serial.println("Type de requete inconnu!");
        }

        // If Current Mode is gyro
    }
    else if (::mode == "gyro")
    {
        // Mode gyro: on applique la requete temporairement
        Serial.println("Mode actif est 'gyro', on applique la requete temporairement et revenir au gyro après");
        if (requestType == "imageFixe")
        {
            // Afficher l'image fixe temporairement puis revenir au mode gyro sans bloquer
            imageFixe(name);
            // On lance le timer de 3000 ms
            pendingGyroReturn = true;
            gyroReturnDeadlineMs = millis() + 3000UL;
        }
        else if (requestType == "animation")
        {
            // Jouer l'animation une fois, puis revenir au mode gyro
            animation(name, qteFrames);
            // Le retour au mode gyro sera géré dans animationTick() après la fin de l'animation
        }
        else if (requestType == "gif")
        {
            // Ignorer les gifs en mode gyro, rester en mode gyro
            Serial.println("Ignorer le gif en mode 'gyro', rester en mode 'gyro'");
        }
        else
        {
            Serial.println("Type de requete inconnu!");
        }

        // Pas manuel ni gyro (allonger ici quand on aura fait les autres modes!)
    }
    else
    {
        Serial.println("Mode inconnu!");
    }
}

// GYRO -- Fonctions de ses variations + tilts + dispatch qui s'active au changement de sa variation + tilt

// Variation Normal
void gyroNormal_Normal() { imageFixe("Fixed_Face_Neutral"); }
void gyroNormal_Left() { animation("GyroNormal_Wink", 9); }
void gyroNormal_Right() { animTransitionVers("GyroNormal_Confused", 7, "GyroNormal_Confused_Final"); } 
void gyroNormal_Forward() { imageFixe("Fixed_Face_Neutral"); }
void gyroNormal_Backward() { gif("GyroNormal_Gif_Laugh", 4); }

// Variation Gremlin
void gyroGremlin_Forward() { animTransitionVers("GyroGremlin_Gremlin", 7, "GyroGremlin_Gremlin_Final"); }

// Variation Punctuation
void gyroPunctuation_Normal() { gif("Gif_Chargement", 12); } // Loading en normal
void gyroPunctuation_Left() { imageFixe("GyroPunctuation_Left"); } // !!
void gyroPunctuation_Right() { imageFixe("GyroPunctuation_Right"); } // ??
void gyroPunctuation_Forward() { gif("GyroPunctuation_Forward", 8); } // . . . .
void gyroPunctuation_Backward() { imageFixe("GyroPunctuation_Backward"); } // ?!]

// Variation Happy
void gyroHappy_Normal() { imageFixe("GyroSmile_Normal"); }
void gyroHappy_Left() { animation("GyroSmile_Wink", 9); }
void gyroHappy_Right() { animTransitionVers("GyroSmile_Confused", 7, "GyroSmile_Confused_Final"); } 
void gyroHappy_Forward() { imageFixe("Fixed_Face_BigSmile"); }

// Variation Heart
void gyroHeart_Normal() { imageFixe("GyroHeart_Normal"); }
void gyroHeart_Left() { imageFixe("GyroHeart_SideTilt"); }
void gyroHeart_Right() { imageFixe("GyroHeart_SideTilt"); } 
void gyroHeart_Forward() { imageFixe("GyroHeart_FrontTilt"); }
void gyroHeart_Backward() { gif("GyroHeart_Laugh", 4); }

// Variation Angry
void gyroAngry_Normal() { imageFixe("GyroAngry_Normal"); }
void gyroAngry_Left() { animation("GyroAngry_Wink", 9); }
void gyroAngry_Right() { animTransitionVers("GyroAngry_Tongue", 4, "GyroAngry_Tongue_Final"); }
void gyroAngry_Forward() { imageFixe("GyroAngry_FrontTilt"); }
void gyroAngry_Backward() { gif("GyroAngry_Laugh", 4); }

// Variation Cute
void gyroCute_Normal() { imageFixe("GyroCute_Normal"); }
void gyroCute_Left() { animation("GyroCute_Wink", 9); }
void gyroCute_Right() { imageFixe("GyroCute_Right"); }
void gyroCute_Forward() { imageFixe("GyroCute_Forward"); }

typedef void (*FonctionDeGyro)(); // Le pointer va tjrs donner quelque chose qui est une fonction void qui ne retourne aucune valeur
// --- Table des variations du mode gyro (qui contiennent chacunes leurs différents "tilts" (requetes)) ---
std::map<String, std::map<String, FonctionDeGyro>> modesTable = {
    {"normal", {                               // Variation
                {"normal", gyroNormal_Normal}, // Tilt "normal" of mode "normal"
                {"left", gyroNormal_Left},     // Tilt "left" of mode "normal"
                {"right", gyroNormal_Right},
                {"forward", gyroNormal_Forward},
                {"backward", gyroNormal_Backward}}},
    {"gremlin", {
                    {"forward", gyroGremlin_Forward}, // This one only has an override for "forward", the other "tilts" will default to "normal mode"
                }},
    {"punctuation", {
                    {"normal", gyroPunctuation_Normal}, // This one only has an override for "forward", the other "tilts" will default to "normal mode"
                    {"left", gyroPunctuation_Left},     // Tilt "left" of mode "normal"
                    {"right", gyroPunctuation_Right},
                    {"forward", gyroPunctuation_Forward},
                    {"backward", gyroPunctuation_Backward}
                }},
    {"happy", {
                    {"normal", gyroHappy_Normal}, // This one only has an override for "forward", the other "tilts" will default to "normal mode"
                    {"left", gyroHappy_Left},     // Tilt "left" of mode "normal"
                    {"right", gyroHappy_Right},
                    {"forward", gyroHappy_Forward},
                }},
    {"heart", {
                    {"normal", gyroHeart_Normal}, // This one only has an override for "forward", the other "tilts" will default to "normal mode"
                    {"left", gyroHeart_Left},     // Tilt "left" of mode "normal"
                    {"right", gyroHeart_Right},
                    {"forward", gyroHeart_Forward},
                    {"backward", gyroHeart_Backward}
                }},
    {"angry", {
                    {"normal", gyroAngry_Normal}, // This one only has an override for "forward", the other "tilts" will default to "normal mode"
                    {"left", gyroAngry_Left},     // Tilt "left" of mode "normal"
                    {"right", gyroAngry_Right},
                    {"forward", gyroAngry_Forward},
                }},
    {"cute", {
                    {"normal", gyroCute_Normal}, // This one only has an override for "forward", the other "tilts" will default to "normal mode"
                    {"left", gyroCute_Left},     // Tilt "left" of mode "normal"
                    {"right", gyroCute_Right},
                    {"forward", gyroCute_Forward},
                }},
                
    // Template doesnt exist (duh, used for templates) its functions like gyroTemplate_Normal were not written so it'll amount to an error in compiler if not commented
    // {"template", {
    // {"normal",   gyroTemplate_Normal}, // Tilt "normal" of mode "template"
    // {"left",     gyroTemplate_Left}, // Tilt "left" of mode "template"
    // {"right",    gyroTemplate_Right}, // Tilt "right" of mode "template"
    // {"forward",  gyroTemplate_Forward}, // Tilt "forward" of mode "template"
    // {"backward", gyroTemplate_Backward} // Tilt "backward" of mode "template"
    // }}
};

void handleGyroUpdate()
{
    if (::mode != "gyro")
    {
        Serial.println("handleGyroUpdate appelé alors que le mode n'est pas 'gyro'! Ignorer.");
        return;
    }

    if (modesTable.count(modeVariation) && modesTable[modeVariation].count(gyroTilt))
    {
        Serial.print("Combo modeGyro + gyroTilt Trouvé || Mode = ");
        Serial.print(modeVariation);
        Serial.print(" | gyroTilt = ");
        Serial.println(gyroTilt);
        modesTable[modeVariation][gyroTilt](); // Fait ce gyrogyroTilt de ce mode
    }
    else
    {
        Serial.print("Combo modeGyro + gyroTilt introuvable | Go to normal + ");
        Serial.println(gyroTilt);
        modesTable["normal"][gyroTilt](); // Sinon fait le gyroTilt actuel du mode par défaut
    }
}

// Fin GYRO

/////////////////////// FONCTIONS UTILITAIRES //////////////////////////////////////
void displayMode_setBrightness(int desiredBrightness, bool temporary) {
    // Set the brightness of the display (0-200) 
    if (temporary){ // Aka flashlight ou flashbang
        FastLED.setBrightness(desiredBrightness);
        FastLED.show();
    } else {
        if (desiredBrightness < 1) { // Eviter les valeurs hors limites
            desiredBrightness = 1;
        } else if (desiredBrightness > 200) {
            desiredBrightness = 200;
        }
        currentBrightness = desiredBrightness;
        Serial.println(desiredBrightness);
        FastLED.setBrightness(desiredBrightness);
        FastLED.show();
        Serial.print("Setting display brightness to: ");
    }
}
/////////////////////// FIN FONCTIONS UTILITAIRES //////////////////////////////////////

// --- Gestion du retour non bloquant au mode gyro après une requête temporaire ---
// Variables de scheduling (scope fichier)

void displayMode_loop()
{
    // Vérifier si un retour programmé est dû
    if (pendingGyroReturn && (long)(millis() - gyroReturnDeadlineMs) >= 0)
    {
        pendingGyroReturn = false;
        // Revenir au mode gyro: réappliquer dessin selon variation + tilt actuels
        handleGyroUpdate(); // Timer est fini, donc on revient au mode gyro
    }
    // Ici on pourrait ajouter d'autres tâches périodiques non bloquantes plus tard
}


////////////////////////////////////////////////////////////////////////////////