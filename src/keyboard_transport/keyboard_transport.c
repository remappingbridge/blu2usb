#include "blu2usb/keyboard_transport/keyboard_transport.h"
#include "blu2usb/classic_hid/classic_hid.h"
#include <string.h>

bool blu2usb_keyboard_transport_decode(const blu2usb_bt_runtime_message_t *message,
                                       blu2usb_keyboard_snapshot_t *snapshot)
{
    if (!message || !snapshot || message->channel != BLU2USB_KEYBOARD_RUNTIME_CHANNEL)
        return false;
    if (message->type == BLU2USB_KEYBOARD_MESSAGE_RELEASE && message->length == 0u) {
        memset(snapshot, 0, sizeof(*snapshot));
        return true;
    }
    if (message->type != BLU2USB_KEYBOARD_MESSAGE_SNAPSHOT ||
        message->length != sizeof(*snapshot)) return false;
    memcpy(snapshot, message->payload, sizeof(*snapshot));
    return true;
}
