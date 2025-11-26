# ESP32 Animated Eye with ToF Sensor Tracking

A high-performance, animated eye project for the ESP32-S3 that uses a VL53L5CX Time-of-Flight sensor to track objects and direct the eye's gaze.

Demo video:  https://youtube.com/shorts/loqei5ePCf8
Project description:  intellar.ca


![Project Demo Image](https://github.com/intellar/Dual_Display_ESP32/blob/bd06864a046d26ce7f1886804b78c7f66b0bc624/demo.png)

## Author & License

*   **Author:** Intellar ([github.com/intellar](https://github.com/intellar))
*   **License:** This project is released under a custom license. See the LICENSE.md file for details.  Acknowledge and cite intellar in derivative works !

---

## Features

*   **Target Tracking:** Uses a VL53L5CX 8x8 ToF sensor to detect the closest object and direct the eye's gaze towards it.
*   **Procedural Animation:** All animation is generated in code.
    *   **Smooth Pursuit:** The eye smoothly follows a moving target.
    *   **Saccadic Idle Movement:** When no target is present, the eye mimics natural, darting motions (saccades) by interpolating to new random points.
*   **High-Performance Rendering:** Achieves smooth frame rates on an ESP32-S3 using several optimizations:
    *   **Double Buffering:** Off-screen framebuffers in PSRAM ensure tear-free updates.
    *   Pre-calculated scanlines for fast circular clipping.
    *   **Optimized Drawing:** Uses fixed-point math and direct framebuffer manipulation.
*   **PlatformIO Environment:** Configured for a professional workflow with VS Code, providing faster compilation and easier dependency management.
*   **Advanced Debugging:**
    *   **Calibration Mode:** A built-in simulation mode (`TOF_CALIBRATION_MODE`) tests the tracking logic with a virtual target pattern.
    *   **Debug Grid:** An optional real-time visualization of the ToF sensor's 8x8 matrix can be overlaid on one of the displays.
*   **Asset-Based:** Uses `.bin` image files for eye textures, loaded from the ESP32's LittleFS filesystem at runtime.

## Hardware Requirements

*   **Microcontroller:** An ESP32-S3 with PSRAM (e.g., ESP32-S3-DevKitC-1-N16R8V).
*   **Displays:** Two round 240x240 TFT displays based on the **GC9A01** driver.
*   **ToF Sensor:** A **VL53L5CX** Time-of-Flight sensor breakout board (e.g., from SparkFun).

## Software & Dependencies

*   **IDE:** Visual Studio Code with the **PlatformIO** extension.
*   **Libraries:**
    *   `bodmer/TFT_eSPI`
    *   `sparkfun/SparkFun VL53L5CX Arduino Library`
    *   These are managed automatically by PlatformIO via the `platformio.ini` file.

## Installation & Setup

1.  **Clone the Repository:**
    ```bash
    git clone https://github.com/intellar/Dual_Display_ESP32.git
    ```

2.  **Open in VS Code:**
    *   Open the cloned `Dual_Display_ESP32/firmware` folder in Visual Studio Code.
    *   PlatformIO should automatically recognize the project and prompt you to install the required library dependencies.

3.  **Configure Hardware Pins in `platformio.ini`:**
    *   Open the `platformio.ini` file. This is the central place for all hardware configuration.
    *   **TFT_eSPI Configuration:** All display settings are defined here using `build_flags`. This method bypasses the need to edit the `TFT_eSPI` library's `User_Setup.h` file.
    *   Adjust the pin numbers (`-D TFT_MOSI=...`, `-D TFT_SCLK=...`, etc.) and other display settings to match your specific wiring.
    *   The chip select pins for the two displays (`PIN_CS1`, `PIN_CS2`) are defined in `firmware/include/config.h`.

4.  **Upload Filesystem:**
    *   The eye texture images (`.bin` files) are located in the `firmware/data` directory.
    *   In the PlatformIO sidebar, expand the project tasks for your environment (e.g., `esp32-s3-devkitc-1-n16r8v`) and run **"Upload Filesystem Image"**.

5.  **Compile and Upload:**
    *   Run the **"Upload"** task from the PlatformIO sidebar.

## Creating Custom Eye Textures

A Python-based GUI tool is included to help you convert your own images into the required `.bin` format.

1.  **Location:** The tool is located at `image_tools/gui-image-tools.py`.

2.  **Requirements:** You need Python 3 and the `Pillow` library installed.
    ```bash
    pip install Pillow
    ```

3.  **How to Use:**
    *   Run the script: `python image_tools/gui-image-tools.py`
    *   Click "Load Image (PNG/BMP)" to select your source image.
    *   Ensure the dimensions match those in `firmware/include/config.h` (e.g., 350x350).
    *   Click "Generate Binary File" and save the output into the `firmware/data` folder, overwriting the existing assets if desired.
    *   Remember to re-run the "Upload Filesystem Image" task in PlatformIO after changing assets.

## How It Works

The animation is driven by a state-based system in the main `loop()`.

1.  **Sensor Update (`update_tof_sensor_data`):**
    *   The VL53L5CX sensor is polled for new data.
    *   `process_measurement_data` analyzes the 8x8 distance matrix to find the pixel with the minimum distance, validating it against its neighbors to ensure it's a reliable target.
    *   If a valid target is found, its coordinates are mapped from the sensor's grid (0-7) to the eye's normalized coordinate system (-1.0 to 1.0).

2.  **Gaze Logic:**
    *   If a target is valid, its coordinates become the goal for the eye's gaze.
    *   If no target is valid, the system falls back to the idle saccade behavior, picking a new random point to look at after a set interval.
    *   The current eye position is smoothly interpolated (LERPed) towards the goal position on every frame, creating fluid motion.

3.  **Rendering (`draw_eye_at_target`):**
    *   The scene is drawn into off-screen framebuffers.
    *   The main eye texture is drawn at an offset calculated from the final eye position.
    *   The optional debug grid and FPS counter are drawn on top.

4.  **Display:**
    *   The contents of the completed framebuffers are pushed to the physical displays.

## Configuration

Key parameters can be adjusted in two files:

### `platformio.ini`
*   **Hardware Pins:** All pin definitions for the displays (SPI) and ToF sensor (I2C) are located in the `build_flags` section. This is where you configure `TFT_eSPI`.

### `firmware/include/config.h`
*   **Features:** Enable or disable the ToF sensor (`USE_TOF_SENSOR`) or activate the calibration simulation (`TOF_CALIBRATION_MODE`).
*   **Animation Behavior:** Adjust the eye's movement range (`MAX_2D_OFFSET_PIXELS`), interpolation speed (`LERP_SPEED`), and the timing for idle saccades.
*   **Sensor Behavior:** Configure the maximum tracking distance (`MAX_DIST_TOF`).

## Contributing

Contributions, issues, and feature requests are welcome!

---
