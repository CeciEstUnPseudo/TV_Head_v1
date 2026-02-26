#include "ServeurSetup.h"
#include <Arduino.h>
#include "SPIFFS.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Variables globales externes utilisées ici
void serveurRequetes_listener();
extern String activeFace;

extern const char *ssid;
extern const char *password;
extern AsyncWebServer monServeur;

// Variables spécifiques à ce module utilisées ici

void serveurSetup_init()
{
  // Initialisation du module ServeurSetup
  Serial.println("Initialisation du module ServeurSetup");
  // lancer SPIFFS et informer s'il y a erreur
  if (!SPIFFS.begin(true))
  {
    Serial.println("erreur avec SPIFFS");
    return;
  }

  // boucle de connexion au réseau Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  { // Tant qu'on est pas connecté pour ne pas passer plus bas
    delay(1000);
    Serial.println("Connexion au réseau WiFi...");
  }

  // afficher adresse IP du ESP32
  Serial.println(WiFi.localIP()); // Peut seulement venir ici une fois le while plus haut fini

  // écoute requête ("/" = page d'accueil)
  monServeur.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { // Fonction anonyme avec parametre (request)
    // envoi du fichier HTML au client
    request->send(SPIFFS, "/index.html", String(), false);
  });

  // écoute requête "/style.css" // Fonction anonyme avec parametre (request)
  monServeur.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
                {
    //envoi du fichier CSS au client
    request->send(SPIFFS, "/style.css", "text/css"); });

  // écoute requête "/boutons.css" // Fonction anonyme avec parametre (request)
  monServeur.on("/boutons.css", HTTP_GET, [](AsyncWebServerRequest *request)
                {
    //envoi du fichier JS au client
    request->send(SPIFFS, "/boutons.css", "text/css"); });

  // écoute requête "/menu-burger.js" // Fonction anonyme avec parametre (request)
  monServeur.on("/menu-burger.js", HTTP_GET, [](AsyncWebServerRequest *request)
                {
    //envoi du fichier JS au client
    request->send(SPIFFS, "/menu-burger.js", "text/javascript"); });

  // écoute requête "/selection-boutons.js" // Fonction anonyme avec parametre (request)
  monServeur.on("/selection-boutons.js", HTTP_GET, [](AsyncWebServerRequest *request)
                {
    //envoi du fichier CSS au client
    request->send(SPIFFS, "/selection-boutons.js", "text/javascript"); });

  // On active les écouteurs de requetes pour images & such
  serveurRequetes_listener();

  // Serve static images from SPIFFS (so requests to /images/* return the files)
  // Ensure your SPIFFS image files are uploaded under /images/<name>.svg (or .png)
  monServeur.serveStatic("/images", SPIFFS, "/images").setCacheControl("max-age=86400");

  // écoute requête "/activeFace" (envoyée avec fonction fetch)
  monServeur.on("/activeFace", HTTP_GET, [](AsyncWebServerRequest *maRequete)
                {
                  maRequete->send(200, "text/plain", activeFace); // 200 = un code serveur comme 404 qui veut dire "ok"
                });

  // Demarrer le serveur
  monServeur.begin();
}

void serveurSetup_loop()
{
  // Boucle principale du module ServeurSetup
}
