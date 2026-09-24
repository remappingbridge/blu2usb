#ifndef BLU2USB_USB_HID_USB_HID_H
#define BLU2USB_USB_HID_USB_HID_H

#include <stdbool.h>
#include <stdint.h>

#define BLU2USB_USB_VID UINT16_C(0xcafe)
#define BLU2USB_USB_PID UINT16_C(0x4010)
#define BLU2USB_USB_BCD_DEVICE UINT16_C(0x0100)
#define BLU2USB_USB_HID_INTERFACE_COUNT 2u
#define BLU2USB_USB_HID_MOUSE_INTERFACE 0u
#define BLU2USB_USB_HID_KEYBOARD_INTERFACE 1u
#define BLU2USB_USB_HID_KEYCODE_COUNT 6u
#define BLU2USB_USB_MANUFACTURER "BLU2USB"
#define BLU2USB_USB_PRODUCT "BLU2USB Mouse + Keyboard"

typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint16_t bcd_device;
    uint8_t interface_count;
    uint8_t mouse_interface;
    uint8_t keyboard_interface;
} blu2usb_usb_hid_identity_t;

typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;
    int8_t pan;
} blu2usb_usb_mouse_report_t;

typedef struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keycodes[BLU2USB_USB_HID_KEYCODE_COUNT];
} blu2usb_usb_keyboard_report_t;

const blu2usb_usb_hid_identity_t *blu2usb_usb_hid_identity(void);
void blu2usb_usb_hid_build_mouse_report(blu2usb_usb_mouse_report_t *report, uint8_t buttons, int8_t x, int8_t y, int8_t wheel, int8_t pan);
/* Fixed rotation: USB X = -source Y, USB Y = source X. After successful
 * transmission, consume source (report.y, -(int32_t)report.x) plus wheel/pan.
 * Unsent/remainder motion stays in the source-coordinate accumulator.
 */
void blu2usb_usb_hid_build_rotated_mouse_report(blu2usb_usb_mouse_report_t *report, uint8_t buttons, int32_t x, int32_t y, int32_t wheel, int32_t pan);
void blu2usb_usb_hid_build_keyboard_report(blu2usb_usb_keyboard_report_t *report, uint8_t modifiers, const uint8_t keycodes[BLU2USB_USB_HID_KEYCODE_COUNT]);

bool blu2usb_usb_hid_pico_init(void);
void blu2usb_usb_hid_pico_task(void);
bool blu2usb_usb_hid_pico_mounted(void);
bool blu2usb_usb_hid_pico_send_mouse(const blu2usb_usb_mouse_report_t *report);
bool blu2usb_usb_hid_pico_send_keyboard(const blu2usb_usb_keyboard_report_t *report);

#endif
