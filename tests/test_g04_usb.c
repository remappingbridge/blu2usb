#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "blu2usb/usb_hid/usb_hid.h"

int main(void)
{
    const blu2usb_usb_hid_identity_t *identity = blu2usb_usb_hid_identity();
    assert(identity != NULL);
    assert(identity->vid == BLU2USB_USB_VID);
    assert(identity->pid == BLU2USB_USB_PID);
    assert(identity->bcd_device == BLU2USB_USB_BCD_DEVICE);
    assert(identity->interface_count == 2u);
    assert(identity->mouse_interface == 0u);
    assert(identity->keyboard_interface == 1u);
    assert(sizeof(blu2usb_usb_mouse_report_t) == 5u);
    assert(sizeof(blu2usb_usb_keyboard_report_t) == 8u);

    blu2usb_usb_mouse_report_t mouse;
    blu2usb_usb_hid_build_mouse_report(&mouse, 0x15u, -7, 9, -1, 2);
    assert(mouse.buttons == 0x15u);
    assert(mouse.x == -7);
    assert(mouse.y == 9);
    assert(mouse.wheel == -1);
    assert(mouse.pan == 2);

    const uint8_t keys[BLU2USB_USB_HID_KEYCODE_COUNT] = {4u, 5u, 6u, 7u, 8u, 9u};
    blu2usb_usb_keyboard_report_t keyboard;
    blu2usb_usb_hid_build_keyboard_report(&keyboard, 0x03u, keys);
    assert(keyboard.modifiers == 0x03u);
    assert(keyboard.reserved == 0u);
    assert(memcmp(keyboard.keycodes, keys, sizeof(keys)) == 0);

    blu2usb_usb_hid_build_keyboard_report(&keyboard, 0u, NULL);
    for (size_t index = 0u; index < BLU2USB_USB_HID_KEYCODE_COUNT; ++index) {
        assert(keyboard.keycodes[index] == 0u);
    }

    return 0;
}
