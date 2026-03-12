/**
 * @file obd1_handler.cpp
 * @brief Honda OBD1 ECU communication handler implementation
 * 
 * Implementation of the Honda OBD1 serial communication protocol.
 * This module handles reading data from the ECU diagnostic port.
 * 
 * Honda OBD1 Protocol:
 * - Uses serial communication at 38400 baud (TTL)
 * - Enter datalogging by sending 16 and receiving 205
 * - Request a 52-byte packet by sending 32
 * - Send 80 to clear stored DTCs
 */

#include "obd1_handler.h"
#include "obd1_decoder.h"
#include "config.h"
#include <Arduino.h>
#include <cstring>

// OBD1 Data storage
static OBD1Data obd1Data;
static bool polling = false;
static TaskHandle_t pollingTaskHandle = nullptr;
static void (*dataCallback)(const OBD1Data& data) = nullptr;

// Serial port for OBD1 communication
static HardwareSerial OBD1Serial(1);  // Use UART1

namespace HondaOBD1Protocol {
    constexpr uint8_t CMD_START_LOGGING = 16;
    constexpr uint8_t CMD_REQUEST_PACKET = 32;
    constexpr uint8_t CMD_CLEAR_DTCS = 80;
    constexpr uint8_t RESP_LOGGING_READY = 205;
    constexpr uint32_t RESPONSE_TIMEOUT_MS = 60;
}

static uint8_t lastPacket[OBD1_PACKET_LENGTH];
static bool packetValid = false;
static bool dataloggingEnabled = false;

static void flushSerial(void) {
    while (OBD1Serial.available()) {
        OBD1Serial.read();
    }
}

static bool readByteWithTimeout(uint8_t& value, uint32_t timeoutMs) {
    const unsigned long startTime = millis();
    while (!OBD1Serial.available()) {
        if (millis() - startTime > timeoutMs) {
            return false;
        }
        vTaskDelay(1);
    }
    value = static_cast<uint8_t>(OBD1Serial.read());
    return true;
}

static bool readBuffer(uint8_t* buffer, size_t length, uint32_t timeoutMs) {
    size_t index = 0;
    unsigned long lastByteTime = millis();
    while (index < length) {
        if (OBD1Serial.available()) {
            buffer[index++] = static_cast<uint8_t>(OBD1Serial.read());
            lastByteTime = millis();
            continue;
        }
        if (millis() - lastByteTime > timeoutMs) {
            return false;
        }
        vTaskDelay(1);
    }
    return true;
}

static bool enterDataloggingMode(void) {
    flushSerial();
    if (OBD1Serial.write(HondaOBD1Protocol::CMD_START_LOGGING) != 1) {
        return false;
    }
    uint8_t response = 0;
    if (!readByteWithTimeout(response, HondaOBD1Protocol::RESPONSE_TIMEOUT_MS)) {
        return false;
    }
    return response == HondaOBD1Protocol::RESP_LOGGING_READY;
}

static bool requestPacket(uint8_t* packet) {
    flushSerial();
    if (OBD1Serial.write(HondaOBD1Protocol::CMD_REQUEST_PACKET) != 1) {
        return false;
    }
    return readBuffer(packet, OBD1_PACKET_LENGTH,
                      HondaOBD1Protocol::RESPONSE_TIMEOUT_MS);
}

/**
 * @brief FreeRTOS task for OBD1 polling
 */
static void obd1PollingTask(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    while (polling) {
        if (!dataloggingEnabled) {
            dataloggingEnabled = enterDataloggingMode();
            packetValid = false;
            obd1Data.connected = dataloggingEnabled;
        } else {
            if (requestPacket(lastPacket)) {
                packetValid = decodeObd1Packet(lastPacket, OBD1_PACKET_LENGTH, obd1Data, millis());
                if (packetValid) {
                    obd1Data.connected = true;
                    if (dataCallback != nullptr) {
                        dataCallback(obd1Data);
                    }
                } else {
                    obd1Data.connected = true;
                }
            } else {
                obd1Data.connected = false;
                dataloggingEnabled = false;
                packetValid = false;
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
    dataloggingEnabled = false;
    packetValid = false;
    
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
    dataloggingEnabled = false;
    packetValid = false;
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

uint8_t readParameter(uint8_t index) {
    if (!packetValid || index >= OBD1_PACKET_LENGTH) {
        return 0xFF;
    }
    return lastPacket[index];
}

bool clearDTCs(void) {
    if (!dataloggingEnabled) {
        dataloggingEnabled = enterDataloggingMode();
        if (!dataloggingEnabled) {
            return false;
        }
    }
    flushSerial();
    return OBD1Serial.write(HondaOBD1Protocol::CMD_CLEAR_DTCS) == 1;
}

} // namespace OBD1Handler
