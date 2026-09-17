#ifndef BLU2USB_BT_BOOT_GATE_H
#define BLU2USB_BT_BOOT_GATE_H
#include <stdbool.h>
#include <stdint.h>
/* PICO-08 8fbb36f: wait for first successful LCD frame, then 500 ms on Core0.
 * Explicit booleans avoid treating timestamp zero as an uninitialized clock. */
typedef struct { bool armed, launched; uint32_t ready_since; } blu2usb_bt_boot_gate_t;
static inline bool blu2usb_bt_boot_due(blu2usb_bt_boot_gate_t *gate,
                                      bool lcd_ready, uint32_t now)
{
    if (gate->launched) return false;
    if (!lcd_ready) { gate->armed = false; return false; }
    if (!gate->armed) { gate->armed = true; gate->ready_since = now; }
    if ((uint32_t)(now - gate->ready_since) < 500u) return false;
    gate->launched = true;
    return true;
}
#endif
