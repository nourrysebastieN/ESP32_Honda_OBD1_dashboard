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
    // Configure SPI bus
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
        cfg.i2c_port = 0;
        cfg.i2c_addr = 0x5D;  // GT911 I2C address
        cfg.pin_sda = TOUCH_SDA;
        cfg.pin_scl = TOUCH_SCL;
        cfg.freq = 400000;
        
        _touch_instance.config(cfg);
        _panel_instance.setTouch(&_touch_instance);
    }
    
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
