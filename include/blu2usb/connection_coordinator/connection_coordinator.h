#ifndef BLU2USB_CONNECTION_COORDINATOR_CONNECTION_COORDINATOR_H
#define BLU2USB_CONNECTION_COORDINATOR_CONNECTION_COORDINATOR_H

#include <stdbool.h>
#include <stdint.h>

#include "blu2usb/device_registry/device_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLU2USB_SEARCH_FIRST_MS UINT64_C(8000)
#define BLU2USB_SEARCH_SAVED_MS UINT64_C(8000)
#define BLU2USB_SEARCH_NEW_MS UINT64_C(15000)

typedef enum {
    BLU2USB_SEARCH_NONE = 0,
    BLU2USB_SEARCH_FIRST,
    BLU2USB_SEARCH_SAVED,
    BLU2USB_SEARCH_NEW,
} blu2usb_search_purpose_t;

typedef enum {
    BLU2USB_SEARCH_IDLE = 0,
    BLU2USB_SEARCH_RUNNING,
    BLU2USB_SEARCH_SUCCEEDED,
    BLU2USB_SEARCH_TIMED_OUT,
    BLU2USB_SEARCH_CANCELLED,
    BLU2USB_SEARCH_FAILED,
} blu2usb_search_status_t;

typedef enum {
    BLU2USB_COORDINATOR_OPERATION_NONE = 0,
    BLU2USB_COORDINATOR_OPERATION_SEARCH,
    BLU2USB_COORDINATOR_OPERATION_REMOVE,
} blu2usb_coordinator_operation_kind_t;

typedef enum {
    BLU2USB_HOME_FIRST_SEARCH = 0,
    BLU2USB_HOME_CONNECTED,
    BLU2USB_HOME_SAVED_SEARCH,
} blu2usb_home_resolution_t;

typedef enum {
    BLU2USB_CANDIDATE_REJECTED = 0,
    BLU2USB_CANDIDATE_ACCEPTED,
    BLU2USB_CANDIDATE_STALE,
} blu2usb_candidate_result_t;

typedef enum {
    BLU2USB_HANDOFF_NONE = 0,
    BLU2USB_HANDOFF_FREEZE_OLD_INPUT,
    BLU2USB_HANDOFF_RELEASE_OLD_OWNERSHIP,
    BLU2USB_HANDOFF_RETIRE_OLD_TRANSPORT,
    BLU2USB_HANDOFF_PERSIST_CANDIDATE,
    BLU2USB_HANDOFF_PROMOTE_CANDIDATE,
} blu2usb_handoff_phase_t;

typedef struct {
    blu2usb_search_purpose_t purpose;
    blu2usb_search_status_t status;
    uint64_t token;
    uint64_t started_at_ms;
    uint64_t deadline_at_ms;
} blu2usb_search_state_t;

typedef struct {
    blu2usb_coordinator_operation_kind_t kind;
    uint64_t token;
    blu2usb_mouse_id_t mouse_id;
} blu2usb_operation_state_t;

typedef struct {
    blu2usb_device_registry_t *registry;
    uint64_t last_now_ms;
    uint64_t next_token;
    uint64_t stale_result_count;
    blu2usb_search_state_t search;
    blu2usb_operation_state_t operation;
    blu2usb_handoff_phase_t handoff_phase;
    blu2usb_mouse_id_t candidate_id;
    char candidate_name[BLU2USB_MOUSE_NAME_CAPACITY];
    bool candidate_persisted;
} blu2usb_connection_coordinator_t;

bool blu2usb_connection_coordinator_init(
    blu2usb_connection_coordinator_t *coordinator,
    blu2usb_device_registry_t *registry,
    uint64_t now_ms);

blu2usb_home_resolution_t blu2usb_connection_coordinator_resolve_home(
    const blu2usb_connection_coordinator_t *coordinator);

bool blu2usb_connection_coordinator_start_search(
    blu2usb_connection_coordinator_t *coordinator,
    blu2usb_search_purpose_t purpose,
    uint64_t now_ms,
    uint64_t *token_out);

bool blu2usb_connection_coordinator_cancel_search(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t token);

bool blu2usb_connection_coordinator_advance_time(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t now_ms);

blu2usb_candidate_result_t blu2usb_connection_coordinator_candidate_ready(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t token,
    blu2usb_mouse_id_t id,
    const char *name);

bool blu2usb_connection_coordinator_authoritative_disconnected(
    blu2usb_connection_coordinator_t *coordinator,
    blu2usb_mouse_id_t id);

bool blu2usb_connection_coordinator_handoff_ack(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t token,
    blu2usb_handoff_phase_t completed_phase);

bool blu2usb_connection_coordinator_begin_remove(
    blu2usb_connection_coordinator_t *coordinator,
    blu2usb_mouse_id_t id,
    uint64_t *token_out);

bool blu2usb_connection_coordinator_complete_remove(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t token,
    bool success);

bool blu2usb_connection_coordinator_cancel_remove(
    blu2usb_connection_coordinator_t *coordinator,
    uint64_t token);

bool blu2usb_connection_coordinator_busy(
    const blu2usb_connection_coordinator_t *coordinator);

#ifdef __cplusplus
}
#endif

#endif
