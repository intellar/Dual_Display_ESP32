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

// --- Display & Image Configuration ---
#define EYE_IMAGE_WIDTH  350
#define EYE_IMAGE_HEIGHT 350
const uint16_t TRANSPARENT_COLOR_KEY = 0x0000; // The color in assets treated as transparent (black).

// --- Asset File Paths ---
static const char* EYE_IMAGE_NORMAL_PATH = "/eye_real.bin";

// --- Saccade (Eye Movement) Behavior ---
// Controls how the eye darts around.
const unsigned long SACCADE_INTERVAL_MS = 1500;     // Time between saccades (darting movements).
const float LERP_SPEED = 0.2;                       // Interpolation speed (0.0 to 1.0). Higher is faster/jerkier.
const int MAX_2D_OFFSET_PIXELS = 40;                // Max pixels the eye can move from the center (e.g., 40px left/right/up/down).
const int RESTING_2D_OFFSET_PIXELS = 0;             // The "resting" position for the eye (0 = centered).

#endif // CONFIG_H