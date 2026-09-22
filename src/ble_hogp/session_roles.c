#include "blu2usb/ble_hogp/session_roles.h"

#include <string.h>

static bool slot_valid(uint8_t slot)
{
    return slot < BLU2USB_BLE_HOGP_SESSION_SLOT_COUNT;
}

static uint8_t free_slot(const blu2usb_ble_hogp_session_roles_t *roles)
{
    for (uint8_t slot = 0u; slot < BLU2USB_BLE_HOGP_SESSION_SLOT_COUNT; ++slot)
        if (roles->roles[slot] == BLU2USB_BLE_HOGP_ROLE_EMPTY)
            return slot;
    return BLU2USB_BLE_HOGP_SESSION_SLOT_NONE;
}

void blu2usb_ble_hogp_session_roles_init(
    blu2usb_ble_hogp_session_roles_t *roles)
{
    if (roles == NULL) return;
    memset(roles, 0, sizeof(*roles));
    roles->authoritative_slot = BLU2USB_BLE_HOGP_SESSION_SLOT_NONE;
    roles->provisional_slot = BLU2USB_BLE_HOGP_SESSION_SLOT_NONE;
}

bool blu2usb_ble_hogp_session_set_authoritative(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint8_t slot,
    bool ready)
{
    if (roles == NULL || !slot_valid(slot) ||
        roles->authoritative_slot != BLU2USB_BLE_HOGP_SESSION_SLOT_NONE ||
        roles->roles[slot] != BLU2USB_BLE_HOGP_ROLE_EMPTY)
        return false;

    roles->roles[slot] = BLU2USB_BLE_HOGP_ROLE_AUTHORITATIVE;
    roles->ready[slot] = ready;
    roles->authoritative_slot = slot;
    return true;
}

bool blu2usb_ble_hogp_session_start_new(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint8_t *slot_out,
    uint32_t *generation_out)
{
    if (roles == NULL || slot_out == NULL || generation_out == NULL ||
        roles->new_active || roles->commit_pending ||
        roles->authoritative_slot == BLU2USB_BLE_HOGP_SESSION_SLOT_NONE ||
        !roles->ready[roles->authoritative_slot])
        return false;

    const uint8_t slot = free_slot(roles);
    if (!slot_valid(slot)) return false;

    ++roles->next_generation;
    if (roles->next_generation == 0u) ++roles->next_generation;

    roles->active_generation = roles->next_generation;
    roles->new_active = true;
    roles->provisional_slot = slot;
    roles->roles[slot] = BLU2USB_BLE_HOGP_ROLE_PROVISIONAL;
    roles->ready[slot] = false;

    *slot_out = slot;
    *generation_out = roles->active_generation;
    return true;
}

bool blu2usb_ble_hogp_session_candidate_ready(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint8_t slot,
    uint32_t generation)
{
    if (roles == NULL || !slot_valid(slot) ||
        !roles->new_active || roles->commit_pending ||
        generation == 0u || generation != roles->active_generation ||
        slot != roles->provisional_slot ||
        roles->roles[slot] != BLU2USB_BLE_HOGP_ROLE_PROVISIONAL)
        return false;

    roles->ready[slot] = true;
    return true;
}

bool blu2usb_ble_hogp_session_cancel_new(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint32_t generation,
    uint8_t *slot_out)
{
    if (roles == NULL || generation == 0u ||
        !roles->new_active || roles->commit_pending ||
        generation != roles->active_generation ||
        !slot_valid(roles->provisional_slot))
        return false;

    const uint8_t slot = roles->provisional_slot;
    roles->roles[slot] = BLU2USB_BLE_HOGP_ROLE_EMPTY;
    roles->ready[slot] = false;
    roles->provisional_slot = BLU2USB_BLE_HOGP_SESSION_SLOT_NONE;
    roles->new_active = false;
    roles->active_generation = 0u;

    if (slot_out != NULL) *slot_out = slot;
    return true;
}

bool blu2usb_ble_hogp_session_begin_commit(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint32_t generation,
    uint8_t *retiring_slot_out)
{
    if (roles == NULL || generation == 0u ||
        !roles->new_active || roles->commit_pending ||
        generation != roles->active_generation ||
        !slot_valid(roles->authoritative_slot) ||
        !slot_valid(roles->provisional_slot) ||
        !roles->ready[roles->provisional_slot] ||
        roles->roles[roles->authoritative_slot] !=
            BLU2USB_BLE_HOGP_ROLE_AUTHORITATIVE)
        return false;

    const uint8_t old_slot = roles->authoritative_slot;
    roles->roles[old_slot] = BLU2USB_BLE_HOGP_ROLE_RETIRING;
    roles->ready[old_slot] = false;
    roles->authoritative_slot = BLU2USB_BLE_HOGP_SESSION_SLOT_NONE;
    roles->commit_pending = true;

    if (retiring_slot_out != NULL) *retiring_slot_out = old_slot;
    return true;
}

bool blu2usb_ble_hogp_session_retired(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint8_t slot,
    uint32_t generation,
    uint8_t *promoted_slot_out)
{
    if (roles == NULL || !slot_valid(slot) || generation == 0u ||
        !roles->new_active || !roles->commit_pending ||
        generation != roles->active_generation ||
        roles->roles[slot] != BLU2USB_BLE_HOGP_ROLE_RETIRING ||
        !slot_valid(roles->provisional_slot) ||
        !roles->ready[roles->provisional_slot])
        return false;

    roles->roles[slot] = BLU2USB_BLE_HOGP_ROLE_EMPTY;
    roles->ready[slot] = false;

    const uint8_t promoted = roles->provisional_slot;
    roles->roles[promoted] = BLU2USB_BLE_HOGP_ROLE_AUTHORITATIVE;
    roles->authoritative_slot = promoted;
    roles->provisional_slot = BLU2USB_BLE_HOGP_SESSION_SLOT_NONE;
    roles->new_active = false;
    roles->commit_pending = false;
    roles->active_generation = 0u;

    if (promoted_slot_out != NULL) *promoted_slot_out = promoted;
    return true;
}

bool blu2usb_ble_hogp_session_disconnected(
    blu2usb_ble_hogp_session_roles_t *roles,
    uint8_t slot)
{
    if (roles == NULL || !slot_valid(slot)) return false;

    if (roles->roles[slot] == BLU2USB_BLE_HOGP_ROLE_AUTHORITATIVE) {
        roles->roles[slot] = BLU2USB_BLE_HOGP_ROLE_EMPTY;
        roles->ready[slot] = false;
        if (roles->authoritative_slot == slot)
            roles->authoritative_slot = BLU2USB_BLE_HOGP_SESSION_SLOT_NONE;
        return true;
    }

    if (roles->roles[slot] == BLU2USB_BLE_HOGP_ROLE_PROVISIONAL) {
        roles->roles[slot] = BLU2USB_BLE_HOGP_ROLE_EMPTY;
        roles->ready[slot] = false;
        if (roles->provisional_slot == slot)
            roles->provisional_slot = BLU2USB_BLE_HOGP_SESSION_SLOT_NONE;
        roles->new_active = false;
        roles->active_generation = 0u;
        return true;
    }

    if (roles->roles[slot] == BLU2USB_BLE_HOGP_ROLE_RETIRING) {
        roles->roles[slot] = BLU2USB_BLE_HOGP_ROLE_EMPTY;
        roles->ready[slot] = false;
        return true;
    }

    return false;
}

bool blu2usb_ble_hogp_session_can_forward(
    const blu2usb_ble_hogp_session_roles_t *roles,
    uint8_t slot)
{
    return roles != NULL && slot_valid(slot) &&
           roles->authoritative_slot == slot &&
           roles->roles[slot] == BLU2USB_BLE_HOGP_ROLE_AUTHORITATIVE &&
           roles->ready[slot];
}
