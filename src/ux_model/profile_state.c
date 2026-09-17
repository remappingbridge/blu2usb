#include "blu2usb/ux_model/ux_model.h"

static bool valid_profile(blu2usb_mouse_profile_kind_t profile)
{
    return profile >= BLU2USB_MOUSE_PROFILE_PASSTHROUGH &&
           profile <= BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP;
}

void blu2usb_ux_profile_applied(blu2usb_ux_model_t *ux,
                                blu2usb_mouse_profile_kind_t active_profile)
{
    if (ux == NULL || !valid_profile(active_profile)) return;

    ux->active_profile = active_profile;
    ux->selection = 0u;

    switch (active_profile) {
    case BLU2USB_MOUSE_PROFILE_PASSTHROUGH:
        ux->screen = BLU2USB_SCREEN_PASSTHROUGH_APPLIED;
        break;
    case BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP:
        ux->screen = BLU2USB_SCREEN_DEFAULT_APPLIED;
        break;
    case BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP:
        ux->screen = BLU2USB_SCREEN_ESCAPE_APPLIED;
        break;
    case BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP:
        ux->custom_dirty = false;
        ux->screen = BLU2USB_SCREEN_EDIT_CUSTOM;
        break;
    default:
        break;
    }
}
