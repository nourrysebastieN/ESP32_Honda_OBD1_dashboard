/**
 * @file dashboard_ui.h
 * @brief Dashboard UI components and layout manager
 * 
 * This file provides the UI framework for the automotive dashboard.
 * It includes gauges, indicators, and layout management using LVGL.
 */

#ifndef DASHBOARD_UI_H
#define DASHBOARD_UI_H

#include <lvgl.h>
#include "obd1_handler.h"

/**
 * @brief Dashboard screen enumeration
 */
enum class DashboardScreen {
    MAIN,       // Main dashboard with gauges
    DETAILED,   // Detailed data view
    SETTINGS,   // Settings menu
    DIAGNOSTICS // Diagnostic codes view
};

/**
 * @brief Dashboard UI namespace
 */
namespace DashboardUI {
    /**
     * @brief Initialize the dashboard UI
     * Creates all screens and widgets
     */
    void init(void);
    
    /**
     * @brief Update dashboard with new OBD1 data
     * @param data OBD1 data structure with current values
     */
    void update(const OBD1Data& data);
    
    /**
     * @brief Switch to a different screen
     * @param screen Screen to display
     */
    void switchScreen(DashboardScreen screen);
    
    /**
     * @brief Get current active screen
     * @return Currently displayed screen
     */
    DashboardScreen getCurrentScreen(void);
    
    /**
     * @brief Set the RPM gauge value
     * @param rpm Current RPM value
     */
    void setRPM(uint16_t rpm);
    
    /**
     * @brief Set the speed gauge value
     * @param speed Current speed value
     */
    void setSpeed(uint8_t speed);
    
    /**
     * @brief Set the coolant temperature
     * @param temp Current temperature in °C
     */
    void setCoolantTemp(uint8_t temp);
    
    /**
     * @brief Set the fuel level
     * @param level Fuel level percentage
     */
    void setFuelLevel(uint8_t level);
    
    /**
     * @brief Set the throttle position
     * @param position Throttle position percentage
     */
    void setThrottlePosition(uint8_t position);
    
    /**
     * @brief Set battery voltage display
     * @param voltage Battery voltage
     */
    void setBatteryVoltage(float voltage);
    
    /**
     * @brief Show/hide check engine warning
     * @param show true to show, false to hide
     */
    void showCheckEngine(bool show);
    
    /**
     * @brief Show/hide connection status indicator
     * @param connected true if ECU is connected
     */
    void setConnectionStatus(bool connected);
    
    /**
     * @brief Set speed unit (km/h or mph)
     * @param useMetric true for km/h, false for mph
     */
    void setMetricUnits(bool useMetric);
    
    /**
     * @brief Set dashboard theme
     * @param darkMode true for dark theme, false for light theme
     */
    void setDarkMode(bool darkMode);
    
    /**
     * @brief Get the main screen object
     * @return Pointer to the main screen
     */
    lv_obj_t* getMainScreen(void);
    
    /**
     * @brief Create a gauge widget
     * @param parent Parent object
     * @param min Minimum value
     * @param max Maximum value
     * @param warning Warning threshold
     * @return Pointer to created gauge (meter object)
     */
    lv_obj_t* createGauge(lv_obj_t* parent, int32_t min, int32_t max, int32_t warning);
    
    /**
     * @brief Create a bar indicator widget
     * @param parent Parent object
     * @param label Label text
     * @param min Minimum value
     * @param max Maximum value
     * @return Pointer to created bar
     */
    lv_obj_t* createBarIndicator(lv_obj_t* parent, const char* label, int32_t min, int32_t max);
}

#endif /* DASHBOARD_UI_H */
