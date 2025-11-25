/**
 * @file eye_logic.h
 * @author Intellar (https://github.com/intellar)
 * @brief Public interface for managing eye movement logic.
 * @version 1.0
 *
 * @copyright Copyright (c) 2025
 *
 * @license See LICENSE.md for details.
 *
 */
#ifndef EYE_LOGIC_H
#define EYE_LOGIC_H

#include "tof_sensor.h" // For TofTarget
#include "drawing_tools.h" // For EyeImageType

// State for a single eye's logical position
struct EyePosition {
    float x = 0.0f; // Current horizontal position (-1.0 to 1.0)
    float y = 0.0f; // Current vertical position (-1.0 to 1.0)

    // Explicit constructor to allow initialization with two floats
    EyePosition(float initial_x = 0.0f, float initial_y = 0.0f) : x(initial_x), y(initial_y) {}
};

// Updates the eye positions based on the sensor target and internal state (saccade/tracking)
void update_eye_positions(const TofTarget& target);

// Gets the current calculated position of a specific eye
EyePosition get_eye_position(int eye_index);

// Determines which eye image to use based on the target's validity
EyeImageType get_current_eye_image_type(const TofTarget& target);

#endif // EYE_LOGIC_H