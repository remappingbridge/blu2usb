#include "blu2usb/device_registry/device_registry.h"

#include <string.h>

static bool profile_is_valid(blu2usb_mouse_profile_kind_t profile)
{
    return profile == BLU2USB_MOUSE_PROFILE_PASSTHROUGH ||
           profile == BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP ||
           profile == BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP ||
           profile == BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP;
}

static bool copy_name(char out[BLU2USB_MOUSE_NAME_CAPACITY], const char *name)
{
    size_t length = 0u;

    if (out == NULL || name == NULL) return false;

    while (name[length] != '\0' && length + 1u < BLU2USB_MOUSE_NAME_CAPACITY)
        ++length;

    if (name[length] != '\0') return false;

    memcpy(out, name, length + 1u);
    return true;
}

static ptrdiff_t find_index(
    const blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id)
{
    if (registry == NULL || !blu2usb_mouse_id_is_valid(id)) return -1;

    for (size_t i = 0u; i < registry->count; ++i)
        if (registry->mice[i].id == id) return (ptrdiff_t)i;

    return -1;
}

void blu2usb_device_registry_init(blu2usb_device_registry_t *registry)
{
    if (registry == NULL) return;
    memset(registry, 0, sizeof(*registry));
    registry->authoritative_id = BLU2USB_MOUSE_ID_INVALID;
}

size_t blu2usb_device_registry_count(const blu2usb_device_registry_t *registry)
{
    return registry == NULL ? 0u : registry->count;
}

const blu2usb_saved_mouse_t *blu2usb_device_registry_at(
    const blu2usb_device_registry_t *registry,
    size_t index)
{
    if (registry == NULL || index >= registry->count) return NULL;
    return &registry->mice[index];
}

const blu2usb_saved_mouse_t *blu2usb_device_registry_find(
    const blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id)
{
    const ptrdiff_t index = find_index(registry, id);
    return index < 0 ? NULL : &registry->mice[index];
}

bool blu2usb_device_registry_add(
    blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id,
    const char *name,
    blu2usb_mouse_profile_kind_t profile)
{
    char bounded_name[BLU2USB_MOUSE_NAME_CAPACITY];

    if (registry == NULL ||
        !blu2usb_mouse_id_is_valid(id) ||
        !profile_is_valid(profile) ||
        registry->count >= BLU2USB_SAVED_MOUSE_CAPACITY ||
        find_index(registry, id) >= 0 ||
        !copy_name(bounded_name, name))
        return false;

    blu2usb_saved_mouse_t *mouse = &registry->mice[registry->count++];
    mouse->id = id;
    memcpy(mouse->name, bounded_name, sizeof(mouse->name));
    mouse->profile = profile;
    return true;
}

bool blu2usb_device_registry_update_name(
    blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id,
    const char *name)
{
    const ptrdiff_t index = find_index(registry, id);
    char bounded_name[BLU2USB_MOUSE_NAME_CAPACITY];

    if (index < 0 || !copy_name(bounded_name, name)) return false;

    memcpy(registry->mice[index].name, bounded_name,
           sizeof(registry->mice[index].name));
    return true;
}

bool blu2usb_device_registry_set_profile(
    blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id,
    blu2usb_mouse_profile_kind_t profile)
{
    const ptrdiff_t index = find_index(registry, id);
    if (index < 0 || !profile_is_valid(profile)) return false;

    registry->mice[index].profile = profile;
    return true;
}

bool blu2usb_device_registry_remove(
    blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id)
{
    const ptrdiff_t index = find_index(registry, id);
    if (index < 0) return false;

    if (registry->has_authoritative && registry->authoritative_id == id)
        blu2usb_device_registry_clear_authoritative(registry);

    const size_t position = (size_t)index;
    if (position + 1u < registry->count) {
        memmove(&registry->mice[position],
                &registry->mice[position + 1u],
                (registry->count - position - 1u) * sizeof(registry->mice[0]));
    }

    --registry->count;
    memset(&registry->mice[registry->count], 0, sizeof(registry->mice[0]));
    return true;
}

bool blu2usb_device_registry_set_authoritative(
    blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id)
{
    if (registry == NULL || find_index(registry, id) < 0) return false;

    registry->has_authoritative = true;
    registry->authoritative_id = id;
    return true;
}

void blu2usb_device_registry_clear_authoritative(
    blu2usb_device_registry_t *registry)
{
    if (registry == NULL) return;

    registry->has_authoritative = false;
    registry->authoritative_id = BLU2USB_MOUSE_ID_INVALID;
}

const blu2usb_saved_mouse_t *blu2usb_device_registry_authoritative(
    const blu2usb_device_registry_t *registry)
{
    if (registry == NULL || !registry->has_authoritative) return NULL;
    return blu2usb_device_registry_find(registry, registry->authoritative_id);
}

bool blu2usb_device_registry_validate(
    const blu2usb_device_registry_t *registry)
{
    if (registry == NULL || registry->count > BLU2USB_SAVED_MOUSE_CAPACITY)
        return false;

    for (size_t i = 0u; i < registry->count; ++i) {
        if (!blu2usb_mouse_id_is_valid(registry->mice[i].id) ||
            !profile_is_valid(registry->mice[i].profile))
            return false;

        for (size_t j = i + 1u; j < registry->count; ++j)
            if (registry->mice[i].id == registry->mice[j].id) return false;
    }

    if (registry->has_authoritative) {
        if (!blu2usb_mouse_id_is_valid(registry->authoritative_id) ||
            find_index(registry, registry->authoritative_id) < 0)
            return false;
    } else if (registry->authoritative_id != BLU2USB_MOUSE_ID_INVALID) {
        return false;
    }

    return true;
}
