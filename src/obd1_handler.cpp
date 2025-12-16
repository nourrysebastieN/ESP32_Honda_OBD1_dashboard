/**
 * @file obd1_handler.cpp
 * @brief Honda OBD1 ECU communication handler implementation
 * 
 * Implementation of the Honda OBD1 serial communication protocol.
 * This module handles reading data from the ECU diagnostic port.
 * 
 * Honda OBD1 Protocol:
 * - Uses serial communication at 9600 baud
 * - Data is requested by sending specific addresses
 * - ECU responds with single-byte values
 */

#include "obd1_handler.h"
#include "config.h"
#include <Arduino.h>

// OBD1 Data storage
static OBD1Data obd1Data;
static bool polling = false;
static TaskHandle_t pollingTaskHandle = nullptr;
static void (*dataCallback)(const OBD1Data& data) = nullptr;

// Serial port for OBD1 communication
static HardwareSerial OBD1Serial(1);  // Use UART1

/**
 * @brief Honda OBD1 ECU addresses
 * 
 * These addresses are specific to Honda OBD1 ECUs.
 * Different Honda models may have slightly different addresses.
 */
namespace HondaOBD1Addresses {
    constexpr uint8_t RPM_HIGH      = 0x00;
    constexpr uint8_t RPM_LOW       = 0x01;
    constexpr uint8_t SPEED         = 0x02;
    constexpr uint8_t COOLANT_TEMP  = 0x03;
    constexpr uint8_t INTAKE_TEMP   = 0x04;
    constexpr uint8_t MAP           = 0x05;
    constexpr uint8_t TPS           = 0x06;
    constexpr uint8_t O2_SENSOR     = 0x07;
    constexpr uint8_t FUEL_TRIM     = 0x08;
    constexpr uint8_t IGN_ADVANCE   = 0x09;
    constexpr uint8_t INJECTOR      = 0x0A;
    constexpr uint8_t BATTERY       = 0x0B;
    constexpr uint8_t ECU_STATUS    = 0x0C;
}

/**
 * @brief Send a request to the ECU and read response
 * @param address ECU address to query
 * @return Response byte from ECU, or 0xFF on error
 */
static uint8_t queryECU(uint8_t address) {
    // Clear any pending data
    while (OBD1Serial.available()) {
        OBD1Serial.read();
    }
    
    // Send request
    OBD1Serial.write(address);
    
    // Wait for response with timeout
    unsigned long startTime = millis();
    while (!OBD1Serial.available()) {
        if (millis() - startTime > 100) {  // 100ms timeout
            return 0xFF;  // Timeout
        }
        vTaskDelay(1);
    }
    
    return OBD1Serial.read();
}

/**
 * @brief Read all OBD1 parameters
 * Updates the global obd1Data structure
 */
static void readAllParameters(void) {
    // Read RPM (16-bit value)
    uint8_t rpmHigh = queryECU(HondaOBD1Addresses::RPM_HIGH);
    uint8_t rpmLow = queryECU(HondaOBD1Addresses::RPM_LOW);
    if (rpmHigh != 0xFF && rpmLow != 0xFF) {
        obd1Data.rpm = (rpmHigh << 8) | rpmLow;
    }
    
    // Read speed
    uint8_t speed = queryECU(HondaOBD1Addresses::SPEED);
    if (speed != 0xFF) {
        obd1Data.speed = speed;
    }
    
    // Read coolant temperature
    uint8_t coolant = queryECU(HondaOBD1Addresses::COOLANT_TEMP);
    if (coolant != 0xFF) {
        // Convert raw value to Celsius (approximation)
        obd1Data.coolantTemp = coolant - 40;
    }
    
    // Read intake temperature
    uint8_t intake = queryECU(HondaOBD1Addresses::INTAKE_TEMP);
    if (intake != 0xFF) {
        obd1Data.intakeTemp = intake - 40;
    }
    
    // Read MAP pressure
    uint8_t map = queryECU(HondaOBD1Addresses::MAP);
    if (map != 0xFF) {
        obd1Data.mapPressure = map;  // Raw value in kPa
    }
    
    // Read throttle position
    uint8_t tps = queryECU(HondaOBD1Addresses::TPS);
    if (tps != 0xFF) {
        obd1Data.throttlePosition = (tps * 100) / 255;  // Convert to percentage
    }
    
    // Read O2 sensor voltage
    uint8_t o2 = queryECU(HondaOBD1Addresses::O2_SENSOR);
    if (o2 != 0xFF) {
        obd1Data.o2Voltage = o2 * 5.0f / 255.0f;  // Convert to voltage (0-5V)
    }
    
    // Read fuel trim
    uint8_t fuelTrim = queryECU(HondaOBD1Addresses::FUEL_TRIM);
    if (fuelTrim != 0xFF) {
        obd1Data.fuelTrim = fuelTrim;
    }
    
    // Read ignition advance
    uint8_t ignAdv = queryECU(HondaOBD1Addresses::IGN_ADVANCE);
    if (ignAdv != 0xFF) {
        obd1Data.ignitionAdvance = ignAdv;
    }
    
    // Read injector pulse width
    uint8_t injector = queryECU(HondaOBD1Addresses::INJECTOR);
    if (injector != 0xFF) {
        obd1Data.injectorPulse = injector;
    }
    
    // Read battery voltage
    uint8_t battery = queryECU(HondaOBD1Addresses::BATTERY);
    if (battery != 0xFF) {
        obd1Data.batteryVoltage = battery;  // Value * 10 (e.g., 140 = 14.0V)
    }
    
    // Read ECU status (check engine light, etc.)
    uint8_t status = queryECU(HondaOBD1Addresses::ECU_STATUS);
    if (status != 0xFF) {
        obd1Data.checkEngine = (status & 0x80) != 0;
    }
    
    // Update timestamp
    obd1Data.timestamp = millis();
}

/**
 * @brief FreeRTOS task for OBD1 polling
 */
static void obd1PollingTask(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    while (polling) {
        // Try to communicate with ECU
        uint8_t testResponse = queryECU(0x00);
        obd1Data.connected = (testResponse != 0xFF);
        
        if (obd1Data.connected) {
            readAllParameters();
            
            // Call callback if registered
            if (dataCallback != nullptr) {
                dataCallback(obd1Data);
            }
        }
        
        // Wait for next poll interval
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(OBD1_REQUEST_INTERVAL));
    }
    
    // Delete task when stopped
    pollingTaskHandle = nullptr;
    vTaskDelete(nullptr);
}

namespace OBD1Handler {

bool init(void) {
    DEBUG_PRINTLN("Initializing OBD1 handler...");
    
    // Initialize serial port for OBD1
    OBD1Serial.begin(OBD1_BAUD_RATE, SERIAL_8N1, OBD1_RX_PIN, OBD1_TX_PIN);
    
    // Initialize data structure
    memset(&obd1Data, 0, sizeof(obd1Data));
    obd1Data.connected = false;
    
    DEBUG_PRINTLN("OBD1 handler initialized");
    return true;
}

void startPolling(void) {
    if (!polling && pollingTaskHandle == nullptr) {
        polling = true;
        
        xTaskCreatePinnedToCore(
            obd1PollingTask,
            "OBD1_Poll",
            TASK_STACK_OBD1,
            nullptr,
            TASK_PRIORITY_OBD1,
            &pollingTaskHandle,
            0  // Run on core 0
        );
        
        DEBUG_PRINTLN("OBD1 polling started");
    }
}

void stopPolling(void) {
    polling = false;
    DEBUG_PRINTLN("OBD1 polling stopped");
}

bool isConnected(void) {
    return obd1Data.connected;
}

const OBD1Data& getData(void) {
    return obd1Data;
}

uint16_t getRPM(void) {
    return obd1Data.rpm;
}

uint8_t getSpeed(void) {
    return obd1Data.speed;
}

uint8_t getCoolantTemp(void) {
    return obd1Data.coolantTemp;
}

uint8_t getThrottlePosition(void) {
    return obd1Data.throttlePosition;
}

uint16_t getMAP(void) {
    return obd1Data.mapPressure;
}

float getBatteryVoltage(void) {
    return obd1Data.batteryVoltage / 10.0f;
}

void setDataCallback(void (*callback)(const OBD1Data& data)) {
    dataCallback = callback;
}

uint8_t readParameter(uint8_t address) {
    return queryECU(address);
}

} // namespace OBD1Handler
