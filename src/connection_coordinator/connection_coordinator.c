#include "blu2usb/connection_coordinator/connection_coordinator.h"

#include <limits.h>
#include <string.h>

static bool search_purpose_valid(blu2usb_search_purpose_t purpose)
{
    return purpose == BLU2USB_SEARCH_FIRST ||
           purpose == BLU2USB_SEARCH_SAVED ||
           purpose == BLU2USB_SEARCH_NEW;
}

static uint64_t search_duration_ms(blu2usb_search_purpose_t purpose)
{
    switch (purpose) {
    case BLU2USB_SEARCH_FIRST: return BLU2USB_SEARCH_FIRST_MS;
    case BLU2USB_SEARCH_SAVED: return BLU2USB_SEARCH_SAVED_MS;
    case BLU2USB_SEARCH_NEW: return BLU2USB_SEARCH_NEW_MS;
    default: return UINT64_C(0);
    }
}

static uint64_t deadline_from(uint64_t now_ms, uint64_t duration_ms)
{
    return UINT64_MAX - now_ms < duration_ms
        ? UINT64_MAX
        : now_ms + duration_ms;
}

static uint64_t next_token(blu2usb_connection_coordinator_t *coordinator)
{
    ++coordinator->next_token;
    if (coordinator->next_token == UINT64_C(0))
        ++coordinator->next_token;
    return coordinator->next_token;
}

static void clear_candidate(blu2usb_connection_coordinator_t *coordinator)
{
    coordinator->candidate_id = BLU2USB_MOUSE_ID_INVALID;
    coordinator->candidate_name[0] = '\0';
    coordinator->candidate_persisted = false;
    coordinator->handoff_phase = BLU2USB_HANDOFF_NONE;
}

static void clear_operation(blu2usb_connection_coordinator_t *coordinator)
{
    coordinator->operation.kind = BLU2USB_COORDINATOR_OPERATION_NONE;
    coordinator->operation.token = UINT64_C(0);
    coordinator->operation.mouse_id = BLU2USB_MOUSE_ID_INVALID;
}

static void close_search_operation(
    blu2usb_connection_coordinator_t *coordinator,
    blu2usb_search_status_t status)
{
    coordinator->search.status = status;
    coordinator->search.token = UINT64_C(0);
    clear_operation(coordinator);
}

static bool copy_candidate_name(
    char out[BLU2USB_MOUSE_NAME_CAPACITY],
    const char *name)
{
    size_t length = 0u;

    if (name == NULL) return false;
    while (name[length] != '\0' && length + 1u < BLU2USB_MOUSE_NAME_CAPACITY)
        ++length;
    if (name[length] != '\0') return false;

    memcpy(out, name, length + 1u);
    return true;
}

static bool token_matches_search(
    const blu2usb_connection_coordinator_t *coordinator,
    uint64_t token)
{
    return coordinator != NULL &&
           token != UINT64_C(0) &&
           coordinator->operation.kind == BLU2USB_COORDINATOR_OPERATION_SEARCH &&
           coordinator->operation.token == token &&
           coordinator->search.status == BLU2USB_SEARCH_RUNNING &&
           coordinator->search.token == token;
}

static void count_stale(blu2usb_connection_coordinator_t *coordinator)
{
    if (coordinator != NULL && coordinator->stale_result_count != UINT64_MAX)
        ++coordinator->stale_result_count;
}

bool blu2usb_connection_coordinator_init(
    blu2usb_connection_coordinator_t *coordinator,
    blu2usb_device_registry_t *registry,
    uint64_t now_ms)
{
    if (coordinator == NULL || registry == NULL ||
        !blu2usb_device_registry_validate(registry))
        return false;

    memset(coordinator, 0, sizeof(*coordinator));
    coordinator->registry = registry;
    coordinator->last_now_ms = now_ms;
    coordinator->candidate_id = BLU2USB_MOUSE_ID_INVALID;
    coordinator->operation.mouse_id = BLU2USB_MOUSE_ID_INVALID;
    return true;
}

blu2usb_home_resolution_t blu2usb_connection_coordinator_resolve_home(
    const blu2usb_connection_coordinator_t *coordinator)
{
    if (coordinator == NULL || coordinator->registry == NULL ||
        blu2usb_device_registry_count(coordinator->registry) == 0u)
        return BLU2USB_HOME_FIRST_SEARCH;

    if (blu2usb_device_registry_authoritative(coordinator->registry) != NULL)
        return BLU2USB_HOME_CONNECTED;

    return BLU2USB_HOME_SAVED_SEARCH;
}

bool blu2usb_connection_coordinator_busy(
    const blu2usb_connection_coordinator_t *coordinator)
{
    return coordinator != NULL &&
           coordinator->operation.kind != BLU2USB_COORDINATOR_OPERATION_NONE;
}

bool blu2usb_connection_coordinator_start_search(
    blu2usb_connection_coordinator_t *coordinator,
    blu2usb_search_purpose_t purpose,
    uint64_t now_ms,
    uint64_t *token_out)
{
    if (coordinator == NULL || coordinator->registry == NULL ||
        token_out == NULL || !search_purpose_valid(purpose) ||
        blu2usb_connection_coordinator_busy(coordinator) ||
        now_ms < coordinator->last_now_ms)
        return false;

    if (purpose == BLU2USB_SEARCH_FIRST &&
        blu2usb_device_registry_count(coordinator->registry) != 0u)
        return false;

    if (purpose == BLU2USB_SEARCH_SAVED &&
        (blu2usb_device_registry_count(coordinator->registry) == 0u ||
         blu2usb_device_registry_authoritative(coordinator->registry) != NULL))
        return false;

    if (purpose == BLU2USB_SEARCH_NEW &&
        blu2usb_device_registry_count(coordinator->registry) == 0u)
        return false;

    const uint64_t token = next_token(coordinator);
    const uint64_t duration = search_duration_ms(purpose);

    coordinator->last_now_ms = now_ms;
    coordinator->search.purpose = purpose;
    coordinator->search.status = BLU2USB_SEARCH_RUNNING;
    coordinator->search.token = token;
    coordinator->search.started_at_ms = now_ms;
    coordinator->search.deadline_at_ms = deadline_from(now_ms, duration);
    coordinator->operation.kind = BLU2USB_COORDINATOR_OPERATION_SEARCH;
    coordinator->operation.token = token;
    coordinator->operation.mouse_id = BLU2USB_MOUSE_ID_INVALID;
    clear_candidate(coordinator);

    *token_out = token;
    return true;
}

bool blu2usb_connection_coordinator_cancel_search(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t token)
{
    if (!token_matches_search(coordinator, token)) {
        count_stale(coordinator);
        return false;
    }

    if (coordinator->search.purpose == BLU2USB_SEARCH_FIRST ||
        coordinator->handoff_phase != BLU2USB_HANDOFF_NONE)
        return false;

    clear_candidate(coordinator);
    close_search_operation(coordinator, BLU2USB_SEARCH_CANCELLED);
    return true;
}

bool blu2usb_connection_coordinator_advance_time(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t now_ms)
{
    if (coordinator == NULL || now_ms < coordinator->last_now_ms)
        return false;

    coordinator->last_now_ms = now_ms;

    if (coordinator->operation.kind != BLU2USB_COORDINATOR_OPERATION_SEARCH ||
        coordinator->search.status != BLU2USB_SEARCH_RUNNING ||
        coordinator->handoff_phase != BLU2USB_HANDOFF_NONE ||
        now_ms < coordinator->search.deadline_at_ms)
        return true;

    if (coordinator->search.purpose == BLU2USB_SEARCH_FIRST) {
        const uint64_t token = next_token(coordinator);
        coordinator->search.token = token;
        coordinator->search.started_at_ms = now_ms;
        coordinator->search.deadline_at_ms =
            deadline_from(now_ms, BLU2USB_SEARCH_FIRST_MS);
        coordinator->operation.token = token;
        return true;
    }

    clear_candidate(coordinator);
    close_search_operation(coordinator, BLU2USB_SEARCH_TIMED_OUT);
    return true;
}

blu2usb_candidate_result_t blu2usb_connection_coordinator_candidate_ready(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t token,
    blu2usb_mouse_id_t id,
    const char *name)
{
    if (coordinator == NULL || !blu2usb_mouse_id_is_valid(id) || name == NULL)
        return BLU2USB_CANDIDATE_REJECTED;

    if (!token_matches_search(coordinator, token)) {
        count_stale(coordinator);
        return BLU2USB_CANDIDATE_STALE;
    }

    if (coordinator->handoff_phase != BLU2USB_HANDOFF_NONE)
        return BLU2USB_CANDIDATE_REJECTED;

    const bool saved =
        blu2usb_device_registry_find(coordinator->registry, id) != NULL;

    switch (coordinator->search.purpose) {
    case BLU2USB_SEARCH_FIRST:
        if (saved || blu2usb_device_registry_count(coordinator->registry) != 0u)
            return BLU2USB_CANDIDATE_REJECTED;
        if (!blu2usb_device_registry_add(
                coordinator->registry, id, name,
                BLU2USB_MOUSE_PROFILE_PASSTHROUGH) ||
            !blu2usb_device_registry_set_authoritative(coordinator->registry, id)) {
            close_search_operation(coordinator, BLU2USB_SEARCH_FAILED);
            return BLU2USB_CANDIDATE_REJECTED;
        }
        coordinator->search.status = BLU2USB_SEARCH_SUCCEEDED;
        coordinator->search.token = UINT64_C(0);
        clear_operation(coordinator);
        return BLU2USB_CANDIDATE_ACCEPTED;

    case BLU2USB_SEARCH_SAVED:
        if (!saved) return BLU2USB_CANDIDATE_REJECTED;
        if (!blu2usb_device_registry_set_authoritative(coordinator->registry, id)) {
            close_search_operation(coordinator, BLU2USB_SEARCH_FAILED);
            return BLU2USB_CANDIDATE_REJECTED;
        }
        coordinator->search.status = BLU2USB_SEARCH_SUCCEEDED;
        coordinator->search.token = UINT64_C(0);
        clear_operation(coordinator);
        return BLU2USB_CANDIDATE_ACCEPTED;

    case BLU2USB_SEARCH_NEW:
        if (saved ||
            blu2usb_device_registry_count(coordinator->registry) >=
                BLU2USB_SAVED_MOUSE_CAPACITY ||
            !copy_candidate_name(coordinator->candidate_name, name))
            return BLU2USB_CANDIDATE_REJECTED;

        coordinator->candidate_id = id;
        coordinator->operation.mouse_id = id;
        coordinator->handoff_phase =
            blu2usb_device_registry_authoritative(coordinator->registry) != NULL
            ? BLU2USB_HANDOFF_FREEZE_OLD_INPUT
            : BLU2USB_HANDOFF_PERSIST_CANDIDATE;
        return BLU2USB_CANDIDATE_ACCEPTED;

    default:
        return BLU2USB_CANDIDATE_REJECTED;
    }
}

bool blu2usb_connection_coordinator_authoritative_disconnected(
    blu2usb_connection_coordinator_t *coordinator,
    blu2usb_mouse_id_t id)
{
    if (coordinator == NULL || coordinator->registry == NULL ||
        !blu2usb_mouse_id_is_valid(id))
        return false;

    const blu2usb_saved_mouse_t *current =
        blu2usb_device_registry_authoritative(coordinator->registry);

    if (current == NULL || current->id != id)
        return false;

    blu2usb_device_registry_clear_authoritative(coordinator->registry);
    return true;
}

bool blu2usb_connection_coordinator_handoff_ack(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t token,
    blu2usb_handoff_phase_t completed_phase)
{
    if (!token_matches_search(coordinator, token)) {
        count_stale(coordinator);
        return false;
    }

    if (coordinator->search.purpose != BLU2USB_SEARCH_NEW ||
        coordinator->handoff_phase == BLU2USB_HANDOFF_NONE ||
        coordinator->handoff_phase != completed_phase ||
        !blu2usb_mouse_id_is_valid(coordinator->candidate_id))
        return false;

    switch (completed_phase) {
    case BLU2USB_HANDOFF_FREEZE_OLD_INPUT:
        coordinator->handoff_phase = BLU2USB_HANDOFF_RELEASE_OLD_OWNERSHIP;
        return true;

    case BLU2USB_HANDOFF_RELEASE_OLD_OWNERSHIP:
        coordinator->handoff_phase = BLU2USB_HANDOFF_RETIRE_OLD_TRANSPORT;
        return true;

    case BLU2USB_HANDOFF_RETIRE_OLD_TRANSPORT:
        blu2usb_device_registry_clear_authoritative(coordinator->registry);
        coordinator->handoff_phase = BLU2USB_HANDOFF_PERSIST_CANDIDATE;
        return true;

    case BLU2USB_HANDOFF_PERSIST_CANDIDATE:
        if (!blu2usb_device_registry_add(
                coordinator->registry,
                coordinator->candidate_id,
                coordinator->candidate_name,
                BLU2USB_MOUSE_PROFILE_PASSTHROUGH)) {
            clear_candidate(coordinator);
            close_search_operation(coordinator, BLU2USB_SEARCH_FAILED);
            return false;
        }
        coordinator->candidate_persisted = true;
        coordinator->handoff_phase = BLU2USB_HANDOFF_PROMOTE_CANDIDATE;
        return true;

    case BLU2USB_HANDOFF_PROMOTE_CANDIDATE:
        if (!coordinator->candidate_persisted ||
            !blu2usb_device_registry_set_authoritative(
                coordinator->registry, coordinator->candidate_id)) {
            if (coordinator->candidate_persisted)
                (void)blu2usb_device_registry_remove(
                    coordinator->registry, coordinator->candidate_id);
            clear_candidate(coordinator);
            close_search_operation(coordinator, BLU2USB_SEARCH_FAILED);
            return false;
        }
        coordinator->search.status = BLU2USB_SEARCH_SUCCEEDED;
        coordinator->search.token = UINT64_C(0);
        clear_candidate(coordinator);
        clear_operation(coordinator);
        return true;

    default:
        return false;
    }
}

bool blu2usb_connection_coordinator_begin_remove(
    blu2usb_connection_coordinator_t *coordinator,
    blu2usb_mouse_id_t id,
    uint64_t *token_out)
{
    if (coordinator == NULL || coordinator->registry == NULL ||
        token_out == NULL || blu2usb_connection_coordinator_busy(coordinator) ||
        blu2usb_device_registry_find(coordinator->registry, id) == NULL)
        return false;

    const uint64_t token = next_token(coordinator);
    coordinator->operation.kind = BLU2USB_COORDINATOR_OPERATION_REMOVE;
    coordinator->operation.token = token;
    coordinator->operation.mouse_id = id;
    *token_out = token;
    return true;
}

bool blu2usb_connection_coordinator_complete_remove(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t token,
    bool success)
{
    if (coordinator == NULL ||
        coordinator->operation.kind != BLU2USB_COORDINATOR_OPERATION_REMOVE ||
        coordinator->operation.token != token ||
        token == UINT64_C(0)) {
        count_stale(coordinator);
        return false;
    }

    const blu2usb_mouse_id_t target = coordinator->operation.mouse_id;
    clear_operation(coordinator);

    if (!success) return true;
    return blu2usb_device_registry_remove(coordinator->registry, target);
}

bool blu2usb_connection_coordinator_cancel_remove(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t token)
{
    if (coordinator == NULL ||
        coordinator->operation.kind != BLU2USB_COORDINATOR_OPERATION_REMOVE ||
        coordinator->operation.token != token ||
        token == UINT64_C(0)) {
        count_stale(coordinator);
        return false;
    }

    clear_operation(coordinator);
    return true;
}
