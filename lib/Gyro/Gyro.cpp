#include "Gyro.h"
#include <Arduino.h>
#include <Wire.h>    // Allow i2C com. between senros & ESP32 (SDA + SCL lines)
#include <MPU6050.h> // Easy sensor functions

// Variables globales externes utilisées ici
extern void handleGyroUpdate();
extern String gyroTilt;

// Variables spécifiques à ce module utilisées ici
MPU6050 mpu;                     // MPU has a gyro, acceleration, & temperature module
const int tiltThreshold = 3000; // So I don't have to write 5000 a lot of times and I can select from here the tilt value threshold to send my signals

void gyro_init()
{
  // Initialisation du module gyro
  Serial.println("Initialisation du module Gyro");
  // Doing those in Main.ino
  Wire.begin(21,40); // i2C com uses SDA and SCL. My ESP32S3 SCL is D9 and SDA D8
  mpu.initialize();   // Literally read the function, it does what it's named
  if (!mpu.testConnection())
  {
    Serial.println("MPU6050 non détecté (NACK sur 0x68) (continues, tentative de lecture)");
  }
  else
  {
    Serial.println("Gyro initialisation terminée");
  }
}

void gyro_loop()
{
  // Boucle principale du module gyro
  // Values coming from the MPU605


  // Pacing non bloquant: une lecture toutes les ~50ms (ancien delay(50))
  static unsigned long nextCycle = 0;
  unsigned long tempsPasseeMillis = millis();
  if (tempsPasseeMillis < nextCycle)
  {
    return; // Trop tôt pour une nouvelle lecture
  }
  nextCycle = tempsPasseeMillis + 50; // Intervalle identique à l'ancien delay(50)
  
  int16_t accX, accY, accZ, gyroX, gyroY, gyroZ; // acc = acceleration. gyro = angular velocity

  mpu.getMotion6(&accX, &accY, &accZ, &gyroX, &gyroY, &gyroZ); // Could have made the values right into the getMotion6, fragmentation is better to learn for beginners tho
                                                               // ---- We check the tilt via the acceleration (aka gravity is applied on what "side" of the MPU (if I understand correctly))

  // Logic || Only 1 "tilt" at a time (I don't want forward-left, for example. Only the one I did first, so on les a tous dans 1 seul "if else if else if")
  // Check if any of the 4 possibilities (right, left, forward, backward) else put tilt as ""

  // Right / Left -- Take priority over forward/backward (most prone to being mistaken as the other two)
  if (accX < -tiltThreshold)
  { // Means its tilted to the right
    if (gyroTilt == "right")
      return;
    Serial.println("Tilt Right detected");
    gyroTilt = "right";
    handleGyroUpdate();
  }
  else if (accX > tiltThreshold)
  { // Means its tilted to the left
    if (gyroTilt == "left")
      return;
    Serial.println("Tilt Left detected");
    gyroTilt = "left";
    handleGyroUpdate();
  }

  // Forward / Backward
  else if (accY < - (tiltThreshold*2)) // Tilted forward a little bit naturally so we use a bigger threshhold
  { // Means its tilted forward
    if (gyroTilt == "forward")
      return;
    Serial.println("Tilt Forward detected");
    gyroTilt = "forward";
    handleGyroUpdate();
  }
  else if (accY > tiltThreshold)
  { // Means its tilted backward
    if (gyroTilt == "backward")
      return;
    Serial.println("Tilt Backward detected");
    gyroTilt = "backward";
    handleGyroUpdate();
  }
  else if (gyroTilt != "normal")
  {
    if (gyroTilt == "normal")
      return;
    Serial.println("Tilt Normal detected");
    gyroTilt = "normal";
    handleGyroUpdate();
  }

  // ----
  // Plus de delay(50); le pacing est géré en début de fonction
}
