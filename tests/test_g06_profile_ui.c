#include <assert.h>

#include "blu2usb/renderer/renderer.h"
#include "blu2usb/ux_model/ux_model.h"

static blu2usb_ux_command_t press_release(blu2usb_ux_model_t *ux,
                                           blu2usb_control_t control)
{
    (void)blu2usb_ux_input(ux, control, true);
    return blu2usb_ux_input(ux, control, false);
}

static void assert_row_tone(const blu2usb_ui_frame_t *frame,
                            unsigned row,
                            blu2usb_ui_tone_t tone)
{
    for (unsigned column = 0; column < BLU2USB_RENDERER_TEXT_COLS; ++column) {
        if (frame->cells[row][column].character != ' ') {
            assert(frame->cells[row][column].tone == tone);
            return;
        }
    }
    assert(!"row has no visible text");
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

    press_release(&ux, BLU2USB_CONTROL_KEY_B);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_OPTIONS);
}

static void test_success_feedback_and_active_option_are_cyan(void)
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
    assert(ux.screen == BLU2USB_SCREEN_DEFAULT_APPLIED);
    assert(ux.active_profile == BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP);

    blu2usb_ui_project(&ux, &frame);
    assert_row_tone(&frame, 1u, BLU2USB_UI_TONE_CURRENT);
    assert(blu2usb_renderer_tone_rgb565(BLU2USB_UI_TONE_CURRENT) == BLU2USB_COLOR_CYAN);

    press_release(&ux, BLU2USB_CONTROL_KEY_B);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_OPTIONS);
    blu2usb_ui_project(&ux, &frame);
    assert_row_tone(&frame, 3u, BLU2USB_UI_TONE_CURRENT);

    ux.selection = 2u;
    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_DEFAULT_APPLIED);
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
    assert(ux.active_profile == BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP);
    assert(!ux.custom_dirty);

    blu2usb_ui_project(&ux, &frame);
    assert_row_tone(&frame, 2u, BLU2USB_UI_TONE_CURRENT);
    for (unsigned column = 0; column < BLU2USB_RENDERER_TEXT_COLS; ++column)
        assert(frame.cells[8][column].character == ' ');

    press_release(&ux, BLU2USB_CONTROL_KEY_B);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_OPTIONS);
    blu2usb_ui_project(&ux, &frame);
    assert_row_tone(&frame, 5u, BLU2USB_UI_TONE_CURRENT);

    ux.selection = 4u;
    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_EDIT_CUSTOM);
    assert(!ux.custom_dirty);
    blu2usb_ui_project(&ux, &frame);
    for (unsigned column = 0; column < BLU2USB_RENDERER_TEXT_COLS; ++column)
        assert(frame.cells[8][column].character == ' ');
}

int main(void)
{
    test_current_profile_opens_feedback_and_back_once();
    test_success_feedback_and_active_option_are_cyan();
    test_custom_success_hides_apply_hint();
    return 0;
}
