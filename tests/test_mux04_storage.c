#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blu2usb/storage/product_state.h"
#include "blu2usb/storage/storage.h"

static int failures = 0;

#define CHECK(condition) do {     if (!(condition)) {         fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);         ++failures;     } } while (0)

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

static uint32_t crc32_local(const uint8_t *data, size_t size)
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

static bool encode_legacy_record(
    uint32_t generation,
    const uint8_t *payload,
    size_t payload_size,
    uint8_t out[BLU2USB_STORAGE_RECORD_SIZE])
{
    if (payload_size > BLU2USB_STORAGE_LEGACY_MAX_PAYLOAD_SIZE)
        return false;

    memset(out, 0xff, BLU2USB_STORAGE_RECORD_SIZE);
    put_u32(&out[0], UINT32_C(0x53325042));
    put_u16(&out[4], UINT16_C(1));
    put_u16(&out[6], (uint16_t)payload_size);
    put_u32(&out[8], generation);
    memcpy(&out[12], payload, payload_size);
    put_u32(&out[76], crc32_local(out, 76u));
    return true;
}

static void fill_full_product(blu2usb_product_state_t *state)
{
    blu2usb_product_state_init(state);

    for (unsigned i = 0u; i < BLU2USB_SAVED_MOUSE_CAPACITY; ++i) {
        char name[BLU2USB_MOUSE_NAME_CAPACITY];
        (void)snprintf(name, sizeof(name), "Mouse %02u", i + 1u);

        const blu2usb_mouse_profile_kind_t profile =
            (blu2usb_mouse_profile_kind_t)(i % 4u);

        CHECK(blu2usb_device_registry_add(
            &state->registry,
            (blu2usb_mouse_id_t)(UINT64_C(1000) + i),
            name,
            profile));
    }

    CHECK(blu2usb_device_registry_set_authoritative(
        &state->registry, UINT64_C(1005)));

    state->custom_targets[BLU2USB_MOUSE_SOURCE_LEFT] =
        BLU2USB_MOUSE_TARGET_ESCAPE;
    state->custom_targets[BLU2USB_MOUSE_SOURCE_FORWARD] =
        BLU2USB_MOUSE_TARGET_RIGHT;
    state->draft_valid = true;
    memcpy(
        state->draft_targets,
        state->custom_targets,
        sizeof(state->draft_targets));
    state->draft_targets[BLU2USB_MOUSE_SOURCE_BACKWARD] =
        BLU2USB_MOUSE_TARGET_MIDDLE;
}

static void test_native_roundtrip_full_capacity(void)
{
    blu2usb_product_state_t original;
    blu2usb_product_state_t restored;
    blu2usb_product_restore_source_t source;

    fill_full_product(&original);

    uint8_t payload[BLU2USB_PRODUCT_SERIALIZED_SIZE];
    uint8_t record[BLU2USB_STORAGE_RECORD_SIZE];
    uint8_t loaded[BLU2USB_STORAGE_MAX_PAYLOAD_SIZE];
    size_t loaded_size = 0u;
    uint32_t generation = 0u;

    CHECK(blu2usb_product_state_serialize(&original, payload));
    CHECK(blu2usb_storage_record_encode(
        42u, payload, sizeof(payload), record));
    CHECK(blu2usb_storage_record_decode(
        record, &generation, loaded, sizeof(loaded), &loaded_size));
    CHECK(generation == 42u);
    CHECK(loaded_size == BLU2USB_PRODUCT_SERIALIZED_SIZE);

    CHECK(blu2usb_product_state_restore(
        &restored, loaded, loaded_size, &source));
    CHECK(source == BLU2USB_PRODUCT_RESTORE_NATIVE);
    CHECK(blu2usb_product_state_validate(&restored));
    CHECK(restored.registry.count == BLU2USB_SAVED_MOUSE_CAPACITY);

    /* Live authority is runtime-only and must never survive a reboot. */
    CHECK(!restored.registry.has_authoritative);
    CHECK(restored.registry.authoritative_id == BLU2USB_MOUSE_ID_INVALID);

    for (unsigned i = 0u; i < BLU2USB_SAVED_MOUSE_CAPACITY; ++i) {
        const blu2usb_saved_mouse_t *mouse =
            blu2usb_device_registry_at(&restored.registry, i);
        CHECK(mouse != NULL);
        if (mouse == NULL) continue;

        CHECK(mouse->id == (blu2usb_mouse_id_t)(UINT64_C(1000) + i));
        CHECK(mouse->profile == (blu2usb_mouse_profile_kind_t)(i % 4u));

        char expected[BLU2USB_MOUSE_NAME_CAPACITY];
        (void)snprintf(expected, sizeof(expected), "Mouse %02u", i + 1u);
        CHECK(strcmp(mouse->name, expected) == 0);
    }

    CHECK(restored.custom_targets[BLU2USB_MOUSE_SOURCE_LEFT] ==
          BLU2USB_MOUSE_TARGET_ESCAPE);
    CHECK(restored.custom_targets[BLU2USB_MOUSE_SOURCE_FORWARD] ==
          BLU2USB_MOUSE_TARGET_RIGHT);
    CHECK(restored.draft_valid);
    CHECK(restored.draft_targets[BLU2USB_MOUSE_SOURCE_BACKWARD] ==
          BLU2USB_MOUSE_TARGET_MIDDLE);
    CHECK(!restored.legacy_profile_pending);
}

static void test_native_payload_rejects_invalid_identity_or_profile(void)
{
    blu2usb_product_state_t state;
    blu2usb_product_state_t restored;
    blu2usb_product_restore_source_t source;
    blu2usb_product_state_init(&state);

    CHECK(blu2usb_device_registry_add(
        &state.registry, UINT64_C(1), "One",
        BLU2USB_MOUSE_PROFILE_PASSTHROUGH));

    uint8_t payload[BLU2USB_PRODUCT_SERIALIZED_SIZE];
    CHECK(blu2usb_product_state_serialize(&state, payload));

    /* First entry starts at byte 16. Zero identity is invalid. */
    uint8_t invalid_id[BLU2USB_PRODUCT_SERIALIZED_SIZE];
    memcpy(invalid_id, payload, sizeof(invalid_id));
    memset(&invalid_id[16], 0, 8u);
    CHECK(!blu2usb_product_state_restore(
        &restored, invalid_id, sizeof(invalid_id), &source));

    uint8_t invalid_profile[BLU2USB_PRODUCT_SERIALIZED_SIZE];
    memcpy(invalid_profile, payload, sizeof(invalid_profile));
    invalid_profile[24] = 99u;
    CHECK(!blu2usb_product_state_restore(
        &restored, invalid_profile, sizeof(invalid_profile), &source));
}

static void test_g06_profile_payload_migration(void)
{
    blu2usb_profiles_t legacy;
    blu2usb_profiles_init(&legacy);

    blu2usb_mouse_profile_config_t escape;
    CHECK(blu2usb_profiles_build_preset(
        BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP, &legacy, &escape));
    blu2usb_profiles_activate(&legacy, &escape);

    CHECK(blu2usb_profiles_draft_set(
        &legacy,
        BLU2USB_MOUSE_SOURCE_LEFT,
        BLU2USB_MOUSE_TARGET_RIGHT));
    CHECK(blu2usb_profiles_draft_set(
        &legacy,
        BLU2USB_MOUSE_SOURCE_BACKWARD,
        BLU2USB_MOUSE_TARGET_ESCAPE));

    uint8_t legacy_payload[BLU2USB_PROFILE_SERIALIZED_SIZE];
    CHECK(blu2usb_profiles_serialize(&legacy, legacy_payload));

    uint8_t legacy_record[BLU2USB_STORAGE_RECORD_SIZE];
    CHECK(encode_legacy_record(
        17u, legacy_payload, sizeof(legacy_payload), legacy_record));

    uint8_t loaded[BLU2USB_STORAGE_MAX_PAYLOAD_SIZE];
    size_t loaded_size = 0u;
    uint32_t generation = 0u;

    CHECK(blu2usb_storage_record_decode(
        legacy_record,
        &generation,
        loaded,
        sizeof(loaded),
        &loaded_size));
    CHECK(generation == 17u);
    CHECK(loaded_size == BLU2USB_PROFILE_SERIALIZED_SIZE);

    blu2usb_product_state_t migrated;
    blu2usb_product_restore_source_t source;
    CHECK(blu2usb_product_state_restore(
        &migrated, loaded, loaded_size, &source));

    CHECK(source == BLU2USB_PRODUCT_RESTORE_G06_MIGRATED);
    CHECK(migrated.registry.count == 0u);
    CHECK(!migrated.registry.has_authoritative);
    CHECK(migrated.legacy_profile_pending);
    CHECK(migrated.legacy_profile_kind ==
          BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP);
    CHECK(migrated.draft_valid);
    CHECK(migrated.draft_targets[BLU2USB_MOUSE_SOURCE_LEFT] ==
          BLU2USB_MOUSE_TARGET_RIGHT);
    CHECK(migrated.draft_targets[BLU2USB_MOUSE_SOURCE_BACKWARD] ==
          BLU2USB_MOUSE_TARGET_ESCAPE);
}

static void test_migration_custom_copy_to_profiles(void)
{
    blu2usb_product_state_t state;
    blu2usb_product_state_init(&state);

    state.custom_targets[BLU2USB_MOUSE_SOURCE_LEFT] =
        BLU2USB_MOUSE_TARGET_FORWARD;
    state.draft_valid = true;
    memcpy(
        state.draft_targets,
        state.custom_targets,
        sizeof(state.draft_targets));
    state.draft_targets[BLU2USB_MOUSE_SOURCE_FORWARD] =
        BLU2USB_MOUSE_TARGET_ESCAPE;

    blu2usb_profiles_t profiles;
    blu2usb_profiles_init(&profiles);
    profiles.active_kind = BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP;

    blu2usb_product_state_copy_custom_to_profiles(&state, &profiles);

    CHECK(profiles.active_kind == BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP);
    CHECK(profiles.custom_targets[BLU2USB_MOUSE_SOURCE_LEFT] ==
          BLU2USB_MOUSE_TARGET_FORWARD);
    CHECK(profiles.draft_valid);
    CHECK(profiles.draft_targets[BLU2USB_MOUSE_SOURCE_FORWARD] ==
          BLU2USB_MOUSE_TARGET_ESCAPE);
}

static void test_dual_slot_current_and_legacy_fallback(void)
{
    blu2usb_profiles_t profiles;
    blu2usb_profiles_init(&profiles);

    uint8_t legacy_payload[BLU2USB_PROFILE_SERIALIZED_SIZE];
    CHECK(blu2usb_profiles_serialize(&profiles, legacy_payload));

    uint8_t old_record[BLU2USB_STORAGE_RECORD_SIZE];
    CHECK(encode_legacy_record(
        100u, legacy_payload, sizeof(legacy_payload), old_record));

    blu2usb_product_state_t state;
    blu2usb_product_state_init(&state);
    CHECK(blu2usb_device_registry_add(
        &state.registry, UINT64_C(7), "Seven",
        BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP));

    uint8_t payload[BLU2USB_PRODUCT_SERIALIZED_SIZE];
    uint8_t new_record[BLU2USB_STORAGE_RECORD_SIZE];
    CHECK(blu2usb_product_state_serialize(&state, payload));
    CHECK(blu2usb_storage_record_encode(
        101u, payload, sizeof(payload), new_record));

    CHECK(blu2usb_storage_select_newest(old_record, new_record) == 1);

    /* Simulated torn/corrupt newest write must fall back to valid G06 slot. */
    new_record[200] ^= 0x40u;
    CHECK(blu2usb_storage_select_newest(old_record, new_record) == 0);

    old_record[0] = 0u;
    CHECK(blu2usb_storage_select_newest(old_record, new_record) == -1);
}

static void test_new_record_corruption_and_capacity_guard(void)
{
    blu2usb_product_state_t state;
    fill_full_product(&state);

    CHECK(!blu2usb_device_registry_add(
        &state.registry, UINT64_C(9999), "Overflow",
        BLU2USB_MOUSE_PROFILE_PASSTHROUGH));

    uint8_t payload[BLU2USB_PRODUCT_SERIALIZED_SIZE];
    uint8_t record[BLU2USB_STORAGE_RECORD_SIZE];
    uint8_t loaded[BLU2USB_STORAGE_MAX_PAYLOAD_SIZE];
    size_t loaded_size = 0u;

    CHECK(blu2usb_product_state_serialize(&state, payload));
    CHECK(blu2usb_storage_record_encode(
        3u, payload, sizeof(payload), record));

    record[1000] ^= 0x80u;
    CHECK(!blu2usb_storage_record_decode(
        record, NULL, loaded, sizeof(loaded), &loaded_size));
}

int main(void)
{
    test_native_roundtrip_full_capacity();
    test_native_payload_rejects_invalid_identity_or_profile();
    test_g06_profile_payload_migration();
    test_migration_custom_copy_to_profiles();
    test_dual_slot_current_and_legacy_fallback();
    test_new_record_corruption_and_capacity_guard();

    if (failures != 0) {
        fprintf(stderr, "%d MUX-04 assertion(s) failed\n", failures);
        return 1;
    }

    puts("MUX-04 durable product storage PASS");
    return 0;
}
