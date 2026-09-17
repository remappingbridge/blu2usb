#ifndef BLU2USB_CLASSIC_HID_CLASSIC_HID_H
#define BLU2USB_CLASSIC_HID_CLASSIC_HID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blu2usb/bt_runtime/bt_runtime.h"
#include "blu2usb/domain/hid.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLU2USB_CLASSIC_HID_RUNTIME_CHANNEL UINT16_C(0x0701)
#define BLU2USB_CLASSIC_HID_KEY_USAGE_COUNT 256u
#define BLU2USB_CLASSIC_HID_KEY_BITMAP_BYTES (BLU2USB_CLASSIC_HID_KEY_USAGE_COUNT / 8u)

typedef enum {
    BLU2USB_CLASSIC_HID_MESSAGE_CONNECTED = 1,
    BLU2USB_CLASSIC_HID_MESSAGE_DISCONNECTED = 2,
    BLU2USB_CLASSIC_HID_MESSAGE_KEYBOARD = 3,
    BLU2USB_CLASSIC_HID_MESSAGE_PAIR_CODE = 4,
} blu2usb_classic_hid_message_type_t;

typedef enum {
    BLU2USB_CLASSIC_HID_EVENT_CONNECTED = 0,
    BLU2USB_CLASSIC_HID_EVENT_DISCONNECTED,
    BLU2USB_CLASSIC_HID_EVENT_KEYBOARD,
    BLU2USB_CLASSIC_HID_EVENT_PAIR_CODE,
} blu2usb_classic_hid_event_type_t;

typedef struct {
    uint32_t value;
    uint8_t digits;
} blu2usb_classic_hid_pair_code_t;

typedef struct {
    blu2usb_classic_hid_event_type_t type;
    blu2usb_canonical_keyboard_event_t keyboard;
    blu2usb_classic_hid_pair_code_t pair_code;
} blu2usb_classic_hid_event_t;

typedef struct {
    uint8_t key_bitmap[BLU2USB_CLASSIC_HID_KEY_BITMAP_BYTES];
    uint8_t modifiers;
} blu2usb_classic_hid_keyboard_state_t;

typedef bool (*blu2usb_classic_hid_emit_keyboard_fn)(
    void *context, const blu2usb_canonical_keyboard_event_t *event);

void blu2usb_classic_hid_keyboard_state_clear(
    blu2usb_classic_hid_keyboard_state_t *state);
bool blu2usb_classic_hid_keyboard_state_set_key(
    blu2usb_classic_hid_keyboard_state_t *state,
    blu2usb_key_t key,
    bool pressed);
bool blu2usb_classic_hid_keyboard_state_set_modifier(
    blu2usb_classic_hid_keyboard_state_t *state,
    blu2usb_modifier_t modifier,
    bool pressed);
bool blu2usb_classic_hid_emit_state_diff(
    blu2usb_classic_hid_keyboard_state_t *previous,
    const blu2usb_classic_hid_keyboard_state_t *next,
    blu2usb_hid_source_t source,
    blu2usb_classic_hid_emit_keyboard_fn emit,
    void *context);
bool blu2usb_classic_hid_decode_runtime_message(
    const blu2usb_bt_runtime_message_t *message,
    blu2usb_classic_hid_event_t *event);

/* Pico transport hooks. Register before blu2usb_bt_runtime_start() is called. */
bool blu2usb_classic_hid_pico_register(void);
bool blu2usb_classic_hid_pico_pair_keyboard(void);
bool blu2usb_classic_hid_pico_retry_keyboard(void);
bool blu2usb_classic_hid_pico_cancel_pairing(void);

#ifdef __cplusplus
}
#endif

#endif
