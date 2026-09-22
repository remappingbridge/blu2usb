#include "blu2usb/storage/product_state.h"

#include <string.h>

#define PRODUCT_HEADER_SIZE 16u
#define PRODUCT_ENTRY_SIZE (8u + 1u + BLU2USB_MOUSE_NAME_CAPACITY)

#define PRODUCT_SCHEMA_OFFSET 0u
#define PRODUCT_MOUSE_COUNT_OFFSET 1u
#define PRODUCT_DRAFT_VALID_OFFSET 2u
#define PRODUCT_LEGACY_PENDING_OFFSET 3u
#define PRODUCT_LEGACY_KIND_OFFSET 4u
#define PRODUCT_CUSTOM_OFFSET 6u
#define PRODUCT_DRAFT_OFFSET     (PRODUCT_CUSTOM_OFFSET + BLU2USB_MOUSE_SOURCE_COUNT)
#define PRODUCT_ENTRIES_OFFSET PRODUCT_HEADER_SIZE

_Static_assert(
    PRODUCT_ENTRIES_OFFSET +
        BLU2USB_SAVED_MOUSE_CAPACITY * PRODUCT_ENTRY_SIZE ==
        BLU2USB_PRODUCT_SERIALIZED_SIZE,
    "product payload layout must stay deterministic");

static bool profile_valid(blu2usb_mouse_profile_kind_t profile)
{
    return profile == BLU2USB_MOUSE_PROFILE_PASSTHROUGH ||
           profile == BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP ||
           profile == BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP ||
           profile == BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP;
}

static bool target_valid(blu2usb_mouse_target_t target)
{
    return (unsigned)target < BLU2USB_MOUSE_TARGET_COUNT;
}

static blu2usb_mouse_target_t identity_target(blu2usb_mouse_source_t source)
{
    switch (source) {
    case BLU2USB_MOUSE_SOURCE_LEFT: return BLU2USB_MOUSE_TARGET_LEFT;
    case BLU2USB_MOUSE_SOURCE_RIGHT: return BLU2USB_MOUSE_TARGET_RIGHT;
    case BLU2USB_MOUSE_SOURCE_MIDDLE: return BLU2USB_MOUSE_TARGET_MIDDLE;
    case BLU2USB_MOUSE_SOURCE_FORWARD: return BLU2USB_MOUSE_TARGET_FORWARD;
    case BLU2USB_MOUSE_SOURCE_BACKWARD: return BLU2USB_MOUSE_TARGET_BACKWARD;
    default: return BLU2USB_MOUSE_TARGET_LEFT;
    }
}

static void put_u64(uint8_t *out, uint64_t value)
{
    for (unsigned i = 0u; i < 8u; ++i)
        out[i] = (uint8_t)(value >> (i * 8u));
}

static uint64_t get_u64(const uint8_t *in)
{
    uint64_t value = 0u;
    for (unsigned i = 0u; i < 8u; ++i)
        value |= (uint64_t)in[i] << (i * 8u);
    return value;
}

void blu2usb_product_state_init(blu2usb_product_state_t *state)
{
    if (state == NULL) return;

    memset(state, 0, sizeof(*state));
    blu2usb_device_registry_init(&state->registry);

    for (unsigned source = 0u; source < BLU2USB_MOUSE_SOURCE_COUNT; ++source) {
        const blu2usb_mouse_target_t target =
            identity_target((blu2usb_mouse_source_t)source);
        state->custom_targets[source] = target;
        state->draft_targets[source] = target;
    }

    state->legacy_profile_kind = BLU2USB_MOUSE_PROFILE_PASSTHROUGH;
}

bool blu2usb_product_state_validate(const blu2usb_product_state_t *state)
{
    if (state == NULL ||
        !blu2usb_device_registry_validate(&state->registry) ||
        (state->legacy_profile_pending &&
         !profile_valid(state->legacy_profile_kind)))
        return false;

    for (unsigned source = 0u; source < BLU2USB_MOUSE_SOURCE_COUNT; ++source) {
        if (!target_valid(state->custom_targets[source]) ||
            !target_valid(state->draft_targets[source]))
            return false;
    }

    return true;
}

bool blu2usb_product_state_serialize(
    const blu2usb_product_state_t *state,
    uint8_t out[BLU2USB_PRODUCT_SERIALIZED_SIZE])
{
    if (state == NULL || out == NULL ||
        !blu2usb_product_state_validate(state))
        return false;

    memset(out, 0, BLU2USB_PRODUCT_SERIALIZED_SIZE);

    out[PRODUCT_SCHEMA_OFFSET] = BLU2USB_PRODUCT_PAYLOAD_SCHEMA_VERSION;
    out[PRODUCT_MOUSE_COUNT_OFFSET] = (uint8_t)state->registry.count;
    out[PRODUCT_DRAFT_VALID_OFFSET] = state->draft_valid ? 1u : 0u;
    out[PRODUCT_LEGACY_PENDING_OFFSET] =
        state->legacy_profile_pending ? 1u : 0u;
    out[PRODUCT_LEGACY_KIND_OFFSET] = (uint8_t)state->legacy_profile_kind;

    for (unsigned source = 0u; source < BLU2USB_MOUSE_SOURCE_COUNT; ++source) {
        out[PRODUCT_CUSTOM_OFFSET + source] =
            (uint8_t)state->custom_targets[source];
        out[PRODUCT_DRAFT_OFFSET + source] =
            (uint8_t)state->draft_targets[source];
    }

    for (size_t i = 0u; i < state->registry.count; ++i) {
        const blu2usb_saved_mouse_t *mouse =
            blu2usb_device_registry_at(&state->registry, i);
        if (mouse == NULL) return false;

        uint8_t *entry =
            &out[PRODUCT_ENTRIES_OFFSET + i * PRODUCT_ENTRY_SIZE];

        put_u64(entry, mouse->id);
        entry[8] = (uint8_t)mouse->profile;
        memcpy(&entry[9], mouse->name, BLU2USB_MOUSE_NAME_CAPACITY);
    }

    return true;
}

static bool restore_native(
    blu2usb_product_state_t *state,
    const uint8_t data[BLU2USB_PRODUCT_SERIALIZED_SIZE])
{
    if (data[PRODUCT_SCHEMA_OFFSET] !=
            BLU2USB_PRODUCT_PAYLOAD_SCHEMA_VERSION ||
        data[PRODUCT_MOUSE_COUNT_OFFSET] > BLU2USB_SAVED_MOUSE_CAPACITY ||
        data[PRODUCT_DRAFT_VALID_OFFSET] > 1u ||
        data[PRODUCT_LEGACY_PENDING_OFFSET] > 1u)
        return false;

    const blu2usb_mouse_profile_kind_t legacy_kind =
        (blu2usb_mouse_profile_kind_t)data[PRODUCT_LEGACY_KIND_OFFSET];

    if (data[PRODUCT_LEGACY_PENDING_OFFSET] != 0u &&
        !profile_valid(legacy_kind))
        return false;

    blu2usb_product_state_t candidate;
    blu2usb_product_state_init(&candidate);
    candidate.draft_valid = data[PRODUCT_DRAFT_VALID_OFFSET] != 0u;
    candidate.legacy_profile_pending =
        data[PRODUCT_LEGACY_PENDING_OFFSET] != 0u;
    candidate.legacy_profile_kind = candidate.legacy_profile_pending
        ? legacy_kind : BLU2USB_MOUSE_PROFILE_PASSTHROUGH;

    for (unsigned source = 0u; source < BLU2USB_MOUSE_SOURCE_COUNT; ++source) {
        const blu2usb_mouse_target_t custom =
            (blu2usb_mouse_target_t)data[PRODUCT_CUSTOM_OFFSET + source];
        const blu2usb_mouse_target_t draft =
            (blu2usb_mouse_target_t)data[PRODUCT_DRAFT_OFFSET + source];

        if (!target_valid(custom) || !target_valid(draft))
            return false;

        candidate.custom_targets[source] = custom;
        candidate.draft_targets[source] =
            candidate.draft_valid ? draft : custom;
    }

    const size_t count = data[PRODUCT_MOUSE_COUNT_OFFSET];
    for (size_t i = 0u; i < count; ++i) {
        const uint8_t *entry =
            &data[PRODUCT_ENTRIES_OFFSET + i * PRODUCT_ENTRY_SIZE];
        const blu2usb_mouse_id_t id = get_u64(entry);
        const blu2usb_mouse_profile_kind_t profile =
            (blu2usb_mouse_profile_kind_t)entry[8];

        if (!profile_valid(profile))
            return false;

        char name[BLU2USB_MOUSE_NAME_CAPACITY];
        memcpy(name, &entry[9], sizeof(name));

        if (memchr(name, '\0', sizeof(name)) == NULL)
            return false;

        if (!blu2usb_device_registry_add(
                &candidate.registry, id, name, profile))
            return false;
    }

    /* Live authority is runtime truth and is never restored from flash. */
    blu2usb_device_registry_clear_authoritative(&candidate.registry);

    if (!blu2usb_product_state_validate(&candidate))
        return false;

    *state = candidate;
    return true;
}

bool blu2usb_product_state_from_profiles(
    blu2usb_product_state_t *state,
    const blu2usb_profiles_t *profiles)
{
    if (state == NULL || profiles == NULL ||
        !profile_valid(profiles->active_kind))
        return false;

    blu2usb_product_state_init(state);

    for (unsigned source = 0u; source < BLU2USB_MOUSE_SOURCE_COUNT; ++source) {
        if (!target_valid(profiles->custom_targets[source]) ||
            !target_valid(profiles->draft_targets[source]))
            return false;

        state->custom_targets[source] = profiles->custom_targets[source];
        state->draft_targets[source] = profiles->draft_valid
            ? profiles->draft_targets[source]
            : profiles->custom_targets[source];
    }

    state->draft_valid = profiles->draft_valid;
    state->legacy_profile_pending = true;
    state->legacy_profile_kind = profiles->active_kind;
    return true;
}

static bool restore_g06(
    blu2usb_product_state_t *state,
    const uint8_t *data,
    size_t data_size)
{
    if (data_size != BLU2USB_PROFILE_SERIALIZED_SIZE)
        return false;

    blu2usb_profiles_t profiles;
    blu2usb_profiles_init(&profiles);

    if (!blu2usb_profiles_restore(&profiles, data))
        return false;

    return blu2usb_product_state_from_profiles(state, &profiles);
}

bool blu2usb_product_state_restore(
    blu2usb_product_state_t *state,
    const uint8_t *data,
    size_t data_size,
    blu2usb_product_restore_source_t *source)
{
    if (state == NULL || data == NULL)
        return false;

    blu2usb_product_state_t candidate;
    bool restored = false;
    blu2usb_product_restore_source_t restore_source =
        BLU2USB_PRODUCT_RESTORE_NATIVE;

    if (data_size == BLU2USB_PRODUCT_SERIALIZED_SIZE) {
        restored = restore_native(&candidate, data);
    } else if (data_size == BLU2USB_PROFILE_SERIALIZED_SIZE) {
        restored = restore_g06(&candidate, data, data_size);
        restore_source = BLU2USB_PRODUCT_RESTORE_G06_MIGRATED;
    }

    if (!restored)
        return false;

    *state = candidate;
    if (source != NULL) *source = restore_source;
    return true;
}

void blu2usb_product_state_copy_custom_to_profiles(
    const blu2usb_product_state_t *state,
    blu2usb_profiles_t *profiles)
{
    if (state == NULL || profiles == NULL ||
        !blu2usb_product_state_validate(state))
        return;

    for (unsigned source = 0u; source < BLU2USB_MOUSE_SOURCE_COUNT; ++source) {
        profiles->custom_targets[source] = state->custom_targets[source];
        profiles->draft_targets[source] = state->draft_valid
            ? state->draft_targets[source]
            : state->custom_targets[source];
    }

    profiles->draft_valid = state->draft_valid;
}
