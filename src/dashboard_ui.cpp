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
#include "bluetooth_uart.h"
#include <cmath>
#include <cstdio>

#if defined(ARDUINO)
#include <Arduino.h>
#endif

// Screen objects
static lv_obj_t* mainScreen = nullptr;
static lv_obj_t* settingsScreen = nullptr;
static lv_obj_t* diagnosticsScreen = nullptr;
static lv_timer_t* gestureTimer = nullptr;

// Main screen widgets
static HaltechClusterWidget haltechCluster;
static bool haltechClusterReady = false;
static lv_obj_t* checkEngineIcon = nullptr;
static lv_obj_t* connectionIcon = nullptr;
static lv_obj_t* dtcLabel = nullptr;
static bool (*clearDtcCallback)(void) = nullptr;

// Current screen tracking
static DashboardScreen currentScreen = DashboardScreen::MAIN;

// Settings
static bool useMetricUnits = true;
static lv_obj_t* bleSwitch = nullptr;
static lv_obj_t* bleStatusLabel = nullptr;

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

static constexpr int32_t SWIPE_THRESHOLD = 80;
static constexpr uint32_t SWIPE_DEBOUNCE_MS = 250;

namespace DashboardUI {
    void switchScreen(DashboardScreen screen);
}

static lv_indev_t* getPointerIndev(void) {
    lv_indev_t* indev = lv_indev_get_next(nullptr);
    while (indev != nullptr) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            return indev;
        }
        indev = lv_indev_get_next(indev);
    }
    return nullptr;
}

static void switchScreenNext(bool forward) {
    DashboardScreen nextScreen = currentScreen;
    if (forward) {
        switch (currentScreen) {
            case DashboardScreen::MAIN:
                nextScreen = DashboardScreen::DIAGNOSTICS;
                break;
            case DashboardScreen::DIAGNOSTICS:
                nextScreen = DashboardScreen::SETTINGS;
                break;
            case DashboardScreen::SETTINGS:
                nextScreen = DashboardScreen::MAIN;
                break;
        }
    } else {
        switch (currentScreen) {
            case DashboardScreen::MAIN:
                nextScreen = DashboardScreen::SETTINGS;
                break;
            case DashboardScreen::DIAGNOSTICS:
                nextScreen = DashboardScreen::MAIN;
                break;
            case DashboardScreen::SETTINGS:
                nextScreen = DashboardScreen::DIAGNOSTICS;
                break;
        }
    }

    if (nextScreen != currentScreen) {
        DashboardUI::switchScreen(nextScreen);
    }
}

static void gestureTimerCb(lv_timer_t* timer) {
    (void)timer;
    static bool tracking = false;
    static lv_point_t startPoint = { 0, 0 };
    static uint32_t lastGestureMs = 0;

    lv_indev_t* indev = getPointerIndev();
    if (indev == nullptr) {
        return;
    }

    lv_point_t point;
    lv_indev_get_point(indev, &point);
    lv_indev_state_t state = lv_indev_get_state(indev);
    const uint32_t now = lv_tick_get();

    if (state == LV_INDEV_STATE_PRESSED) {
        if (!tracking) {
            tracking = true;
            startPoint = point;
        }
        return;
    }

    if (state == LV_INDEV_STATE_RELEASED && tracking) {
        tracking = false;
        if (now - lastGestureMs < SWIPE_DEBOUNCE_MS) {
            return;
        }

        const int32_t dx = point.x - startPoint.x;
        const int32_t dy = point.y - startPoint.y;
        if (std::abs(dx) >= SWIPE_THRESHOLD && std::abs(dx) > std::abs(dy)) {
            switchScreenNext(dx < 0);
            lastGestureMs = now;
        }
    }
}

static constexpr uint8_t CARD_LEFT_MAP = 0;
static constexpr uint8_t CARD_LEFT_COOLANT = 1;
static constexpr uint8_t CARD_LEFT_INTAKE = 2;
static constexpr uint8_t CARD_LEFT_FUEL = 3;

static constexpr uint8_t CARD_RIGHT_IGN = 0;
static constexpr uint8_t CARD_RIGHT_TPS = 1;
static constexpr uint8_t CARD_RIGHT_BATTERY = 2;
static constexpr uint8_t CARD_RIGHT_INJECTOR = 3;

static const HaltechClusterConfig HALTECH_CLUSTER_CONFIG = {
    true,
    {
        "",
        "km/h",
        "",
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
        "",
        "RPM",
        "",
        "RPM",
        0,
        RPM_MAX,
        RPM_WARNING,
        RPM_REDLINE,
        (RPM_MAX / 500) + 1,
        2,
        -40,
        false,
        true,
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

struct DtcInfo {
    uint8_t code;
    const char* description;
};

static const DtcInfo DTC_TABLE[] = {
    { 0, "ECU - Faulty ECU or ECU ROM" },
    { 1, "O2A - Oxygen sensor #1" },
    { 2, "O2B - Oxygen sensor #2" },
    { 3, "MAP - Manifold absolute pressure sensor" },
    { 4, "CKP - Crank position sensor" },
    { 5, "MAP - Manifold absolute pressure sensor" },
    { 6, "ECT - Water temperature sensor" },
    { 7, "TPS - Throttle position sensor" },
    { 8, "TDC - Top dead center sensor" },
    { 9, "CYP - Cylinder sensor" },
    { 10, "IAT - Intake air temperature sensor" },
    { 11, "Engine overheating" },
    { 12, "EGR - Exhaust gas recirculation lift valve" },
    { 13, "BARO - Atmospheric pressure sensor" },
    { 14, "IAC (EACV) - Idle air control valve" },
    { 15, "Ignition output signal" },
    { 16, "Fuel injectors" },
    { 17, "VSS - Vehicle speed sensor" },
    { 19, "Automatic transmission lockup control valve" },
    { 20, "ELD - Electrical load detector" },
    { 21, "VTEC spool solenoid valve" },
    { 22, "VTEC pressure valve" },
    { 23, "Knock sensor" },
    { 30, "Automatic transmission A signal" },
    { 31, "Automatic transmission B signal" },
    { 36, "Traction control (some JDM ECUs)" },
    { 38, "Secondary VTEC solenoid (JDM 3 stage D15B VTEC)" },
    { 41, "Primary oxygen sensor heater" },
    { 43, "Fuel supply system" },
    { 45, "Fuel system too rich or lean" },
    { 48, "LAF - Lean air fuel sensor" },
    { 54, "CKF - Crank fluctuation sensor" },
    { 58, "TDC sensor #2" },
    { 61, "Primary oxygen sensor" },
    { 63, "Secondary oxygen sensor circuit" },
    { 65, "Secondary oxygen sensor heater wire" },
    { 67, "Catalytic converter" },
    { 71, "Random misfire cylinder 1" },
    { 72, "Random misfire cylinder 2" },
    { 73, "Random misfire cylinder 3" },
    { 74, "Random misfire cylinder 4" },
    { 80, "EGR valve/line" },
    { 86, "ECT sensor - Cooling system" },
    { 91, "Fuel tank pressure sensor" },
    { 92, "EVAP solenoid/valve/vacuum lines" },
};

static const char* lookupDtcDescription(uint8_t code) {
    for (const auto& entry : DTC_TABLE) {
        if (entry.code == code) {
            return entry.description;
        }
    }
    return "Unknown code";
}

static void updateBleStatusLabel(bool enabled) {
    if (bleStatusLabel == nullptr) {
        return;
    }
    lv_label_set_text(bleStatusLabel, enabled ? "On" : "Off");
    lv_obj_set_style_text_color(bleStatusLabel, enabled ? COLOR_SUCCESS : COLOR_TEXT_DIM, 0);
}

static void applyBleToggle(bool enabled) {
    BluetoothUART::setEnabled(enabled);
    updateBleStatusLabel(enabled);
    if (bleSwitch != nullptr) {
        if (enabled) {
            lv_obj_add_state(bleSwitch, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(bleSwitch, LV_STATE_CHECKED);
        }
    }
}

static void updateDiagnosticsLabel(const OBD1Data& data) {
    if (dtcLabel == nullptr) {
        return;
    }

    char buffer[768];
    size_t offset = 0;
    int written = 0;

    if (!data.connected) {
        written = std::snprintf(buffer, sizeof(buffer), "ECU not connected.");
    } else if (data.dtcCount == 0) {
        written = std::snprintf(buffer, sizeof(buffer), "No DTCs stored.");
    } else {
        written = std::snprintf(buffer, sizeof(buffer), "DTCs (%u):\n", data.dtcCount);
        if (written < 0) {
            return;
        }
        offset = static_cast<size_t>(written);
        if (offset >= sizeof(buffer)) {
            offset = sizeof(buffer) - 1;
        }
        for (uint8_t i = 0; i < data.dtcCount; ++i) {
            const size_t remaining = sizeof(buffer) - offset;
            if (remaining <= 1) {
                break;
            }
            written = std::snprintf(buffer + offset,
                                    remaining,
                                    "%u - %s\n",
                                    data.dtcCodes[i],
                                    lookupDtcDescription(data.dtcCodes[i]));
            if (written < 0) {
                break;
            }
            offset += static_cast<size_t>(written);
            if (offset >= sizeof(buffer)) {
                offset = sizeof(buffer) - 1;
                break;
            }
        }
    }

    lv_label_set_text(dtcLabel, buffer);
}

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
    lv_obj_set_style_pad_all(rootContainer, 0, 0);
    lv_obj_set_flex_flow(rootContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(rootContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(rootContainer, LV_OBJ_FLAG_SCROLLABLE);
    // Frame container
    lv_obj_t* frame = lv_obj_create(rootContainer);
    lv_obj_set_size(frame, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(frame, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(frame, 0, 0);
    lv_obj_set_style_radius(frame, 0, 0);
    lv_obj_set_style_shadow_width(frame, 0, 0);
    lv_obj_set_style_shadow_opa(frame, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(frame, 0, 0);
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
    lv_obj_set_width(clusterArea, LV_PCT(100));
    lv_obj_set_flex_grow(clusterArea, 1);
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
    
    // Card container
    lv_obj_t* card = lv_obj_create(settingsScreen);
    lv_obj_set_size(card, DISPLAY_WIDTH - 40, DISPLAY_HEIGHT - 120);
    lv_obj_align(card, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(card, COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(card, 20, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 24, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Title inside card
    lv_obj_t* cardTitle = lv_label_create(card);
    lv_label_set_text(cardTitle, "Connectivity");
    lv_obj_set_style_text_font(cardTitle, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(cardTitle, COLOR_TEXT, 0);
    lv_obj_align(cardTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    // BLE toggle row
    lv_obj_t* row = lv_obj_create(card);
    lv_obj_set_size(row, lv_pct(100), 70);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, 40);
    lv_obj_set_style_bg_color(row, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(row, 14, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_left(row, 16, 0);
    lv_obj_set_style_pad_right(row, 16, 0);
    lv_obj_set_style_pad_top(row, 12, 0);
    lv_obj_set_style_pad_bottom(row, 12, 0);
    lv_obj_set_style_pad_row(row, 0, 0);
    lv_obj_set_style_pad_column(row, 10, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* bleLabel = lv_label_create(row);
    lv_label_set_text(bleLabel, "Bluetooth telemetry");
    lv_obj_set_style_text_font(bleLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(bleLabel, COLOR_TEXT, 0);

    bleStatusLabel = lv_label_create(row);
    updateBleStatusLabel(BluetoothUART::isEnabled());
    lv_obj_set_style_text_font(bleStatusLabel, &lv_font_montserrat_16, 0);

    bleSwitch = lv_switch_create(row);
    lv_obj_add_state(bleSwitch, BluetoothUART::isEnabled() ? LV_STATE_CHECKED : 0);
    lv_obj_add_event_cb(bleSwitch, [](lv_event_t* e) {
        const bool en = lv_obj_has_state(static_cast<lv_obj_t*>(lv_event_get_target(e)), LV_STATE_CHECKED);
        applyBleToggle(en);
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    // Allow tapping anywhere on the row to toggle
    lv_obj_add_event_cb(row, [](lv_event_t* e) {
        if (bleSwitch == nullptr) return;
        const bool current = lv_obj_has_state(bleSwitch, LV_STATE_CHECKED);
        applyBleToggle(!current);
    }, LV_EVENT_CLICKED, nullptr);
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

    // Clear DTCs button
    lv_obj_t* clearBtn = lv_btn_create(diagnosticsScreen);
    lv_obj_set_size(clearBtn, 140, 40);
    lv_obj_align(clearBtn, LV_ALIGN_TOP_RIGHT, -20, 15);
    lv_obj_set_style_bg_color(clearBtn, COLOR_WARNING, 0);

    lv_obj_t* clearLabel = lv_label_create(clearBtn);
    lv_label_set_text(clearLabel, LV_SYMBOL_TRASH " Clear");
    lv_obj_center(clearLabel);

    lv_obj_add_event_cb(clearBtn, [](lv_event_t* e) {
        if (clearDtcCallback != nullptr) {
            clearDtcCallback();
        }
    }, LV_EVENT_CLICKED, nullptr);
    
    // DTC list
    dtcLabel = lv_label_create(diagnosticsScreen);
    lv_label_set_text(dtcLabel, "No DTCs stored.");
    lv_obj_set_style_text_font(dtcLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(dtcLabel, COLOR_TEXT_DIM, 0);
    lv_label_set_long_mode(dtcLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(dtcLabel, DISPLAY_WIDTH - 80);
    lv_obj_align(dtcLabel, LV_ALIGN_TOP_LEFT, 40, 80);
}

namespace DashboardUI {

void init(void) {
    DEBUG_PRINTLN("Initializing Dashboard UI...");
    
    // Create all screens
    createMainScreen();
    createSettingsScreen();
    createDiagnosticsScreen();

    if (gestureTimer == nullptr) {
        gestureTimer = lv_timer_create(gestureTimerCb, 20, nullptr);
    }
    
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

        if (haltechClusterReady) {
            haltech_cluster_set_card_value(haltechCluster, true, CARD_LEFT_MAP, data.mapPressure);
            haltech_cluster_set_card_value(haltechCluster, true, CARD_LEFT_INTAKE, data.intakeTemp);
            haltech_cluster_set_card_value(haltechCluster, false, CARD_RIGHT_IGN, data.ignitionAdvance);
            const float injectorMs = static_cast<float>(data.injectorPulse) / 10.0f;
            haltech_cluster_set_card_value(haltechCluster, false, CARD_RIGHT_INJECTOR, injectorMs);
        }
    } else if (currentScreen == DashboardScreen::DIAGNOSTICS) {
        updateDiagnosticsLabel(data);
    }
}

void setClearDtcCallback(bool (*callback)(void)) {
    clearDtcCallback = callback;
}

void switchScreen(DashboardScreen screen) {
    lv_obj_t* targetScreen = nullptr;
    
    switch (screen) {
        case DashboardScreen::MAIN:
            targetScreen = mainScreen;
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
    haltech_cluster_set_right_value(haltechCluster, clamped);
    haltech_cluster_set_center_value(haltechCluster, false, clamped);
    haltech_cluster_set_digital_value(haltechCluster, false, clamped, RPM_WARNING, RPM_REDLINE);
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
