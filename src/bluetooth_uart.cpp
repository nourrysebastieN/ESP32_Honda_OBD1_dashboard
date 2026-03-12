/**
 * @file bluetooth_uart.cpp
 * @brief BLE UART bridge implementation (Nordic UART Service profile)
 */

#include "bluetooth_uart.h"

#if BLE_UART_ENABLED

#include <NimBLEDevice.h>
#include <Arduino.h>

namespace {
    // Nordic UART Service UUIDs (TX = notify, RX = write)
    constexpr const char* BLE_UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    constexpr const char* BLE_UART_CHAR_RX_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    constexpr const char* BLE_UART_CHAR_TX_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

    NimBLEServer* server = nullptr;
    NimBLECharacteristic* txCharacteristic = nullptr;
    NimBLEAdvertising* advertising = nullptr;
    QueueHandle_t rxQueue = nullptr;
    bool connected = false;
    bool enabled = true;
    uint32_t lastTelemetryMs = 0;

    class ServerCallbacks : public NimBLEServerCallbacks {
        void onConnect(NimBLEServer* s, ble_gap_conn_desc* desc) override {
            connected = true;
            DEBUG_PRINTF("BLE: Central connected; interval=%u latency=%u timeout=%u\n",
                         desc ? desc->conn_itvl : 0,
                         desc ? desc->conn_latency : 0,
                         desc ? desc->supervision_timeout : 0);
            (void)s;
        }
        void onDisconnect(NimBLEServer* s) override {
            connected = false;
            DEBUG_PRINTLN("BLE: Central disconnected; restarting advertising");
            if (advertising) {
                advertising->start();
            } else if (s) {
                NimBLEDevice::startAdvertising();
            }
        }
    };

    class RxCallbacks : public NimBLECharacteristicCallbacks {
        void onWrite(NimBLECharacteristic* chr) override {
            if (rxQueue == nullptr) {
                return;
            }
            const std::string& value = chr->getValue();
            for (unsigned char c : value) {
                // Drop oldest if queue is full to avoid blocking callbacks
                if (uxQueueSpacesAvailable(rxQueue) == 0) {
                    uint8_t dummy;
                    xQueueReceive(rxQueue, &dummy, 0);
                }
                xQueueSend(rxQueue, &c, 0);
            }
        }
    };
}  // namespace

namespace BluetoothUART {

bool init(const char* deviceName) {
    if (server != nullptr) {
        return true;  // Already initialized
    }

    rxQueue = xQueueCreate(BLE_RX_QUEUE_LENGTH, sizeof(uint8_t));
    if (rxQueue == nullptr) {
        DEBUG_PRINTLN("BLE UART: Failed to create RX queue");
        return false;
    }

    NimBLEDevice::init(deviceName);
#if defined(ESP_PWR_LVL_P0)
    NimBLEDevice::setPower(ESP_PWR_LVL_P0);  // 0 dBm to reduce dropouts
#elif defined(ESP_PWR_LVL_N0)
    NimBLEDevice::setPower(ESP_PWR_LVL_N0);
#endif
    // Use a modest MTU to avoid negotiation failures on some phones
    NimBLEDevice::setMTU(69);
    NimBLEDevice::setSecurityAuth(false, false, false);  // no bonding or secure conn required
    enabled = true;

    server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    NimBLEService* service = server->createService(BLE_UART_SERVICE_UUID);

    NimBLECharacteristic* rxCharacteristic = service->createCharacteristic(
        BLE_UART_CHAR_RX_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    rxCharacteristic->setCallbacks(new RxCallbacks());

    txCharacteristic = service->createCharacteristic(
        BLE_UART_CHAR_TX_UUID,
        NIMBLE_PROPERTY::NOTIFY);

    service->start();

    advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(service->getUUID());
    advertising->setScanResponse(true);
    advertising->start();

    DEBUG_PRINTLN("BLE UART advertising started");
    return true;
}

void loop(void) {
    // NimBLE handles most work internally; nothing required here now.
}

bool isConnected(void) {
    return connected;
}

bool isEnabled(void) {
    return enabled;
}

bool setEnabled(bool en) {
    enabled = en;
    if (!enabled) {
        if (advertising) {
            advertising->stop();
        } else {
            NimBLEDevice::stopAdvertising();
        }
        // Clear inbound queue so stale commands don't apply later
        if (rxQueue) {
            uint8_t tmp;
            while (xQueueReceive(rxQueue, &tmp, 0) == pdTRUE) {}
        }
    } else {
        if (advertising) {
            advertising->start();
        } else if (server != nullptr) {
            NimBLEDevice::startAdvertising();
        }
    }
    return true;
}

size_t write(const uint8_t* data, size_t len) {
    if (!enabled || !connected || txCharacteristic == nullptr || len == 0) {
        return 0;
    }
    txCharacteristic->setValue(data, len);
    txCharacteristic->notify();
    return len;
}

size_t write(const char* str) {
    if (str == nullptr) {
        return 0;
    }
    return write(reinterpret_cast<const uint8_t*>(str), strlen(str));
}

int available(void) {
    if (!enabled || rxQueue == nullptr) {
        return 0;
    }
    return static_cast<int>(uxQueueMessagesWaiting(rxQueue));
}

int read(void) {
    if (!enabled || rxQueue == nullptr) {
        return -1;
    }
    uint8_t value = 0;
    if (xQueueReceive(rxQueue, &value, 0) == pdTRUE) {
        return static_cast<int>(value);
    }
    return -1;
}

void sendObdSnapshot(const OBD1Data& data) {
    if (!enabled || !connected) {
        return;
    }
    const uint32_t now = millis();
    if (now - lastTelemetryMs < BLE_OBD_PUSH_INTERVAL_MS) {
        return;
    }
    lastTelemetryMs = now;

    char buffer[160];
    const int len = snprintf(
        buffer,
        sizeof(buffer),
        "rpm:%u,speed:%u,temp:%u,map:%u,tps:%u,batt:%.1f,dtc:%u,conn:%u\n",
        data.rpm,
        data.speed,
        data.coolantTemp,
        data.mapPressure,
        data.throttlePosition,
        data.batteryVoltage / 10.0f,
        data.dtcCount,
        data.connected ? 1u : 0u);
    if (len > 0) {
        write(reinterpret_cast<const uint8_t*>(buffer), static_cast<size_t>(len));
    }
}

}  // namespace BluetoothUART

#else  // BLE_UART_ENABLED == 0

// Stubbed-out implementation when BLE is disabled at compile time
namespace BluetoothUART {
    bool init(const char*) { return true; }
    void loop(void) {}
    bool isConnected(void) { return false; }
    bool isEnabled(void) { return false; }
    bool setEnabled(bool) { return true; }
    size_t write(const uint8_t*, size_t) { return 0; }
    size_t write(const char*) { return 0; }
    int available(void) { return 0; }
    int read(void) { return -1; }
    void sendObdSnapshot(const OBD1Data&) {}
}  // namespace BluetoothUART

#endif  // BLE_UART_ENABLED
