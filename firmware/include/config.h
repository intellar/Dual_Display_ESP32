/**
 * @file config.h
 * @author Intellar (https://github.com/intellar)
 * @brief Configuration file for the animated eye project.
 * @version 1.0
 *
 * @copyright Copyright (c) 2025
 *
 * @license See LICENSE.md for details.
 *
 */
#ifndef CONFIG_H
#define CONFIG_H

// --- Hardware & Pinout Configuration ---
#define PIN_CS1 5 // Chip-select for screen 1
#define PIN_CS2 7 // Chip-select for screen 2

// --- ToF Sensor (VL53L5CX) Configuration ---
#define USE_TOF_SENSOR 1 // Set to 1 to enable the ToF sensor, 0 to disable it.
#define TOF_CALIBRATION_MODE 0 // Set to 1 to simulate sensor data for debugging.
#define SHOW_TOF_DEBUG_GRID 1 // Set to 1 to display the debug grid, 0 to hide it

#define PIN_TOF_SCL 15
#define PIN_TOF_SDA 16
#define PIN_TOF_INT 17

// --- Display & Image Configuration ---
#define SCR_WD 240 // Screen width in pixels
#define SCR_HT 240 // Screen height in pixels

#define EYE_IMAGE_WIDTH  350
#define EYE_IMAGE_HEIGHT 350
const uint16_t TRANSPARENT_COLOR_KEY = 0x0000; // The color in assets treated as transparent (black).

// --- Asset File Paths ---
static const char* EYE_IMAGE_NORMAL_PATH = "/image_giant.bin"; // Image for random/idle mode
static const char* EYE_IMAGE_BAD_PATH = "/image_giant_bad.bin"; // Image for tracking mode

// --- Saccade (Eye Movement) Behavior ---
// Controls how the eye darts around.
const unsigned long SACCADE_DELAY_AFTER_TRACK_MS = 2000; // Time to wait before starting random movement after losing a target.
const unsigned long SACCADE_INTERVAL_MS = 1500;     // Time between saccades (darting movements).
const float LERP_SPEED = 0.2;                       // Interpolation speed (0.0 to 1.0). Higher is faster/jerkier.
const int MAX_2D_OFFSET_PIXELS = 55;                // Max pixels the eye can move from the center.
const int RESTING_2D_OFFSET_PIXELS = (SCR_WD - EYE_IMAGE_WIDTH) / 2; // The "resting" position for the eye.


// --- ToF Sensor Behavior ---
const int MAX_DIST_TOF = 400; // Maximum distance in mm to consider a ToF target "close".



#endif // CONFIG_H