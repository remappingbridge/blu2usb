#ifndef BLU2USB_STORAGE_STORAGE_H
#define BLU2USB_STORAGE_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The product record occupies six RP2350 flash pages inside one dedicated
 * sector. Two sectors remain alternated for torn-write fallback. */
#define BLU2USB_STORAGE_MAX_PAYLOAD_SIZE 1280u
#define BLU2USB_STORAGE_RECORD_SIZE 1536u
#define BLU2USB_STORAGE_LEGACY_RECORD_SIZE 80u
#define BLU2USB_STORAGE_LEGACY_MAX_PAYLOAD_SIZE 64u

bool blu2usb_storage_record_encode(
    uint32_t generation,
    const uint8_t *payload,
    size_t payload_size,
    uint8_t out[BLU2USB_STORAGE_RECORD_SIZE]);

/* Decodes both the current envelope and the accepted G06 legacy envelope.
 * Legacy bytes occupy the prefix of the larger record buffer. */
bool blu2usb_storage_record_decode(
    const uint8_t record[BLU2USB_STORAGE_RECORD_SIZE],
    uint32_t *generation,
    uint8_t *payload,
    size_t payload_capacity,
    size_t *payload_size);

int blu2usb_storage_select_newest(
    const uint8_t left[BLU2USB_STORAGE_RECORD_SIZE],
    const uint8_t right[BLU2USB_STORAGE_RECORD_SIZE]);

bool blu2usb_storage_load(uint8_t *payload,
                          size_t payload_capacity,
                          size_t *payload_size);
bool blu2usb_storage_store(const uint8_t *payload, size_t payload_size);

#endif
