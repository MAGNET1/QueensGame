#include <crc.h>

#define POLY 0x1021  // CRC-16-CCITT polynomial

constexpr uint16 CRC16_INIT_VALUE = 0xFFFFu;

uint16 CRC_CalculateCRC16(const uint8 *data, size_t length)
{
    uint16 crc = CRC16_INIT_VALUE;

    for (size_t i = 0; i < length; i++) {
        crc ^= (data[i] << 8);
        for (uint8 j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (uint16)((crc << 1) ^ POLY);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}
