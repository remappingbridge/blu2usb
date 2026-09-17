#include "blu2usb/remap/remap.h"
#include <string.h>

static blu2usb_mouse_target_t identity_target(blu2usb_mouse_source_t source)
{
    switch (source) {
    case BLU2USB_MOUSE_SOURCE_LEFT: return BLU2USB_MOUSE_TARGET_LEFT;
    case BLU2USB_MOUSE_SOURCE_RIGHT: return BLU2USB_MOUSE_TARGET_RIGHT;
    case BLU2USB_MOUSE_SOURCE_MIDDLE: return BLU2USB_MOUSE_TARGET_MIDDLE;
    case BLU2USB_MOUSE_SOURCE_FORWARD: return BLU2USB_MOUSE_TARGET_FORWARD;
    case BLU2USB_MOUSE_SOURCE_BACKWARD: return BLU2USB_MOUSE_TARGET_BACKWARD;
    default: return BLU2USB_MOUSE_TARGET_LEFT;
    }
}

static bool button_to_source(blu2usb_mouse_button_t button, blu2usb_mouse_source_t *source)
{
    if (source == NULL) return false;
    switch (button) {
    case BLU2USB_MOUSE_BUTTON_LEFT: *source = BLU2USB_MOUSE_SOURCE_LEFT; return true;
    case BLU2USB_MOUSE_BUTTON_RIGHT: *source = BLU2USB_MOUSE_SOURCE_RIGHT; return true;
    case BLU2USB_MOUSE_BUTTON_MIDDLE: *source = BLU2USB_MOUSE_SOURCE_MIDDLE; return true;
    case BLU2USB_MOUSE_BUTTON_FORWARD: *source = BLU2USB_MOUSE_SOURCE_FORWARD; return true;
    case BLU2USB_MOUSE_BUTTON_BACK: *source = BLU2USB_MOUSE_SOURCE_BACKWARD; return true;
    default: return false;
    }
}

static bool target_to_button(blu2usb_mouse_target_t target, blu2usb_mouse_button_t *button)
{
    if (button == NULL) return false;
    switch (target) {
    case BLU2USB_MOUSE_TARGET_LEFT: *button = BLU2USB_MOUSE_BUTTON_LEFT; return true;
    case BLU2USB_MOUSE_TARGET_RIGHT: *button = BLU2USB_MOUSE_BUTTON_RIGHT; return true;
    case BLU2USB_MOUSE_TARGET_MIDDLE: *button = BLU2USB_MOUSE_BUTTON_MIDDLE; return true;
    case BLU2USB_MOUSE_TARGET_BACKWARD: *button = BLU2USB_MOUSE_BUTTON_BACK; return true;
    case BLU2USB_MOUSE_TARGET_FORWARD: *button = BLU2USB_MOUSE_BUTTON_FORWARD; return true;
    case BLU2USB_MOUSE_TARGET_ESCAPE:
    default: return false;
    }
}

void blu2usb_remap_init(blu2usb_remap_t *remap)
{
    if (remap == NULL) return;
    memset(remap, 0, sizeof(*remap));
    remap->profile.kind = BLU2USB_MOUSE_PROFILE_PASSTHROUGH;
    for (unsigned source = 0u; source < BLU2USB_MOUSE_SOURCE_COUNT; ++source)
        remap->profile.targets[source] = identity_target((blu2usb_mouse_source_t)source);
}

void blu2usb_remap_set_profile(blu2usb_remap_t *remap,
                               const blu2usb_mouse_profile_config_t *profile)
{
    if (remap != NULL && profile != NULL) remap->profile = *profile;
}

bool blu2usb_remap_process_mouse(const blu2usb_remap_t *remap,
                                  const blu2usb_canonical_mouse_event_t *input,
                                  blu2usb_remap_result_t *result)
{
    if (remap == NULL || input == NULL || result == NULL ||
        !blu2usb_hid_source_is_valid(input->source)) return false;
    memset(result, 0, sizeof(*result));
    if (input->type != BLU2USB_MOUSE_EVENT_BUTTON) {
        result->has_mouse = true;
        result->mouse = *input;
        return true;
    }

    blu2usb_mouse_source_t source;
    if (!button_to_source(input->data.button.button, &source)) {
        result->has_mouse = true;
        result->mouse = *input;
        return true;
    }

    blu2usb_mouse_target_t target = remap->profile.targets[source];
    if (remap->profile.kind == BLU2USB_MOUSE_PROFILE_PASSTHROUGH)
        target = identity_target(source);

    if (target == BLU2USB_MOUSE_TARGET_ESCAPE) {
        result->has_keyboard = true;
        result->keyboard.source = blu2usb_hid_source_make(
            BLU2USB_HID_SOURCE_SYNTHETIC_REMAP, input->source.instance);
        result->keyboard.type = BLU2USB_KEYBOARD_EVENT_KEY;
        result->keyboard.data.key.key = BLU2USB_KEY_ESCAPE;
        result->keyboard.data.key.pressed = input->data.button.pressed;
        return true;
    }

    blu2usb_mouse_button_t mapped_button;
    if (!target_to_button(target, &mapped_button)) return false;
    result->has_mouse = true;
    result->mouse = *input;
    result->mouse.data.button.button = mapped_button;
    return true;
}
