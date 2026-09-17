#include "blu2usb/renderer/renderer.h"

static void set_row_tone(blu2usb_ui_frame_t *frame,
                         uint8_t row,
                         blu2usb_ui_tone_t tone)
{
    if (frame == NULL || row >= BLU2USB_RENDERER_TEXT_ROWS) return;
    for (uint8_t column = 0u; column < BLU2USB_RENDERER_TEXT_COLS; ++column) {
        if (frame->cells[row][column].character != ' ')
            frame->cells[row][column].tone = tone;
    }
}

static bool is_success_feedback(blu2usb_screen_id_t screen)
{
    return screen == BLU2USB_SCREEN_MOUSE_SAVED ||
           screen == BLU2USB_SCREEN_KEYBOARD_SAVED ||
           screen == BLU2USB_SCREEN_COMPOSITE_SAVED ||
           screen == BLU2USB_SCREEN_PASSTHROUGH_APPLIED ||
           screen == BLU2USB_SCREEN_DEFAULT_APPLIED ||
           screen == BLU2USB_SCREEN_ESCAPE_APPLIED;
}

static uint8_t active_profile_row(const blu2usb_ux_model_t *ux)
{
    if (ux == NULL || ux->screen != BLU2USB_SCREEN_MOUSE_OPTIONS) return 0u;
    switch (ux->active_profile) {
    case BLU2USB_MOUSE_PROFILE_PASSTHROUGH: return 2u;
    case BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP: return 3u;
    case BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP: return 4u;
    case BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP: return 5u;
    default: return 0u;
    }
}

void blu2usb_ui_enforce_applied_visual_contract(const blu2usb_ux_model_t *ux,
                                                 blu2usb_ui_frame_t *frame)
{
    if (ux == NULL || frame == NULL) return;

    if (is_success_feedback(ux->screen)) {
        const uint8_t end = frame->hint_start_row < BLU2USB_RENDERER_TEXT_ROWS
            ? frame->hint_start_row
            : BLU2USB_RENDERER_TEXT_ROWS;
        for (uint8_t row = 1u; row < end; ++row)
            set_row_tone(frame, row, BLU2USB_UI_TONE_CURRENT);
    }

    const uint8_t profile_row = active_profile_row(ux);
    if (profile_row != 0u)
        set_row_tone(frame, profile_row, BLU2USB_UI_TONE_CURRENT);

    if (ux->screen == BLU2USB_SCREEN_EDIT_CUSTOM &&
        ux->active_profile == BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP &&
        !ux->custom_dirty) {
        for (uint8_t row = 1u; row <= 5u; ++row)
            set_row_tone(frame, row, BLU2USB_UI_TONE_CURRENT);
        for (uint8_t column = 0u; column < BLU2USB_RENDERER_TEXT_COLS; ++column)
            frame->cells[8u][column].character = ' ';
    }

    if (ux->screen >= BLU2USB_SCREEN_LEFT_WILL_BECOME &&
        ux->screen <= BLU2USB_SCREEN_BACKWARD_WILL_BECOME) {
        const uint8_t current_row =
            (uint8_t)(1u + (unsigned)ux->custom_targets[ux->custom_source]);
        set_row_tone(frame, current_row, BLU2USB_UI_TONE_CURRENT);
    }
}
