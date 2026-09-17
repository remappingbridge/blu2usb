#include "blu2usb/storage/storage.h"

#include <string.h>

#define BLU2USB_STORAGE_MAGIC UINT32_C(0x53325042)
#define BLU2USB_STORAGE_SCHEMA_VERSION UINT16_C(1)
#define BLU2USB_STORAGE_HEADER_SIZE 12u
#define BLU2USB_STORAGE_CRC_OFFSET 76u

static void put_u16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8u);
}

static void put_u32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8u);
    out[2] = (uint8_t)(value >> 16u);
    out[3] = (uint8_t)(value >> 24u);
}

static uint16_t get_u16(const uint8_t *in)
{
    return (uint16_t)in[0] | (uint16_t)((uint16_t)in[1] << 8u);
}

static uint32_t get_u32(const uint8_t *in)
{
    return (uint32_t)in[0] |
           ((uint32_t)in[1] << 8u) |
           ((uint32_t)in[2] << 16u) |
           ((uint32_t)in[3] << 24u);
}

static uint32_t crc32(const uint8_t *data, size_t size)
{
    uint32_t crc = UINT32_C(0xffffffff);
    for (size_t i = 0u; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0u; bit < 8u; ++bit)
            crc = (crc >> 1u) ^ (UINT32_C(0xedb88320) &
                (uint32_t)-(int32_t)(crc & 1u));
    }
    return ~crc;
}

bool blu2usb_storage_record_encode(
    uint32_t generation,
    const uint8_t *payload,
    size_t payload_size,
    uint8_t out[BLU2USB_STORAGE_RECORD_SIZE])
{
    if (out == NULL || payload_size > BLU2USB_STORAGE_MAX_PAYLOAD_SIZE ||
        (payload_size != 0u && payload == NULL)) return false;

    memset(out, 0xff, BLU2USB_STORAGE_RECORD_SIZE);
    put_u32(&out[0], BLU2USB_STORAGE_MAGIC);
    put_u16(&out[4], BLU2USB_STORAGE_SCHEMA_VERSION);
    put_u16(&out[6], (uint16_t)payload_size);
    put_u32(&out[8], generation);
    if (payload_size != 0u)
        memcpy(&out[BLU2USB_STORAGE_HEADER_SIZE], payload, payload_size);
    put_u32(&out[BLU2USB_STORAGE_CRC_OFFSET],
            crc32(out, BLU2USB_STORAGE_CRC_OFFSET));
    return true;
}

bool blu2usb_storage_record_decode(
    const uint8_t record[BLU2USB_STORAGE_RECORD_SIZE],
    uint32_t *generation,
    uint8_t *payload,
    size_t payload_capacity,
    size_t *payload_size)
{
    if (record == NULL) return false;
    if (get_u32(&record[0]) != BLU2USB_STORAGE_MAGIC ||
        get_u16(&record[4]) != BLU2USB_STORAGE_SCHEMA_VERSION) return false;

    const size_t stored_size = get_u16(&record[6]);
    if (stored_size > BLU2USB_STORAGE_MAX_PAYLOAD_SIZE ||
        stored_size > payload_capacity ||
        (stored_size != 0u && payload == NULL)) return false;
    if (get_u32(&record[BLU2USB_STORAGE_CRC_OFFSET]) !=
        crc32(record, BLU2USB_STORAGE_CRC_OFFSET)) return false;

    if (generation != NULL) *generation = get_u32(&record[8]);
    if (payload_size != NULL) *payload_size = stored_size;
    if (stored_size != 0u) memcpy(payload, &record[BLU2USB_STORAGE_HEADER_SIZE], stored_size);
    return true;
}

static bool generation_is_newer(uint32_t candidate, uint32_t reference)
{
    return (int32_t)(candidate - reference) > 0;
}

int blu2usb_storage_select_newest(
    const uint8_t left[BLU2USB_STORAGE_RECORD_SIZE],
    const uint8_t right[BLU2USB_STORAGE_RECORD_SIZE])
{
    uint32_t left_generation = 0u;
    uint32_t right_generation = 0u;
    uint8_t scratch[BLU2USB_STORAGE_MAX_PAYLOAD_SIZE];
    size_t scratch_size = 0u;
    const bool left_valid = blu2usb_storage_record_decode(
        left, &left_generation, scratch, sizeof(scratch), &scratch_size);
    const bool right_valid = blu2usb_storage_record_decode(
        right, &right_generation, scratch, sizeof(scratch), &scratch_size);

    if (!left_valid) return right_valid ? 1 : -1;
    if (!right_valid) return 0;
    return generation_is_newer(right_generation, left_generation) ? 1 : 0;
}
