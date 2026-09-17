#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blu2usb/profiles/profiles.h"
#include "blu2usb/storage/storage.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "CHECK failed %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

static void activate(blu2usb_profiles_t *profiles, blu2usb_mouse_profile_kind_t kind)
{
    blu2usb_mouse_profile_config_t config;
    CHECK(blu2usb_profiles_build_preset(kind, profiles, &config));
    blu2usb_profiles_activate(profiles, &config);
}

static void test_profile_roundtrip_through_product_record(void)
{
    blu2usb_profiles_t profiles;
    blu2usb_profiles_init(&profiles);
    activate(&profiles, BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP);

    uint8_t serialized[BLU2USB_PROFILE_SERIALIZED_SIZE];
    CHECK(blu2usb_profiles_serialize(&profiles, serialized));

    uint8_t record[BLU2USB_STORAGE_RECORD_SIZE];
    CHECK(blu2usb_storage_record_encode(7u, serialized, sizeof(serialized), record));

    uint8_t loaded[BLU2USB_PROFILE_SERIALIZED_SIZE];
    size_t loaded_size = 0u;
    uint32_t generation = 0u;
    CHECK(blu2usb_storage_record_decode(record, &generation,
                                        loaded, sizeof(loaded), &loaded_size));
    CHECK(generation == 7u);
    CHECK(loaded_size == sizeof(serialized));
    CHECK(memcmp(loaded, serialized, sizeof(serialized)) == 0);

    blu2usb_profiles_t restored;
    blu2usb_profiles_init(&restored);
    CHECK(blu2usb_profiles_restore(&restored, loaded));
    CHECK(restored.active_kind == BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP);
}

static void test_custom_template_roundtrip(void)
{
    blu2usb_profiles_t profiles;
    blu2usb_profiles_init(&profiles);
    CHECK(blu2usb_profiles_draft_set(&profiles, BLU2USB_MOUSE_SOURCE_LEFT,
                                     BLU2USB_MOUSE_TARGET_ESCAPE));
    CHECK(blu2usb_profiles_draft_set(&profiles, BLU2USB_MOUSE_SOURCE_FORWARD,
                                     BLU2USB_MOUSE_TARGET_RIGHT));
    blu2usb_mouse_profile_config_t config;
    CHECK(blu2usb_profiles_custom_candidate(&profiles, &config));
    blu2usb_profiles_activate(&profiles, &config);

    uint8_t serialized[BLU2USB_PROFILE_SERIALIZED_SIZE];
    uint8_t record[BLU2USB_STORAGE_RECORD_SIZE];
    uint8_t loaded[BLU2USB_PROFILE_SERIALIZED_SIZE];
    size_t loaded_size = 0u;
    CHECK(blu2usb_profiles_serialize(&profiles, serialized));
    CHECK(blu2usb_storage_record_encode(8u, serialized, sizeof(serialized), record));
    CHECK(blu2usb_storage_record_decode(record, NULL,
                                        loaded, sizeof(loaded), &loaded_size));

    blu2usb_profiles_t restored;
    blu2usb_profiles_init(&restored);
    CHECK(loaded_size == BLU2USB_PROFILE_SERIALIZED_SIZE);
    CHECK(blu2usb_profiles_restore(&restored, loaded));
    CHECK(restored.active_kind == BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP);
    CHECK(restored.custom_targets[BLU2USB_MOUSE_SOURCE_LEFT] == BLU2USB_MOUSE_TARGET_ESCAPE);
    CHECK(restored.custom_targets[BLU2USB_MOUSE_SOURCE_FORWARD] == BLU2USB_MOUSE_TARGET_RIGHT);
}

static void test_dual_slot_falls_back_after_torn_write(void)
{
    const uint8_t old_payload[] = {1u, 2u, 3u};
    const uint8_t new_payload[] = {4u, 5u, 6u};
    uint8_t old_record[BLU2USB_STORAGE_RECORD_SIZE];
    uint8_t new_record[BLU2USB_STORAGE_RECORD_SIZE];
    CHECK(blu2usb_storage_record_encode(10u, old_payload, sizeof(old_payload), old_record));
    CHECK(blu2usb_storage_record_encode(11u, new_payload, sizeof(new_payload), new_record));
    CHECK(blu2usb_storage_select_newest(old_record, new_record) == 1);

    new_record[20] ^= 0x80u;
    CHECK(blu2usb_storage_select_newest(old_record, new_record) == 0);

    old_record[0] = 0u;
    CHECK(blu2usb_storage_select_newest(old_record, new_record) == -1);
}

int main(void)
{
    test_profile_roundtrip_through_product_record();
    test_custom_template_roundtrip();
    test_dual_slot_falls_back_after_torn_write();
    puts("G06 product persistence tests passed");
    return 0;
}
