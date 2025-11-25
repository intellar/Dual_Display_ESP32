/**
 * @file Dual_Display_Firmware.ino
 * @author Intellar (https://github.com/intellar)
 * @brief Main firmware for the dual-display animated eye project.
 * @version 1.1
 *
 * @copyright Copyright (c) 2025
 *
 * @license See LICENSE.md for details.
 *
 */

#include <Arduino.h>
#include "config.h"
#include "drawing_tools.h"
#include "eye_logic.h"
#include "tof_sensor.h"
#include "LittleFS.h"

// --- FPS Counter Variables ---
static unsigned long last_fps_time = 0;
static int frame_count = 0;
static float current_fps = 0.0f;


// --- Debugging ---

/**
 * @brief Initializes all subsystems.
 */
void setup() {
  Serial.begin(115200);
  Serial.println("Booting Dual Display Firmware...");

  // Initialize LittleFS for asset loading
  if (!LittleFS.begin()) {
    Serial.println("FATAL: LittleFS mount failed. Halting.");
    while (1) { delay(100); }
  }

  sleep(1);

  // Initialize displays and load graphical assets
  init_tft();

  // Perform an initial clear of both physical screens to ensure a clean state
  clear_all_screens(TFT_BLACK);
  delay(50); // Short delay to ensure screens are cleared

  // Show splash screen to user while the rest initializes
  show_splash_screen();
  delay(1000); // Keep splash visible for a moment

  // Initialize the ToF sensor (this part is slow)
  #if USE_TOF_SENSOR
    init_tof_sensor();
    #if TOF_CALIBRATION_MODE
      Serial.println("!!! ToF CALIBRATION MODE IS ACTIVE !!!");
    #endif
  #endif

  Serial.println("Initialization complete. Starting main loop.");
}

/**
 * @brief Main application loop.
 */
void loop() {
  // --- FPS Calculation ---
  frame_count++;
  unsigned long current_millis = millis();
  if (current_millis - last_fps_time >= 1000) {
    // Calculate FPS over the last second
    current_fps = frame_count / ((current_millis - last_fps_time) / 1000.0f);
    last_fps_time = current_millis;
    frame_count = 0;
    Serial.printf("FPS: %.1f\n", current_fps); // Print FPS to serial log
  }

  // --- 1. Sensor Update ---
  #if USE_TOF_SENSOR
    #if TOF_CALIBRATION_MODE
      // In calibration mode, force an update on every frame
      update_tof_sensor_data();
    #else
    update_tof_sensor_data();
    #endif
  #endif
  TofTarget target = get_tof_target();

  // --- 2. Eye Position Logic ---
  // Update the logical positions of the eyes based on the target
  update_eye_positions(target);

  // --- 3. Drawing ---
  for (int i = 0; i < NUM_SCREEN; i++) {
    select_screen(i);
    clear_buffer(TFT_BLACK);

    // Get the final calculated position and image type for the current eye
    EyePosition pos = get_eye_position(i);
    EyeImageType image_type = get_current_eye_image_type(target);

    // Draw the eye at its final calculated position
    draw_eye_at_target(pos.x, pos.y, 0, image_type); // 0 = eyelid open

    // Optional: Draw the ToF debug grid on one of the screens
    #if USE_TOF_SENSOR && SHOW_TOF_DEBUG_GRID
      if (i == EYE_RIGHT) { // Draw only on the right eye screen
        const int16_t grid_size = 80;
        const int16_t grid_pos = (SCR_WD - grid_size) / 2;
        // Get raw sensor data for display
        const VL53L5CX_ResultsData* tof_data = get_tof_measurement_data();
        // Use the same 'target' that was used for the eye movement
       draw_tof_debug_grid(grid_pos, grid_pos, grid_size, tof_data, target.min_dist_pixel_x, target.min_dist_pixel_y);

        // --- Draw FPS Counter ---
        char fps_str[10];
        dtostrf(current_fps, 4, 1, fps_str); // Format float to string (width 4, 1 decimal)
        char display_str[15];
        sprintf(display_str, "FPS: %s", fps_str);
        drawString_fb(display_str, 5, 5, TFT_WHITE);
      }
    #endif
  }

  // --- 4. Display Update ---
  // Push the completed framebuffers to the physical screens
  display_all_buffers();
}