/**
 * @file display_driver.cpp
 * @brief Display driver implementation for LVGL with LovyanGFX
 * 
 * Implementation of the display driver for the 7" LCD panel.
 * This provides the bridge between LVGL and the LovyanGFX graphics library.
 */

#include "display_driver.h"
#include "config.h"
#include <Arduino.h>

// Static instances
static LGFX lcd;
static lv_display_t* lv_disp = nullptr;
static lv_indev_t* lv_indev = nullptr;

// Display buffer
static lv_color_t* buf1 = nullptr;
static lv_color_t* buf2 = nullptr;

/**
 * @brief LGFX constructor - configure display hardware
 * 
 * Modify this constructor based on your specific display hardware.
 * This example is configured for a SPI-based display.
 * For RGB parallel displays, use Bus_Parallel16 instead.
 */
LGFX::LGFX(void) {
#if defined(DISPLAY_USE_RGB) && DISPLAY_USE_RGB
    // Configure panel (RGB / dot-clock, uses PSRAM framebuffer)
    {
        auto cfg = _panel_instance.config();

        cfg.memory_width  = DISPLAY_WIDTH;
        cfg.panel_width   = DISPLAY_WIDTH;
        cfg.memory_height = DISPLAY_HEIGHT;
        cfg.panel_height  = DISPLAY_HEIGHT;

        cfg.offset_x = 0;
        cfg.offset_y = 0;

        _panel_instance.config(cfg);
    }

    {
        auto cfg = _panel_instance.config_detail();
#ifdef BOARD_HAS_PSRAM
        cfg.use_psram = 1;
#else
        cfg.use_psram = 0;
#endif
        _panel_instance.config_detail(cfg);
    }

    // Configure RGB bus (16-bit)
    {
        auto cfg = _bus_instance.config();
        cfg.panel = &_panel_instance;

        cfg.pin_d0  = TFT_B0;
        cfg.pin_d1  = TFT_B1;
        cfg.pin_d2  = TFT_B2;
        cfg.pin_d3  = TFT_B3;
        cfg.pin_d4  = TFT_B4;
        cfg.pin_d5  = TFT_G0;
        cfg.pin_d6  = TFT_G1;
        cfg.pin_d7  = TFT_G2;
        cfg.pin_d8  = TFT_G3;
        cfg.pin_d9  = TFT_G4;
        cfg.pin_d10 = TFT_G5;
        cfg.pin_d11 = TFT_R0;
        cfg.pin_d12 = TFT_R1;
        cfg.pin_d13 = TFT_R2;
        cfg.pin_d14 = TFT_R3;
        cfg.pin_d15 = TFT_R4;

        cfg.pin_henable = TFT_DE;
        cfg.pin_vsync   = TFT_VSYNC;
        cfg.pin_hsync   = TFT_HSYNC;
        cfg.pin_pclk    = TFT_PCLK;
        cfg.freq_write  = TFT_PCLK_FREQ;

        cfg.hsync_polarity    = TFT_HSYNC_POLARITY;
        cfg.hsync_front_porch = TFT_HSYNC_FRONT_PORCH;
        cfg.hsync_pulse_width = TFT_HSYNC_PULSE_WIDTH;
        cfg.hsync_back_porch  = TFT_HSYNC_BACK_PORCH;
        cfg.vsync_polarity    = TFT_VSYNC_POLARITY;
        cfg.vsync_front_porch = TFT_VSYNC_FRONT_PORCH;
        cfg.vsync_pulse_width = TFT_VSYNC_PULSE_WIDTH;
        cfg.vsync_back_porch  = TFT_VSYNC_BACK_PORCH;
        cfg.pclk_idle_high    = TFT_PCLK_IDLE_HIGH;

        _bus_instance.config(cfg);
    }
    _panel_instance.setBus(&_bus_instance);

    // Backlight (PWM)
    {
        auto cfg = _light_instance.config();
        cfg.pin_bl = TFT_BL;
#if defined(TFT_BL_INVERT)
        cfg.invert = TFT_BL_INVERT;
#endif
        _light_instance.config(cfg);
    }
    _panel_instance.light(&_light_instance);

    // Touch (GT911 over I2C)
    {
        auto cfg = _touch_instance.config();

        cfg.x_min = 0;
        cfg.x_max = DISPLAY_WIDTH;
        cfg.y_min = 0;
        cfg.y_max = DISPLAY_HEIGHT;
        cfg.pin_int = TOUCH_INT;
        cfg.pin_rst = TOUCH_RST;
        cfg.bus_shared = false;
        cfg.offset_rotation = 0;

        cfg.i2c_port = TOUCH_I2C_PORT;
        cfg.i2c_addr = 0x5D;
        cfg.pin_sda = TOUCH_SDA;
        cfg.pin_scl = TOUCH_SCL;
        cfg.freq = 400000;

        _touch_instance.config(cfg);
        _panel_instance.setTouch(&_touch_instance);
    }
#else
    // Configure SPI bus (fallback)
    {
        auto cfg = _bus_instance.config();

        cfg.spi_host = SPI2_HOST;     // SPI2 for ESP32-S3
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000;    // SPI clock for writing (40MHz)
        cfg.freq_read = 16000000;     // SPI clock for reading
        cfg.spi_3wire = true;
        cfg.use_lock = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;

        cfg.pin_sclk = TFT_SCLK;
        cfg.pin_mosi = TFT_MOSI;
        cfg.pin_miso = TFT_MISO;
        cfg.pin_dc = TFT_DC;

        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
    }

    // Configure panel
    {
        auto cfg = _panel_instance.config();

        cfg.pin_cs = TFT_CS;
        cfg.pin_rst = TFT_RST;
        cfg.pin_busy = -1;

        cfg.panel_width = DISPLAY_WIDTH;
        cfg.panel_height = DISPLAY_HEIGHT;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        cfg.offset_rotation = 0;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits = 1;
        cfg.readable = true;
        cfg.invert = false;
        cfg.rgb_order = false;
        cfg.dlen_16bit = false;
        cfg.bus_shared = true;

        _panel_instance.config(cfg);
    }

    // Configure touch (GT911 capacitive)
    {
        auto cfg = _touch_instance.config();

        cfg.x_min = 0;
        cfg.x_max = DISPLAY_WIDTH - 1;
        cfg.y_min = 0;
        cfg.y_max = DISPLAY_HEIGHT - 1;
        cfg.pin_int = TOUCH_INT;
        cfg.pin_rst = TOUCH_RST;
        cfg.bus_shared = false;
        cfg.offset_rotation = 0;

        // I2C configuration
        cfg.i2c_port = TOUCH_I2C_PORT;
        cfg.i2c_addr = 0x5D;  // GT911 I2C address
        cfg.pin_sda = TOUCH_SDA;
        cfg.pin_scl = TOUCH_SCL;
        cfg.freq = 400000;

        _touch_instance.config(cfg);
        _panel_instance.setTouch(&_touch_instance);
    }
#endif

    setPanel(&_panel_instance);
}

/**
 * @brief LVGL display flush callback
 * Sends pixel data to the display
 */
static void lvgl_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.writePixels((lgfx::rgb565_t*)px_map, w * h);
    lcd.endWrite();
    
    lv_display_flush_ready(disp);
}

/**
 * @brief LVGL touch read callback
 * Reads touch input from the display
 */
static void lvgl_touch_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    uint16_t touchX, touchY;
    
    if (lcd.getTouch(&touchX, &touchY)) {
        data->point.x = touchX;
        data->point.y = touchY;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/**
 * @brief LVGL tick callback for timing
 */
static uint32_t lvgl_tick_cb(void) {
    return millis();
}

namespace DisplayDriver {

bool init(void) {
    DEBUG_PRINTLN("Initializing display driver...");
    
    // Initialize display
    lcd.init();
    lcd.setRotation(0);  // Adjust rotation as needed (0-3)
    lcd.setBrightness(255);
    lcd.fillScreen(TFT_BLACK);
    
    DEBUG_PRINTLN("Display initialized");
    
    // Initialize LVGL
    lv_init();
    
    // Set tick callback
    lv_tick_set_cb(lvgl_tick_cb);
    
    // Calculate buffer size (use partial buffer to save memory)
    uint32_t buf_size = DISPLAY_WIDTH * DISPLAY_BUF_LINES;
    
    // Allocate display buffers in PSRAM if available
    #ifdef BOARD_HAS_PSRAM
        buf1 = (lv_color_t*)ps_malloc(buf_size * sizeof(lv_color_t));
        buf2 = (lv_color_t*)ps_malloc(buf_size * sizeof(lv_color_t));
        DEBUG_PRINTLN("Using PSRAM for display buffers");
    #else
        buf1 = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
        buf2 = nullptr;  // Single buffer if no PSRAM
        DEBUG_PRINTLN("Using internal RAM for display buffer");
    #endif
    
    if (buf1 == nullptr) {
        DEBUG_PRINTLN("ERROR: Failed to allocate display buffer!");
        return false;
    }
    
    // Create LVGL display
    lv_disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_display_set_flush_cb(lv_disp, lvgl_flush_cb);
    
    if (buf2 != nullptr) {
        lv_display_set_buffers(lv_disp, buf1, buf2, buf_size * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    } else {
        lv_display_set_buffers(lv_disp, buf1, nullptr, buf_size * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    }
    
    DEBUG_PRINTLN("LVGL display created");
    
    // Create input device for touch
    lv_indev = lv_indev_create();
    lv_indev_set_type(lv_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lv_indev, lvgl_touch_cb);
    
    DEBUG_PRINTLN("Touch input device created");
    DEBUG_PRINTLN("Display driver initialized successfully");
    
    return true;
}

LGFX& getDisplay(void) {
    return lcd;
}

lv_display_t* getLvDisplay(void) {
    return lv_disp;
}

lv_indev_t* getLvInputDevice(void) {
    return lv_indev;
}

void update(void) {
    lv_timer_handler();
}

void setBrightness(uint8_t brightness) {
    lcd.setBrightness(brightness);
}

int getWidth(void) {
    return DISPLAY_WIDTH;
}

int getHeight(void) {
    return DISPLAY_HEIGHT;
}

} // namespace DisplayDriver
