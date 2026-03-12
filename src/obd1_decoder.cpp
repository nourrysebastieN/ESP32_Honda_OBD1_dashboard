/**
 * @file obd1_decoder.cpp
 * @brief Decode Honda OBD1 52-byte datalogging packets
 */

#include "obd1_decoder.h"

#include <cmath>
#include <cstring>

namespace {

uint8_t fixMil(uint8_t value) {
    switch (value) {
        case 0x18:
            return 0x30;
        case 0x19:
            return 0x1f;
        case 0x1a:
            return 0x24;
        case 0x1b:
            return 0x29;
        case 0x1c:
            return 0x2b;
        default:
            return value;
    }
}

uint8_t mapMilToDtc(uint8_t milIndex) {
    switch (milIndex) {
        case 24:
            return 30;
        case 25:
            return 31;
        case 26:
            return 36;
        case 27:
            return 41;
        case 28:
            return 43;
        case 29:
            return 45;
        case 30:
            return 48;
        default:
            return milIndex;
    }
}

uint8_t clampU8(float value) {
    if (value < 0.0f) {
        return 0;
    }
    if (value > 255.0f) {
        return 255;
    }
    return static_cast<uint8_t>(value + 0.5f);
}

uint16_t clampU16(float value) {
    if (value < 0.0f) {
        return 0;
    }
    if (value > 65535.0f) {
        return 65535;
    }
    return static_cast<uint16_t>(value + 0.5f);
}

float convertTempC(uint8_t raw) {
    const float scaled = static_cast<float>(raw) / 51.0f;
    const float tempF = (0.1423f * std::pow(scaled, 6.0f))
        - (2.4938f * std::pow(scaled, 5.0f))
        + (17.837f * std::pow(scaled, 4.0f))
        - (68.698f * std::pow(scaled, 3.0f))
        + (154.69f * std::pow(scaled, 2.0f))
        - (232.75f * scaled)
        + 284.24f;
    return (tempF - 32.0f) * 5.0f / 9.0f;
}

bool validateChecksum(const uint8_t* packet) {
    uint16_t checksum = packet[0];
    for (size_t index = 1; index < OBD1_PACKET_LENGTH - 1; ++index) {
        checksum += packet[index];
    }
    const uint8_t calc = static_cast<uint8_t>(checksum & 0xff);
    const uint8_t expected = packet[OBD1_PACKET_LENGTH - 1];
    return calc == expected;
}

} // namespace

bool decodeObd1Packet(const uint8_t* packet,
                      size_t length,
                      OBD1Data& out,
                      uint32_t timestampMs) {
    if (packet == nullptr || length < OBD1_PACKET_LENGTH) {
        return false;
    }
    if (!validateChecksum(packet)) {
        return false;
    }

    const uint8_t rpmLowRaw = packet[6];
    const uint8_t rpmHighRaw = packet[7];
    const uint8_t ignRaw = packet[19];
    const uint8_t o2Raw = packet[2];
    const uint8_t vssRaw = packet[16];
    const uint8_t batteryRaw = packet[25];
    const float mapRaw = static_cast<float>(packet[4]);
    const uint8_t tpsRaw = packet[5];
    const uint8_t iatRaw = packet[1];
    const uint8_t ectRaw = packet[0];
    const uint8_t injLowRaw = packet[17];
    const uint8_t injHighRaw = packet[18];
    const uint8_t cel1 = fixMil(packet[12]);
    const uint8_t cel2 = fixMil(packet[13]);
    const uint8_t cel3 = fixMil(packet[14]);
    const uint8_t cel4 = fixMil(packet[15]);

    const uint16_t rpmRaw = (static_cast<uint16_t>(rpmHighRaw) << 8) | rpmLowRaw;
    if (rpmRaw > 0) {
        const float rpm = 1875000.0f / static_cast<float>(rpmRaw);
        out.rpm = clampU16(rpm);
    } else {
        out.rpm = 0;
    }

    const float injMs = (static_cast<uint16_t>(injHighRaw) << 8 | injLowRaw)
        * 3.20000004768372f / 1000.0f;
    const float ignitionAdvance = (0.25f * ignRaw) - 6.0f;
    const float tps = (0.472637f * tpsRaw) - 11.46119f;
    const float battery = (26.0f * batteryRaw) / 270.0f;
    const float map = (1764.0f / 255.0f) * mapRaw * 2.0f + 6.0f;

    out.speed = vssRaw;
    out.intakeTemp = clampU8(convertTempC(iatRaw));
    out.coolantTemp = clampU8(convertTempC(ectRaw));
    out.throttlePosition = clampU8(tps < 0.0f ? 0.0f : (tps > 100.0f ? 100.0f : tps));
    out.mapPressure = clampU16(map);
    out.o2Voltage = static_cast<float>(o2Raw) / 51.0f;
    out.fuelTrim = 0;
    out.ignitionAdvance = clampU8(ignitionAdvance);
    out.injectorPulse = clampU8(injMs * 10.0f);
    out.batteryVoltage = clampU8(battery * 10.0f);
    out.dtcCount = 0;
    std::memset(out.dtcCodes, 0, sizeof(out.dtcCodes));

    const uint8_t celBytes[4] = {cel1, cel2, cel3, cel4};
    uint8_t milIndex = 0;
    for (size_t byteIndex = 0; byteIndex < 4; ++byteIndex) {
        for (uint8_t bit = 0; bit < 8; ++bit) {
            ++milIndex;
            if (celBytes[byteIndex] & (1u << bit)) {
                const uint8_t dtc = mapMilToDtc(milIndex);
                if (out.dtcCount < sizeof(out.dtcCodes)) {
                    out.dtcCodes[out.dtcCount++] = dtc;
                }
            }
        }
    }

    out.checkEngine = out.dtcCount > 0;
    out.timestamp = timestampMs;
    return true;
}
