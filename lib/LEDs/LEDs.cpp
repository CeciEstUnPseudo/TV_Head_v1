#include "LEDs.h"
#include <Arduino.h>
#include <FastLED.h>
#include "SPIFFS.h"
#include <map>

// Variables globales externes utilisées ici (centralisées dans DisplayGlobals.h)
#include "DisplayGlobals.h"

// Variables spécifiques à ce module utilisées ici
// Déclarations des fonctions
CRGB stringToCRGB(String couleur);
void setLEDColor(int y, int x, CRGB couleurVoulue);
CRGB getLEDColor(int y, int x);
int getLED(int y, int x);


////////////////////////////////////////////////////////////////////////////////
//////////////// FONCTION DE CONVERSION COULEUR STRING -> CRGB  ////////////////
////////////////////////////////////////////////////////////////////////////////

CRGB stringToCRGB(String couleur)
{
  couleur.trim(); // Enlever les espaces au cas où

  // Format RGB numérique (ex: "255,0,0" ou "255,128,64")
  if (couleur.indexOf(',') != -1)
  {                                                                  // Si on trouve une virgule, c'est le bon format
    int premiereVirgule = couleur.indexOf(',');                      // Trouver la position de la première virgule
    int deuxiemeVirgule = couleur.indexOf(',', premiereVirgule + 1); // Trouver la position de la deuxième virgule

    if (premiereVirgule != -1 && deuxiemeVirgule != -1)
    {                                                                          // Si on a bien trouvé les deux virgules
      int r = couleur.substring(0, premiereVirgule).toInt();                   // Convertir la partie avant la première virgule en int
      int g = couleur.substring(premiereVirgule + 1, deuxiemeVirgule).toInt(); // Convertir la partie entre les deux virgules en int
      int b = couleur.substring(deuxiemeVirgule + 1).toInt();                  // Convertir la partie après la deuxième virgule en int

      // S'assurer que les valeurs sont entre 0-255
      r = constrain(r, 0, 255);
      g = constrain(g, 0, 255);
      b = constrain(b, 0, 255);

      return CRGB(r, g, b);
    }
    else
    {
      // Si format non-valide, retourne noir (éteint)
      return CRGB::Black;
    }
  }
  else
  {
    // Si format non-valide, retourne noir (éteint)
    return CRGB::Black;
  }
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
//////////////// FONCTIONS D'UTILISATION / PARCOURS DE LA MATRICE  ////////////////
///////////////////////////////////////////////////////////////////////////////////

// Fonction pour convertir coordonnées 2D vers index 1D (En gros pour piocher dans le tableau 1D comme si c'était une matrice 2D)

int getLED(int y, int x)
{
  // Adaptation pour matrice dont le premier LED physique est en haut à DROITE.
  int led;
  if (y % 2 == 0) {
    // Ligne paire (0,2,4,...): direction physique droite->gauche, on retourne x
    led = y * matrixLength + (matrixLength - 1 - x);
  } else {
    // Ligne impaire: direction physique gauche->droite
    led = y * matrixLength + x;
  }
  return led;
}

// Fonction pour définir une LED à une position (y,x) (utile pour allumer une LED spécifique (x,y) à une couleur donnée)
void setLEDColor(int y, int x, CRGB couleurVoulue)
{
  if (x >= 0 && x < matrixLength && y >= 0 && y < matrixHeight)
  {
    maMatrixLEDs[getLED(y, x)] = couleurVoulue;
  }
}

// Fonction pour obtenir la couleur d'une LED à une position (y,x) (utile pour lire la couleur d'une LED spécifique, sinon retourne noir (éteint))
CRGB getLEDColor(int y, int x)
{
  if (x >= 0 && x < matrixLength && y >= 0 && y < matrixHeight)
  {
    return maMatrixLEDs[getLED(y, x)];
  }
  return CRGB::Black;
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////