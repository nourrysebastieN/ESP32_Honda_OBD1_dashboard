/**
 * @file obd1_handler.h
 * @brief Honda OBD1 ECU communication handler
 * 
 * This file provides the interface for communicating with Honda OBD1 ECUs.
 * Honda OBD1 uses a specific serial protocol to read ECU data.
 */

#ifndef OBD1_HANDLER_H
#define OBD1_HANDLER_H

#include <stdint.h>

/**
 * @brief Honda OBD1 data structure
 * 
 * Contains all the parameters that can be read from a Honda OBD1 ECU.
 * Values are updated in real-time when communication is active.
 */
struct OBD1Data {
    uint16_t rpm;              // Engine RPM
    uint8_t  speed;            // Vehicle speed (km/h or mph)
    uint8_t  coolantTemp;      // Coolant temperature (°C)
    uint8_t  intakeTemp;       // Intake air temperature (°C)
    uint8_t  throttlePosition; // Throttle position (%)
    uint16_t mapPressure;      // Manifold absolute pressure (kPa)
    float    o2Voltage;        // O2 sensor voltage
    uint8_t  fuelTrim;         // Short term fuel trim
    uint8_t  ignitionAdvance;  // Ignition timing advance
    uint8_t  injectorPulse;    // Injector pulse width
    uint8_t  batteryVoltage;   // Battery voltage * 10 (e.g., 140 = 14.0V)
    bool     checkEngine;      // Check engine light status
    uint32_t timestamp;        // Last update timestamp
    bool     connected;        // ECU connection status
};

/**
 * @brief OBD1 Handler namespace
 */
namespace OBD1Handler {
    /**
     * @brief Initialize the OBD1 communication
     * @return true if initialization successful, false otherwise
     */
    bool init(void);
    
    /**
     * @brief Start the OBD1 data polling task
     */
    void startPolling(void);
    
    /**
     * @brief Stop the OBD1 data polling task
     */
    void stopPolling(void);
    
    /**
     * @brief Check if ECU is connected
     * @return true if connected, false otherwise
     */
    bool isConnected(void);
    
    /**
     * @brief Get the current OBD1 data
     * @return Const reference to the OBD1 data structure
     */
    const OBD1Data& getData(void);
    
    /**
     * @brief Get RPM value
     * @return Current RPM
     */
    uint16_t getRPM(void);
    
    /**
     * @brief Get vehicle speed
     * @return Current speed in km/h
     */
    uint8_t getSpeed(void);
    
    /**
     * @brief Get coolant temperature
     * @return Current coolant temperature in °C
     */
    uint8_t getCoolantTemp(void);
    
    /**
     * @brief Get throttle position
     * @return Current throttle position percentage
     */
    uint8_t getThrottlePosition(void);
    
    /**
     * @brief Get MAP pressure
     * @return Current manifold absolute pressure in kPa
     */
    uint16_t getMAP(void);
    
    /**
     * @brief Get battery voltage
     * @return Current battery voltage
     */
    float getBatteryVoltage(void);
    
    /**
     * @brief Set callback for data update
     * @param callback Function to call when data is updated
     */
    void setDataCallback(void (*callback)(const OBD1Data& data));
    
    /**
     * @brief Read a specific Honda OBD1 parameter
     * @param address ECU address to read
     * @return Raw value from ECU
     */
    uint8_t readParameter(uint8_t address);
}

#endif /* OBD1_HANDLER_H */
