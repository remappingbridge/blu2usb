#ifndef BLU2USB_STORAGE_STORAGE_H
#define BLU2USB_STORAGE_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BLU2USB_STORAGE_MAX_PAYLOAD_SIZE 768u
#define BLU2USB_STORAGE_RECORD_SIZE 784u

bool blu2usb_storage_record_encode(
    uint32_t generation,
    const uint8_t *payload,
    size_t payload_size,
    uint8_t out[BLU2USB_STORAGE_RECORD_SIZE]);

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
