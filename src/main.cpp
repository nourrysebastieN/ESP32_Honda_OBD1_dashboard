/**
 * @file main.cpp
 * @brief Main application entry point for ESP32-S3 Honda OBD1 Dashboard
 * 
 * This is the main application file that initializes all subsystems
 * and runs the main loop for the dashboard.
 * 
 * Hardware Requirements:
 * - ESP32-S3 with PSRAM
 * - 7" TFT LCD display (800x480)
 * - Capacitive touch panel (GT911)
 * - Honda OBD1 ECU connection
 * 
 * @author Your Name
 * @version 1.0.0
 * @date 2024
 */

#include <Arduino.h>
#include "config.h"
#include "display_driver.h"
#include "dashboard_ui.h"
#include "obd1_handler.h"

// Task handles
static TaskHandle_t lvglTaskHandle = nullptr;
static TaskHandle_t obd1TaskHandle = nullptr;

// Timing
static unsigned long lastLvglUpdate = 0;
static unsigned long lastObd1Update = 0;

/**
 * @brief LVGL update task
 * Runs on Core 1 to handle display updates
 */
void lvglTask(void* parameter) {
    DEBUG_PRINTLN("LVGL Task started on core " + String(xPortGetCoreID()));
    
    while (true) {
        DisplayDriver::update();
        vTaskDelay(pdMS_TO_TICKS(LVGL_TICK_PERIOD_MS));
    }
}

/**
 * @brief OBD1 data callback
 * Called when new data is received from the ECU
 */
void onOBD1DataReceived(const OBD1Data& data) {
    // Update the dashboard UI with new data
    DashboardUI::update(data);
}

/**
 * @brief Initialize all hardware and software components
 * @return true if initialization successful
 */
bool initializeSystem(void) {
    // Initialize serial for debug output
    Serial.begin(DEBUG_BAUD_RATE);
    delay(1000);  // Wait for serial to initialize
    
    DEBUG_PRINTLN("\n================================");
    DEBUG_PRINTLN("ESP32-S3 Honda OBD1 Dashboard");
    DEBUG_PRINTLN("================================");
    DEBUG_PRINTLN("Initializing system...");
    
    // Print system info
    DEBUG_PRINTF("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    DEBUG_PRINTF("Free Heap: %d bytes\n", ESP.getFreeHeap());
    DEBUG_PRINTF("PSRAM Size: %d bytes\n", ESP.getPsramSize());
    DEBUG_PRINTF("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    
    // Initialize display driver
    DEBUG_PRINTLN("\nInitializing display...");
    if (!DisplayDriver::init()) {
        DEBUG_PRINTLN("ERROR: Display initialization failed!");
        return false;
    }
    
    // Initialize dashboard UI
    DEBUG_PRINTLN("Initializing UI...");
    DashboardUI::init();
    
    // Initialize OBD1 handler
    DEBUG_PRINTLN("Initializing OBD1 handler...");
    if (!OBD1Handler::init()) {
        DEBUG_PRINTLN("WARNING: OBD1 initialization failed!");
        // Continue anyway - dashboard can work without ECU connection
    }
    
    // Set OBD1 data callback
    OBD1Handler::setDataCallback(onOBD1DataReceived);
    
    // Create LVGL task on Core 1
    xTaskCreatePinnedToCore(
        lvglTask,
        "LVGL",
        TASK_STACK_DISPLAY,
        nullptr,
        TASK_PRIORITY_DISPLAY,
        &lvglTaskHandle,
        1  // Run on Core 1
    );
    
    DEBUG_PRINTLN("\n================================");
    DEBUG_PRINTLN("System initialization complete!");
    DEBUG_PRINTLN("================================\n");
    Serial.println("System initialization complete!");
    
    return true;
}

/**
 * @brief Arduino setup function
 * Called once at startup
 */
void setup() {
    // Initialize the system
    if (!initializeSystem()) {
        DEBUG_PRINTLN("FATAL: System initialization failed!");
        // Blink LED to indicate error
        if (LED_PIN >= 0) {
            pinMode(LED_PIN, OUTPUT);
        }
        while (true) {
            if (LED_PIN >= 0) {
                digitalWrite(LED_PIN, HIGH);
                delay(200);
                digitalWrite(LED_PIN, LOW);
                delay(200);
            } else {
                delay(1000);
            }
        }
    }
    
    // Start OBD1 polling
    OBD1Handler::startPolling();
    
    // Set initial display brightness
    DisplayDriver::setBrightness(255);
    
    DEBUG_PRINTLN("Dashboard ready!");
}

/**
 * @brief Arduino main loop
 * Runs continuously after setup
 */
void loop() {
    // The main work is done in FreeRTOS tasks
    // This loop handles any non-time-critical operations
    
    // Check for serial commands (for debugging)
    if (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case '1':
                DashboardUI::switchScreen(DashboardScreen::MAIN);
                DEBUG_PRINTLN("Switched to Main screen");
                break;
            case '2':
                DashboardUI::switchScreen(DashboardScreen::DETAILED);
                DEBUG_PRINTLN("Switched to Detailed screen");
                break;
            case '3':
                DashboardUI::switchScreen(DashboardScreen::SETTINGS);
                DEBUG_PRINTLN("Switched to Settings screen");
                break;
            case '4':
                DashboardUI::switchScreen(DashboardScreen::DIAGNOSTICS);
                DEBUG_PRINTLN("Switched to Diagnostics screen");
                break;
            case 'r':
                // Simulate RPM change for testing
                DashboardUI::setRPM(random(800, 7000));
                break;
            case 's':
                // Simulate speed change for testing
                DashboardUI::setSpeed(random(0, 160));
                break;
            case 't':
                // Simulate temperature change for testing
                DashboardUI::setCoolantTemp(random(60, 120));
                break;
            case 'i':
                // Print system info
                DEBUG_PRINTF("Free Heap: %d bytes\n", ESP.getFreeHeap());
                DEBUG_PRINTF("Free PSRAM: %d bytes\n", ESP.getFreePsram());
                DEBUG_PRINTF("OBD1 Connected: %s\n", OBD1Handler::isConnected() ? "Yes" : "No");
                break;
            case 'h':
            case '?':
                DEBUG_PRINTLN("\n--- Debug Commands ---");
                DEBUG_PRINTLN("1-4: Switch screens");
                DEBUG_PRINTLN("r: Random RPM");
                DEBUG_PRINTLN("s: Random Speed");
                DEBUG_PRINTLN("t: Random Temperature");
                DEBUG_PRINTLN("i: System info");
                DEBUG_PRINTLN("h/?: This help");
                DEBUG_PRINTLN("--------------------\n");
                break;
        }
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}
