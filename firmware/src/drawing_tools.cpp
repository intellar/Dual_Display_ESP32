/**
 * @file drawing_tools.cpp
 * @author Intellar (https://github.com/intellar)
 * @brief Implementation of all graphics, drawing, and display functions.
 * @version 1.0
 *
 * @copyright Copyright (c) 2025
 *
 * @license See LICENSE.md for details.
 *
 */
#include "drawing_tools.h"
#include <Arduino.h>

// TFT library
#include <TFT_eSPI.h> 
#include "LittleFS.h"

TFT_eSPI tft = TFT_eSPI();

// --- Global Variables ---

Screen screens[NUM_SCREEN]; // Definition for the screen configuration array

// Framebuffers for off-screen drawing
uint16_t* framebuffers[NUM_SCREEN];
static int8_t active_screen_index = 0;

// --- Image Buffer ---
EyeTexture eye_texture; // Definition for the eye textures

// --- Text Sprite ---
TFT_eSprite spr = TFT_eSprite(&tft);

// --- Optimisation: Scanline definition ---
Scanline circular_scanlines[SCR_HT];

// --- Forward Declarations for internal functions ---
void pushSpriteToFb(TFT_eSprite* sprite, int32_t x, int32_t y, uint16_t* framebuffer, uint16_t transparent_color);
/**
 * @brief Logs the detailed setup and pin configuration of the TFT_eSPI library.
 * This is a helper function for debugging to confirm that the build flags
 * have been correctly applied.
 */
void log_tft_setup() {
  // Log the pins that TFT_eSPI is configured to use, to verify build flags
  Serial.println("TFT_eSPI pins from build flags:");
  Serial.printf("  MOSI: %d, SCLK: %d, DC: %d, RST: %d, CS: %d\n",
                TFT_MOSI, TFT_SCLK, TFT_DC, TFT_RST, TFT_CS);

  // Get a diagnostic report from the library to confirm all settings
  Serial.println("\n--- TFT_eSPI Setup Report ---");
  setup_t tft_settings;
  tft.getSetup(tft_settings);

  Serial.printf("TFT_eSPI Ver: %s\n", tft_settings.version.c_str());
  Serial.printf("Processor:    %d\n", tft_settings.esp);
  Serial.printf("Transactions: %s\n", tft_settings.trans ? "Yes" : "No");
  Serial.printf("Interface:    %s\n", tft_settings.serial ? "SPI" : "Parallel");
  if (tft_settings.serial) {
    Serial.printf("SPI overlap:  %s\n", tft_settings.overlap ? "Yes" : "No");
  }

  Serial.printf("Driver:       %d\n", tft_settings.tft_driver);
  Serial.printf("Resolution:   %d x %d\n", tft_settings.tft_width, tft_settings.tft_height);

  if (tft_settings.serial) {
    Serial.printf("SPI Freq:     %.2f MHz\n", tft_settings.tft_spi_freq / 10.0);
    if (tft_settings.tft_rd_freq > 0) Serial.printf("Read Freq:    %.2f MHz\n", tft_settings.tft_rd_freq / 10.0);
  }

  Serial.printf("TFT_MOSI: %d, TFT_MISO: %d, TFT_SCLK: %d, TFT_CS: %d, TFT_DC: %d, TFT_RST: %d\n",
                tft_settings.pin_tft_mosi, tft_settings.pin_tft_miso, tft_settings.pin_tft_clk,
                tft_settings.pin_tft_cs, tft_settings.pin_tft_dc, tft_settings.pin_tft_rst);
  Serial.println("---------------------------------");
}





// --- Screen & Buffer Management ---

/**
 * @brief Selects the active screen for subsequent drawing operations.
 * @param ind The index of the screen to select (EYE_LEFT or EYE_RIGHT).
 */
void select_screen(int16_t ind) {
  if (ind < 0 || ind >= NUM_SCREEN) return;
  digitalWrite(screens[EYE_LEFT].CS, (ind == EYE_LEFT) ? LOW : HIGH);
  digitalWrite(screens[EYE_RIGHT].CS, (ind == EYE_RIGHT) ? LOW : HIGH);
  active_screen_index = ind;
}
/**
 * @brief Swaps the byte order of a 16-bit color value.
 * Required for compatibility between standard RGB565 and the display's byte order.
 * @param color The input color.
 * @return The color with bytes swapped.
 */
uint16_t swap_color_bytes(uint16_t color) {
  return (color << 8) | (color >> 8);
}

/**
 * @brief Clears the active framebuffer to a specified color.
 * Uses an optimized `memset` for single-byte colors.
 * @param color The 16-bit color to clear the buffer with.
 */
void clear_buffer(uint16_t color) {
  uint16_t corrected_color = swap_color_bytes(color);
  uint16_t* current_buffer = framebuffers[active_screen_index];
  uint8_t hi = corrected_color >> 8, lo = corrected_color;
  if (hi == lo) {
    memset(current_buffer, lo, SCR_WD * SCR_HT * 2);
  } else {
    for (uint32_t i = 0; i < SCR_WD * SCR_HT; i++) current_buffer[i] = corrected_color;
  }
}

/**
 * @brief Clears both physical screens to a specified color.
 * @param color The 16-bit color to fill the screens with.
 */
void clear_all_screens(uint16_t color) {
  for (int i = 0; i < NUM_SCREEN; i++) {
    select_screen(i);
    tft.fillScreen(color);
  }
}

/**
 * @brief Pushes the content of a framebuffer to its corresponding physical screen.
 * @param ind The index of the screen/framebuffer to display.
 */
void display_buffer(int16_t ind) {
  if (ind < 0 || ind >= NUM_SCREEN) return;
  select_screen(ind); // Ensure correct screen is selected
  tft.pushImage(0, 0, SCR_WD, SCR_HT, framebuffers[ind]);
}

/**
 * @brief Pushes both framebuffers to their respective screens.
 */
void display_all_buffers() {
  display_buffer(EYE_LEFT);
  display_buffer(EYE_RIGHT);
}

/**
 * @brief Pre-calculates the start and end x-coordinates for each horizontal
 * line of a circle that fits the screen. This is a major optimization for
 * circular drawing operations.
 */
void precalculate_scanlines() {
    const int16_t screen_center = SCR_WD / 2;
    const int32_t radius_sq = screen_center * screen_center;

    for (int16_t y = 0; y < SCR_HT; y++) {
        int32_t dist_y = y - screen_center;
        int32_t dist_y_sq = dist_y * dist_y;
        if (dist_y_sq < radius_sq) {
            // Use integer square root for performance if available, otherwise float is fine for one-time setup
            int16_t x_extent = sqrt(radius_sq - dist_y_sq);
            circular_scanlines[y].x_start = screen_center - x_extent;
            circular_scanlines[y].x_end = screen_center + x_extent;
        } else {
            circular_scanlines[y].x_start = -1; // Mark as not drawable
            circular_scanlines[y].x_end = -1;
        }
    }
}

/**
 * @brief Loads a binary image file from LittleFS into a specified buffer in PSRAM. 
 *        If PSRAM fails, it tries to allocate in internal RAM.
 * @param filename The path to the image file on LittleFS.
 * @param width The width of the image.
 * @param height The height of the image.
 * @param buffer A pointer to the buffer pointer that will store the image data.
 * @return true if the image was loaded successfully, false otherwise.
 */
bool load_specific_eye_image(const char* filename, int16_t width, int16_t height, uint16_t** buffer) {
    fs::File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.print("Failed to open file for reading: ");
        Serial.println(filename);
        return false;
    }
    
    size_t file_size = file.size();
    size_t expected_size = width * height * sizeof(uint16_t);
    
    if (file_size != expected_size) {
        Serial.printf("File size mismatch! Expected %d, got %d\n", expected_size, file_size);
        file.close();
        return false;
    }
    
    // Allocate memory and read the file
    *buffer = (uint16_t*)ps_malloc(file_size);
    if (!*buffer) {
        // If PSRAM allocation fails, try internal RAM as a fallback.
        Serial.println("ps_malloc failed, trying malloc...");
        *buffer = (uint16_t*)malloc(file_size);
    }
    if (!*buffer) { // If both allocations fail, report the error.
        Serial.printf("Failed to allocate memory for eye image buffer: %s\n", filename);
        file.close();
        return false;
    }
    
    file.read((uint8_t*)*buffer, file_size);
    file.close();
    
    Serial.printf("Image '%s' loaded successfully into RAM.\n", filename);
    return true;
}

/**
 * @brief Initializes the sprite used for drawing text.
 */
void init_text_sprite() {
    spr.setTextFont(2);
    spr.setColorDepth(16);
    spr.createSprite(80, 16); // Large enough for "FPS: 99.9"
    spr.setTextDatum(TL_DATUM);
}

/**
 * @brief Draws a string of text into the active framebuffer using a sprite.
 * This is used for displaying the FPS counter.
 * @param string The text to draw.
 * @param x The x-coordinate of the top-left of the text.
 * @param y The y-coordinate of the top-left of the text.
 * @param fgcolor The 16-bit color of the text.
 */
void drawString_fb(const char *string, int32_t x, int32_t y, uint16_t fgcolor) {
    // Use the sprite to draw the text, then copy it to the framebuffer
    spr.fillSprite(TFT_BLACK); // Clear the previous content of the sprite
    spr.setTextColor(fgcolor);
    spr.drawString(string, 0, 0); // Draw the text in the corner of the sprite
    // Copy the rendered text from the sprite to the active framebuffer.
    pushSpriteToFb(&spr, x, y, framebuffers[active_screen_index], TFT_BLACK);
}
/**
 * @brief Initializes the TFT displays, framebuffers, and all graphical assets.
 */
void init_tft() {
  // Allocate framebuffers in PSRAM
  for (int i = 0; i < NUM_SCREEN; i++) {
    framebuffers[i] = (uint16_t*)ps_malloc(SCR_WD * SCR_HT * sizeof(uint16_t));
    if (framebuffers[i] == nullptr) {
      Serial.printf("FATAL: Failed to allocate framebuffer %d in PSRAM\n", i);
      while(1); // Halt
    }
    Serial.printf("Framebuffer %d allocated in PSRAM\n", i);
  }

  Serial.print("init tft ");
  screens[EYE_LEFT].CS = PIN_CS1;
  screens[EYE_RIGHT].CS = PIN_CS2;
  pinMode(screens[EYE_LEFT].CS, OUTPUT);
  pinMode(screens[EYE_RIGHT].CS, OUTPUT);
  
  // Select both screens for simultaneous initialization
  digitalWrite(screens[EYE_LEFT].CS, LOW);
  digitalWrite(screens[EYE_RIGHT].CS, LOW);
  delay(50); // Give displays time to register selection

  
  log_tft_setup(); // Log the configuration details

  
  Serial.print("call tft.init ");
  tft.init();
  Serial.print("call setRotation ");
  tft.setRotation(0); // Set rotation to 0 degrees to correct inverted display
  
  digitalWrite(screens[EYE_LEFT].CS, HIGH);
  digitalWrite(screens[EYE_RIGHT].CS, HIGH);

  precalculate_scanlines(); // Fill our circular screen map

  // Load eye image from LittleFS
  load_specific_eye_image(EYE_IMAGE_NORMAL_PATH, EYE_IMAGE_WIDTH, EYE_IMAGE_HEIGHT, &eye_texture.buffers[EYE_IMAGE_NORMAL]);
  load_specific_eye_image(EYE_IMAGE_BAD_PATH, EYE_IMAGE_WIDTH, EYE_IMAGE_HEIGHT, &eye_texture.buffers[EYE_IMAGE_BAD]);

  init_text_sprite();
}

// --- Image & Asset Management ---

// --- Core Drawing & Rendering ---

/**
 * @brief Draws the circular eye image with eyelid occlusion.
 * This is the main rendering function for the eye itself, using multiple
 * optimizations like pre-calculated scanlines and fixed-point math.
 * @param x_pos The x-coordinate of the top-left of the image.
 * @param y_pos The y-coordinate of the top-left of the image.
 * @param eyelid_level The current level of the eyelid (0=open).
 * @param image_type The type of eye image to draw (normal or bad).
 */
void draw_eye_image(int16_t x_pos, int16_t y_pos, uint8_t eyelid_level, EyeImageType image_type) {
  // If the image buffer has not been loaded, do nothing.
  if (!eye_texture.buffers[image_type]) {
    return;
  }

  const int16_t scaled_width = EYE_IMAGE_WIDTH;
  const int16_t scaled_height = EYE_IMAGE_HEIGHT;

  // Eyelid cutoff calculation
  int16_t eyelid_y_cutoff = (eyelid_level / 128.0f) * (scaled_height / 2.0f);

  // --- Fixed-point integer optimization ---
  uint32_t src_y_accum = 0;
  uint32_t src_increment = 1 * 65536; // Scale factor is implicitly 1.0

  for (int16_t y = 0; y < scaled_height; y++) {
    int16_t dest_y = y_pos + y;

    // Skip lines that are off-screen or closed by the eyelid
    if (dest_y < 0 || dest_y >= SCR_HT || y < eyelid_y_cutoff || y >= (scaled_height - eyelid_y_cutoff)) {
      src_y_accum += src_increment;
      continue;
    }

    // --- Using pre-calculated scanlines ---
    int16_t x_start_visible = circular_scanlines[dest_y].x_start;
    int16_t x_end_visible = circular_scanlines[dest_y].x_end;

    // If the line is not visible, skip to the next one
    if (x_start_visible == -1) {
        src_y_accum += src_increment;
        continue;
    }

    // Determine the actual drawing range, taking the image position into account
    int16_t x_start_draw = max(x_start_visible, (int16_t)x_pos);
    int16_t x_end_draw = min(x_end_visible, (int16_t)(x_pos + scaled_width));

    int16_t src_y = src_y_accum >> 16;
    uint16_t* source_line = &eye_texture.buffers[image_type][src_y * EYE_IMAGE_WIDTH]; // Calculate source line address once
    uint16_t* framebuffer_line = &framebuffers[active_screen_index][(dest_y * SCR_WD)];
    
    // Initialize the X accumulator for the first visible coordinate
    uint32_t src_x_accum = (x_start_draw - x_pos) * src_increment;

    // Do not draw parts of the eye closed by the eyelid
    for (int16_t dest_x = x_start_draw; dest_x < x_end_draw; dest_x++) {
        int16_t src_x = src_x_accum >> 16;

        uint16_t source_color = source_line[src_x]; // Read from the pre-calculated line
        if (source_color == TRANSPARENT_COLOR_KEY) {
            // Do nothing (the color key is transparent)
        } else { // For all other colors
            // Inline the drawPixel logic for performance
            framebuffer_line[dest_x] = swap_color_bytes(source_color);
        }
        src_x_accum += src_increment; // Increment for the next pixel
    }
    src_y_accum += src_increment;
  }
}

/**
 * @brief Draws the eye centered and looking at a normalized target coordinate.
 * This function simplifies the main loop logic by handling all position
 * calculations internally.
 * @param target_x The horizontal target, from -1.0 (left) to 1.0 (right).
 * @param target_y The vertical target, from -1.0 (up) to 1.0 (down).
 * @param eyelid_level The current level of the eyelid (0=open).
 * @param image_type The type of eye image to draw (normal or bad).
 */
void draw_eye_at_target(float target_x, float target_y, uint8_t eyelid_level, EyeImageType image_type) {
    // Calculate the final pixel offset based on the normalized target coordinates
    int16_t x_offset = target_x * MAX_2D_OFFSET_PIXELS;
    int16_t y_offset = target_y * MAX_2D_OFFSET_PIXELS;
    draw_eye_image(RESTING_2D_OFFSET_PIXELS + x_offset, RESTING_2D_OFFSET_PIXELS + y_offset, eyelid_level, image_type);
}

/**
 * @brief Draws a simple crosshair at a given center point.
 * @param center_x The x-coordinate of the crosshair center.
 * @param center_y The y-coordinate of the crosshair center.
 * @param size The size of the crosshair arms.
 * @param color The 16-bit color of the crosshair.
 */
void draw_crosshair(int16_t center_x, int16_t center_y, int16_t size, uint16_t color) {
    uint16_t* current_buffer = framebuffers[active_screen_index];
    uint16_t swapped_color = swap_color_bytes(color);

    // Horizontal line
    for (int16_t x = center_x - size; x <= center_x + size; ++x) {
        if (x >= 0 && x < SCR_WD && center_y >= 0 && center_y < SCR_HT) {
            current_buffer[center_y * SCR_WD + x] = swapped_color;
        }
    }

    // Vertical line
    for (int16_t y = center_y - size; y <= center_y + size; ++y) {
        if (y >= 0 && y < SCR_HT && center_x >= 0 && center_x < SCR_WD) {
            current_buffer[y * SCR_WD + center_x] = swapped_color;
        }
    }
}

/**
 * @brief Draws a debug grid representing the ToF sensor's 8x8 matrix.
 * @param x_pos The top-left x-coordinate of the grid on the screen.
 * @param y_pos The top-left y-coordinate of the grid on the screen.
 * @param grid_size The total size of the grid in pixels.
 * @param data A pointer to the ToF sensor's measurement data.
 */
void draw_tof_debug_grid(int16_t x_pos, int16_t y_pos, int16_t grid_size, const VL53L5CX_ResultsData* data, int8_t highlight_x, int8_t highlight_y) {
    if (!data) return;

    uint16_t* current_buffer = framebuffers[active_screen_index];
    float cell_size = (float)grid_size / 8.0f;

    const int min_dist = 10;  // Distance en mm pour le noir
    const int max_dist = 500; // Distance en mm pour le blanc

    for (int cell_y = 0; cell_y < 8; ++cell_y) {
        for (int cell_x = 0; cell_x < 8; ++cell_x) {
            int zone_index = cell_x * 8 + cell_y; // Transposed: Read data as (y * width + x) to match physical orientation
            int dist = data->distance_mm[zone_index];
            uint8_t status = data->target_status[zone_index];

            uint16_t color;
            // Mettre en surbrillance uniquement si les coordonnées sont valides (pas -1)
            if (highlight_x != -1 && cell_y == highlight_x && cell_x == highlight_y) { // Transposed check to match content drawing
                color = swap_color_bytes(TFT_RED); // Mettre en surbrillance le pixel minimum en rouge
            }
            else {
                // Mapper la distance à une intensité de 0.0 à 1.0
                float intensity = (float)(dist - min_dist) / (float)(max_dist - min_dist);
                intensity = constrain(intensity, 0.0f, 1.0f); // Borner entre 0 et 1
                
                //if (status == 5) { // Détection valide -> Nuances de gris
                    // Convertir l'intensité en une valeur de gris 8 bits (0-255)
                    uint8_t gray_8bit = intensity * 255;
                    // Convertir le gris 8 bits en couleur RGB565
                    uint8_t r = gray_8bit >> 3; // 5 bits pour le rouge
                    uint8_t g = gray_8bit >> 2; // 6 bits pour le vert
                    uint8_t b = gray_8bit >> 3; // 5 bits pour le bleu
                    color = swap_color_bytes((r << 11) | (g << 5) | b);
                //} else { // Détection non valide -> Nuances de bleu
                 //   // Utiliser l'intensité pour moduler la composante bleue
                //    uint8_t b = (intensity * 31); // 5 bits pour le bleu (0-31)
                //    color = swap_color_bytes(b); // Crée une couleur avec seulement du bleu
                //}
            }

            // Dessiner le rectangle pour cette cellule
            int16_t start_px = x_pos + cell_x * cell_size;
            int16_t start_py = y_pos + cell_y * cell_size;
            for (int py = start_py; py < start_py + cell_size; ++py) {
                for (int px = start_px; px < start_px + cell_size; ++px) {
                    if (px >= 0 && px < SCR_WD && py >= 0 && py < SCR_HT) {
                        current_buffer[py * SCR_WD + px] = color;
                    }
                }
            }
        }
    }
}

/**
 * @brief Draws a debug grid representing the correlation scores.
 * @param x_pos The top-left x-coordinate of the grid on the screen.
 * @param y_pos The top-left y-coordinate of the grid on the screen.
 * @param grid_size The total size of the grid in pixels.
 * @param scores A pointer to the 64-element score array.
 */
void draw_score_grid(int16_t x_pos, int16_t y_pos, int16_t grid_size, const long* scores, int8_t highlight_x, int8_t highlight_y) {
    if (!scores) return;

    uint16_t* current_buffer = framebuffers[active_screen_index];
    float cell_size = (float)grid_size / 8.0f;

    // Trouver dynamiquement le min et max score pour normaliser les couleurs
    long min_score = -1;
    long max_score = 0;
    for (int i = 0; i < 64; i++) {
        if (scores[i] >= 0) { // On ne considère que les scores valides (0 ou plus)
            if (min_score == -1 || scores[i] < min_score) {
                min_score = scores[i];
            }
            if (scores[i] > max_score) {
                max_score = scores[i];
            }
        }
    }

    for (int cell_y = 0; cell_y < 8; ++cell_y) {
        for (int cell_x = 0; cell_x < 8; ++cell_x) {
            int zone_index = cell_y * 8 + cell_x;
            long score = scores[zone_index];

            uint16_t color;
            if (highlight_x != -1 && cell_y == highlight_x && cell_x == highlight_y) { // Transposed check
                color = swap_color_bytes(TFT_GREEN); // Mettre en surbrillance le pixel choisi en vert
            } else {
                // Mapper le score à une intensité de 0.0 (bon) à 1.0 (mauvais)
                float intensity = 0.0f;
                if (max_score > min_score) { // Éviter la division par zéro
                    intensity = (float)(score - min_score) / (float)(max_score - min_score);
                }
                // Les pixels de bordure (marqués avec -1) sont dessinés en noir
                if (score < 0) {
                    intensity = -1; // Marqueur pour dessiner en noir
                }

                // Créer un dégradé de Bleu (mauvais) à Rouge (bon)
                uint8_t r = intensity * 255; // Le rouge augmente avec l'intensité (bon score)
                uint8_t g = 0;
                uint8_t b = (1.0f - intensity) * 255; // Le bleu diminue avec l'intensité

                // Convertir en RGB565
                if (intensity == -1) {
                    color = swap_color_bytes(TFT_BLACK);
                } else {
                    color = swap_color_bytes(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
                }
            }

            // Dessiner le rectangle pour cette cellule
            int16_t start_px = x_pos + cell_y * cell_size; // Use cell_y for horizontal position
            int16_t start_py = y_pos + cell_x * cell_size; // Use cell_x for vertical position
            for (int py = start_py; py < start_py + cell_size; ++py) {
                for (int px = start_px; px < start_px + cell_size; ++px) {
                    if (px >= 0 && px < SCR_WD && py >= 0 && py < SCR_HT) {
                        current_buffer[py * SCR_WD + px] = color;
                    }
                }
            }
        }
    }
}

// --- Drawing Primitives ---

// --- Sprite & Animation Functions ---

/**
 * @brief Copies a sprite's content to a target framebuffer ("blitting").
 * Treats a specified color as transparent.
 * @param sprite Pointer to the source sprite.
 * @param x Target x-coordinate in the framebuffer.
 * @param y Target y-coordinate in the framebuffer.
 * @param framebuffer Pointer to the destination framebuffer.
 * @param transparent_color The color in the sprite to treat as transparent.
 */
void pushSpriteToFb(TFT_eSprite* sprite, int32_t x, int32_t y, uint16_t* framebuffer, uint16_t transparent_color) {
    uint16_t* sprite_buffer = (uint16_t*)sprite->getPointer();
    int16_t w = sprite->width();
    int16_t h = sprite->height();

    uint16_t transparent_color_swapped = swap_color_bytes(transparent_color);

    for (int16_t j = 0; j < h; j++) {
        int32_t dest_y = y + j;
        if (dest_y < 0 || dest_y >= SCR_HT) continue;

        uint16_t* framebuffer_line = &framebuffer[dest_y * SCR_WD];
        uint16_t* sprite_line = &sprite_buffer[j * w];

        for (int16_t i = 0; i < w; i++) {
            int32_t dest_x = x + i;
            if (dest_x < 0 || dest_x >= SCR_WD) continue;

            uint16_t color = sprite_line[i];
            if (color != transparent_color_swapped) { // Compare with swapped color
                framebuffer_line[dest_x] = color; // Color is already swapped from sprite
            }
        }
    }
}

/**
 * @brief Displays a splash screen with centered text on both displays.
 * This function is called once at startup.
 */
void show_splash_screen() {
    const char* text = "intellar.ca";
    const int text_font = 4; // Use a larger font for the splash screen
    const uint16_t text_color = TFT_WHITE;
    const uint16_t bg_color = TFT_BLACK;

    // Use the global text sprite
    int16_t text_width = tft.textWidth(text, text_font);
    int16_t text_height = tft.fontHeight(text_font);
    spr.setColorDepth(16);
    spr.setTextFont(text_font); // Définir la police avant de créer le sprite
    spr.createSprite(text_width, text_height + 4); // Add a 4-pixel margin for descenders
    spr.fillSprite(bg_color); // Clear sprite
    spr.setTextColor(text_color, bg_color);
    spr.setTextDatum(MC_DATUM); // Set datum to Middle Center for perfect vertical alignment
    spr.drawString(text, spr.width() / 2, spr.height() / 2); // Draw string in the center of the sprite

    // Calculate y position to center the sprite vertically on the screen
    int16_t x_pos = (SCR_WD - spr.width()) / 2; // Use spr.width() for robustness
    int16_t y_pos = (SCR_HT - spr.height()) / 2; // Use spr.height() which includes the descender margin

    // Draw the splash screen to both framebuffers
    for (int i = 0; i < NUM_SCREEN; i++) {
        select_screen(i);
        clear_buffer(bg_color);
        pushSpriteToFb(&spr, x_pos, y_pos, framebuffers[i], bg_color);
    }

    // Display on both screens and wait
    display_all_buffers();

    // Clean up the large sprite to prevent it from interfering with other functions
    spr.deleteSprite();

    // Re-initialize the smaller sprite for the main loop's FPS counter
    init_text_sprite();
}
