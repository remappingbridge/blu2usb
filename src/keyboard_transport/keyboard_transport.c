#include "blu2usb/keyboard_transport/keyboard_transport.h"

#include <string.h>

#include "blu2usb/classic_hid/classic_hid.h"

bool blu2usb_keyboard_transport_decode_runtime_message(
    const blu2usb_bt_runtime_message_t *message,
    blu2usb_keyboard_transport_event_t *event)
{
    if (message == NULL || event == NULL) return false;

    blu2usb_classic_hid_event_t classic_event;
    if (!blu2usb_classic_hid_decode_runtime_message(message, &classic_event))
        return false;

    memset(event, 0, sizeof(*event));
    switch (classic_event.type) {
    case BLU2USB_CLASSIC_HID_EVENT_CONNECTED:
        event->type = BLU2USB_KEYBOARD_TRANSPORT_EVENT_CONNECTED;
        return true;
    case BLU2USB_CLASSIC_HID_EVENT_DISCONNECTED:
        event->type = BLU2USB_KEYBOARD_TRANSPORT_EVENT_DISCONNECTED;
        return true;
    case BLU2USB_CLASSIC_HID_EVENT_KEYBOARD:
        event->type = BLU2USB_KEYBOARD_TRANSPORT_EVENT_KEYBOARD;
        event->keyboard = classic_event.keyboard;
        return true;
    case BLU2USB_CLASSIC_HID_EVENT_PAIR_CODE:
        event->type = BLU2USB_KEYBOARD_TRANSPORT_EVENT_PAIR_CODE;
        event->pair_code.value = classic_event.pair_code.value;
        event->pair_code.digits = classic_event.pair_code.digits;
        return true;
    default:
        return false;
    }
}
