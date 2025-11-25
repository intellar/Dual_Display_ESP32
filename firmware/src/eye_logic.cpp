/**
 * @file eye_logic.cpp
 * @author Intellar (https://github.com/intellar)
 * @brief Implementation of the eye movement and state logic.
 * @version 1.0
 *
 * @copyright Copyright (c) 2025
 *
 * @license See LICENSE.md for details.
 *
 */
#include "eye_logic.h" // Correctly includes the header from the 'include' path
#include "config.h"
#include <Arduino.h>

// --- Module-Private State ---

// Global state for each eye's position
static EyePosition eye_positions[NUM_SCREEN];

// Saccade (random movement) variables
static unsigned long last_saccade_time = 0;
static float saccade_target_x = 0.0f;
static float saccade_target_y = 0.0f;

// Tracking state variables
static unsigned long last_track_time = 0;
static float last_known_target_x = 0.0f;
static float last_known_target_y = 0.0f;

/**
 * @brief Updates the eye positions based on the sensor target.
 * This function contains the core logic for switching between tracking a target
 * and performing idle saccade movements.
 * @param target The target data from the ToF sensor.
 */
void update_eye_positions(const TofTarget& target) {
    float final_target_x, final_target_y;

    if (target.is_valid) {
        // A valid target is present, so we aim for it.
        final_target_x = target.x;
        final_target_y = target.y;
        last_track_time = millis(); // Update the time we last had a valid track

        // Store the last known good position
        last_known_target_x = target.x;
        last_known_target_y = target.y;
    } else {
        // No valid target, switch to idle behavior.
        #if !TOF_CALIBRATION_MODE 
        if (millis() - last_track_time > SACCADE_DELAY_AFTER_TRACK_MS) {
            // If enough time has passed since losing a target, get a new random saccade target.
            if (millis() - last_saccade_time > SACCADE_INTERVAL_MS) {
                last_saccade_time = millis();
                saccade_target_x = random(-100, 101) / 100.0f;
                saccade_target_y = random(-100, 101) / 100.0f;
            }
        }
        #endif

        // If not in calibration mode, use the saccade target. Otherwise, hold the last known position.
        #if TOF_CALIBRATION_MODE
            final_target_x = last_known_target_x;
            final_target_y = last_known_target_y;
        #else
            final_target_x = saccade_target_x;
            final_target_y = saccade_target_y;
        #endif
    }

    // Smoothly interpolate (LERP) each eye's position towards the final target.
    for (int i = 0; i < NUM_SCREEN; i++) {
        eye_positions[i].x += (final_target_x - eye_positions[i].x) * LERP_SPEED;
        eye_positions[i].y += (final_target_y - eye_positions[i].y) * LERP_SPEED;
    }
}

EyePosition get_eye_position(int eye_index) {
    if (eye_index >= 0 && eye_index < NUM_SCREEN) {
        return eye_positions[eye_index];
    }
    return EyePosition{0.0f, 0.0f}; // Return a default/safe value by explicitly constructing it
}

EyeImageType get_current_eye_image_type(const TofTarget& target) {
    return target.is_valid ? EYE_IMAGE_BAD : EYE_IMAGE_NORMAL;
}