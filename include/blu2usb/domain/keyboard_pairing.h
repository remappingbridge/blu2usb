#ifndef BLU2USB_DOMAIN_KEYBOARD_PAIRING_H
#define BLU2USB_DOMAIN_KEYBOARD_PAIRING_H
#include <stdint.h>
typedef enum {
    BLU2USB_KEYBOARD_PAIR_STARTING = 0,
    BLU2USB_KEYBOARD_PAIR_WAIT_RADIO,
    BLU2USB_KEYBOARD_PAIR_START_SEARCH,
    BLU2USB_KEYBOARD_PAIR_SEARCHING,
    BLU2USB_KEYBOARD_PAIR_READ_NAME,
    BLU2USB_KEYBOARD_PAIR_CONNECTING,
    BLU2USB_KEYBOARD_PAIR_SETUP,
    BLU2USB_KEYBOARD_PAIR_RETRY,
    BLU2USB_KEYBOARD_PAIR_ERROR,
} blu2usb_keyboard_pair_phase_t;
typedef struct {
    uint8_t phase;
    uint8_t error;
    uint8_t last_phase;
    uint16_t attempt;
    uint16_t found;
} blu2usb_keyboard_pair_progress_t;
#endif
