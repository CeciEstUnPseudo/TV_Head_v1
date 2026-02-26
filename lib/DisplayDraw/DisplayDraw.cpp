#include "DisplayDraw.h"
#include <Arduino.h>
#include <FastLED.h>
#include "SPIFFS.h"
#include <map>
#include "DisplayMode.h" // for mode and modeVariation
#include "VoiceRecognition.h" // for WearerTalking

// Variables globales externes utilisées ici
#include "DisplayGlobals.h"  // fournit pinData, matrixLength, matrixHeight, nbLEDTotal, maMatrixLEDs
extern String gyroTilt; // renommer tilt -> gyroTilt cohérence (si utilisé)

// Variables spécifiques à ce module utilisées ici
  // Variables et fonctions pour le controle des LEDs 
  CRGB stringToCRGB(String couleur);
  void setLEDColor(int y, int x, CRGB couleurVoulue);
  CRGB getLEDColor(int y, int x);
  int getLED(int y, int x);

  // Variables pour gérer les dessins et animations
  String dernierDessinVoulu = "Canard"; // Valeur par défaut au démarrage
  String retourApresAnimation; // Dessin à revenir après l'animation one-time
  String typeDeRetour = "";
  int retourQteFrames;

  // Controles et settings des animations
  bool animPlaying = false;
  String animName = "";
  int animFrameIndex = 0;
  int animQteFrames = 0;
  int animStopped = false;
  
  // Info pour timers (en ms)
  unsigned long tempsPasseeMillis;
  unsigned long animPrevMillis = 0;
  unsigned long animInterval = 100; // en ms 50-100 est bon (100 est assez bon, 50 un peu vite)
  bool gifInfini = false;




void displayDraw_init() {
    // Initialisation du module DisplayDraw
    Serial.println("Initialisation du module DisplayDraw");
}


void displayDraw_loop() {
    // Boucle principale du module DisplayDraw
}


////////////////////////////////////////////////////////////////////////////////
/////////////////////// FONCTIONS DE DESSIN ////////////////////////////////////
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv //


  // Bouton off 
  void eteindre_matrix(){
    typeDeRetour = "";
    FastLED.clear();
    // Serial.println("Eteindre la matrice | " + millis());
  }

  // Fonction de dessin (called par boutons, prends d'un CSV)
    void dessin(String nomDessin){
        dernierDessinVoulu = nomDessin;
      
      // nomDessin = le dessin à faire
      Serial.println("[DisplayDraw] demande dessin: " + nomDessin);

      // Aller chercher le bon .csv dans SPIFFS (dossier data/csv)
      File fichierDessin = SPIFFS.open("/" + nomDessin + ".csv", "r");
      if(!fichierDessin){ // En gros si le fichierDessin n'existe pas
        Serial.println("[DisplayDraw] Fichier introuvable: " + nomDessin + ".csv");
        // Ne pas vider la matrice si le fichier est manquant; conserver l'affichage précédent
        return;
      }
      // Lecture et application en flux pour éviter une grosse allocation sur la pile (prévention overflow)
      int i = 0;
      int applied = 0;
      
      // Check if we should skip mouth region (when mouth animation is active)
      bool isAllowedFace = (modeVariation == "normal" || modeVariation == "gremlin" || 
                            modeVariation == "smile" || modeVariation == "happy" || 
                            modeVariation == "heart" || modeVariation == "angry" || 
                            modeVariation == "cute");
      bool shouldSkipMouth =  WearerTalking && mode == "gyro" && isAllowedFace;
      
      while (fichierDessin.available() && i < nbLEDTotal) {
        String segmentCSV = fichierDessin.readStringUntil(';');
        if (segmentCSV.length() > 0 && nomDessin == dernierDessinVoulu) {
          int y = i / matrixLength;
          int x = i % matrixLength;
          
          // Skip mouth region if mouth animation is active
          if (shouldSkipMouth && 
              y >= mouthBeginsY && y < (mouthBeginsY + mouthHeight) &&
              x >= mouthBeginsX && x < (mouthBeginsX + mouthLength)) {
            i++;
            continue;
          }
          
          CRGB couleurCRGB = stringToCRGB(segmentCSV);
          setLEDColor(y, x, couleurCRGB);
          applied++;
        }
        i++;
      }
      fichierDessin.close();
      if (nomDessin == dernierDessinVoulu) {
        FastLED.show();
        Serial.println("[DisplayDraw] Dessin applique: " + nomDessin + " | LEDs mises à jour: " + String(applied));
      }

  }

  // Fonction de dessin (juste la section de la bouche)
  void dessinBouche(String nomDessin, int x1, int y1, int x2, int y2){
    // nomDessin = le dessin à faire
    Serial.println("[DisplayDraw] demande dessin bouche: " + nomDessin);

    // Make sure mouth drawing isn't skipped by the "dernierDessinVoulu" guard
    // used in the generic dessin() function.  We want the mouth to draw even
    // if another drawing was previously requested, so update the global here.
    dernierDessinVoulu = nomDessin;

    // Aller chercher le bon .csv dans SPIFFS (dossier data/csv)
    File fichierDessin = SPIFFS.open("/" + nomDessin + ".csv", "r");
    if (!fichierDessin)
    { // En gros si le fichierDessin n'existe pas
      Serial.println("[DisplayDraw] Fichier introuvable: " + nomDessin + ".csv");
      // Ne pas vider la matrice si le fichier est manquant; conserver l'affichage précédent
      return;
    }
    // Lecture et application en flux pour éviter une grosse allocation sur la pile (prévention overflow)
    int i = 0;
    int applied = 0;
    while (fichierDessin.available() && i < nbLEDMouth)
    {
      String segmentCSV = fichierDessin.readStringUntil(';');
      if (segmentCSV.length() > 0) // always draw every valid pixel
      {
        int y = i / mouthLength;
        int x = i % mouthLength;
        CRGB couleurCRGB = stringToCRGB(segmentCSV);
        setLEDColor(mouthBeginsY + y, mouthBeginsX + x, couleurCRGB);
        applied++;
      }
      i++;
    }
    fichierDessin.close();
    if (nomDessin == dernierDessinVoulu)
    {
        FastLED.show();
        Serial.println("[DisplayDraw] Dessin applique: " + nomDessin + " | LEDs mises à jour: " + String(applied));
    }
    Serial.println("[DisplayDraw] Dessin bouche applique: " + nomDessin + " | LEDs mises à jour: " + String(applied));
  } 

  // Commence l'animation
  void startAnimation(String name, int qteFrames, unsigned long intervalMs, bool infinite) {
    animName = name;
    animQteFrames = qteFrames;
    animInterval = intervalMs;
    gifInfini = infinite;
    animFrameIndex = 0;

    // Dessiner le premier dessin directement
    Serial.println("Start animation:" + animName + " with " + String(animQteFrames) + " frames, interval: " + String(animInterval) + "ms, infinite: " + String(gifInfini));


    dessin(animName + String(animFrameIndex));

    animPrevMillis = millis();
    animPlaying = true;
  }

  // Arrêter l'animation
  void stopAnimation() {
    //Serial.println("Stop animation! | " + millis());
    animPlaying = false;
    animStopped = true;
    animPrevMillis = 0;
    animFrameIndex = 0;
    FastLED.clear();
  }

  void imageFixe(String nomDessin){
    stopAnimation(); // On arrête les animations si une joue
    retourApresAnimation = nomDessin; // On garde le dernier dessin pour y revenir après l'animation one-time
    typeDeRetour = "imageFixe";
    dessin(nomDessin);    
  }

  void animation(String nomAnimation, int qteFrames) {
    startAnimation(nomAnimation, qteFrames, animInterval, false);
  }

  void gif(String nomAnimation, int qteFrames) { // Seule différence: on joue en boucle donc gifInfini = true
    retourApresAnimation = nomAnimation; // On garde le dernier dessin pour y revenir après l'animation one-time
    typeDeRetour = "gif";
    retourQteFrames = qteFrames;
    startAnimation(nomAnimation, qteFrames, animInterval, true);
  }

  // Joue une animation one-time, puis affiche une image fixe une fois l'animation terminée.
  // Cela fonctionne en réglant les variables de retour avant de démarrer l'animation
  // pour que `animationTick()` affiche `retourApresAnimation` quand l'animation est finie.
  void animTransitionVers(String nomAnimTransition, int qteFrames, String nomImage) {
    retourApresAnimation = nomImage;
    typeDeRetour = "imageFixe";
    startAnimation(nomAnimTransition, qteFrames, animInterval, false);
  }

  // Faire avancer l'animation de frame en frame
  void animationTick() {
    if (!animPlaying) return; // Si pas d'animation en cours, sortir

    tempsPasseeMillis = millis(); // Obtenir le temps actuel
    if (tempsPasseeMillis - animPrevMillis >= animInterval && animPlaying) { //Si interval écoulé
      animPrevMillis = tempsPasseeMillis; // Mettre à jour le temps du dernier tick
      
      animFrameIndex++; // Index
      if (animFrameIndex >= animQteFrames) { // Si on a dépassé le nombre de frames
        if (gifInfini) { // Si on doit jouer en boule (gif infini)
          animFrameIndex = 0;
        } else { // Sinon, arrêter l'animation one-time et retourner au truc (imageFixe ou gif) d'avant
          stopAnimation();
          if (typeDeRetour == ""){ // Si on avant rien avant l'animation (one-time est la 1ere chose qu'on joue)
            eteindre_matrix();
          } else if (typeDeRetour == "gif") {
            gif(retourApresAnimation, retourQteFrames); // Revenir au gif de avant l'animation one-time
          } else { // Else on retourne a une image
            dessin(retourApresAnimation); // Revenir au dessin de avant l'animation one-time
            Serial.print("RETOUR | Type de retour = ");
            Serial.println(retourApresAnimation);
          }
          return; 
        }
      }

      // Vu qu'on sort si: pas d'animation ou si on a fini l'animation, ici on peut dessiner la frame en toute sécurité
      // Serial.println("Commence dessin animation depuis animationTick! | " + millis());
      if (animPlaying){
        dessin(animName + String(animFrameIndex)); // Dessine la frame
      }
    }
  }


  // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ //
  /////////////////////// FONCTIONS DE DESSIN ////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

