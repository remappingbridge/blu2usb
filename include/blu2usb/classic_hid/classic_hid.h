#ifndef BLU2USB_CLASSIC_HID_H
#define BLU2USB_CLASSIC_HID_H
#include <stddef.h>
#include "blu2usb/domain/keyboard.h"
#define BLU2USB_KEYBOARD_RUNTIME_CHANNEL 2u
#define BLU2USB_KEYBOARD_MESSAGE_SNAPSHOT 1u
#define BLU2USB_KEYBOARD_MESSAGE_RELEASE 2u

// Called only during radio runtime setup. No lifecycle ownership here.
void blu2usb_classic_hid_setup(void);
// Atomic mailbox APIs, safe from the application core.
void blu2usb_classic_hid_request_pair(void);
void blu2usb_classic_hid_cancel_pair(void);
blu2usb_keyboard_status_t blu2usb_classic_hid_status(void);
// Adapter parser: true only for a valid keyboard-usage input report.
bool blu2usb_classic_hid_parse(const uint8_t *descriptor, uint16_t descriptor_len,
    const uint8_t *report, uint16_t report_len, blu2usb_keyboard_snapshot_t *snapshot);
#endif
