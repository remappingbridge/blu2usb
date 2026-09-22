#ifndef BLU2USB_STORAGE_PRODUCT_STATE_H
#define BLU2USB_STORAGE_PRODUCT_STATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blu2usb/device_registry/device_registry.h"
#include "blu2usb/profiles/profiles.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLU2USB_PRODUCT_SERIALIZED_SIZE 1184u
#define BLU2USB_PRODUCT_PAYLOAD_SCHEMA_VERSION 1u

typedef enum {
    BLU2USB_PRODUCT_RESTORE_NATIVE = 0,
    BLU2USB_PRODUCT_RESTORE_G06_MIGRATED,
} blu2usb_product_restore_source_t;

typedef struct {
    blu2usb_device_registry_t registry;
    blu2usb_mouse_target_t custom_targets[BLU2USB_MOUSE_SOURCE_COUNT];
    blu2usb_mouse_target_t draft_targets[BLU2USB_MOUSE_SOURCE_COUNT];
    bool draft_valid;

    /* G06 had one global active profile but no persistent Mouse identity.
     * Preserve that profile until MUX-06 can associate it with the recovered
     * bonded Mouse. Native MUX records normally keep this false. */
    bool legacy_profile_pending;
    blu2usb_mouse_profile_kind_t legacy_profile_kind;
} blu2usb_product_state_t;

void blu2usb_product_state_init(blu2usb_product_state_t *state);

bool blu2usb_product_state_validate(const blu2usb_product_state_t *state);

bool blu2usb_product_state_serialize(
    const blu2usb_product_state_t *state,
    uint8_t out[BLU2USB_PRODUCT_SERIALIZED_SIZE]);

bool blu2usb_product_state_restore(
    blu2usb_product_state_t *state,
    const uint8_t *data,
    size_t data_size,
    blu2usb_product_restore_source_t *source);

bool blu2usb_product_state_from_profiles(
    blu2usb_product_state_t *state,
    const blu2usb_profiles_t *profiles);

void blu2usb_product_state_copy_custom_to_profiles(
    const blu2usb_product_state_t *state,
    blu2usb_profiles_t *profiles);

#ifdef __cplusplus
}
#endif

#endif
