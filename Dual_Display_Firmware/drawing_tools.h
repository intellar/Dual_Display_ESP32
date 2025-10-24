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


// A struct to hold per-screen configuration.
struct Screen {
  int16_t CS; // Chip select pin
};
// Declare the screens array as an external variable; it will be defined in drawing_tools.cpp
extern Screen screens[NUM_SCREEN];

// Make framebuffers accessible to other files
extern uint16_t* framebuffers[NUM_SCREEN];

// --- Eye Asset Management ---
// Holds the buffer for a single eye texture asset.
struct EyeTexture {
    uint16_t* buffer = nullptr;
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

void init_tft();

void init_text_sprite();
void drawString_fb(const char *string, int32_t x, int32_t y, uint16_t fgcolor);
void draw_eye_image(int16_t x_pos, int16_t y_pos, uint8_t eyelid_level);
void show_splash_screen();

#endif