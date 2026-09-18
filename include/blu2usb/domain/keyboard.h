#ifndef BLU2USB_DOMAIN_KEYBOARD_H
#define BLU2USB_DOMAIN_KEYBOARD_H
#include "blu2usb/domain/hid.h"

// Firmware-owned logical keyboard snapshot, never a remote report layout.
typedef struct {
    uint8_t modifiers;
    blu2usb_key_t keys[6];
} blu2usb_keyboard_snapshot_t;

typedef enum {
    BLU2USB_KEYBOARD_IDLE = 0,
    BLU2USB_KEYBOARD_WAITING,
    BLU2USB_KEYBOARD_SEARCHING,
    BLU2USB_KEYBOARD_READING_NAME,
    BLU2USB_KEYBOARD_PAIRING,
    BLU2USB_KEYBOARD_CONNECTING,
    BLU2USB_KEYBOARD_READY,
    BLU2USB_KEYBOARD_ERROR,
    BLU2USB_KEYBOARD_STOPPING,
} blu2usb_keyboard_status_t;
#endif
