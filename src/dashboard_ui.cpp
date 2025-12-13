/**
 * @file dashboard_ui.cpp
 * @brief Dashboard UI components and layout manager implementation
 * 
 * Implementation of the dashboard user interface using LVGL.
 * Creates gauges, indicators, and manages screen layouts.
 */

#include "dashboard_ui.h"
#include "config.h"
#include "haltech_widget.h"
#include <cstdio>

#if defined(ARDUINO)
#include <Arduino.h>
#endif

// Screen objects
static lv_obj_t* mainScreen = nullptr;
static lv_obj_t* detailedScreen = nullptr;
static lv_obj_t* settingsScreen = nullptr;
static lv_obj_t* diagnosticsScreen = nullptr;

// Main screen widgets
static HaltechClusterWidget haltechCluster;
static bool haltechClusterReady = false;
static lv_obj_t* checkEngineIcon = nullptr;
static lv_obj_t* connectionIcon = nullptr;

// Current screen tracking
static DashboardScreen currentScreen = DashboardScreen::MAIN;

// Settings
static bool useMetricUnits = true;

// Colors
static const lv_color_t COLOR_BG = lv_color_hex(0x050506);
static const lv_color_t COLOR_PRIMARY = lv_color_hex(0x0E1015);
static const lv_color_t COLOR_ACCENT = lv_color_hex(0x181A22);
static const lv_color_t COLOR_HIGHLIGHT = lv_color_hex(0xF0C44C);
static const lv_color_t COLOR_SUCCESS = lv_color_hex(0x7ED957);
static const lv_color_t COLOR_WARNING = lv_color_hex(0xFF9F43);
static const lv_color_t COLOR_DANGER = lv_color_hex(0xFF4D4F);
static const lv_color_t COLOR_TEXT = lv_color_hex(0xF6F6F6);
static const lv_color_t COLOR_TEXT_DIM = lv_color_hex(0x7A7B85);

static constexpr uint8_t CARD_LEFT_MAP = 0;
static constexpr uint8_t CARD_LEFT_COOLANT = 1;
static constexpr uint8_t CARD_LEFT_INTAKE = 2;
static constexpr uint8_t CARD_LEFT_FUEL = 3;

static constexpr uint8_t CARD_RIGHT_IGN = 0;
static constexpr uint8_t CARD_RIGHT_TPS = 1;
static constexpr uint8_t CARD_RIGHT_BATTERY = 2;
static constexpr uint8_t CARD_RIGHT_INJECTOR = 3;

static const HaltechClusterConfig HALTECH_CLUSTER_CONFIG = {
    false,
    {
        "TACHO",
        "RPM",
        "Haltech",
        "km/h",
        0,
        SPEED_MAX,
        SPEED_WARNING,
        SPEED_MAX - 10,
        (SPEED_MAX / 10) + 1,
        1,
        -40,
        false,
        true,
    },
    {
        "SPEED",
        "km/h",
        "uC10",
        "km/h",
        0,
        SPEED_MAX,
        SPEED_WARNING,
        SPEED_MAX - 10,
        33,
        3,
        -40,
        true,
        false,
    },
    {{
        { "MAP", "kPa", 0, 120, false },
        { "COOLANT", "°C", TEMP_MIN, TEMP_MAX, false },
        { "INTAKE", "°C", TEMP_MIN - 20, TEMP_MAX - 10, false },
        { "FUEL", "%", 0, 100, true },
    }},
    {{
        { "IGN", "°", 0, 60, false },
        { "TPS", "%", 0, 100, false },
        { "BATTERY", "V", 10, 16, true },
        { "INJECTOR", "ms", 0, 20, false },
    }},
};

/**
 * @brief Create the main dashboard screen
 */
static void createMainScreen(void) {
    mainScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(mainScreen, COLOR_BG, 0);
    lv_obj_set_style_border_width(mainScreen, 0, 0);
    lv_obj_clear_flag(mainScreen, LV_OBJ_FLAG_SCROLLABLE);
    // Root container 
    lv_obj_t* rootContainer = lv_obj_create(mainScreen);
    lv_obj_set_size(rootContainer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_style_bg_color(rootContainer, COLOR_BG, 0);
    lv_obj_set_style_border_width(rootContainer, 0, 0);
    lv_obj_set_style_pad_all(rootContainer, 20, 0);
    lv_obj_set_flex_flow(rootContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(rootContainer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(rootContainer, LV_OBJ_FLAG_SCROLLABLE);
    // Frame container
    lv_obj_t* frame = lv_obj_create(rootContainer);
    lv_obj_set_size(frame, DISPLAY_WIDTH - 40, DISPLAY_HEIGHT - 130);
    lv_obj_set_style_bg_color(frame, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_grad_color(frame, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_grad_dir(frame, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_color(frame, lv_color_hex(0x2B2C34), 0);
    lv_obj_set_style_border_width(frame, 2, 0);
    lv_obj_set_style_radius(frame, 30, 0);
    lv_obj_set_style_shadow_color(frame, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_width(frame, 18, 0);
    lv_obj_set_style_shadow_opa(frame, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(frame, 18, 0);
    lv_obj_set_flex_flow(frame, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(frame, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* indicatorRow = lv_obj_create(frame);
    lv_obj_set_width(indicatorRow, LV_PCT(100));
    lv_obj_set_height(indicatorRow, 50);
    lv_obj_set_style_bg_opa(indicatorRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(indicatorRow, 0, 0);
    lv_obj_set_style_pad_all(indicatorRow, 0, 0);
    lv_obj_set_flex_flow(indicatorRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(indicatorRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(indicatorRow, LV_OBJ_FLAG_SCROLLABLE);
    
    
    lv_obj_t* clusterArea = lv_obj_create(frame);
    lv_obj_set_size(clusterArea, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(clusterArea, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(clusterArea, 0, 0);
    lv_obj_set_style_pad_all(clusterArea, 0, 0);
    lv_obj_clear_flag(clusterArea, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(clusterArea, LV_FLEX_FLOW_COLUMN);
    
    haltechClusterReady = haltech_cluster_init(haltechCluster, clusterArea, HALTECH_CLUSTER_CONFIG);
    if (!haltechClusterReady) {
        DEBUG_PRINTLN("WARNING: Failed to initialize Haltech cluster widget");
    }
    
    // lv_obj_t* statusBar = lv_obj_create(rootContainer);
    // lv_obj_set_size(statusBar, DISPLAY_WIDTH - 40, 60);
    // lv_obj_set_style_bg_color(statusBar, COLOR_PRIMARY, 0);
    // lv_obj_set_style_border_width(statusBar, 0, 0);
    // lv_obj_set_style_radius(statusBar, 16, 0);
    // lv_obj_set_style_pad_all(statusBar, 20, 0);
    // lv_obj_set_flex_flow(statusBar, LV_FLEX_FLOW_ROW);
    // lv_obj_set_flex_align(statusBar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // lv_obj_clear_flag(statusBar, LV_OBJ_FLAG_SCROLLABLE);
    
    // checkEngineIcon = lv_label_create(statusBar);
    // lv_label_set_text(checkEngineIcon, LV_SYMBOL_WARNING " CHECK ENGINE");
    // lv_obj_set_style_text_font(checkEngineIcon, &lv_font_montserrat_16, 0);
    // lv_obj_set_style_text_color(checkEngineIcon, COLOR_TEXT_DIM, 0);
    
    // connectionIcon = lv_label_create(statusBar);
    // lv_label_set_text(connectionIcon, LV_SYMBOL_WIFI " ECU LINK");
    // lv_obj_set_style_text_font(connectionIcon, &lv_font_montserrat_16, 0);
    // lv_obj_set_style_text_color(connectionIcon, COLOR_DANGER, 0);
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
    if (currentScreen != DashboardScreen::MAIN) {
        return;
    }

    setRPM(data.rpm);
    setSpeed(data.speed);
    setCoolantTemp(data.coolantTemp);
    setThrottlePosition(data.throttlePosition);
    setBatteryVoltage(data.batteryVoltage / 10.0f);
    showCheckEngine(data.checkEngine);
    setConnectionStatus(data.connected);

    if (haltechClusterReady) {
        haltech_cluster_set_card_value(haltechCluster, true, CARD_LEFT_MAP, data.mapPressure);
        haltech_cluster_set_card_value(haltechCluster, true, CARD_LEFT_INTAKE, data.intakeTemp);
        haltech_cluster_set_card_value(haltechCluster, false, CARD_RIGHT_IGN, data.ignitionAdvance);
        const float injectorMs = static_cast<float>(data.injectorPulse) / 10.0f;
        haltech_cluster_set_card_value(haltechCluster, false, CARD_RIGHT_INJECTOR, injectorMs);
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
    if (!haltechClusterReady) {
        return;
    }
    uint16_t clamped = rpm;
    if (clamped > RPM_MAX) {
        clamped = RPM_MAX;
    }
    haltech_cluster_set_digital_value(haltechCluster, true, clamped, RPM_WARNING, RPM_REDLINE);
}

void setSpeed(uint8_t speed) {
    if (!haltechClusterReady) {
        return;
    }
    uint16_t clamped = speed;
    if (clamped > SPEED_MAX) {
        clamped = SPEED_MAX;
    }
    haltech_cluster_set_left_value(haltechCluster, clamped);
    haltech_cluster_set_center_value(haltechCluster, true, clamped);
}

void setCoolantTemp(uint8_t temp) {
    if (!haltechClusterReady) {
        return;
    }
    haltech_cluster_set_card_value(haltechCluster, true, CARD_LEFT_COOLANT, temp);
}

void setFuelLevel(uint8_t level) {
    if (!haltechClusterReady) {
        return;
    }
    haltech_cluster_set_card_value(haltechCluster, true, CARD_LEFT_FUEL, level);
}

void setThrottlePosition(uint8_t position) {
    if (!haltechClusterReady) {
        return;
    }
    haltech_cluster_set_card_value(haltechCluster, false, CARD_RIGHT_TPS, position);
}

void setBatteryVoltage(float voltage) {
    if (!haltechClusterReady) {
        return;
    }
    haltech_cluster_set_card_value(haltechCluster, false, CARD_RIGHT_BATTERY, voltage);
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
    if (haltechClusterReady) {
        const char* unit = useMetric ? "km/h" : "mph";
        haltech_cluster_set_center_unit(haltechCluster, true, unit);
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
    static lv_style_t gaugeWarningStyle;
    static bool gaugeWarningStyleInit = false;
    
    if (!gaugeWarningStyleInit) {
        lv_style_init(&gaugeWarningStyle);
        lv_style_set_arc_color(&gaugeWarningStyle, COLOR_DANGER);
        lv_style_set_arc_width(&gaugeWarningStyle, 10);
        gaugeWarningStyleInit = true;
    }
    
    lv_obj_t* gauge = lv_scale_create(parent);
    lv_obj_set_size(gauge, 200, 200);
    lv_obj_set_style_bg_color(gauge, COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(gauge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(gauge, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(gauge, 2, 0);
    lv_obj_set_style_radius(gauge, LV_RADIUS_CIRCLE, 0);
    
    lv_scale_set_mode(gauge, LV_SCALE_MODE_ROUND_OUTER);
    lv_scale_set_label_show(gauge, false);
    lv_scale_set_range(gauge, min, max);
    lv_scale_set_angle_range(gauge, 270);
    lv_scale_set_rotation(gauge, 135);
    lv_scale_set_total_tick_count(gauge, 21);
    lv_scale_set_major_tick_every(gauge, 4);
    
    lv_obj_set_style_length(gauge, 14, LV_PART_INDICATOR);
    lv_obj_set_style_length(gauge, 8, LV_PART_ITEMS);
    lv_obj_set_style_line_width(gauge, 3, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(gauge, 2, LV_PART_ITEMS);
    
    lv_scale_section_t* warningSection = lv_scale_add_section(gauge);
    lv_scale_section_set_range(warningSection, warning, max);
    lv_scale_section_set_style(warningSection, LV_PART_MAIN, &gaugeWarningStyle);
    
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
