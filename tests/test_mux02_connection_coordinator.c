#include <stdio.h>
#include <string.h>

#include "blu2usb/connection_coordinator/connection_coordinator.h"

static int failures = 0;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static void seed_mouse(
    blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id,
    const char *name,
    blu2usb_mouse_profile_kind_t profile)
{
    CHECK(blu2usb_device_registry_add(registry, id, name, profile));
}

static void test_home_resolver(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_connection_coordinator_t coordinator;
    blu2usb_device_registry_init(&registry);
    CHECK(blu2usb_connection_coordinator_init(&coordinator, &registry, 0u));

    CHECK(blu2usb_connection_coordinator_resolve_home(&coordinator) ==
          BLU2USB_HOME_FIRST_SEARCH);

    seed_mouse(&registry, UINT64_C(1), "One",
               BLU2USB_MOUSE_PROFILE_PASSTHROUGH);
    CHECK(blu2usb_connection_coordinator_resolve_home(&coordinator) ==
          BLU2USB_HOME_SAVED_SEARCH);

    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(1)));
    CHECK(blu2usb_connection_coordinator_resolve_home(&coordinator) ==
          BLU2USB_HOME_CONNECTED);

    blu2usb_device_registry_clear_authoritative(&registry);
    CHECK(blu2usb_connection_coordinator_resolve_home(&coordinator) ==
          BLU2USB_HOME_SAVED_SEARCH);
}

static void test_first_cycles_and_stale_result(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_connection_coordinator_t coordinator;
    uint64_t token1 = 0u;
    uint64_t token2 = 0u;

    blu2usb_device_registry_init(&registry);
    CHECK(blu2usb_connection_coordinator_init(&coordinator, &registry, 100u));
    CHECK(blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_FIRST, 100u, &token1));
    CHECK(token1 != 0u);
    CHECK(coordinator.search.deadline_at_ms == UINT64_C(8100));
    CHECK(!blu2usb_connection_coordinator_cancel_search(&coordinator, token1));

    CHECK(blu2usb_connection_coordinator_advance_time(&coordinator, 8099u));
    CHECK(coordinator.search.token == token1);

    CHECK(blu2usb_connection_coordinator_advance_time(&coordinator, 8100u));
    token2 = coordinator.search.token;
    CHECK(token2 != 0u && token2 != token1);
    CHECK(coordinator.search.status == BLU2USB_SEARCH_RUNNING);

    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token1, UINT64_C(90), "Old Candidate") ==
        BLU2USB_CANDIDATE_STALE);
    CHECK(coordinator.stale_result_count == 1u);
    CHECK(blu2usb_device_registry_count(&registry) == 0u);

    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token2, UINT64_C(10), "First Mouse") ==
        BLU2USB_CANDIDATE_ACCEPTED);
    CHECK(coordinator.search.status == BLU2USB_SEARCH_SUCCEEDED);
    CHECK(!blu2usb_connection_coordinator_busy(&coordinator));
    CHECK(blu2usb_device_registry_count(&registry) == 1u);
    CHECK(blu2usb_device_registry_authoritative(&registry) != NULL);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(10));
    CHECK(blu2usb_device_registry_authoritative(&registry)->profile ==
          BLU2USB_MOUSE_PROFILE_PASSTHROUGH);

    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token2, UINT64_C(11), "Second Winner") ==
        BLU2USB_CANDIDATE_STALE);
    CHECK(blu2usb_device_registry_count(&registry) == 1u);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(10));
}

static void test_saved_search_eligibility_timeout_cancel(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_connection_coordinator_t coordinator;
    uint64_t token = 0u;

    blu2usb_device_registry_init(&registry);
    seed_mouse(&registry, UINT64_C(1), "One",
               BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP);
    seed_mouse(&registry, UINT64_C(2), "Two",
               BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP);
    CHECK(blu2usb_connection_coordinator_init(&coordinator, &registry, 0u));

    CHECK(blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_SAVED, 0u, &token));
    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token, UINT64_C(99), "Unsaved") ==
        BLU2USB_CANDIDATE_REJECTED);
    CHECK(coordinator.search.status == BLU2USB_SEARCH_RUNNING);

    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token, UINT64_C(2), "Two") ==
        BLU2USB_CANDIDATE_ACCEPTED);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(2));

    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token, UINT64_C(1), "One") ==
        BLU2USB_CANDIDATE_STALE);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(2));

    blu2usb_device_registry_clear_authoritative(&registry);
    CHECK(blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_SAVED, 100u, &token));
    CHECK(blu2usb_connection_coordinator_advance_time(&coordinator, 8100u));
    CHECK(coordinator.search.status == BLU2USB_SEARCH_TIMED_OUT);
    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token, UINT64_C(1), "Late") ==
        BLU2USB_CANDIDATE_STALE);
    CHECK(blu2usb_device_registry_authoritative(&registry) == NULL);

    CHECK(blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_SAVED, 8200u, &token));
    CHECK(blu2usb_connection_coordinator_cancel_search(&coordinator, token));
    CHECK(coordinator.search.status == BLU2USB_SEARCH_CANCELLED);
    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token, UINT64_C(1), "Cancelled Late") ==
        BLU2USB_CANDIDATE_STALE);
}

static void complete_handoff(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t token,
    bool had_old)
{
    if (had_old) {
        CHECK(coordinator->handoff_phase == BLU2USB_HANDOFF_FREEZE_OLD_INPUT);
        CHECK(blu2usb_connection_coordinator_handoff_ack(
            coordinator, token, BLU2USB_HANDOFF_FREEZE_OLD_INPUT));
        CHECK(coordinator->handoff_phase == BLU2USB_HANDOFF_RELEASE_OLD_OWNERSHIP);
        CHECK(blu2usb_connection_coordinator_handoff_ack(
            coordinator, token, BLU2USB_HANDOFF_RELEASE_OLD_OWNERSHIP));
        CHECK(coordinator->handoff_phase == BLU2USB_HANDOFF_RETIRE_OLD_TRANSPORT);
        CHECK(blu2usb_connection_coordinator_handoff_ack(
            coordinator, token, BLU2USB_HANDOFF_RETIRE_OLD_TRANSPORT));
    }

    CHECK(coordinator->handoff_phase == BLU2USB_HANDOFF_PERSIST_CANDIDATE);
    CHECK(blu2usb_connection_coordinator_handoff_ack(
        coordinator, token, BLU2USB_HANDOFF_PERSIST_CANDIDATE));
    CHECK(coordinator->handoff_phase == BLU2USB_HANDOFF_PROMOTE_CANDIDATE);
    CHECK(blu2usb_connection_coordinator_handoff_ack(
        coordinator, token, BLU2USB_HANDOFF_PROMOTE_CANDIDATE));
}

static void test_new_search_preserves_current_and_handoff_order(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_connection_coordinator_t coordinator;
    uint64_t token = 0u;

    blu2usb_device_registry_init(&registry);
    seed_mouse(&registry, UINT64_C(1), "Current",
               BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP);
    seed_mouse(&registry, UINT64_C(2), "Saved Other",
               BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP);
    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(1)));
    CHECK(blu2usb_connection_coordinator_init(&coordinator, &registry, 0u));

    CHECK(blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_NEW, 0u, &token));

    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token, UINT64_C(2), "Saved Other") ==
        BLU2USB_CANDIDATE_REJECTED);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(1));

    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token, UINT64_C(3), "Replacement") ==
        BLU2USB_CANDIDATE_ACCEPTED);

    CHECK(coordinator.handoff_phase == BLU2USB_HANDOFF_FREEZE_OLD_INPUT);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(1));
    CHECK(blu2usb_device_registry_find(&registry, UINT64_C(3)) == NULL);

    CHECK(!blu2usb_connection_coordinator_cancel_search(&coordinator, token));
    CHECK(!blu2usb_connection_coordinator_handoff_ack(
        &coordinator, token, BLU2USB_HANDOFF_RELEASE_OLD_OWNERSHIP));
    CHECK(coordinator.handoff_phase == BLU2USB_HANDOFF_FREEZE_OLD_INPUT);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(1));

    CHECK(blu2usb_connection_coordinator_handoff_ack(
        &coordinator, token, BLU2USB_HANDOFF_FREEZE_OLD_INPUT));
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(1));

    CHECK(blu2usb_connection_coordinator_handoff_ack(
        &coordinator, token, BLU2USB_HANDOFF_RELEASE_OLD_OWNERSHIP));
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(1));

    CHECK(blu2usb_connection_coordinator_handoff_ack(
        &coordinator, token, BLU2USB_HANDOFF_RETIRE_OLD_TRANSPORT));
    CHECK(blu2usb_device_registry_authoritative(&registry) == NULL);
    CHECK(blu2usb_device_registry_find(&registry, UINT64_C(1)) != NULL);

    CHECK(blu2usb_connection_coordinator_handoff_ack(
        &coordinator, token, BLU2USB_HANDOFF_PERSIST_CANDIDATE));
    CHECK(blu2usb_device_registry_find(&registry, UINT64_C(3)) != NULL);
    CHECK(blu2usb_device_registry_authoritative(&registry) == NULL);
    CHECK(blu2usb_device_registry_find(&registry, UINT64_C(3))->profile ==
          BLU2USB_MOUSE_PROFILE_PASSTHROUGH);

    CHECK(blu2usb_connection_coordinator_handoff_ack(
        &coordinator, token, BLU2USB_HANDOFF_PROMOTE_CANDIDATE));
    CHECK(coordinator.search.status == BLU2USB_SEARCH_SUCCEEDED);
    CHECK(!blu2usb_connection_coordinator_busy(&coordinator));
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(3));
    CHECK(blu2usb_device_registry_find(&registry, UINT64_C(1)) != NULL);
    CHECK(blu2usb_device_registry_count(&registry) == 3u);
}

static void test_new_cancel_timeout_and_disconnect(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_connection_coordinator_t coordinator;
    uint64_t token = 0u;

    blu2usb_device_registry_init(&registry);
    seed_mouse(&registry, UINT64_C(1), "Current",
               BLU2USB_MOUSE_PROFILE_PASSTHROUGH);
    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(1)));
    CHECK(blu2usb_connection_coordinator_init(&coordinator, &registry, 0u));

    CHECK(blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_NEW, 0u, &token));
    CHECK(blu2usb_connection_coordinator_cancel_search(&coordinator, token));
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(1));

    CHECK(blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_NEW, 10u, &token));
    CHECK(blu2usb_connection_coordinator_advance_time(&coordinator, 15010u));
    CHECK(coordinator.search.status == BLU2USB_SEARCH_TIMED_OUT);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(1));
    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token, UINT64_C(2), "Late New") ==
        BLU2USB_CANDIDATE_STALE);

    CHECK(blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_NEW, 15020u, &token));
    CHECK(blu2usb_connection_coordinator_authoritative_disconnected(
        &coordinator, UINT64_C(1)));
    CHECK(blu2usb_device_registry_authoritative(&registry) == NULL);
    CHECK(blu2usb_connection_coordinator_resolve_home(&coordinator) ==
          BLU2USB_HOME_SAVED_SEARCH);

    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token, UINT64_C(2), "Replacement") ==
        BLU2USB_CANDIDATE_ACCEPTED);
    CHECK(coordinator.handoff_phase == BLU2USB_HANDOFF_PERSIST_CANDIDATE);
    CHECK(blu2usb_device_registry_authoritative(&registry) == NULL);

    complete_handoff(&coordinator, token, false);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(2));
}

static void test_registry_full_rejects_new_without_retiring_current(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_connection_coordinator_t coordinator;
    uint64_t token = 0u;

    blu2usb_device_registry_init(&registry);
    for (unsigned i = 0u; i < BLU2USB_SAVED_MOUSE_CAPACITY; ++i) {
        char name[24];
        (void)snprintf(name, sizeof(name), "Mouse %u", i + 1u);
        seed_mouse(&registry, (blu2usb_mouse_id_t)(i + 1u), name,
                   BLU2USB_MOUSE_PROFILE_PASSTHROUGH);
    }
    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(1)));
    CHECK(blu2usb_connection_coordinator_init(&coordinator, &registry, 0u));
    CHECK(blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_NEW, 0u, &token));

    CHECK(blu2usb_connection_coordinator_candidate_ready(
        &coordinator, token, UINT64_C(100), "Overflow Candidate") ==
        BLU2USB_CANDIDATE_REJECTED);
    CHECK(coordinator.handoff_phase == BLU2USB_HANDOFF_NONE);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(1));
    CHECK(blu2usb_device_registry_count(&registry) ==
          BLU2USB_SAVED_MOUSE_CAPACITY);

    CHECK(blu2usb_connection_coordinator_cancel_search(&coordinator, token));
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(1));
}

static void test_remove_transactions_and_operation_exclusivity(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_connection_coordinator_t coordinator;
    uint64_t token = 0u;
    uint64_t search_token = 0u;

    blu2usb_device_registry_init(&registry);
    seed_mouse(&registry, UINT64_C(1), "Current",
               BLU2USB_MOUSE_PROFILE_PASSTHROUGH);
    seed_mouse(&registry, UINT64_C(2), "Inactive",
               BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP);
    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(1)));
    CHECK(blu2usb_connection_coordinator_init(&coordinator, &registry, 0u));

    CHECK(blu2usb_connection_coordinator_begin_remove(
        &coordinator, UINT64_C(2), &token));
    CHECK(!blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_NEW, 0u, &search_token));
    CHECK(blu2usb_connection_coordinator_cancel_remove(&coordinator, token));
    CHECK(blu2usb_device_registry_find(&registry, UINT64_C(2)) != NULL);
    CHECK(!blu2usb_connection_coordinator_complete_remove(
        &coordinator, token, true));
    CHECK(coordinator.stale_result_count == 1u);

    CHECK(blu2usb_connection_coordinator_begin_remove(
        &coordinator, UINT64_C(2), &token));
    CHECK(blu2usb_connection_coordinator_complete_remove(
        &coordinator, token, true));
    CHECK(blu2usb_device_registry_find(&registry, UINT64_C(2)) == NULL);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(1));

    CHECK(blu2usb_connection_coordinator_begin_remove(
        &coordinator, UINT64_C(1), &token));
    CHECK(blu2usb_connection_coordinator_complete_remove(
        &coordinator, token, false));
    CHECK(blu2usb_device_registry_find(&registry, UINT64_C(1)) != NULL);
    CHECK(blu2usb_device_registry_authoritative(&registry)->id == UINT64_C(1));

    CHECK(blu2usb_connection_coordinator_begin_remove(
        &coordinator, UINT64_C(1), &token));
    CHECK(blu2usb_connection_coordinator_complete_remove(
        &coordinator, token, true));
    CHECK(blu2usb_device_registry_count(&registry) == 0u);
    CHECK(blu2usb_device_registry_authoritative(&registry) == NULL);
    CHECK(blu2usb_connection_coordinator_resolve_home(&coordinator) ==
          BLU2USB_HOME_FIRST_SEARCH);
}

static void test_remove_inactive_then_last_saved_without_current(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_connection_coordinator_t coordinator;
    uint64_t token = 0u;

    blu2usb_device_registry_init(&registry);
    seed_mouse(&registry, UINT64_C(1), "One",
               BLU2USB_MOUSE_PROFILE_PASSTHROUGH);
    seed_mouse(&registry, UINT64_C(2), "Two",
               BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP);
    CHECK(blu2usb_connection_coordinator_init(&coordinator, &registry, 0u));

    CHECK(blu2usb_connection_coordinator_begin_remove(
        &coordinator, UINT64_C(1), &token));
    CHECK(blu2usb_connection_coordinator_complete_remove(
        &coordinator, token, true));
    CHECK(blu2usb_device_registry_count(&registry) == 1u);
    CHECK(blu2usb_connection_coordinator_resolve_home(&coordinator) ==
          BLU2USB_HOME_SAVED_SEARCH);

    CHECK(blu2usb_connection_coordinator_begin_remove(
        &coordinator, UINT64_C(2), &token));
    CHECK(blu2usb_connection_coordinator_complete_remove(
        &coordinator, token, true));
    CHECK(blu2usb_connection_coordinator_resolve_home(&coordinator) ==
          BLU2USB_HOME_FIRST_SEARCH);
}

static void test_start_guards(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_connection_coordinator_t coordinator;
    uint64_t token = 0u;

    blu2usb_device_registry_init(&registry);
    CHECK(blu2usb_connection_coordinator_init(&coordinator, &registry, 100u));

    CHECK(!blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_SAVED, 100u, &token));
    CHECK(!blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_NEW, 100u, &token));
    CHECK(!blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_FIRST, 99u, &token));

    seed_mouse(&registry, UINT64_C(1), "Saved",
               BLU2USB_MOUSE_PROFILE_PASSTHROUGH);
    CHECK(!blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_FIRST, 100u, &token));

    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(1)));
    CHECK(!blu2usb_connection_coordinator_start_search(
        &coordinator, BLU2USB_SEARCH_SAVED, 100u, &token));
}

int main(void)
{
    test_home_resolver();
    test_first_cycles_and_stale_result();
    test_saved_search_eligibility_timeout_cancel();
    test_new_search_preserves_current_and_handoff_order();
    test_new_cancel_timeout_and_disconnect();
    test_registry_full_rejects_new_without_retiring_current();
    test_remove_transactions_and_operation_exclusivity();
    test_remove_inactive_then_last_saved_without_current();
    test_start_guards();

    if (failures != 0) {
        fprintf(stderr, "%d MUX-02 assertion(s) failed\n", failures);
        return 1;
    }

    puts("MUX-02 connection coordinator PASS");
    return 0;
}
