#ifndef BLU2USB_BLE_HOGP_SESSION_ROLES_H
#define BLU2USB_BLE_HOGP_SESSION_ROLES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLU2USB_BLE_HOGP_SESSION_SLOT_COUNT 2u
#define BLU2USB_BLE_HOGP_SESSION_SLOT_NONE UINT8_C(0xff)

typedef enum {
    BLU2USB_BLE_HOGP_ROLE_EMPTY = 0,
    BLU2USB_BLE_HOGP_ROLE_AUTHORITATIVE,
    BLU2USB_BLE_HOGP_ROLE_PROVISIONAL,
    BLU2USB_BLE_HOGP_ROLE_RETIRING,
} blu2usb_ble_hogp_session_role_t;

typedef struct {
    blu2usb_ble_hogp_session_role_t roles[BLU2USB_BLE_HOGP_SESSION_SLOT_COUNT];
    bool ready[BLU2USB_BLE_HOGP_SESSION_SLOT_COUNT];
    uint8_t authoritative_slot;
    uint8_t provisional_slot;
    uint32_t next_generation;
    uint32_t active_generation;
    bool new_active;
    bool commit_pending;
} blu2usb_ble_hogp_session_roles_t;

void blu2usb_ble_hogp_session_roles_init(
    blu2usb_ble_hogp_session_roles_t *roles);

bool blu2usb_ble_hogp_session_set_authoritative(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint8_t slot,
    bool ready);

bool blu2usb_ble_hogp_session_start_new(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint8_t *slot_out,
    uint32_t *generation_out);

bool blu2usb_ble_hogp_session_candidate_ready(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint8_t slot,
    uint32_t generation);

bool blu2usb_ble_hogp_session_cancel_new(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint32_t generation,
    uint8_t *slot_out);

bool blu2usb_ble_hogp_session_begin_commit(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint32_t generation,
    uint8_t *retiring_slot_out);

bool blu2usb_ble_hogp_session_retired(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint8_t slot,
    uint32_t generation,
    uint8_t *promoted_slot_out);

bool blu2usb_ble_hogp_session_disconnected(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint8_t slot);

bool blu2usb_ble_hogp_session_can_forward(
    const blu2usb_ble_hogp_session_roles_t *roles,
    uint8_t slot);

#ifdef __cplusplus
}
#endif

#endif
