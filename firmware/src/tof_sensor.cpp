/**
 * @file tof_sensor.cpp
 * @author Intellar (https://github.com/intellar)
 * @brief Implementation for controlling the VL53L5CX Time-of-Flight sensor.
 * @version 1.0
 *
 * @copyright Copyright (c) 2025
 *
 * @license See LICENSE.md for details.
 *
 */
#include "tof_sensor.h"
#include <Wire.h>
#include <cmath> // Pour fabsf
#include "config.h" // Pour accéder à USE_TOF_SENSOR
#if USE_TOF_SENSOR

// --- ToF Sensor State (private to this file) ---
static SparkFun_VL53L5CX myImager;
static VL53L5CX_ResultsData measurementData; // Raw measurement data from the sensor
static TofTarget current_target = {0, 0, 0, false, -1, -1, 0}; // The currently tracked target, initialized

/**
 * @brief Initializes the VL53L5CX ToF sensor.
 */
void init_tof_sensor() {
  Serial.println("Initializing VL53L5CX ToF Sensor...");
  Wire.begin(PIN_TOF_SDA, PIN_TOF_SCL);
  // Increase I2C bus speed for faster data transfer.
  // We are switching to 1MHz (1000000Hz) for maximum performance.
  Wire.setClock(1000000); 

  if (myImager.begin() == false) {
    Serial.println("ERROR: VL53L5CX Sensor not found. Rebooting in 3 seconds...");
    delay(3000);
    ESP.restart();
  }

  myImager.setResolution(8 * 8); // 64 zones de mesure
  myImager.setRangingFrequency(15); // 15 Hz
  myImager.startRanging();

  Serial.println("VL53L5CX Sensor Initialized.");
}

#if TOF_CALIBRATION_MODE
/**
 * @brief Generates simulated ToF data for calibration and debugging.
 * This function creates a synthetic 8x8 matrix with a clear target pattern
 * that moves to different positions at a regular interval.
 */
static void run_calibration_simulation() {
    static unsigned long last_calib_change_time = 0;
    static int calib_position_index = 0;
    const int CALIB_INTERVAL_MS = 1000; // Shortened interval to cycle through more points faster
    const int NUM_CALIB_POSITIONS = 13;

    // Cycle through target positions at a regular interval
    if (millis() - last_calib_change_time > CALIB_INTERVAL_MS) {
        last_calib_change_time = millis();
        calib_position_index = (calib_position_index + 1) % NUM_CALIB_POSITIONS; // Cycle through 13 positions
    }

    // Define test positions: 9 inside and 4 on the corners to test partial detection
    const int calib_positions[NUM_CALIB_POSITIONS][2] = {
        // --- Fully visible patterns ---
        {1, 1}, {1, 4}, {1, 6}, // Top row
        {4, 1}, {4, 4}, {4, 6}, // Middle row
        {6, 1}, {6, 4}, {6, 6}, // Bottom row
        // --- Partially visible patterns (center of pattern is on the corner pixel) ---
        {0, 0}, // Top-left corner (only 2x2 of the 3x3 pattern is visible)
        {0, 7}, // Top-right corner
        {7, 0}, // Bottom-left corner
        {7, 7}  // Bottom-right corner
    };

    int target_x = calib_positions[calib_position_index][0];
    int target_y = calib_positions[calib_position_index][1];

    // Simulate the data matrix
    const int BG_DIST = 1000; // Background distance
    const int TARGET_DIST = 200; // Target distance
    const int NEIGHBOR_DIST = 300; // Target's neighbor distance

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int index = y * 8 + x;
            int dist_x = abs(x - target_x);
            int dist_y = abs(y - target_y);

            if (dist_x == 0 && dist_y == 0) {
                measurementData.distance_mm[index] = TARGET_DIST;
            } else if (dist_x <= 1 && dist_y <= 1) {
                measurementData.distance_mm[index] = NEIGHBOR_DIST;
            } else {
                measurementData.distance_mm[index] = BG_DIST;
            }
            measurementData.target_status[index] = 5; // Mark all points as valid for the simulation
        }
    }
}
#endif

/**
 * @brief Logs the ToF sensor's measurement matrix to the Serial monitor
 * in a Python-friendly format for debugging and analysis.
 * @param data Pointer to the measurement data structure.
 */
static void log_measurement_matrix(const VL53L5CX_ResultsData* data) {
    if (!data) {
        Serial.println("No measurement data to log.");
        return;
    }

    Serial.println("distance_matrix = [");
    for (int y = 0; y < 8; ++y) {
        Serial.print("  [");
        for (int x = 0; x < 8; ++x) {
            int index = y * 8 + x;
            Serial.printf("%4d", data->distance_mm[index]);
            if (x < 7) {
                Serial.print(", ");
            }
        }
        Serial.print(y == 7 ? "]" : "],");
        Serial.println();
    }
    Serial.println("]");

    Serial.println("\nstatus_matrix = [");
    for (int y = 0; y < 8; ++y) {
        Serial.print("  [");
        for (int x = 0; x < 8; ++x) {
            int index = y * 8 + x;
            Serial.printf("%d", data->target_status[index]);
            if (x < 7) {
                Serial.print(", ");
            }
        }
        Serial.print(y == 7 ? "]" : "],");
        Serial.println();
    }
    Serial.println("---------------------------------\n");
}

/**
 * @brief Processes the raw measurement data to find a stable target.
 * This function implements a sliding window averaging algorithm to find the
 * center of the most stable region of low distances.
 * @param profile_start_time The start time for profiling purposes.
 */
static void process_measurement_data(unsigned long profile_start_time) {
    const int MIN_RELIABLE_PIXELS_IN_WINDOW = 4; // Require at least 4 valid pixels in a 3x3 window to consider it a target.
    float best_avg_dist = 3.4028235E+38; // Initialize with FLT_MAX
    int best_target_index = -1;

    // Iterate through all 64 pixels as potential centers of a target.
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            int center_index = r * 8 + c;

            // Skip this pixel if it's not a valid starting point for a target.
            if (measurementData.target_status[center_index] != 5 || measurementData.distance_mm[center_index] >= MAX_DIST_TOF) {
                continue;
            }

            // --- Evaluate the 3x3 window around the current pixel ---
            long distance_sum = 0;
            int reliable_pixel_count = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = c + dx;
                    int ny = r + dy;

                    // Check if the neighbor is within the 8x8 grid.
                    if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
                        int neighbor_index = ny * 8 + nx;
                        if (measurementData.target_status[neighbor_index] == 5 && measurementData.distance_mm[neighbor_index] < MAX_DIST_TOF) {
                            distance_sum += measurementData.distance_mm[neighbor_index];
                            reliable_pixel_count++;
                        }
                    }
                }
            }

            // If this window is reliable, see if it's the best one we've found so far.
            if (reliable_pixel_count >= MIN_RELIABLE_PIXELS_IN_WINDOW) {
                float avg_dist = (float)distance_sum / reliable_pixel_count;
                if (avg_dist < best_avg_dist) {
                    best_avg_dist = avg_dist;
                    best_target_index = center_index;
                }
            }
        }
    }

    // After checking all pixels, if we found a reliable target, update the state.
    if (best_target_index != -1) {
        int pixel_y = best_target_index / 8; // Row
        int pixel_x = best_target_index % 8; // Column

        current_target.x = (static_cast<float>(pixel_y) - 3.5f) / 3.5f;
        current_target.y = (static_cast<float>(pixel_x) - 3.5f) / 3.5f;
        current_target.distance_mm = measurementData.distance_mm[best_target_index];
        current_target.min_dist_pixel_x = pixel_x;
        current_target.min_dist_pixel_y = pixel_y;
        current_target.is_valid = true;
        current_target.match_score = best_avg_dist;
    } else {
        // If no reliable target was found anywhere, invalidate the current target.
        current_target.is_valid = false;
        current_target.min_dist_pixel_x = -1;
        current_target.min_dist_pixel_y = -1;
        current_target.match_score = 0;
    }
}

/**
 * @brief Reads data from the ToF sensor and processes it.
 */
void update_tof_sensor_data() {
#if TOF_CALIBRATION_MODE
    // --- SIMULATION LOGIC FOR CALIBRATION ---
    run_calibration_simulation();
    // Analyze simulated data with the standard code
    process_measurement_data(micros());

#else
  // Run detection logic only when new data is available
  if (myImager.isDataReady()) {
    unsigned long profile_start_time = micros();
    if (myImager.getRangingData(&measurementData)) {
        process_measurement_data(profile_start_time);
    }
  }
#endif
}

TofTarget get_tof_target() {
    return current_target;
}

/**
 * @brief Returns a pointer to the raw measurement data structure.
 */
const VL53L5CX_ResultsData* get_tof_measurement_data() {
    return &measurementData;
}

#else // If USE_TOF_SENSOR is 0

// Provide empty functions so the program compiles without the sensor.
void init_tof_sensor() { /* Does nothing */ }
void update_tof_sensor_data() { /* Does nothing */ }
TofTarget get_tof_target() {
    return {0, 0, 0, false, -1, -1, 0}; // Always return an invalid target
}
const VL53L5CX_ResultsData* get_tof_measurement_data() {
    return nullptr; // Return a null pointer when the sensor is disabled
}

#endif