#ifndef BLU2USB_DEVICE_REGISTRY_PRODUCT_SNAPSHOT_H
#define BLU2USB_DEVICE_REGISTRY_PRODUCT_SNAPSHOT_H

#include <stdbool.h>
#include <stddef.h>

#include "blu2usb/device_registry/device_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    blu2usb_mouse_id_t id;
    char name[BLU2USB_MOUSE_NAME_CAPACITY];
    blu2usb_mouse_profile_kind_t profile;
    bool connected;
} blu2usb_mouse_snapshot_t;

typedef struct {
    blu2usb_mouse_snapshot_t mice[BLU2USB_SAVED_MOUSE_CAPACITY];
    size_t saved_count;
    bool has_authoritative;
    blu2usb_mouse_id_t authoritative_id;
} blu2usb_product_snapshot_t;

bool blu2usb_product_snapshot_build(
    const blu2usb_device_registry_t *registry,
    blu2usb_product_snapshot_t *snapshot);

const blu2usb_mouse_snapshot_t *blu2usb_product_snapshot_find(
    const blu2usb_product_snapshot_t *snapshot,
    blu2usb_mouse_id_t id);

#ifdef __cplusplus
}
#endif

#endif
