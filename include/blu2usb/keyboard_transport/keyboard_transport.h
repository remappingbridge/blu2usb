#ifndef BLU2USB_KEYBOARD_TRANSPORT_H
#define BLU2USB_KEYBOARD_TRANSPORT_H
#include "blu2usb/domain/keyboard.h"
#include "blu2usb/bt_runtime/bt_runtime.h"
void blu2usb_keyboard_transport_setup(void);
void blu2usb_keyboard_transport_pair(void);
void blu2usb_keyboard_transport_cancel(void);
blu2usb_keyboard_status_t blu2usb_keyboard_transport_status(void);
bool blu2usb_keyboard_transport_decode(const blu2usb_bt_runtime_message_t *message,
    blu2usb_keyboard_snapshot_t *snapshot);
#endif
