#pragma once

// L'équivalent du "setup()" d'Arduino pour l'initialisation de l'affichage
void serveurRequetes_init(); 

// L'équivalent du "loop()" d'Arduino pour la mise à jour de l'affichage
void serveurRequetes_loop();

// Spécifiques à ce module: 
void serveurRequetes_listener();
