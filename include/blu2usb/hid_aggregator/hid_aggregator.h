#ifndef BLU2USB_HID_AGGREGATOR_HID_AGGREGATOR_H
#define BLU2USB_HID_AGGREGATOR_HID_AGGREGATOR_H

#include <stdbool.h>
#include <stdint.h>

#include "blu2usb/domain/hid.h"
#include "blu2usb/domain/keyboard.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLU2USB_HID_AGGREGATOR_MAX_SOURCES 16u
#define BLU2USB_HID_KEY_COUNT 256u
#define BLU2USB_HID_KEY_BITMAP_BYTES (BLU2USB_HID_KEY_COUNT / 8u)

typedef struct {
    bool active;
    blu2usb_hid_source_t id;
    uint8_t mouse_buttons;
    uint8_t key_bitmap[BLU2USB_HID_KEY_BITMAP_BYTES];
    uint8_t modifiers;
} blu2usb_hid_source_state_t;

typedef struct {
    blu2usb_hid_source_state_t sources[BLU2USB_HID_AGGREGATOR_MAX_SOURCES];
    uint8_t mouse_button_refs[BLU2USB_MOUSE_BUTTON_COUNT];
    uint8_t key_refs[BLU2USB_HID_KEY_COUNT];
    uint8_t modifier_refs[BLU2USB_MOD_COUNT];
    int32_t pending_dx;
    int32_t pending_dy;
    int32_t pending_wheel_vertical;
    int32_t pending_wheel_horizontal;
} blu2usb_hid_aggregator_t;

typedef struct {
    uint8_t mouse_buttons;
    int32_t dx;
    int32_t dy;
    int32_t wheel_vertical;
    int32_t wheel_horizontal;
    uint8_t key_bitmap[BLU2USB_HID_KEY_BITMAP_BYTES];
    uint8_t modifiers;
} blu2usb_hid_output_state_t;

void blu2usb_hid_aggregator_init(blu2usb_hid_aggregator_t *aggregator);
bool blu2usb_hid_aggregator_apply_mouse(blu2usb_hid_aggregator_t *aggregator, const blu2usb_canonical_mouse_event_t *event);
bool blu2usb_hid_aggregator_apply_keyboard(blu2usb_hid_aggregator_t *aggregator, const blu2usb_canonical_keyboard_event_t *event);
bool blu2usb_hid_aggregator_release_source(blu2usb_hid_aggregator_t *aggregator, blu2usb_hid_source_t source);
void blu2usb_hid_aggregator_snapshot(const blu2usb_hid_aggregator_t *aggregator, blu2usb_hid_output_state_t *out_state);
void blu2usb_hid_aggregator_apply_keyboard_snapshot(blu2usb_hid_aggregator_t *aggregator,
                                                   const blu2usb_keyboard_snapshot_t *snapshot);

/* Consume only relative deltas that were actually accepted by USB. Each
 * component must have the same sign as, and not exceed, its pending value. */
bool blu2usb_hid_aggregator_consume_relative(blu2usb_hid_aggregator_t *aggregator,
                                              int32_t dx,
                                              int32_t dy,
                                              int32_t wheel_vertical,
                                              int32_t wheel_horizontal);

void blu2usb_hid_aggregator_take_output(blu2usb_hid_aggregator_t *aggregator, blu2usb_hid_output_state_t *out_state);
bool blu2usb_hid_output_mouse_button_is_down(const blu2usb_hid_output_state_t *state, blu2usb_mouse_button_t button);
bool blu2usb_hid_output_key_is_down(const blu2usb_hid_output_state_t *state, blu2usb_key_t key);

#ifdef __cplusplus
}
#endif

#endif
