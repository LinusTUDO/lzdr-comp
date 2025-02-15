#include "compressor.h"

#include <cstdint>

void u32_to_bytes(const uint32_t value, uint8_t *bytes) {
    bytes[0] = static_cast<uint8_t>(value & 0xFF);
    bytes[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    bytes[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    bytes[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

uint32_t u32_from_bytes(const uint8_t *bytes) {
    uint32_t value = 0;
    value |= static_cast<uint32_t>(bytes[0]);
    value |= static_cast<uint32_t>(bytes[1]) << 8;
    value |= static_cast<uint32_t>(bytes[2]) << 16;
    value |= static_cast<uint32_t>(bytes[3]) << 24;
    return value;
}
