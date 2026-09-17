#ifndef BLU2USB_PICO08_REPORT_SINK_H
#define BLU2USB_PICO08_REPORT_SINK_H
#include <stdbool.h>
#include <stdint.h>
/* A2 boundary only: original parser runs, but physical Keyboard USB ownership
 * is not introduced before connection acceptance. No historical USB driver. */
typedef struct { uint8_t report_id; uint16_t report_len; uint8_t report[8]; } ST_HID_RPT;
static inline bool keyboard_hid_queue_enqueue(const ST_HID_RPT *report)
{ (void)report; return true; }
static inline void keyboard_hid_queue_clear(void) {}
#endif
