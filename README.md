# ESP32 Animated Eye Project

A high-performance, procedurally animated eye project for the ESP32, designed to run on dual round TFT displays. This project simulates a realistic gaze with natural, darting movements.

## Author & License

*   **Author:** Intellar ([github.com/intellar](https://github.com/intellar))
*   **License:** This project is released under a custom license. See the LICENSE.md file for details.

---

## Features

*   **2D Gaze Model:** Simulates eye movement by choosing random points on a 2D plane.
*   **Procedural Animation:** All animation is generated in code, not pre-rendered.
    *   **Saccadic Eye Movements:** Mimics natural, darting motions by smoothly interpolating to new random targets.
*   **High-Performance Rendering:** Achieves smooth frame rates on an ESP32 using several optimization techniques:
    *   Double-buffering for tear-free updates.    
    *   Pre-calculated scanlines for fast circular clipping.
*   **Highly Configurable:** Almost all parameters, from animation timings to hardware pins, are easily tweakable in the central `config.h` file.
*   **Memory Usage Logging:** Prints detailed heap and PSRAM usage to the serial monitor at startup.
*   **Asset-Based:** Uses `.bin` image files for eye textures, loaded from the ESP32's LittleFS filesystem.

## Hardware Requirements

*   **Microcontroller:** An ESP32 (WROOM, S2, S3, etc.). PSRAM is highly recommended.
*   **Displays:** Two round 240x240 TFT displays based on the ST7789 driver.
*   **Wiring:** Appropriate wiring to connect the displays to the ESP32's SPI pins.

## Software & Dependencies

*   **Framework:** Arduino IDE or PlatformIO.
*   **Libraries:**
    *   `TFT_eSPI` by Bodmer. This must be configured for your specific ESP32 and ST7789 displays.
    *   `LittleFS` library for ESP32 (usually included with the ESP32 board package).

## Installation & Setup

1.  **Install Libraries:**
    *   Install the `TFT_eSPI` library using the Arduino Library Manager.
    *   **Crucially**, you must configure `TFT_eSPI` for your hardware. Edit the `User_Setup.h` file inside the `TFT_eSPI` library folder to define your ESP32 model and the pins connected to your displays.

2.  **Prepare Image Assets:**
    *   This project uses raw 16-bit (RGB565) binary image files for the eye textures. The default size is 350x350 pixels.
    *   Create your eye texture (e.g., `eye_real.bin`). A helper tool is provided in the `image_tools` directory.
    *   Place your `.bin` file (e.g., `eye_real.bin`) inside a `data` folder in your Arduino sketch directory.

3.  **Upload Filesystem to ESP32:**
    *   In the Arduino IDE, you need the **ESP32 Sketch Data Upload** tool. If you don't have it, you can find installation instructions online for "ESP32 Sketch Data Upload".
    *   Once installed, go to `Tools` -> `ESP32 Sketch Data Upload` to upload the contents of your `data` folder to the ESP32's LittleFS partition.

4.  **Configure Project:**
    *   Open the `config.h` file in this project.
    *   Set `PIN_CS1` and `PIN_CS2` to match the chip-select pins you have wired for your left and right displays.

5.  **Compile and Upload:**
    *   Select your ESP32 board and port in the Arduino IDE.
    *   Compile and upload the sketch.

## Creating Custom Eye Textures

A Python-based GUI tool is included to help you convert your own images into the required `.bin` format.

1.  **Location:** The tool is located at `image_tools/gui-image-tools.py`.

2.  **Requirements:** You need Python and the `Pillow` library installed.
    ```bash
    pip install Pillow
    ```

3.  **How to Use:**
    *   Run the script: `python image_tools/gui-image-tools.py`
    *   Click "Load Image (PNG/BMP)" to select your source image.
    *   The width and height will be auto-filled. Ensure they match the dimensions in `config.h` (e.g., 350x350).
    *   Click "Generate Binary File" and save the output (e.g., `eye_real.bin`) into the `data` folder of your sketch.

## How It Works

The animation is driven by a state-based system in the main `loop()`.

1.  **Gaze Calculation (`update_eye_target`, `update_eye_position`):**
    *   A random 2D `target_offset` is chosen within a circular area.
    *   The code smoothly interpolates the `current_offset` towards this target to create fluid movement.
    *   This final `(x, y)` offset is used to position the eye texture on each screen.
2.  **Rendering (`draw_scene`):**
    *   The scene is drawn into off-screen framebuffers (one for each eye).
    *   The main eye texture is drawn using `draw_scaled_circular_image_with_eyelid`, which performs circular clipping.
3.  **Display:**
    *   The contents of both framebuffers are pushed to the physical displays simultaneously.

## Configuration (`config.h`)

This file is the central control panel for the project.

*   **Hardware & Pinout:** Define your screen chip-select pins.
*   **Display & Image:** Set the dimensions of your source images and the transparent color key.
*   **Asset File Paths:** Change the names of the `.bin` files if you use different assets.
*   **Animation Behavior:** Adjust the eye's movement range, speed, and the timing for saccades.
*   **Memory Monitoring:** The project logs memory usage to the serial monitor at startup.

## Contributing

Contributions, issues, and feature requests are welcome!

---