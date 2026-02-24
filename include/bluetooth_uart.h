/**
 * @file bluetooth_uart.h
 * @brief BLE UART bridge using NimBLE (Nordic UART Service compatible)
 *
 * Provides a lightweight wireless serial channel so the dashboard can be
 * monitored or controlled over Bluetooth LE. Incoming bytes are queued and
 * can be read similarly to HardwareSerial. Outgoing bytes are sent as BLE
 * notifications.
 */
#pragma once

#include <stddef.h>
#include "config.h"
#include "obd1_handler.h"

namespace BluetoothUART {
    /**
     * @brief Initialize BLE UART (no-op if disabled).
     * @param deviceName Advertised device name (defaults to BLE_DEVICE_NAME)
     * @return true on success or when disabled, false on init failure
     */
    bool init(const char* deviceName = BLE_DEVICE_NAME);

    /**
     * @brief Handle connection/advertising housekeeping (call from loop()).
     */
    void loop(void);

    /**
     * @brief Enable or disable the BLE UART bridge (stops advertising when disabled).
     * @return true on success
     */
    bool setEnabled(bool enabled);

    /**
     * @brief Check whether BLE UART is enabled (may still be disconnected).
     */
    bool isEnabled(void);

    /**
     * @brief Check if a BLE central is connected.
     */
    bool isConnected(void);

    /**
     * @brief Send bytes to the connected central.
     */
    size_t write(const uint8_t* data, size_t len);

    /**
     * @brief Convenience overload for C-strings.
     */
    size_t write(const char* str);

    /**
     * @brief Number of pending bytes received over BLE.
     */
    int available(void);

    /**
     * @brief Read a single byte from the BLE RX queue.
     * @return Byte value (0-255) or -1 when no data is available.
     */
    int read(void);

    /**
     * @brief Send a throttled snapshot of current OBD1 data to BLE central.
     */
    void sendObdSnapshot(const OBD1Data& data);
}  // namespace BluetoothUART
