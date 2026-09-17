#include <assert.h>
#include <string.h>

#include "blu2usb/renderer/renderer.h"
#include "blu2usb/ux_model/ux_model.h"

static blu2usb_ux_command_t press_release(blu2usb_ux_model_t *ux,
                                           blu2usb_control_t control)
{
    (void)blu2usb_ux_input(ux, control, true);
    return blu2usb_ux_input(ux, control, false);
}

static void project_physical(const blu2usb_ux_model_t *ux,
                             blu2usb_ui_frame_t *frame)
{
    blu2usb_ui_project(ux, frame);
    blu2usb_ui_enforce_applied_visual_contract(ux, frame);
}

static void assert_row_tone(const blu2usb_ui_frame_t *frame,
                            unsigned row,
                            blu2usb_ui_tone_t tone)
{
    bool found = false;
    for (unsigned column = 0; column < BLU2USB_RENDERER_TEXT_COLS; ++column) {
        if (frame->cells[row][column].character == ' ') continue;
        found = true;
        assert(frame->cells[row][column].tone == tone);
    }
    assert(found);
}

static void assert_success_body_is_cyan(const blu2usb_ux_model_t *ux)
{
    blu2usb_ui_frame_t frame;
    project_physical(ux, &frame);
    for (unsigned row = 1u; row < frame.hint_start_row; ++row) {
        bool visible = false;
        for (unsigned column = 0u; column < BLU2USB_RENDERER_TEXT_COLS; ++column)
            visible |= frame.cells[row][column].character != ' ';
        if (visible) assert_row_tone(&frame, row, BLU2USB_UI_TONE_CURRENT);
    }
    assert(blu2usb_renderer_tone_rgb565(BLU2USB_UI_TONE_CURRENT) == BLU2USB_COLOR_CYAN);
}

static void test_current_profile_opens_feedback_and_back_once(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ux_init(&ux);
    ux.screen = BLU2USB_SCREEN_MOUSE_OPTIONS;
    ux.selection = 1u;

    blu2usb_ux_command_t command = press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(command.kind == BLU2USB_UX_COMMAND_NONE);
    assert(ux.screen == BLU2USB_SCREEN_PASSTHROUGH_APPLIED);
    assert_success_body_is_cyan(&ux);

    press_release(&ux, BLU2USB_CONTROL_KEY_B);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_OPTIONS);
}

static void test_default_apply_feedback_back_and_reentry(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ui_frame_t frame;
    blu2usb_ux_init(&ux);
    ux.screen = BLU2USB_SCREEN_MOUSE_OPTIONS;
    ux.selection = 2u;

    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_APPLY_DEFAULT);
    blu2usb_ux_command_t command = press_release(&ux, BLU2USB_CONTROL_KEY_A);
    assert(command.kind == BLU2USB_UX_COMMAND_APPLY_DEFAULT);

    blu2usb_ux_profile_applied(&ux, BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP);
    assert(ux.screen == BLU2USB_SCREEN_DEFAULT_APPLIED);
    assert(ux.active_profile == BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP);
    assert_success_body_is_cyan(&ux);

    press_release(&ux, BLU2USB_CONTROL_KEY_B);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_OPTIONS);

    ux.selection = 2u;
    project_physical(&ux, &frame);
    assert_row_tone(&frame, 3u, BLU2USB_UI_TONE_CURRENT);

    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_DEFAULT_APPLIED);
    assert_success_body_is_cyan(&ux);
}

static void test_escape_apply_feedback_back_and_reentry(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ui_frame_t frame;
    blu2usb_ux_init(&ux);
    ux.active_profile = BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP;
    ux.screen = BLU2USB_SCREEN_MOUSE_OPTIONS;
    ux.selection = 3u;

    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_APPLY_ESCAPE);
    blu2usb_ux_command_t command = press_release(&ux, BLU2USB_CONTROL_KEY_A);
    assert(command.kind == BLU2USB_UX_COMMAND_APPLY_ESCAPE);

    blu2usb_ux_profile_applied(&ux, BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP);
    assert(ux.screen == BLU2USB_SCREEN_ESCAPE_APPLIED);
    assert_success_body_is_cyan(&ux);

    press_release(&ux, BLU2USB_CONTROL_KEY_B);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_OPTIONS);

    ux.selection = 3u;
    project_physical(&ux, &frame);
    assert_row_tone(&frame, 4u, BLU2USB_UI_TONE_CURRENT);

    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_ESCAPE_APPLIED);
}

static void test_screen_text_fixes_profile_contract(void)
{
    const blu2usb_screen_template_t *d =
        blu2usb_ux_screen_template(BLU2USB_SCREEN_APPLY_DEFAULT);
    assert(strcmp(d->rows[1], "FORWARD IS LEFT") == 0);
    assert(strcmp(d->rows[2], "LEFT IS FORWARD") == 0);
    assert(strcmp(d->rows[3], "BACKWARD IS RIGHT") == 0);
    assert(strcmp(d->rows[4], "RIGHT IS BACKWARD") == 0);

    const blu2usb_screen_template_t *e =
        blu2usb_ux_screen_template(BLU2USB_SCREEN_APPLY_ESCAPE);
    assert(strcmp(e->rows[1], "FORWARD IS LEFT") == 0);
    assert(strcmp(e->rows[2], "BACKWARD IS RIGHT") == 0);
    assert(strcmp(e->rows[3], "LEFT IS ESCAPE") == 0);
    assert(strcmp(e->rows[4], "RIGHT IS BACKWARD") == 0);
    assert(strcmp(e->rows[5], "MIDDLE IS FORWARD") == 0);
}

static void test_custom_success_hides_apply_hint(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ui_frame_t frame;
    blu2usb_ux_init(&ux);
    ux.screen = BLU2USB_SCREEN_MOUSE_OPTIONS;
    ux.selection = 4u;

    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_EDIT_CUSTOM);
    assert(ux.custom_dirty);

    blu2usb_ux_command_t command = press_release(&ux, BLU2USB_CONTROL_KEY_A);
    assert(command.kind == BLU2USB_UX_COMMAND_APPLY_CUSTOM);
    blu2usb_ux_profile_applied(&ux, BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP);
    assert(ux.active_profile == BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP);
    assert(!ux.custom_dirty);

    project_physical(&ux, &frame);
    for (unsigned row = 1u; row <= 5u; ++row)
        assert_row_tone(&frame, row, BLU2USB_UI_TONE_CURRENT);
    for (unsigned column = 0; column < BLU2USB_RENDERER_TEXT_COLS; ++column)
        assert(frame.cells[8][column].character == ' ');

    press_release(&ux, BLU2USB_CONTROL_KEY_B);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_OPTIONS);
    ux.selection = 4u;
    project_physical(&ux, &frame);
    assert_row_tone(&frame, 5u, BLU2USB_UI_TONE_CURRENT);

    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_EDIT_CUSTOM);
    assert(!ux.custom_dirty);
    project_physical(&ux, &frame);
    for (unsigned column = 0; column < BLU2USB_RENDERER_TEXT_COLS; ++column)
        assert(frame.cells[8][column].character == ' ');
}

int main(void)
{
    test_current_profile_opens_feedback_and_back_once();
    test_default_apply_feedback_back_and_reentry();
    test_escape_apply_feedback_back_and_reentry();
    test_screen_text_fixes_profile_contract();
    test_custom_success_hides_apply_hint();
    return 0;
}