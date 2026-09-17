#include "blu2usb/classic_hid/classic_hid.h"

#include <string.h>

static bool bit_get(const uint8_t *bitmap, unsigned bit)
{
    return (bitmap[bit >> 3u] & (uint8_t)(1u << (bit & 7u))) != 0u;
}

static void bit_set(uint8_t *bitmap, unsigned bit, bool value)
{
    const uint8_t mask = (uint8_t)(1u << (bit & 7u));
    if (value) bitmap[bit >> 3u] |= mask;
    else bitmap[bit >> 3u] &= (uint8_t)~mask;
}

void blu2usb_classic_hid_keyboard_state_clear(
    blu2usb_classic_hid_keyboard_state_t *state)
{
    if (state != NULL) memset(state, 0, sizeof(*state));
}

bool blu2usb_classic_hid_keyboard_state_set_key(
    blu2usb_classic_hid_keyboard_state_t *state,
    blu2usb_key_t key,
    bool pressed)
{
    if (state == NULL) return false;
    bit_set(state->key_bitmap, (unsigned)key, pressed);
    return true;
}

bool blu2usb_classic_hid_keyboard_state_set_modifier(
    blu2usb_classic_hid_keyboard_state_t *state,
    blu2usb_modifier_t modifier,
    bool pressed)
{
    if (state == NULL || (unsigned)modifier >= BLU2USB_MOD_COUNT) return false;
    const uint8_t mask = (uint8_t)(1u << (unsigned)modifier);
    if (pressed) state->modifiers |= mask;
    else state->modifiers &= (uint8_t)~mask;
    return true;
}

bool blu2usb_classic_hid_emit_state_diff(
    blu2usb_classic_hid_keyboard_state_t *previous,
    const blu2usb_classic_hid_keyboard_state_t *next,
    blu2usb_hid_source_t source,
    blu2usb_classic_hid_emit_keyboard_fn emit,
    void *context)
{
    if (previous == NULL || next == NULL || emit == NULL ||
        !blu2usb_hid_source_is_valid(source)) return false;

    for (unsigned modifier = 0u; modifier < BLU2USB_MOD_COUNT; ++modifier) {
        const bool before = (previous->modifiers & (uint8_t)(1u << modifier)) != 0u;
        const bool after = (next->modifiers & (uint8_t)(1u << modifier)) != 0u;
        if (before == after) continue;
        blu2usb_canonical_keyboard_event_t event;
        memset(&event, 0, sizeof(event));
        event.source = source;
        event.type = BLU2USB_KEYBOARD_EVENT_MODIFIER;
        event.data.modifier.modifier = (blu2usb_modifier_t)modifier;
        event.data.modifier.pressed = after;
        if (!emit(context, &event)) return false;
    }

    for (unsigned key = 1u; key < BLU2USB_CLASSIC_HID_KEY_USAGE_COUNT; ++key) {
        const bool before = bit_get(previous->key_bitmap, key);
        const bool after = bit_get(next->key_bitmap, key);
        if (before == after) continue;
        blu2usb_canonical_keyboard_event_t event;
        memset(&event, 0, sizeof(event));
        event.source = source;
        event.type = BLU2USB_KEYBOARD_EVENT_KEY;
        event.data.key.key = (blu2usb_key_t)key;
        event.data.key.pressed = after;
        if (!emit(context, &event)) return false;
    }

    *previous = *next;
    return true;
}

bool blu2usb_classic_hid_decode_runtime_message(
    const blu2usb_bt_runtime_message_t *message,
    blu2usb_classic_hid_event_t *event)
{
    if (message == NULL || event == NULL ||
        message->channel != BLU2USB_CLASSIC_HID_RUNTIME_CHANNEL) return false;

    memset(event, 0, sizeof(*event));
    switch ((blu2usb_classic_hid_message_type_t)message->type) {
    case BLU2USB_CLASSIC_HID_MESSAGE_PROGRESS:
        if (message->length != sizeof(event->progress)) return false;
        memcpy(&event->progress, message->payload, sizeof(event->progress));
        if (event->progress.phase > BLU2USB_KEYBOARD_PAIR_ERROR ||
            event->progress.last_phase > BLU2USB_KEYBOARD_PAIR_ERROR) return false;
        event->type = BLU2USB_CLASSIC_HID_EVENT_PROGRESS;
        return true;
    case BLU2USB_CLASSIC_HID_MESSAGE_CONNECTED:
        if (message->length != 0u) return false;
        event->type = BLU2USB_CLASSIC_HID_EVENT_CONNECTED;
        return true;
    case BLU2USB_CLASSIC_HID_MESSAGE_DISCONNECTED:
        if (message->length != 0u) return false;
        event->type = BLU2USB_CLASSIC_HID_EVENT_DISCONNECTED;
        return true;
    case BLU2USB_CLASSIC_HID_MESSAGE_KEYBOARD:
        if (message->length != sizeof(event->keyboard)) return false;
        memcpy(&event->keyboard, message->payload, sizeof(event->keyboard));
        event->type = BLU2USB_CLASSIC_HID_EVENT_KEYBOARD;
        return true;
    case BLU2USB_CLASSIC_HID_MESSAGE_PAIR_CODE:
        if (message->length != sizeof(event->pair_code)) return false;
        memcpy(&event->pair_code, message->payload, sizeof(event->pair_code));
        if (event->pair_code.digits != 4u && event->pair_code.digits != 6u) return false;
        event->type = BLU2USB_CLASSIC_HID_EVENT_PAIR_CODE;
        return true;
    default:
        return false;
    }
}
