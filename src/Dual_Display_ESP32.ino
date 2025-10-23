/**
 * @file animated_eye_image_simple.ino
 * @author Intellar (https://github.com/intellar)
 * @brief Main application file for the animated eye project.
 * @version 1.0
 *
 * @copyright Copyright (c) 2025
 *
 * @license See LICENSE.md for details.
 *
 */
#include "drawing_tools.h"
#include <esp_heap_caps.h> // Pour les fonctions de gestion de la mémoire
#include "LittleFS.h"
#include "config.h"

// Variables for FPS calculation
uint32_t last_time_fps = 0;
uint16_t frame_count = 0;
float fps = 0;

// --- 2D Gaze Point Logic ---
// Represents a 2D offset for the eye to look at.
struct Point2D {
    float x, y;
};

// --- Eye State Struct & Variables ---
struct EyeState {
    float x, y;         // Current 2D position of the eye texture
};
EyeState eyes[NUM_SCREEN];

struct DisplayState {
    float center_x, center_y; // Neutral 2D center for the texture
};
DisplayState display_state;

// --- Animation State ---
Point2D target_offset = {RESTING_2D_OFFSET_PIXELS, RESTING_2D_OFFSET_PIXELS};
Point2D current_offset = {RESTING_2D_OFFSET_PIXELS, RESTING_2D_OFFSET_PIXELS};
unsigned long last_saccade_time = 0;

// Function prototypes for new refactored functions
void update_eye_target();
void update_eye_position();
void draw_scene();
void calculate_and_show_fps();

/**
 * @brief Lists all files and their sizes on the LittleFS filesystem for debugging.
 */
void list_littlefs_files() {
  if (!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  Serial.println("--- Listing files on LittleFS ---");
  fs::File root = LittleFS.open("/");
  fs::File file = root.openNextFile();
  while(file){
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
      file = root.openNextFile();
  }
  Serial.println("---------------------------------");
}

/**
 * @brief Prints detailed memory usage (internal RAM and PSRAM) to the serial monitor.
 */
void printMemoryUsage() {
  Serial.println("\n--- Utilisation Mémoire ---");
  Serial.print("Mémoire libre (RAM interne): ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("Mémoire totale (RAM interne): ");
  Serial.println(ESP.getHeapSize());
  Serial.print("Mémoire PSRAM libre: ");
  Serial.println(ESP.getFreePsram());
  Serial.print("Mémoire PSRAM totale: ");
  Serial.println(ESP.getPsramSize());
  Serial.println("---------------------------\n");
}

void setup() {
  Serial.begin(115200);
  list_littlefs_files(); // Lists the present files
  init_tft();

  // Display the splash screen
  show_splash_screen();

  // Wait for the splash screen duration before starting the main loop
  delay(2000);

  // Re-initialize the text sprite after the splash screen has used and deleted it.
  init_text_sprite();

  // Print initial memory usage
  printMemoryUsage();

  // Initialize center based on image scale
  // Calculate the position so the center of the source image is at the center of the screen.
  display_state.center_x = -(EYE_IMAGE_WIDTH / 2.0f - SCR_WD / 2.0f);
  display_state.center_y = -(EYE_IMAGE_HEIGHT / 2.0f - SCR_HT / 2.0f);
  
  // Initialize 2D eye positions
  for (int i = 0; i < NUM_SCREEN; i++) {
    eyes[i].x = display_state.center_x;
    eyes[i].y = display_state.center_y;
  }
  
  randomSeed(analogRead(0));
  last_time_fps = millis();
  last_saccade_time = millis();

  // Halt execution if the primary eye texture failed to load.
  if (eye_texture.buffer == nullptr) {
    Serial.println("FATAL: Eye image buffer not loaded. Halting.");
    while(1) { delay(1000); }
  }
}

void update_eye_target() {
  unsigned long current_time = millis();
  if (current_time - last_saccade_time > SACCADE_INTERVAL_MS) {
    last_saccade_time = current_time;

    // Generate a random angle and radius
    float angle = random(0, 36000) / 100.0f * (PI / 180.0f);
    float radius = random(0, MAX_2D_OFFSET_PIXELS * 100) / 100.0f;

    // Convert polar coordinates (angle, radius) to Cartesian (x, y)
    target_offset.x = radius * cos(angle);
    target_offset.y = radius * sin(angle);
  }
}

void update_eye_position() {
  // Smoothly interpolate the current eye offset towards the target offset.
  current_offset.x += (target_offset.x - current_offset.x) * LERP_SPEED;
  current_offset.y += (target_offset.y - current_offset.y) * LERP_SPEED;

  // The final eye position is its center plus the current animated offset.
  // Both eyes will have the same position in this simplified 2D model.
  for (int i = 0; i < NUM_SCREEN; i++) {
    eyes[i].x = display_state.center_x + current_offset.x;
    eyes[i].y = display_state.center_y + current_offset.y;
  }
}

void draw_scene() {
  // --- Drawing Loop ---
  for (int i = 0; i < NUM_SCREEN; i++) {
    select_screen(i);
    clear_buffer(TFT_BLACK); // Clear the entire buffer for this screen

    // Draw the eye image with the eyelid fully open (level 0)
    draw_eye_image(eyes[i].x, eyes[i].y, 0);
  }
}

/**
 * @brief Calculates and prints the current frames per second (FPS) to the serial monitor.
 */
void calculate_and_show_fps() {
  uint32_t loop_time = millis();
  if (loop_time - last_time_fps >= 1000) {
    fps = (float)frame_count * 1000.0 / (float)(loop_time - last_time_fps);
    Serial.printf("FPS: %.1f\n", fps);
    frame_count = 0;
    last_time_fps = loop_time;
  }

  frame_count++;
}

void loop() {
  // 1. Update the state of the world (where to look, animations)
  update_eye_target();
  update_eye_position();

  // 2. Draw the scene based on the new state
  draw_scene();

  // 3. Push the final image to the screens
  display_all_buffers();

  // 4. Performance monitoring
  calculate_and_show_fps();
}
