#include <stdio.h>
#include <string.h>

#include "blu2usb/device_registry/device_registry.h"
#include "blu2usb/device_registry/product_snapshot.h"

static int failures = 0;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static void test_identity_crud_and_profiles(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_device_registry_init(&registry);

    CHECK(blu2usb_device_registry_validate(&registry));
    CHECK(blu2usb_device_registry_count(&registry) == 0u);
    CHECK(blu2usb_device_registry_authoritative(&registry) == NULL);

    CHECK(blu2usb_device_registry_add(
        &registry, UINT64_C(101), "Logitech Lift",
        BLU2USB_MOUSE_PROFILE_PASSTHROUGH));
    CHECK(blu2usb_device_registry_add(
        &registry, UINT64_C(202), "Office Mouse",
        BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP));

    CHECK(!blu2usb_device_registry_add(
        &registry, UINT64_C(101), "Duplicate",
        BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP));
    CHECK(blu2usb_device_registry_count(&registry) == 2u);

    CHECK(blu2usb_device_registry_update_name(
        &registry, UINT64_C(202), "Office Mouse 2"));
    CHECK(blu2usb_device_registry_set_profile(
        &registry, UINT64_C(202), BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP));

    const blu2usb_saved_mouse_t *office =
        blu2usb_device_registry_find(&registry, UINT64_C(202));
    CHECK(office != NULL);
    if (office != NULL) {
        CHECK(strcmp(office->name, "Office Mouse 2") == 0);
        CHECK(office->profile == BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP);
    }

    CHECK(!blu2usb_device_registry_update_name(
        &registry, UINT64_C(999), "Missing"));
    CHECK(!blu2usb_device_registry_set_profile(
        &registry, UINT64_C(999), BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP));
    CHECK(!blu2usb_device_registry_remove(&registry, UINT64_C(999)));
    CHECK(!blu2usb_device_registry_add(
        &registry, BLU2USB_MOUSE_ID_INVALID, "Invalid",
        BLU2USB_MOUSE_PROFILE_PASSTHROUGH));
    CHECK(!blu2usb_device_registry_set_profile(
        &registry, UINT64_C(101), (blu2usb_mouse_profile_kind_t)99));

    CHECK(blu2usb_device_registry_validate(&registry));
}

static void test_connected_first_is_projection_not_identity_mutation(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_product_snapshot_t snapshot;
    blu2usb_device_registry_init(&registry);

    CHECK(blu2usb_device_registry_add(
        &registry, UINT64_C(11), "First",
        BLU2USB_MOUSE_PROFILE_PASSTHROUGH));
    CHECK(blu2usb_device_registry_add(
        &registry, UINT64_C(22), "Second",
        BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP));
    CHECK(blu2usb_device_registry_add(
        &registry, UINT64_C(33), "Third",
        BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP));

    const blu2usb_mouse_id_t removal_target = UINT64_C(11);

    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(22)));
    CHECK(blu2usb_device_registry_at(&registry, 0u)->id == UINT64_C(11));
    CHECK(blu2usb_device_registry_at(&registry, 1u)->id == UINT64_C(22));

    CHECK(blu2usb_product_snapshot_build(&registry, &snapshot));
    CHECK(snapshot.saved_count == 3u);
    CHECK(snapshot.has_authoritative);
    CHECK(snapshot.authoritative_id == UINT64_C(22));
    CHECK(snapshot.mice[0].id == UINT64_C(22));
    CHECK(snapshot.mice[0].connected);
    CHECK(snapshot.mice[1].id == UINT64_C(11));
    CHECK(!snapshot.mice[1].connected);
    CHECK(snapshot.mice[2].id == UINT64_C(33));

    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(33)));
    CHECK(blu2usb_product_snapshot_build(&registry, &snapshot));
    CHECK(snapshot.mice[0].id == UINT64_C(33));
    CHECK(snapshot.mice[1].id == UINT64_C(11));
    CHECK(snapshot.mice[2].id == UINT64_C(22));

    const blu2usb_saved_mouse_t *captured =
        blu2usb_device_registry_find(&registry, removal_target);
    CHECK(captured != NULL);
    if (captured != NULL)
        CHECK(captured->id == UINT64_C(11));

    CHECK(blu2usb_device_registry_set_profile(
        &registry, UINT64_C(22), BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP));
    const blu2usb_mouse_snapshot_t *second =
        blu2usb_product_snapshot_find(&snapshot, UINT64_C(22));
    CHECK(second != NULL);
    /* Existing snapshots are immutable views; rebuild publishes the new profile. */
    if (second != NULL)
        CHECK(second->profile == BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP);

    CHECK(blu2usb_product_snapshot_build(&registry, &snapshot));
    second = blu2usb_product_snapshot_find(&snapshot, UINT64_C(22));
    CHECK(second != NULL);
    if (second != NULL)
        CHECK(second->profile == BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP);

    CHECK(blu2usb_device_registry_validate(&registry));
}

static void test_remove_authoritative_and_inactive(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_device_registry_init(&registry);

    CHECK(blu2usb_device_registry_add(
        &registry, UINT64_C(1), "One",
        BLU2USB_MOUSE_PROFILE_PASSTHROUGH));
    CHECK(blu2usb_device_registry_add(
        &registry, UINT64_C(2), "Two",
        BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP));
    CHECK(blu2usb_device_registry_add(
        &registry, UINT64_C(3), "Three",
        BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP));
    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(2)));

    CHECK(blu2usb_device_registry_remove(&registry, UINT64_C(1)));
    CHECK(blu2usb_device_registry_authoritative(&registry) != NULL);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(2));
    CHECK(blu2usb_device_registry_count(&registry) == 2u);

    CHECK(blu2usb_device_registry_remove(&registry, UINT64_C(2)));
    CHECK(blu2usb_device_registry_authoritative(&registry) == NULL);
    CHECK(!registry.has_authoritative);
    CHECK(registry.authoritative_id == BLU2USB_MOUSE_ID_INVALID);
    CHECK(blu2usb_device_registry_count(&registry) == 1u);
    CHECK(blu2usb_device_registry_at(&registry, 0u)->id == UINT64_C(3));
    CHECK(blu2usb_device_registry_validate(&registry));
}

static void test_capacity_and_transactional_name_update(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_device_registry_init(&registry);

    for (unsigned i = 0u; i < BLU2USB_SAVED_MOUSE_CAPACITY; ++i) {
        char name[24];
        (void)snprintf(name, sizeof(name), "Mouse %u", i + 1u);
        CHECK(blu2usb_device_registry_add(
            &registry, (blu2usb_mouse_id_t)(i + 1u), name,
            BLU2USB_MOUSE_PROFILE_PASSTHROUGH));
    }

    CHECK(blu2usb_device_registry_count(&registry) ==
          BLU2USB_SAVED_MOUSE_CAPACITY);
    CHECK(!blu2usb_device_registry_add(
        &registry, UINT64_C(1000), "Overflow",
        BLU2USB_MOUSE_PROFILE_PASSTHROUGH));

    char max_name[BLU2USB_MOUSE_NAME_CAPACITY];
    memset(max_name, 'A', sizeof(max_name));
    max_name[BLU2USB_MOUSE_NAME_CAPACITY - 1u] = '\0';
    CHECK(blu2usb_device_registry_update_name(
        &registry, UINT64_C(1), max_name));

    char too_long[BLU2USB_MOUSE_NAME_CAPACITY + 1u];
    memset(too_long, 'B', sizeof(too_long));
    too_long[BLU2USB_MOUSE_NAME_CAPACITY] = '\0';
    CHECK(!blu2usb_device_registry_update_name(
        &registry, UINT64_C(1), too_long));

    const blu2usb_saved_mouse_t *first =
        blu2usb_device_registry_find(&registry, UINT64_C(1));
    CHECK(first != NULL);
    if (first != NULL)
        CHECK(strcmp(first->name, max_name) == 0);

    CHECK(blu2usb_device_registry_validate(&registry));
}

static void test_snapshot_rejects_invalid_registry(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_product_snapshot_t snapshot;
    blu2usb_device_registry_init(&registry);

    CHECK(blu2usb_device_registry_add(
        &registry, UINT64_C(1), "One",
        BLU2USB_MOUSE_PROFILE_PASSTHROUGH));
    CHECK(blu2usb_device_registry_add(
        &registry, UINT64_C(2), "Two",
        BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP));

    registry.mice[1].id = registry.mice[0].id;
    CHECK(!blu2usb_device_registry_validate(&registry));
    CHECK(!blu2usb_product_snapshot_build(&registry, &snapshot));
}

int main(void)
{
    test_identity_crud_and_profiles();
    test_connected_first_is_projection_not_identity_mutation();
    test_remove_authoritative_and_inactive();
    test_capacity_and_transactional_name_update();
    test_snapshot_rejects_invalid_registry();

    if (failures != 0) {
        fprintf(stderr, "%d MUX-01 assertion(s) failed\n", failures);
        return 1;
    }

    puts("MUX-01 device registry and product snapshot PASS");
    return 0;
}
