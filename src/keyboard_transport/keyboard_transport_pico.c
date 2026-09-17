#include "blu2usb/keyboard_transport/keyboard_transport.h"

#include "blu2usb/classic_hid/classic_hid.h"

bool blu2usb_keyboard_transport_pico_start(void)
{
    return blu2usb_classic_hid_pico_register();
}

bool blu2usb_keyboard_transport_pico_pair(void)
{
    return blu2usb_classic_hid_pico_pair_keyboard();
}

bool blu2usb_keyboard_transport_pico_retry(void)
{
    return blu2usb_classic_hid_pico_retry_keyboard();
}

bool blu2usb_keyboard_transport_pico_cancel(void)
{
    return blu2usb_classic_hid_pico_cancel_pairing();
}
