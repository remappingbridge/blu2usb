#include "blu2usb/ux_model/ux_model.h"
void blu2usb_ux_keyboard_status(blu2usb_ux_model_t *ux, blu2usb_keyboard_status_t status)
{
    if (!ux) return;
    ux->keyboard_status = status;
    if (status == BLU2USB_KEYBOARD_READY) {
        if (ux->screen == BLU2USB_SCREEN_PAIR_KEYBOARD) {
            ux->screen = BLU2USB_SCREEN_KEYBOARD_SAVED;
            ux->selection = 0;
        } else if (ux->screen == BLU2USB_SCREEN_PAIR_KEYBOARD_HELP) {
            ux->return_screen = BLU2USB_SCREEN_KEYBOARD_SAVED;
        }
    } else if (ux->screen == BLU2USB_SCREEN_KEYBOARD_SAVED) {
        ux->screen = BLU2USB_SCREEN_PAIR_KEYBOARD;
    }
}
