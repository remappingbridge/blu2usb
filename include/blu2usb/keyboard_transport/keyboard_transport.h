#ifndef BLU2USB_KEYBOARD_TRANSPORT_KEYBOARD_TRANSPORT_H
#define BLU2USB_KEYBOARD_TRANSPORT_KEYBOARD_TRANSPORT_H

#include <stdbool.h>
#include <stdint.h>

#include "blu2usb/bt_runtime/bt_runtime.h"
#include "blu2usb/domain/hid.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLU2USB_KEYBOARD_TRANSPORT_EVENT_CONNECTED = 0,
    BLU2USB_KEYBOARD_TRANSPORT_EVENT_DISCONNECTED,
    BLU2USB_KEYBOARD_TRANSPORT_EVENT_KEYBOARD,
    BLU2USB_KEYBOARD_TRANSPORT_EVENT_PAIR_CODE,
} blu2usb_keyboard_transport_event_type_t;

typedef struct {
    uint32_t value;
    uint8_t digits;
} blu2usb_keyboard_transport_pair_code_t;

typedef struct {
    blu2usb_keyboard_transport_event_type_t type;
    blu2usb_canonical_keyboard_event_t keyboard;
    blu2usb_keyboard_transport_pair_code_t pair_code;
} blu2usb_keyboard_transport_event_t;

bool blu2usb_keyboard_transport_decode_runtime_message(
    const blu2usb_bt_runtime_message_t *message,
    blu2usb_keyboard_transport_event_t *event);

/* Pico facade. The UX requests a logical Keyboard; the enabled adapter is
 * currently Classic HID for the physically proven BKB-3G path. */
bool blu2usb_keyboard_transport_pico_start(void);
bool blu2usb_keyboard_transport_pico_pair(void);
bool blu2usb_keyboard_transport_pico_retry(void);
bool blu2usb_keyboard_transport_pico_cancel(void);

#ifdef __cplusplus
}
#endif

#endif
