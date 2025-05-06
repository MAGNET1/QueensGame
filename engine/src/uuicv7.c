#include <uuidv7.h>

static uint64 UUIDv7_GetUnitTimeMs();

void UUIDv7_Generate(uint8 uuid[16]) {
    uint64 timestamp_ms = UUIDv7_GetUnitTimeMs();

    // Fill timestamp (48 bits)
    uuid[0] = (timestamp_ms >> 40) & 0xFF;
    uuid[1] = (timestamp_ms >> 32) & 0xFF;
    uuid[2] = (timestamp_ms >> 24) & 0xFF;
    uuid[3] = (timestamp_ms >> 16) & 0xFF;
    uuid[4] = (timestamp_ms >> 8) & 0xFF;
    uuid[5] = timestamp_ms & 0xFF;

    // Version 7
    uuid[6] = (7 << 4) | (rand() & 0x0F);

    // Random bytes
    uuid[7] = rand() & 0xFF;
    for (int i = 8; i < 16; ++i) {
        uuid[i] = rand() & 0xFF;
    }

    // Variant (RFC 4122)
    uuid[8] &= 0x3F;
    uuid[8] |= 0x80;
}

void UUIDv7_Print(const uint8 uuid[16]) {
    printf("UUIDv7: ");
    debug_print("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
        uuid[0], uuid[1], uuid[2], uuid[3],
        uuid[4], uuid[5],
        uuid[6], uuid[7],
        uuid[8], uuid[9],
        uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
}

#if defined(_WIN32)
#include <windows.h>
static uint64 UUIDv7_GetUnitTimeMs() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64 time = ((uint64)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    // Convert from 100-nanosecond intervals since 1601 to milliseconds since 1970
    return (time - 116444736000000000ULL) / 10000;
}
#else
#include <sys/time.h>
static uint64 UUIDv7_GetUnitTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64)(tv.tv_sec) * 1000ULL + (uint64)(tv.tv_usec) / 1000ULL;
}
#endif
