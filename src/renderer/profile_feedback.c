#include "blu2usb/renderer/renderer.h"

#include <stdio.h>

static void clear_row(blu2usb_ui_frame_t *frame, uint8_t row)
{
    if (frame == NULL || row >= BLU2USB_RENDERER_TEXT_ROWS) return;
    for (uint8_t column = 0u; column < BLU2USB_RENDERER_TEXT_COLS; ++column) {
        frame->cells[row][column].character = ' ';
        frame->cells[row][column].tone = BLU2USB_UI_TONE_ACTIONABLE;
    }
}

static void set_row_current_preserving_selection(blu2usb_ui_frame_t *frame,
                                                  uint8_t row)
{
    if (frame == NULL || row >= BLU2USB_RENDERER_TEXT_ROWS) return;
    for (uint8_t column = 0u; column < BLU2USB_RENDERER_TEXT_COLS; ++column) {
        if (frame->cells[row][column].character == ' ') continue;
        if (frame->cells[row][column].tone == BLU2USB_UI_TONE_EMPHASIZED) continue;
        frame->cells[row][column].tone = BLU2USB_UI_TONE_CURRENT;
    }
}

static bool is_success_feedback(blu2usb_screen_id_t screen)
{
    return screen == BLU2USB_SCREEN_MOUSE_SAVED ||
           screen == BLU2USB_SCREEN_KEYBOARD_SAVED ||
           screen == BLU2USB_SCREEN_COMPOSITE_SAVED ||
           screen == BLU2USB_SCREEN_PASSTHROUGH_APPLIED ||
           screen == BLU2USB_SCREEN_DEFAULT_APPLIED ||
           screen == BLU2USB_SCREEN_ESCAPE_APPLIED ||
           screen == BLU2USB_SCREEN_CUSTOM_APPLIED;
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

static const char *profile_name(blu2usb_mouse_profile_kind_t profile)
{
    switch (profile) {
    case BLU2USB_MOUSE_PROFILE_PASSTHROUGH: return "PASSTHROUGH";
    case BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP: return "DEFAULT";
    case BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP: return "ESCAPE";
    case BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP: return "CUSTOM";
    default: return "PASSTHROUGH";
    }
}

static const char *custom_source_name(unsigned source)
{
    static const char *const names[BLU2USB_MOUSE_SOURCE_COUNT] = {
        "LEFT", "RIGHT", "MIDDLE", "FORWARD", "BACKWARD"
    };
    return source < BLU2USB_MOUSE_SOURCE_COUNT ? names[source] : "LEFT";
}

static const char *custom_target_name(blu2usb_mouse_target_t target)
{
    static const char *const names[BLU2USB_MOUSE_TARGET_COUNT] = {
        "LEFT", "RIGHT", "MIDDLE", "BACKWARD", "FORWARD", "ESCAPE"
    };
    return (unsigned)target < BLU2USB_MOUSE_TARGET_COUNT ? names[target] : "LEFT";
}

static void project_custom_rows(const blu2usb_ux_model_t *ux,
                                blu2usb_ui_frame_t *frame,
                                bool applied_feedback)
{
    const bool clean_applied =
        ux->active_profile == BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP && !ux->custom_dirty;

    for (uint8_t source = 0u; source < BLU2USB_MOUSE_SOURCE_COUNT; ++source) {
        const uint8_t row = (uint8_t)(source + 1u);
        char text[BLU2USB_RENDERER_TEXT_COLS + 1u];
        (void)snprintf(text, sizeof(text), applied_feedback ? "%s IS %s" : " %s IS %s",
                       custom_source_name(source),
                       custom_target_name(ux->custom_targets[source]));

        clear_row(frame, row);

        const blu2usb_ui_tone_t tone = applied_feedback
            ? BLU2USB_UI_TONE_CURRENT
            : ux->selection == source
                ? BLU2USB_UI_TONE_EMPHASIZED
                : clean_applied
                    ? BLU2USB_UI_TONE_CURRENT
                    : BLU2USB_UI_TONE_ACTIONABLE;
        (void)blu2usb_ui_frame_set_text(frame, row, 0u, text, tone);
    }

    if (!applied_feedback && clean_applied)
        clear_row(frame, 8u);
}

static void project_mouse_status(const blu2usb_ux_model_t *ux,
                                 blu2usb_ui_frame_t *frame)
{
    char profile[BLU2USB_RENDERER_TEXT_COLS + 1u];

    clear_row(frame, 1u);
    (void)blu2usb_ui_frame_set_text(
        frame, 1u, 0u,
        blu2usb_ux_mouse_connected() ? "MOUSE CONNECTED" : "MOUSE NOT CONNECTED",
        blu2usb_ux_mouse_connected() ? BLU2USB_UI_TONE_CURRENT : BLU2USB_UI_TONE_STATIC);

    (void)snprintf(profile, sizeof(profile), "PROFILE: %s", profile_name(ux->active_profile));
    clear_row(frame, 2u);
    (void)blu2usb_ui_frame_set_text(frame, 2u, 0u, profile, BLU2USB_UI_TONE_STATIC);
}

static void keyboard_row(blu2usb_ui_frame_t *frame, unsigned row,
                         const char *text, blu2usb_ui_tone_t tone)
{
    clear_row(frame, (uint8_t)row);
    (void)blu2usb_ui_frame_set_text(frame, (uint8_t)row, 0u, text, tone);
}

static void project_keyboard(const blu2usb_ux_model_t *ux, blu2usb_ui_frame_t *frame)
{
    const bool ready = ux->keyboard_status == BLU2USB_KEYBOARD_READY;
    if (ux->screen == BLU2USB_SCREEN_OTHER_DEVICES_STATUS) {
        keyboard_row(frame, 2, ready ? "CONNECTED" : "NOT CONNECTED",
                     ready ? BLU2USB_UI_TONE_CURRENT : BLU2USB_UI_TONE_STATIC);
    } else if (ux->screen == BLU2USB_SCREEN_OTHER_OPTIONS && ready) {
        set_row_current_preserving_selection(frame, 1u);
    } else if (ux->screen == BLU2USB_SCREEN_KEYBOARD_SAVED) {
        keyboard_row(frame, 1, "TYPE KEYBOARD", BLU2USB_UI_TONE_CURRENT);
        keyboard_row(frame, 2, "KEYBOARD CONNECTED", BLU2USB_UI_TONE_CURRENT);
        keyboard_row(frame, 3, "READY TO USE", BLU2USB_UI_TONE_CURRENT);
    } else if (ux->screen == BLU2USB_SCREEN_PAIR_KEYBOARD) {
        const char *phase = "SEARCHING KEYBOARD";
        switch (ux->keyboard_status) {
        case BLU2USB_KEYBOARD_WAITING: phase = "WAITING FOR RADIO"; break;
        case BLU2USB_KEYBOARD_READING_NAME: phase = "READING DEVICE NAME"; break;
        case BLU2USB_KEYBOARD_PAIRING: phase = "PAIRING KEYBOARD"; break;
        case BLU2USB_KEYBOARD_CONNECTING: phase = "CONNECTING KEYBOARD"; break;
        case BLU2USB_KEYBOARD_READY: phase = "KEYBOARD CONNECTED"; break;
        case BLU2USB_KEYBOARD_ERROR: phase = "CONNECTION FAILED"; break;
        case BLU2USB_KEYBOARD_STOPPING: phase = "FINISHING REQUEST"; break;
        default: break;
        }
        keyboard_row(frame, 1, phase, ready ? BLU2USB_UI_TONE_CURRENT : BLU2USB_UI_TONE_STATIC);
        keyboard_row(frame, 2, "TARGET KEYBOARD", BLU2USB_UI_TONE_STATIC);
        keyboard_row(frame, 3, ux->keyboard_status == BLU2USB_KEYBOARD_ERROR
                     ? "KEY A TO TRY AGAIN" : "KEEP PAIRING ACTIVE", BLU2USB_UI_TONE_STATIC);
        keyboard_row(frame, 4, "", BLU2USB_UI_TONE_STATIC);
    }
}

void blu2usb_ui_enforce_applied_visual_contract(const blu2usb_ux_model_t *ux,
                                                 blu2usb_ui_frame_t *frame)
{
    if (ux == NULL || frame == NULL) return;

    project_keyboard(ux, frame);

    if (is_success_feedback(ux->screen)) {
        const uint8_t end = frame->hint_start_row < BLU2USB_RENDERER_TEXT_ROWS
            ? frame->hint_start_row
            : BLU2USB_RENDERER_TEXT_ROWS;
        for (uint8_t row = 1u; row < end; ++row)
            set_row_current_preserving_selection(frame, row);
    }

    if (ux->screen == BLU2USB_SCREEN_MOUSE_STATUS)
        project_mouse_status(ux, frame);

    if (ux->screen == BLU2USB_SCREEN_MOUSE_OPTIONS && blu2usb_ux_mouse_connected())
        set_row_current_preserving_selection(frame, 1u);

    const uint8_t profile_row = active_profile_row(ux);
    if (profile_row != 0u)
        set_row_current_preserving_selection(frame, profile_row);

    if (ux->screen == BLU2USB_SCREEN_EDIT_CUSTOM)
        project_custom_rows(ux, frame, false);
    else if (ux->screen == BLU2USB_SCREEN_CUSTOM_APPLIED)
        project_custom_rows(ux, frame, true);

    if (ux->screen >= BLU2USB_SCREEN_LEFT_WILL_BECOME &&
        ux->screen <= BLU2USB_SCREEN_BACKWARD_WILL_BECOME) {
        const uint8_t current_row =
            (uint8_t)(1u + (unsigned)ux->custom_targets[ux->custom_source]);
        set_row_current_preserving_selection(frame, current_row);
    }
}
