#ifndef BLU2USB_DOMAIN_DEVICE_H
#define BLU2USB_DOMAIN_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLU2USB_SAVED_MOUSE_CAPACITY 16u
#define BLU2USB_MOUSE_NAME_CAPACITY 64u

typedef uint64_t blu2usb_mouse_id_t;

#define BLU2USB_MOUSE_ID_INVALID UINT64_C(0)

static inline bool blu2usb_mouse_id_is_valid(blu2usb_mouse_id_t id)
{
    return id != BLU2USB_MOUSE_ID_INVALID;
}

#ifdef __cplusplus
}
#endif

#endif
