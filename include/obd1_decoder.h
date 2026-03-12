#ifndef OBD1_DECODER_H
#define OBD1_DECODER_H

#include <stddef.h>
#include <stdint.h>

#include "obd1_handler.h"

constexpr size_t OBD1_PACKET_LENGTH = 52;

bool decodeObd1Packet(const uint8_t* packet,
                      size_t length,
                      OBD1Data& out,
                      uint32_t timestampMs);

#endif /* OBD1_DECODER_H */
