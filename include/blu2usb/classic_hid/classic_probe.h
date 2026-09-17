#ifndef BLU2USB_CLASSIC_PROBE_H
#define BLU2USB_CLASSIC_PROBE_H

#include <stdbool.h>
#include <stdint.h>

/* Experiment A: connection evidence only. No Keyboard USB forwarding yet. */
typedef enum {
    BLU2USB_PROBE_STARTING,
    BLU2USB_PROBE_IDLE,
    BLU2USB_PROBE_WAIT_ACK,
    BLU2USB_PROBE_SEARCHING,
    BLU2USB_PROBE_READ_NAME,
    BLU2USB_PROBE_CONNECTING,
    BLU2USB_PROBE_PIN,
    BLU2USB_PROBE_SETUP,
    BLU2USB_PROBE_READY,
    BLU2USB_PROBE_DRAINING,
    BLU2USB_PROBE_ERROR
} blu2usb_probe_phase_t;

typedef struct {
    blu2usb_probe_phase_t phase;
    uint8_t error;
    uint16_t found;
    uint32_t pin;
    uint8_t pin_digits;
    char message[22];
} blu2usb_classic_probe_snapshot_t;

/* Setup is called once by the shared bootstrap before HCI power-on. */
void blu2usb_classic_probe_shared_init(void);
void blu2usb_classic_probe_setup(void);
void blu2usb_classic_probe_pair(void);
void blu2usb_classic_probe_cancel(void);
blu2usb_classic_probe_snapshot_t blu2usb_classic_probe_snapshot(void);

#endif
