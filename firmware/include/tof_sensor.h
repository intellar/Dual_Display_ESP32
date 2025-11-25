/**
 * @file tof_sensor.h
 * @author Intellar (https://github.com/intellar)
 * @brief Header file for the VL53L5CX Time-of-Flight sensor control.
 * @version 1.0
 *
 * @copyright Copyright (c) 2025
 *
 * @license See LICENSE.md for details.
 *
 */
#ifndef TOF_SENSOR_H
#define TOF_SENSOR_H

#include <Arduino.h>
#include "config.h"
#include <Wire.h>

// Bibliothèque pour le capteur de distance VL53L5CX
// Assurez-vous de l'installer via le gestionnaire de bibliothèques : "SparkFun VL53L5CX"
#include <SparkFun_VL53L5CX_Library.h>


// Structure pour stocker la position de la cible détectée par le ToF
struct TofTarget {
    float x; // Position horizontale (-1.0 à 1.0)
    float y; // Position verticale (-1.0 à 1.0)
    int distance_mm; // Distance en mm
    bool is_valid; // Si une cible a été détectée
    int8_t min_dist_pixel_x; // Coordonnée X du pixel le plus proche (pour débogage)
    int8_t min_dist_pixel_y; // Coordonnée Y du pixel le plus proche (pour débogage)
    long match_score; // Score de corrélation du template matching (pour débogage)
};

// Initializes the ToF sensor. Must be called in setup().
void init_tof_sensor();

// Lit les données du capteur et met à jour la structure de la cible.
void update_tof_sensor_data();

// Retourne la dernière position de cible détectée.
TofTarget get_tof_target();

// Retourne un pointeur vers les données de mesure brutes du capteur.
const VL53L5CX_ResultsData* get_tof_measurement_data();

#endif // TOF_SENSOR_H