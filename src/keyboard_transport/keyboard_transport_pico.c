#include "blu2usb/keyboard_transport/keyboard_transport.h"
#include "blu2usb/classic_hid/classic_hid.h"
void blu2usb_keyboard_transport_setup(void) { blu2usb_classic_hid_setup(); }
void blu2usb_keyboard_transport_pair(void) { blu2usb_classic_hid_request_pair(); }
void blu2usb_keyboard_transport_cancel(void) { blu2usb_classic_hid_cancel_pair(); }
blu2usb_keyboard_status_t blu2usb_keyboard_transport_status(void)
{ return blu2usb_bt_runtime_failed() ? BLU2USB_KEYBOARD_ERROR : blu2usb_classic_hid_status(); }
