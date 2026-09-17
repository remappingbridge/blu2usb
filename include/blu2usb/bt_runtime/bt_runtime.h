#ifndef BLU2USB_BT_RUNTIME_BT_RUNTIME_H
#define BLU2USB_BT_RUNTIME_BT_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLU2USB_BT_RUNTIME_MESSAGE_PAYLOAD_SIZE 32u
#define BLU2USB_BT_RUNTIME_QUEUE_CAPACITY 128u

typedef struct {
    uint16_t channel;
    uint16_t type;
    uint16_t length;
    uint8_t payload[BLU2USB_BT_RUNTIME_MESSAGE_PAYLOAD_SIZE];
} blu2usb_bt_runtime_message_t;

typedef void (*blu2usb_bt_runtime_session_setup_fn)(void);

void blu2usb_bt_runtime_reset(void);
bool blu2usb_bt_runtime_publish(uint16_t channel, uint16_t type, const void *payload, uint16_t length);
bool blu2usb_bt_runtime_poll(blu2usb_bt_runtime_message_t *message);
bool blu2usb_bt_runtime_take_overflow(void);

/* Explicit composition: both setups execute once before HCI power-on on Core1.
 * Preparation (HIDS) precedes Classic, then BLE handler registration. */
bool blu2usb_bt_runtime_start(blu2usb_bt_runtime_session_setup_fn session_setup,
                             blu2usb_bt_runtime_session_setup_fn companion_setup,
                             blu2usb_bt_runtime_session_setup_fn transport_prepare);

#ifdef __cplusplus
}
#endif
#endif
