#include "blu2usb/device_registry/product_snapshot.h"

#include <string.h>

static void copy_mouse(
    blu2usb_mouse_snapshot_t *destination,
    const blu2usb_saved_mouse_t *source,
    bool connected)
{
    destination->id = source->id;
    memcpy(destination->name, source->name, sizeof(destination->name));
    destination->profile = source->profile;
    destination->connected = connected;
}

bool blu2usb_product_snapshot_build(
    const blu2usb_device_registry_t *registry,
    blu2usb_product_snapshot_t *snapshot)
{
    if (registry == NULL || snapshot == NULL ||
        !blu2usb_device_registry_validate(registry))
        return false;

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->saved_count = registry->count;
    snapshot->has_authoritative = registry->has_authoritative;
    snapshot->authoritative_id = registry->authoritative_id;

    size_t output = 0u;

    if (registry->has_authoritative) {
        const blu2usb_saved_mouse_t *current =
            blu2usb_device_registry_authoritative(registry);
        if (current == NULL) return false;
        copy_mouse(&snapshot->mice[output++], current, true);
    }

    for (size_t i = 0u; i < registry->count; ++i) {
        const blu2usb_saved_mouse_t *mouse =
            blu2usb_device_registry_at(registry, i);
        if (mouse == NULL) return false;
        if (registry->has_authoritative &&
            mouse->id == registry->authoritative_id)
            continue;
        copy_mouse(&snapshot->mice[output++], mouse, false);
    }

    return output == registry->count;
}

const blu2usb_mouse_snapshot_t *blu2usb_product_snapshot_find(
    const blu2usb_product_snapshot_t *snapshot,
    blu2usb_mouse_id_t id)
{
    if (snapshot == NULL || !blu2usb_mouse_id_is_valid(id)) return NULL;

    for (size_t i = 0u; i < snapshot->saved_count; ++i)
        if (snapshot->mice[i].id == id) return &snapshot->mice[i];

    return NULL;
}
