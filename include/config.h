/**
 * @file config.h
 * @brief Main configuration header for ESP32-S3 Honda OBD1 Dashboard
 * 
 * This file contains all the hardware pin definitions and system configurations.
 * Modify these values according to your specific hardware setup.
 */

#ifndef CONFIG_H
#define CONFIG_H

/*====================
   DISPLAY CONFIGURATION
   ====================
   
   Configure these pins based on your 7" display module.
   Common 7" ESP32-S3 displays use RGB parallel interface.
   Adjust according to your specific display board.
   ====================*/

// Display dimensions
#define DISPLAY_WIDTH  1024
#define DISPLAY_HEIGHT 600

// Display backlight pin
#define TFT_BL 45

// Display SPI pins (for SPI-based displays)
#define TFT_MOSI 11
#define TFT_MISO 13
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   46
#define TFT_RST  -1  // Connected to EN/Reset

// For RGB parallel displays (common for 7" displays)
// Uncomment and configure if using RGB interface
/*
#define TFT_DE   40
#define TFT_VSYNC 41
#define TFT_HSYNC 39
#define TFT_PCLK  42

#define TFT_R0   45
#define TFT_R1   48
#define TFT_R2   47
#define TFT_R3   21
#define TFT_R4   14

#define TFT_G0   5
#define TFT_G1   6
#define TFT_G2   7
#define TFT_G3   15
#define TFT_G4   16
#define TFT_G5   4

#define TFT_B0   8
#define TFT_B1   3
#define TFT_B2   46
#define TFT_B3   9
#define TFT_B4   1
*/

/*====================
   TOUCH CONFIGURATION
   ====================*/

// I2C Touch controller pins (GT911 or similar)
#define TOUCH_SDA 17
#define TOUCH_SCL 18
#define TOUCH_INT 38
#define TOUCH_RST 16

/*====================
   OBD1 CONFIGURATION
   ====================
   
   Honda OBD1 ECU communication settings.
   Uses a single data line protocol at specific baud rate.
   ====================*/

// OBD1 serial communication pin
#define OBD1_RX_PIN 44
#define OBD1_TX_PIN 43

// Honda OBD1 baud rate (9600 baud for most Honda ECUs)
#define OBD1_BAUD_RATE 9600

// OBD1 data request interval (ms)
#define OBD1_REQUEST_INTERVAL 50

/*====================
   LED/STATUS INDICATORS
   ====================*/

// Onboard RGB LED (ESP32-S3 DevKitC-1)
#define LED_PIN 48

// External status LED
#define STATUS_LED_PIN 2

/*====================
   SYSTEM CONFIGURATION
   ====================*/

// FreeRTOS task priorities
#define TASK_PRIORITY_OBD1     5
#define TASK_PRIORITY_DISPLAY  4
#define TASK_PRIORITY_UI       3

// Task stack sizes
#define TASK_STACK_OBD1     4096
#define TASK_STACK_DISPLAY  8192
#define TASK_STACK_UI       8192

// Display buffer size (lines)
#define DISPLAY_BUF_LINES  40

// LVGL tick period (ms)
#define LVGL_TICK_PERIOD_MS 2

/*====================
   DASHBOARD PARAMETERS
   ====================*/

// Speed gauge configuration
#define SPEED_MAX       200   // Maximum speed display (km/h or mph)
#define SPEED_WARNING   120   // Warning threshold

// RPM gauge configuration  
#define RPM_MAX         9000  // Maximum RPM display
#define RPM_REDLINE     7500  // Redline threshold
#define RPM_WARNING     7000  // Warning threshold

// Temperature gauge configuration
#define TEMP_MIN        60    // Minimum temperature display (°C)
#define TEMP_MAX        130   // Maximum temperature display (°C)
#define TEMP_WARNING    100   // Warning threshold (°C)

// Fuel level configuration
#define FUEL_WARNING    15    // Warning threshold (%)

/*====================
   DEBUG CONFIGURATION
   ====================*/

// Enable debug output via Serial
#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED 1
#endif

// Debug serial baud rate
#define DEBUG_BAUD_RATE 115200

#if DEBUG_ENABLED
    #define DEBUG_PRINT(x)    Serial.print(x)
    #define DEBUG_PRINTLN(x)  Serial.println(x)
    #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

#endif /* CONFIG_H */
