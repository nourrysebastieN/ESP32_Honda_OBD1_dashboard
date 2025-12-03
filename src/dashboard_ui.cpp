/**
 * @file dashboard_ui.cpp
 * @brief Dashboard UI components and layout manager implementation
 * 
 * Implementation of the dashboard user interface using LVGL.
 * Creates gauges, indicators, and manages screen layouts.
 */

#include "dashboard_ui.h"
#include "config.h"
#include <Arduino.h>

// Screen objects
static lv_obj_t* mainScreen = nullptr;
static lv_obj_t* detailedScreen = nullptr;
static lv_obj_t* settingsScreen = nullptr;
static lv_obj_t* diagnosticsScreen = nullptr;

// Main screen widgets
static lv_obj_t* rpmGauge = nullptr;
static lv_obj_t* rpmNeedle = nullptr;
static lv_obj_t* speedLabel = nullptr;
static lv_obj_t* speedUnitLabel = nullptr;
static lv_obj_t* coolantBar = nullptr;
static lv_obj_t* coolantLabel = nullptr;
static lv_obj_t* fuelBar = nullptr;
static lv_obj_t* fuelLabel = nullptr;
static lv_obj_t* batteryLabel = nullptr;
static lv_obj_t* throttleBar = nullptr;
static lv_obj_t* checkEngineIcon = nullptr;
static lv_obj_t* connectionIcon = nullptr;

// Current screen tracking
static DashboardScreen currentScreen = DashboardScreen::MAIN;

// Settings
static bool useMetricUnits = true;

// Colors
static const lv_color_t COLOR_BG = lv_color_hex(0x1A1A2E);
static const lv_color_t COLOR_PRIMARY = lv_color_hex(0x16213E);
static const lv_color_t COLOR_ACCENT = lv_color_hex(0x0F3460);
static const lv_color_t COLOR_HIGHLIGHT = lv_color_hex(0xE94560);
static const lv_color_t COLOR_SUCCESS = lv_color_hex(0x00FF88);
static const lv_color_t COLOR_WARNING = lv_color_hex(0xFFAA00);
static const lv_color_t COLOR_DANGER = lv_color_hex(0xFF4444);
static const lv_color_t COLOR_TEXT = lv_color_hex(0xFFFFFF);
static const lv_color_t COLOR_TEXT_DIM = lv_color_hex(0x888888);

/**
 * @brief Create the main dashboard screen
 */
static void createMainScreen(void) {
    mainScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(mainScreen, COLOR_BG, 0);
    
    // Create main container with flex layout
    lv_obj_t* mainContainer = lv_obj_create(mainScreen);
    lv_obj_set_size(mainContainer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_style_bg_color(mainContainer, COLOR_BG, 0);
    lv_obj_set_style_border_width(mainContainer, 0, 0);
    lv_obj_set_style_pad_all(mainContainer, 10, 0);
    lv_obj_align(mainContainer, LV_ALIGN_CENTER, 0, 0);
    
    // ==================
    // RPM Gauge (Left side)
    // ==================
    rpmGauge = lv_meter_create(mainContainer);
    lv_obj_set_size(rpmGauge, 280, 280);
    lv_obj_align(rpmGauge, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_set_style_bg_color(rpmGauge, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_color(rpmGauge, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(rpmGauge, 3, 0);
    
    // Remove default indicator
    lv_meter_set_scale_ticks(rpmGauge, 37, 2, 10, COLOR_TEXT_DIM);
    lv_meter_set_scale_major_ticks(rpmGauge, 4, 4, 15, COLOR_TEXT, 15);
    lv_meter_set_scale_range(rpmGauge, 0, 9000, 270, 135);
    
    // Add colored arc sections
    lv_meter_indicator_t* indic;
    
    // Normal range (green tint)
    indic = lv_meter_add_arc(rpmGauge, 12, COLOR_SUCCESS, 0);
    lv_meter_set_indicator_start_value(rpmGauge, indic, 0);
    lv_meter_set_indicator_end_value(rpmGauge, indic, 5000);
    
    // Mid range (yellow)
    indic = lv_meter_add_arc(rpmGauge, 12, COLOR_WARNING, 0);
    lv_meter_set_indicator_start_value(rpmGauge, indic, 5000);
    lv_meter_set_indicator_end_value(rpmGauge, indic, 7000);
    
    // Redline (red)
    indic = lv_meter_add_arc(rpmGauge, 12, COLOR_DANGER, 0);
    lv_meter_set_indicator_start_value(rpmGauge, indic, 7000);
    lv_meter_set_indicator_end_value(rpmGauge, indic, 9000);
    
    // Add needle
    rpmNeedle = (lv_obj_t*)lv_meter_add_needle_line(rpmGauge, 5, COLOR_HIGHLIGHT, -15);
    lv_meter_set_indicator_value(rpmGauge, (lv_meter_indicator_t*)rpmNeedle, 0);
    
    // RPM label
    lv_obj_t* rpmTextLabel = lv_label_create(rpmGauge);
    lv_label_set_text(rpmTextLabel, "RPM");
    lv_obj_set_style_text_font(rpmTextLabel, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(rpmTextLabel, COLOR_TEXT, 0);
    lv_obj_align(rpmTextLabel, LV_ALIGN_CENTER, 0, 60);
    
    // ==================
    // Speed Display (Center)
    // ==================
    lv_obj_t* speedContainer = lv_obj_create(mainContainer);
    lv_obj_set_size(speedContainer, 200, 200);
    lv_obj_align(speedContainer, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(speedContainer, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_color(speedContainer, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(speedContainer, 2, 0);
    lv_obj_set_style_radius(speedContainer, 20, 0);
    
    speedLabel = lv_label_create(speedContainer);
    lv_label_set_text(speedLabel, "0");
    lv_obj_set_style_text_font(speedLabel, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(speedLabel, COLOR_TEXT, 0);
    lv_obj_align(speedLabel, LV_ALIGN_CENTER, 0, -10);
    
    speedUnitLabel = lv_label_create(speedContainer);
    lv_label_set_text(speedUnitLabel, "km/h");
    lv_obj_set_style_text_font(speedUnitLabel, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(speedUnitLabel, COLOR_TEXT_DIM, 0);
    lv_obj_align(speedUnitLabel, LV_ALIGN_CENTER, 0, 40);
    
    // ==================
    // Right Side - Bars and Info
    // ==================
    lv_obj_t* rightPanel = lv_obj_create(mainContainer);
    lv_obj_set_size(rightPanel, 250, 400);
    lv_obj_align(rightPanel, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_set_style_bg_color(rightPanel, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(rightPanel, 0, 0);
    lv_obj_set_style_pad_all(rightPanel, 15, 0);
    lv_obj_set_flex_flow(rightPanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(rightPanel, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Coolant Temperature
    lv_obj_t* coolantContainer = lv_obj_create(rightPanel);
    lv_obj_set_size(coolantContainer, 220, 60);
    lv_obj_set_style_bg_color(coolantContainer, lv_color_hex(0x0A0A15), 0);
    lv_obj_set_style_border_width(coolantContainer, 0, 0);
    lv_obj_set_style_radius(coolantContainer, 10, 0);
    
    lv_obj_t* coolantTitle = lv_label_create(coolantContainer);
    lv_label_set_text(coolantTitle, "COOLANT");
    lv_obj_set_style_text_font(coolantTitle, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(coolantTitle, COLOR_TEXT_DIM, 0);
    lv_obj_align(coolantTitle, LV_ALIGN_TOP_LEFT, 5, 5);
    
    coolantBar = lv_bar_create(coolantContainer);
    lv_obj_set_size(coolantBar, 180, 15);
    lv_obj_align(coolantBar, LV_ALIGN_BOTTOM_LEFT, 5, -8);
    lv_bar_set_range(coolantBar, TEMP_MIN, TEMP_MAX);
    lv_bar_set_value(coolantBar, TEMP_MIN, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(coolantBar, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(coolantBar, COLOR_SUCCESS, LV_PART_INDICATOR);
    
    coolantLabel = lv_label_create(coolantContainer);
    lv_label_set_text(coolantLabel, "-- °C");
    lv_obj_set_style_text_font(coolantLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(coolantLabel, COLOR_TEXT, 0);
    lv_obj_align(coolantLabel, LV_ALIGN_TOP_RIGHT, -5, 5);
    
    // Fuel Level
    lv_obj_t* fuelContainer = lv_obj_create(rightPanel);
    lv_obj_set_size(fuelContainer, 220, 60);
    lv_obj_set_style_bg_color(fuelContainer, lv_color_hex(0x0A0A15), 0);
    lv_obj_set_style_border_width(fuelContainer, 0, 0);
    lv_obj_set_style_radius(fuelContainer, 10, 0);
    
    lv_obj_t* fuelTitle = lv_label_create(fuelContainer);
    lv_label_set_text(fuelTitle, "FUEL");
    lv_obj_set_style_text_font(fuelTitle, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(fuelTitle, COLOR_TEXT_DIM, 0);
    lv_obj_align(fuelTitle, LV_ALIGN_TOP_LEFT, 5, 5);
    
    fuelBar = lv_bar_create(fuelContainer);
    lv_obj_set_size(fuelBar, 180, 15);
    lv_obj_align(fuelBar, LV_ALIGN_BOTTOM_LEFT, 5, -8);
    lv_bar_set_range(fuelBar, 0, 100);
    lv_bar_set_value(fuelBar, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(fuelBar, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(fuelBar, COLOR_SUCCESS, LV_PART_INDICATOR);
    
    fuelLabel = lv_label_create(fuelContainer);
    lv_label_set_text(fuelLabel, "-- %");
    lv_obj_set_style_text_font(fuelLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(fuelLabel, COLOR_TEXT, 0);
    lv_obj_align(fuelLabel, LV_ALIGN_TOP_RIGHT, -5, 5);
    
    // Throttle Position
    lv_obj_t* throttleContainer = lv_obj_create(rightPanel);
    lv_obj_set_size(throttleContainer, 220, 60);
    lv_obj_set_style_bg_color(throttleContainer, lv_color_hex(0x0A0A15), 0);
    lv_obj_set_style_border_width(throttleContainer, 0, 0);
    lv_obj_set_style_radius(throttleContainer, 10, 0);
    
    lv_obj_t* throttleTitle = lv_label_create(throttleContainer);
    lv_label_set_text(throttleTitle, "THROTTLE");
    lv_obj_set_style_text_font(throttleTitle, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(throttleTitle, COLOR_TEXT_DIM, 0);
    lv_obj_align(throttleTitle, LV_ALIGN_TOP_LEFT, 5, 5);
    
    throttleBar = lv_bar_create(throttleContainer);
    lv_obj_set_size(throttleBar, 180, 15);
    lv_obj_align(throttleBar, LV_ALIGN_BOTTOM_LEFT, 5, -8);
    lv_bar_set_range(throttleBar, 0, 100);
    lv_bar_set_value(throttleBar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(throttleBar, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(throttleBar, COLOR_HIGHLIGHT, LV_PART_INDICATOR);
    
    // Battery Voltage
    lv_obj_t* batteryContainer = lv_obj_create(rightPanel);
    lv_obj_set_size(batteryContainer, 220, 50);
    lv_obj_set_style_bg_color(batteryContainer, lv_color_hex(0x0A0A15), 0);
    lv_obj_set_style_border_width(batteryContainer, 0, 0);
    lv_obj_set_style_radius(batteryContainer, 10, 0);
    
    lv_obj_t* batteryTitle = lv_label_create(batteryContainer);
    lv_label_set_text(batteryTitle, "BATTERY");
    lv_obj_set_style_text_font(batteryTitle, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(batteryTitle, COLOR_TEXT_DIM, 0);
    lv_obj_align(batteryTitle, LV_ALIGN_LEFT_MID, 10, 0);
    
    batteryLabel = lv_label_create(batteryContainer);
    lv_label_set_text(batteryLabel, "-- V");
    lv_obj_set_style_text_font(batteryLabel, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(batteryLabel, COLOR_TEXT, 0);
    lv_obj_align(batteryLabel, LV_ALIGN_RIGHT_MID, -10, 0);
    
    // ==================
    // Bottom Status Bar
    // ==================
    lv_obj_t* statusBar = lv_obj_create(mainContainer);
    lv_obj_set_size(statusBar, DISPLAY_WIDTH - 40, 50);
    lv_obj_align(statusBar, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(statusBar, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_width(statusBar, 0, 0);
    lv_obj_set_style_radius(statusBar, 10, 0);
    lv_obj_set_flex_flow(statusBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(statusBar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Check Engine Light
    checkEngineIcon = lv_label_create(statusBar);
    lv_label_set_text(checkEngineIcon, LV_SYMBOL_WARNING " CEL");
    lv_obj_set_style_text_font(checkEngineIcon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(checkEngineIcon, COLOR_TEXT_DIM, 0);
    
    // Connection Status
    connectionIcon = lv_label_create(statusBar);
    lv_label_set_text(connectionIcon, LV_SYMBOL_WIFI " ECU");
    lv_obj_set_style_text_font(connectionIcon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(connectionIcon, COLOR_DANGER, 0);
}

/**
 * @brief Create the detailed data screen
 */
static void createDetailedScreen(void) {
    detailedScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(detailedScreen, COLOR_BG, 0);
    
    // Title
    lv_obj_t* title = lv_label_create(detailedScreen);
    lv_label_set_text(title, "Detailed Data View");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Back button
    lv_obj_t* backBtn = lv_btn_create(detailedScreen);
    lv_obj_set_size(backBtn, 100, 40);
    lv_obj_align(backBtn, LV_ALIGN_TOP_LEFT, 20, 15);
    lv_obj_set_style_bg_color(backBtn, COLOR_ACCENT, 0);
    
    lv_obj_t* backLabel = lv_label_create(backBtn);
    lv_label_set_text(backLabel, LV_SYMBOL_LEFT " Back");
    lv_obj_center(backLabel);
    
    lv_obj_add_event_cb(backBtn, [](lv_event_t* e) {
        DashboardUI::switchScreen(DashboardScreen::MAIN);
    }, LV_EVENT_CLICKED, nullptr);
    
    // Data table placeholder
    lv_obj_t* infoLabel = lv_label_create(detailedScreen);
    lv_label_set_text(infoLabel, "Detailed sensor data will be displayed here.\nImplement additional data views as needed.");
    lv_obj_set_style_text_font(infoLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(infoLabel, COLOR_TEXT_DIM, 0);
    lv_obj_align(infoLabel, LV_ALIGN_CENTER, 0, 0);
}

/**
 * @brief Create the settings screen
 */
static void createSettingsScreen(void) {
    settingsScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(settingsScreen, COLOR_BG, 0);
    
    // Title
    lv_obj_t* title = lv_label_create(settingsScreen);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Back button
    lv_obj_t* backBtn = lv_btn_create(settingsScreen);
    lv_obj_set_size(backBtn, 100, 40);
    lv_obj_align(backBtn, LV_ALIGN_TOP_LEFT, 20, 15);
    lv_obj_set_style_bg_color(backBtn, COLOR_ACCENT, 0);
    
    lv_obj_t* backLabel = lv_label_create(backBtn);
    lv_label_set_text(backLabel, LV_SYMBOL_LEFT " Back");
    lv_obj_center(backLabel);
    
    lv_obj_add_event_cb(backBtn, [](lv_event_t* e) {
        DashboardUI::switchScreen(DashboardScreen::MAIN);
    }, LV_EVENT_CLICKED, nullptr);
    
    // Settings placeholder
    lv_obj_t* infoLabel = lv_label_create(settingsScreen);
    lv_label_set_text(infoLabel, "Settings options:\n- Units (Metric/Imperial)\n- Brightness\n- Theme\n- Calibration");
    lv_obj_set_style_text_font(infoLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(infoLabel, COLOR_TEXT_DIM, 0);
    lv_obj_align(infoLabel, LV_ALIGN_CENTER, 0, 0);
}

/**
 * @brief Create the diagnostics screen
 */
static void createDiagnosticsScreen(void) {
    diagnosticsScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(diagnosticsScreen, COLOR_BG, 0);
    
    // Title
    lv_obj_t* title = lv_label_create(diagnosticsScreen);
    lv_label_set_text(title, "Diagnostics");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Back button
    lv_obj_t* backBtn = lv_btn_create(diagnosticsScreen);
    lv_obj_set_size(backBtn, 100, 40);
    lv_obj_align(backBtn, LV_ALIGN_TOP_LEFT, 20, 15);
    lv_obj_set_style_bg_color(backBtn, COLOR_ACCENT, 0);
    
    lv_obj_t* backLabel = lv_label_create(backBtn);
    lv_label_set_text(backLabel, LV_SYMBOL_LEFT " Back");
    lv_obj_center(backLabel);
    
    lv_obj_add_event_cb(backBtn, [](lv_event_t* e) {
        DashboardUI::switchScreen(DashboardScreen::MAIN);
    }, LV_EVENT_CLICKED, nullptr);
    
    // Diagnostics placeholder
    lv_obj_t* infoLabel = lv_label_create(diagnosticsScreen);
    lv_label_set_text(infoLabel, "Diagnostic Trouble Codes (DTCs)\nwill be displayed here.\n\nRead and clear codes functionality.");
    lv_obj_set_style_text_font(infoLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(infoLabel, COLOR_TEXT_DIM, 0);
    lv_obj_align(infoLabel, LV_ALIGN_CENTER, 0, 0);
}

namespace DashboardUI {

void init(void) {
    DEBUG_PRINTLN("Initializing Dashboard UI...");
    
    // Create all screens
    createMainScreen();
    createDetailedScreen();
    createSettingsScreen();
    createDiagnosticsScreen();
    
    // Load main screen
    lv_scr_load(mainScreen);
    currentScreen = DashboardScreen::MAIN;
    
    DEBUG_PRINTLN("Dashboard UI initialized");
}

void update(const OBD1Data& data) {
    if (currentScreen == DashboardScreen::MAIN) {
        setRPM(data.rpm);
        setSpeed(data.speed);
        setCoolantTemp(data.coolantTemp);
        setThrottlePosition(data.throttlePosition);
        setBatteryVoltage(data.batteryVoltage / 10.0f);
        showCheckEngine(data.checkEngine);
        setConnectionStatus(data.connected);
    }
}

void switchScreen(DashboardScreen screen) {
    lv_obj_t* targetScreen = nullptr;
    
    switch (screen) {
        case DashboardScreen::MAIN:
            targetScreen = mainScreen;
            break;
        case DashboardScreen::DETAILED:
            targetScreen = detailedScreen;
            break;
        case DashboardScreen::SETTINGS:
            targetScreen = settingsScreen;
            break;
        case DashboardScreen::DIAGNOSTICS:
            targetScreen = diagnosticsScreen;
            break;
    }
    
    if (targetScreen != nullptr) {
        lv_scr_load_anim(targetScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
        currentScreen = screen;
    }
}

DashboardScreen getCurrentScreen(void) {
    return currentScreen;
}

void setRPM(uint16_t rpm) {
    if (rpmNeedle != nullptr) {
        lv_meter_set_indicator_value(rpmGauge, (lv_meter_indicator_t*)rpmNeedle, rpm);
    }
}

void setSpeed(uint8_t speed) {
    if (speedLabel != nullptr) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", speed);
        lv_label_set_text(speedLabel, buf);
    }
}

void setCoolantTemp(uint8_t temp) {
    if (coolantBar != nullptr && coolantLabel != nullptr) {
        lv_bar_set_value(coolantBar, temp, LV_ANIM_ON);
        
        // Update color based on temperature
        if (temp >= TEMP_WARNING) {
            lv_obj_set_style_bg_color(coolantBar, COLOR_DANGER, LV_PART_INDICATOR);
        } else if (temp >= TEMP_WARNING - 10) {
            lv_obj_set_style_bg_color(coolantBar, COLOR_WARNING, LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(coolantBar, COLOR_SUCCESS, LV_PART_INDICATOR);
        }
        
        char buf[16];
        snprintf(buf, sizeof(buf), "%d °C", temp);
        lv_label_set_text(coolantLabel, buf);
    }
}

void setFuelLevel(uint8_t level) {
    if (fuelBar != nullptr && fuelLabel != nullptr) {
        lv_bar_set_value(fuelBar, level, LV_ANIM_ON);
        
        // Update color based on fuel level
        if (level <= FUEL_WARNING) {
            lv_obj_set_style_bg_color(fuelBar, COLOR_DANGER, LV_PART_INDICATOR);
        } else if (level <= FUEL_WARNING + 10) {
            lv_obj_set_style_bg_color(fuelBar, COLOR_WARNING, LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(fuelBar, COLOR_SUCCESS, LV_PART_INDICATOR);
        }
        
        char buf[8];
        snprintf(buf, sizeof(buf), "%d %%", level);
        lv_label_set_text(fuelLabel, buf);
    }
}

void setThrottlePosition(uint8_t position) {
    if (throttleBar != nullptr) {
        lv_bar_set_value(throttleBar, position, LV_ANIM_ON);
    }
}

void setBatteryVoltage(float voltage) {
    if (batteryLabel != nullptr) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f V", voltage);
        lv_label_set_text(batteryLabel, buf);
        
        // Update color based on voltage
        if (voltage < 11.5f || voltage > 15.0f) {
            lv_obj_set_style_text_color(batteryLabel, COLOR_DANGER, 0);
        } else if (voltage < 12.2f) {
            lv_obj_set_style_text_color(batteryLabel, COLOR_WARNING, 0);
        } else {
            lv_obj_set_style_text_color(batteryLabel, COLOR_SUCCESS, 0);
        }
    }
}

void showCheckEngine(bool show) {
    if (checkEngineIcon != nullptr) {
        if (show) {
            lv_obj_set_style_text_color(checkEngineIcon, COLOR_WARNING, 0);
        } else {
            lv_obj_set_style_text_color(checkEngineIcon, COLOR_TEXT_DIM, 0);
        }
    }
}

void setConnectionStatus(bool connected) {
    if (connectionIcon != nullptr) {
        if (connected) {
            lv_obj_set_style_text_color(connectionIcon, COLOR_SUCCESS, 0);
        } else {
            lv_obj_set_style_text_color(connectionIcon, COLOR_DANGER, 0);
        }
    }
}

void setMetricUnits(bool useMetric) {
    useMetricUnits = useMetric;
    if (speedUnitLabel != nullptr) {
        lv_label_set_text(speedUnitLabel, useMetric ? "km/h" : "mph");
    }
}

void setDarkMode(bool darkMode) {
    // Theme switching can be implemented here
    // For now, we default to dark mode
    (void)darkMode;
}

lv_obj_t* getMainScreen(void) {
    return mainScreen;
}

lv_obj_t* createGauge(lv_obj_t* parent, int32_t min, int32_t max, int32_t warning) {
    lv_obj_t* gauge = lv_meter_create(parent);
    lv_obj_set_size(gauge, 200, 200);
    lv_obj_set_style_bg_color(gauge, COLOR_PRIMARY, 0);
    lv_obj_set_style_border_color(gauge, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(gauge, 2, 0);
    
    lv_meter_set_scale_ticks(gauge, 21, 2, 10, COLOR_TEXT_DIM);
    lv_meter_set_scale_major_ticks(gauge, 4, 4, 15, COLOR_TEXT, 15);
    lv_meter_set_scale_range(gauge, min, max, 270, 135);
    
    // Add warning zone
    lv_meter_indicator_t* indic = lv_meter_add_arc(gauge, 10, COLOR_DANGER, 0);
    lv_meter_set_indicator_start_value(gauge, indic, warning);
    lv_meter_set_indicator_end_value(gauge, indic, max);
    
    return gauge;
}

lv_obj_t* createBarIndicator(lv_obj_t* parent, const char* label, int32_t min, int32_t max) {
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, 200, 50);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x0A0A15), 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_radius(container, 8, 0);
    
    lv_obj_t* titleLabel = lv_label_create(container);
    lv_label_set_text(titleLabel, label);
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(titleLabel, COLOR_TEXT_DIM, 0);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_LEFT, 5, 5);
    
    lv_obj_t* bar = lv_bar_create(container);
    lv_obj_set_size(bar, 160, 12);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_LEFT, 5, -8);
    lv_bar_set_range(bar, min, max);
    lv_bar_set_value(bar, min, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, COLOR_HIGHLIGHT, LV_PART_INDICATOR);
    
    return bar;
}

} // namespace DashboardUI
