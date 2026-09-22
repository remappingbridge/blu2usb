#ifndef BLU2USB_DEVICE_REGISTRY_DEVICE_REGISTRY_H
#define BLU2USB_DEVICE_REGISTRY_DEVICE_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>

#include "blu2usb/domain/device.h"
#include "blu2usb/domain/profile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    blu2usb_mouse_id_t id;
    char name[BLU2USB_MOUSE_NAME_CAPACITY];
    blu2usb_mouse_profile_kind_t profile;
} blu2usb_saved_mouse_t;

typedef struct {
    blu2usb_saved_mouse_t mice[BLU2USB_SAVED_MOUSE_CAPACITY];
    size_t count;
    bool has_authoritative;
    blu2usb_mouse_id_t authoritative_id;
} blu2usb_device_registry_t;

void blu2usb_device_registry_init(blu2usb_device_registry_t *registry);

size_t blu2usb_device_registry_count(const blu2usb_device_registry_t *registry);

const blu2usb_saved_mouse_t *blu2usb_device_registry_at(
    const blu2usb_device_registry_t *registry,
    size_t index);

const blu2usb_saved_mouse_t *blu2usb_device_registry_find(
    const blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id);

bool blu2usb_device_registry_add(
    blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id,
    const char *name,
    blu2usb_mouse_profile_kind_t profile);

bool blu2usb_device_registry_update_name(
    blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id,
    const char *name);

bool blu2usb_device_registry_set_profile(
    blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id,
    blu2usb_mouse_profile_kind_t profile);

bool blu2usb_device_registry_remove(
    blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id);

bool blu2usb_device_registry_set_authoritative(
    blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id);

void blu2usb_device_registry_clear_authoritative(
    blu2usb_device_registry_t *registry);

const blu2usb_saved_mouse_t *blu2usb_device_registry_authoritative(
    const blu2usb_device_registry_t *registry);

bool blu2usb_device_registry_validate(
    const blu2usb_device_registry_t *registry);

#ifdef __cplusplus
}
#endif

#endif
