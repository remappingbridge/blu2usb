#ifndef BLU2USB_REMAP_REMAP_H
#define BLU2USB_REMAP_REMAP_H

#include <stdbool.h>
#include "blu2usb/domain/hid.h"
#include "blu2usb/domain/profile.h"

typedef struct {
    blu2usb_mouse_profile_config_t profile;
} blu2usb_remap_t;

typedef struct {
    bool has_mouse;
    bool has_keyboard;
    blu2usb_canonical_mouse_event_t mouse;
    blu2usb_canonical_keyboard_event_t keyboard;
} blu2usb_remap_result_t;

void blu2usb_remap_init(blu2usb_remap_t *remap);
void blu2usb_remap_set_profile(blu2usb_remap_t *remap,
                               const blu2usb_mouse_profile_config_t *profile);
bool blu2usb_remap_process_mouse(const blu2usb_remap_t *remap,
                                  const blu2usb_canonical_mouse_event_t *input,
                                  blu2usb_remap_result_t *result);

#endif
