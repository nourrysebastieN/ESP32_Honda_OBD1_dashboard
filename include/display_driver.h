/**
 * @file display_driver.h
 * @brief Display driver interface for LVGL with LovyanGFX
 * 
 * This file provides the display driver configuration and initialization
 * for the 7" LCD panel with touch support.
 */

#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include "config.h"

#ifndef LGFX_USE_V1
#define LGFX_USE_V1
#endif

#include <LovyanGFX.hpp>
#include <lvgl.h>

#if defined(DISPLAY_USE_RGB) && DISPLAY_USE_RGB
  #include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
  #include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#endif

/**
 * @brief Custom LGFX class for the 7" display
 * 
 * Inherit from LGFX_Device and configure for your specific display.
 * This example is configured for a common 7" 800x480 display.
 * Modify the Bus_SPI or Bus_Parallel configuration based on your display type.
 */
class LGFX : public lgfx::LGFX_Device {
    // Define the bus/panel type based on your display interface.
    // Makerfabs MaTouch 7" uses an RGB interface; SPI fallback remains for other boards.
#if defined(DISPLAY_USE_RGB) && DISPLAY_USE_RGB
    lgfx::Bus_RGB     _bus_instance;
    lgfx::Panel_RGB   _panel_instance;
    lgfx::Light_PWM   _light_instance;
    lgfx::Touch_GT911 _touch_instance;    // GT911 capacitive touch
#else
    lgfx::Bus_SPI     _bus_instance;
    lgfx::Panel_ILI9488 _panel_instance;  // Change panel type as needed
    lgfx::Touch_GT911 _touch_instance;    // GT911 capacitive touch
#endif

public:
    LGFX(void);
};

/**
 * @brief Display driver namespace for LVGL integration
 */
namespace DisplayDriver {
    /**
     * @brief Initialize the display driver
     * @return true if initialization successful, false otherwise
     */
    bool init(void);
    
    /**
     * @brief Get the LGFX display instance
     * @return Reference to the display instance
     */
    LGFX& getDisplay(void);
    
    /**
     * @brief Get the LVGL display object
     * @return Pointer to the LVGL display
     */
    lv_display_t* getLvDisplay(void);
    
    /**
     * @brief Get the LVGL input device object
     * @return Pointer to the LVGL input device
     */
    lv_indev_t* getLvInputDevice(void);
    
    /**
     * @brief Update the display (call in main loop)
     */
    void update(void);
    
    /**
     * @brief Set display brightness
     * @param brightness Brightness level (0-255)
     */
    void setBrightness(uint8_t brightness);
    
    /**
     * @brief Get display width
     * @return Display width in pixels
     */
    int getWidth(void);
    
    /**
     * @brief Get display height
     * @return Display height in pixels
     */
    int getHeight(void);
}

#endif /* DISPLAY_DRIVER_H */
