/**
 * @file drawing_tools.h
 * @author Intellar (https://github.com/intellar)
 * @brief Header file for all graphics, drawing, and display functions.
 * @version 1.0
 *
 * @copyright Copyright (c) 2025
 *
 * @license See LICENSE.md for details.
 *
 */
#include <Arduino.h>
#include <SPI.h>
#include <SparkFun_VL53L5CX_Library.h>
#include <TFT_eSPI.h>

#include "config.h"


#ifndef _DRAWING_TOOLSH_
#define _DRAWING_TOOLSH_



#define NUM_SCREEN 2

#define SCR_WD   240
#define SCR_HT   240

// Enum to provide clear names for eye/screen indexing
enum EyeIndex {
    EYE_LEFT = 0,
    EYE_RIGHT = 1
};

// --- Eye State Struct ---
// Holds the dynamic state for each eye, such as position and tracking status.
struct EyeState {
    float x, y;         // Current 2D position of the eye texture
    bool is_tracking;   // True if the eye is currently tracking a ToF target
    bool was_tracking;  // True if the eye was tracking on the previous frame
};
// Declare the global eye state array; it will be defined in the main .ino file.
extern EyeState eyes[NUM_SCREEN];

// A struct to hold per-screen configuration.
struct Screen {
  int16_t CS; // Chip select pin
};
// Declare the screens array as an external variable; it will be defined in drawing_tools.cpp
extern Screen screens[NUM_SCREEN];

// Make framebuffers accessible to other files
extern uint16_t* framebuffers[NUM_SCREEN];

// --- Eye Asset Management ---
// Enum to identify different eye image types
enum EyeImageType {
    EYE_IMAGE_NORMAL = 0, // Default image for random/idle mode
    EYE_IMAGE_BAD,        // Image for tracking mode (e.g., "bad" or focused)
    NUM_EYE_IMAGE_TYPES   // Total number of eye image types
};

// Holds the buffer for a single eye texture asset.
struct EyeTexture {
    uint16_t* buffers[NUM_EYE_IMAGE_TYPES]; // Array of pointers to image buffers
};
extern EyeTexture eye_texture;

// --- Optimisation: Scanline pre-calculation for circular screen ---
// Defines the start and end x-coordinates for a single horizontal line of a circle.
struct Scanline {
    int16_t x_start;
    int16_t x_end;
};
extern Scanline circular_scanlines[SCR_HT];


void select_screen(int16_t ind);
void display_buffer(int16_t ind);
void display_all_buffers();
void clear_buffer(uint16_t color);

void log_tft_setup();
void init_tft();

void init_text_sprite();
void drawString_fb(const char *string, int32_t x, int32_t y, uint16_t fgcolor);
void draw_eye_image(int16_t x_pos, int16_t y_pos, uint8_t eyelid_level, EyeImageType image_type);
void draw_eye_at_target(float target_x, float target_y, uint8_t eyelid_level, EyeImageType image_type);
void draw_crosshair(int16_t center_x, int16_t center_y, int16_t size, uint16_t color);
void draw_tof_debug_grid(int16_t x_pos, int16_t y_pos, int16_t grid_size, const VL53L5CX_ResultsData* data, int8_t highlight_x, int8_t highlight_y);
void draw_score_grid(int16_t x_pos, int16_t y_pos, int16_t grid_size, const long* scores, int8_t highlight_x, int8_t highlight_y);
void show_splash_screen();
void clear_all_screens(uint16_t color);
#endif