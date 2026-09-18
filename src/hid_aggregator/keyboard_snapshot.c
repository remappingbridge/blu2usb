#include "blu2usb/hid_aggregator/hid_aggregator.h"

void blu2usb_hid_aggregator_apply_keyboard_snapshot(blu2usb_hid_aggregator_t *aggregator,
                                                   const blu2usb_keyboard_snapshot_t *snapshot)
{
    if (!aggregator || !snapshot) return;
    const blu2usb_hid_source_t source = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_KEYBOARD, 1u);
    (void)blu2usb_hid_aggregator_release_source(aggregator, source);
    blu2usb_canonical_keyboard_event_t event = {.source = source};
    event.type = BLU2USB_KEYBOARD_EVENT_MODIFIER;
    event.data.modifier.pressed = true;
    for (unsigned i = 0; i < BLU2USB_MOD_COUNT; ++i) {
        if (!(snapshot->modifiers & (1u << i))) continue;
        event.data.modifier.modifier = (blu2usb_modifier_t)i;
        (void)blu2usb_hid_aggregator_apply_keyboard(aggregator, &event);
    }
    event.type = BLU2USB_KEYBOARD_EVENT_KEY;
    event.data.key.pressed = true;
    for (unsigned i = 0; i < 6u; ++i) {
        if (!snapshot->keys[i]) continue;
        event.data.key.key = snapshot->keys[i];
        (void)blu2usb_hid_aggregator_apply_keyboard(aggregator, &event);
    }
}
