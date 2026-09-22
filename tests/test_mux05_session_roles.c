#include <stdio.h>

#include "blu2usb/ble_hogp/session_roles.h"

static int failures;

#define CHECK(x) do { if (!(x)) {     fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);     ++failures; } } while (0)

static void test_candidate_never_forwards_before_promotion(void)
{
    blu2usb_ble_hogp_session_roles_t roles;
    uint8_t candidate = 0xffu;
    uint32_t generation = 0u;

    blu2usb_ble_hogp_session_roles_init(&roles);
    CHECK(blu2usb_ble_hogp_session_set_authoritative(&roles, 0u, true));
    CHECK(blu2usb_ble_hogp_session_can_forward(&roles, 0u));

    CHECK(blu2usb_ble_hogp_session_start_new(
        &roles, &candidate, &generation));
    CHECK(candidate == 1u);
    CHECK(generation != 0u);
    CHECK(blu2usb_ble_hogp_session_can_forward(&roles, 0u));
    CHECK(!blu2usb_ble_hogp_session_can_forward(&roles, candidate));

    CHECK(blu2usb_ble_hogp_session_candidate_ready(
        &roles, candidate, generation));
    CHECK(blu2usb_ble_hogp_session_can_forward(&roles, 0u));
    CHECK(!blu2usb_ble_hogp_session_can_forward(&roles, candidate));
}

static void test_cancel_invalidates_candidate_only(void)
{
    blu2usb_ble_hogp_session_roles_t roles;
    uint8_t candidate = 0xffu;
    uint8_t disposed = 0xffu;
    uint32_t generation = 0u;

    blu2usb_ble_hogp_session_roles_init(&roles);
    CHECK(blu2usb_ble_hogp_session_set_authoritative(&roles, 0u, true));
    CHECK(blu2usb_ble_hogp_session_start_new(
        &roles, &candidate, &generation));

    CHECK(blu2usb_ble_hogp_session_cancel_new(
        &roles, generation, &disposed));
    CHECK(disposed == candidate);
    CHECK(blu2usb_ble_hogp_session_can_forward(&roles, 0u));
    CHECK(roles.authoritative_slot == 0u);
    CHECK(roles.provisional_slot == BLU2USB_BLE_HOGP_SESSION_SLOT_NONE);

    CHECK(!blu2usb_ble_hogp_session_candidate_ready(
        &roles, candidate, generation));
}

static void test_stale_generation_cannot_commit(void)
{
    blu2usb_ble_hogp_session_roles_t roles;
    uint8_t candidate = 0xffu;
    uint32_t first = 0u;
    uint32_t second = 0u;

    blu2usb_ble_hogp_session_roles_init(&roles);
    CHECK(blu2usb_ble_hogp_session_set_authoritative(&roles, 0u, true));

    CHECK(blu2usb_ble_hogp_session_start_new(&roles, &candidate, &first));
    CHECK(blu2usb_ble_hogp_session_cancel_new(&roles, first, NULL));

    CHECK(blu2usb_ble_hogp_session_start_new(&roles, &candidate, &second));
    CHECK(second != first);

    CHECK(!blu2usb_ble_hogp_session_candidate_ready(
        &roles, candidate, first));
    CHECK(blu2usb_ble_hogp_session_candidate_ready(
        &roles, candidate, second));
    CHECK(!blu2usb_ble_hogp_session_begin_commit(
        &roles, first, NULL));
    CHECK(blu2usb_ble_hogp_session_can_forward(&roles, 0u));
}

static void test_commit_start_can_roll_back_without_losing_current(void)
{
    blu2usb_ble_hogp_session_roles_t roles;
    uint8_t candidate = 0xffu;
    uint8_t retiring = 0xffu;
    uint32_t generation = 0u;

    blu2usb_ble_hogp_session_roles_init(&roles);
    CHECK(blu2usb_ble_hogp_session_set_authoritative(&roles, 0u, true));
    CHECK(blu2usb_ble_hogp_session_start_new(
        &roles, &candidate, &generation));
    CHECK(blu2usb_ble_hogp_session_candidate_ready(
        &roles, candidate, generation));
    CHECK(blu2usb_ble_hogp_session_begin_commit(
        &roles, generation, &retiring));

    CHECK(!blu2usb_ble_hogp_session_can_forward(&roles, 0u));
    CHECK(blu2usb_ble_hogp_session_abort_commit(
        &roles, generation));
    CHECK(blu2usb_ble_hogp_session_can_forward(&roles, 0u));
    CHECK(roles.authoritative_slot == 0u);
    CHECK(roles.roles[candidate] == BLU2USB_BLE_HOGP_ROLE_PROVISIONAL);
    CHECK(roles.ready[candidate]);
    CHECK(roles.new_active);
    CHECK(!roles.commit_pending);
}

static void test_ordered_handoff_and_old_late_disconnect(void)
{
    blu2usb_ble_hogp_session_roles_t roles;
    uint8_t candidate = 0xffu;
    uint8_t retiring = 0xffu;
    uint8_t promoted = 0xffu;
    uint32_t generation = 0u;

    blu2usb_ble_hogp_session_roles_init(&roles);
    CHECK(blu2usb_ble_hogp_session_set_authoritative(&roles, 0u, true));
    CHECK(blu2usb_ble_hogp_session_start_new(
        &roles, &candidate, &generation));
    CHECK(blu2usb_ble_hogp_session_candidate_ready(
        &roles, candidate, generation));

    CHECK(blu2usb_ble_hogp_session_begin_commit(
        &roles, generation, &retiring));
    CHECK(retiring == 0u);

    /* Old input is frozen after application-level held-state release. */
    CHECK(!blu2usb_ble_hogp_session_can_forward(&roles, retiring));
    CHECK(!blu2usb_ble_hogp_session_can_forward(&roles, candidate));

    CHECK(blu2usb_ble_hogp_session_retired(
        &roles, retiring, generation, &promoted));
    CHECK(promoted == candidate);
    CHECK(blu2usb_ble_hogp_session_can_forward(&roles, promoted));

    /* A duplicate/late old disconnect cannot clear the promoted source. */
    CHECK(!blu2usb_ble_hogp_session_disconnected(&roles, retiring));
    CHECK(blu2usb_ble_hogp_session_can_forward(&roles, promoted));
    CHECK(roles.authoritative_slot == promoted);
}

static void test_candidate_disconnect_preserves_current(void)
{
    blu2usb_ble_hogp_session_roles_t roles;
    uint8_t candidate = 0xffu;
    uint32_t generation = 0u;

    blu2usb_ble_hogp_session_roles_init(&roles);
    CHECK(blu2usb_ble_hogp_session_set_authoritative(&roles, 0u, true));
    CHECK(blu2usb_ble_hogp_session_start_new(
        &roles, &candidate, &generation));

    CHECK(blu2usb_ble_hogp_session_disconnected(&roles, candidate));
    CHECK(blu2usb_ble_hogp_session_can_forward(&roles, 0u));
    CHECK(!roles.new_active);
    CHECK(roles.authoritative_slot == 0u);
}

int main(void)
{
    test_candidate_never_forwards_before_promotion();
    test_cancel_invalidates_candidate_only();
    test_stale_generation_cannot_commit();
    test_commit_start_can_roll_back_without_losing_current();
    test_ordered_handoff_and_old_late_disconnect();
    test_candidate_disconnect_preserves_current();

    if (failures != 0) {
        fprintf(stderr, "%d MUX-05 role assertion(s) failed\n", failures);
        return 1;
    }
    puts("MUX-05 authoritative/provisional roles PASS");
    return 0;
}
