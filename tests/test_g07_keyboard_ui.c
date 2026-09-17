#include <assert.h>
#include <string.h>

#include "blu2usb/renderer/renderer.h"
#include "blu2usb/ux_model/ux_model.h"

static void init_ux(blu2usb_ux_model_t *ux)
{
    blu2usb_ux_init(ux);
    blu2usb_ux_set_mouse_connected(false);
    blu2usb_ux_set_keyboard_connected(false);
    blu2usb_ux_clear_keyboard_pair_code();
}

static void project_physical(const blu2usb_ux_model_t *ux,
                             blu2usb_ui_frame_t *frame)
{
    blu2usb_ui_project(ux, frame);
    blu2usb_ui_enforce_applied_visual_contract(ux, frame);
}

static void assert_row_text(const blu2usb_ui_frame_t *frame,
                            unsigned row,
                            const char *expected)
{
    char actual[BLU2USB_RENDERER_TEXT_COLS + 1u];
    unsigned end = BLU2USB_RENDERER_TEXT_COLS;
    for (unsigned column = 0u; column < BLU2USB_RENDERER_TEXT_COLS; ++column)
        actual[column] = frame->cells[row][column].character;
    while (end > 0u && actual[end - 1u] == ' ') --end;
    actual[end] = '\0';
    assert(strcmp(actual, expected) == 0);
}

static void assert_row_tone(const blu2usb_ui_frame_t *frame,
                            unsigned row,
                            blu2usb_ui_tone_t expected)
{
    bool found = false;
    for (unsigned column = 0u; column < BLU2USB_RENDERER_TEXT_COLS; ++column) {
        if (frame->cells[row][column].character == ' ') continue;
        found = true;
        assert(frame->cells[row][column].tone == expected);
    }
    assert(found);
}

static void test_other_devices_status_tracks_keyboard(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ui_frame_t frame;
    init_ux(&ux);
    ux.screen = BLU2USB_SCREEN_OTHER_DEVICES_STATUS;

    project_physical(&ux, &frame);
    assert_row_text(&frame, 1u, "KEYBOARD");
    assert_row_text(&frame, 2u, "NOT CONNECTED");
    assert_row_tone(&frame, 2u, BLU2USB_UI_TONE_STATIC);

    blu2usb_ux_set_keyboard_connected(true);
    project_physical(&ux, &frame);
    assert_row_text(&frame, 2u, "CONNECTED");
    assert_row_tone(&frame, 2u, BLU2USB_UI_TONE_CURRENT);
}

static void test_pair_keyboard_row_cyan_but_selection_white(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ui_frame_t frame;
    init_ux(&ux);
    blu2usb_ux_set_keyboard_connected(true);
    ux.screen = BLU2USB_SCREEN_OTHER_OPTIONS;

    ux.selection = 1u;
    project_physical(&ux, &frame);
    assert_row_tone(&frame, 1u, BLU2USB_UI_TONE_CURRENT);

    ux.selection = 0u;
    project_physical(&ux, &frame);
    assert_row_tone(&frame, 1u, BLU2USB_UI_TONE_EMPHASIZED);
}

static void test_pair_keyboard_pin_projection(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ui_frame_t frame;
    init_ux(&ux);
    ux.screen = BLU2USB_SCREEN_PAIR_KEYBOARD;

    blu2usb_ux_set_keyboard_pair_code(123456u, 6u);
    project_physical(&ux, &frame);
    assert_row_text(&frame, 1u, "TYPE PIN ON KEYBOARD");
    assert_row_text(&frame, 2u, "PIN: 123456");
    assert_row_tone(&frame, 2u, BLU2USB_UI_TONE_CURRENT);
    assert_row_text(&frame, 3u, "THEN PRESS ENTER");
    assert_row_text(&frame, 4u, "PAIRING IN PROGRESS");

    blu2usb_ux_set_keyboard_pair_code(0u, 4u);
    project_physical(&ux, &frame);
    assert_row_text(&frame, 2u, "PIN: 0000");
}

static void test_pair_keyboard_success_body_is_cyan(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ui_frame_t frame;
    init_ux(&ux);
    ux.screen = BLU2USB_SCREEN_KEYBOARD_SAVED;
    project_physical(&ux, &frame);
    assert_row_text(&frame, 0u, "KEYBOARD SAVED");
    assert_row_tone(&frame, 1u, BLU2USB_UI_TONE_CURRENT);
    assert_row_tone(&frame, 2u, BLU2USB_UI_TONE_CURRENT);
}

static void test_keyboard_saved_back_returns_other_options(void)
{
    blu2usb_ux_model_t ux;
    init_ux(&ux);
    ux.screen = BLU2USB_SCREEN_KEYBOARD_SAVED;

    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_KEY_B, true);
    assert(ux.screen == BLU2USB_SCREEN_KEYBOARD_SAVED);
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_KEY_B, false);
    assert(ux.screen == BLU2USB_SCREEN_OTHER_OPTIONS);
}

static void test_pair_progress_preserves_geometry_and_error_stage(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ui_frame_t frame;
    init_ux(&ux);
    ux.screen = BLU2USB_SCREEN_PAIR_KEYBOARD;
    blu2usb_keyboard_pair_progress_t p = {0};
    p.phase = BLU2USB_KEYBOARD_PAIR_START_SEARCH;
    p.attempt = 2; p.found = 1;
    blu2usb_ux_set_keyboard_pair_progress(p);
    project_physical(&ux, &frame);
    assert_row_text(&frame, 1u, "STARTING SEARCH");
    assert_row_text(&frame, 2u, "SEARCH 2 FOUND 1");
    assert_row_text(&frame, 6u, "KEY A: RETRY ON ERROR");
    assert_row_text(&frame, 7u, "KEY B: CANCEL");
    assert_row_text(&frame, 8u, "KEY X: HELP");
    blu2usb_ux_set_keyboard_pair_code(123456, 6);
    p.phase = BLU2USB_KEYBOARD_PAIR_ERROR;
    p.last_phase = BLU2USB_KEYBOARD_PAIR_START_SEARCH;
    p.error = 0xf0;
    blu2usb_ux_set_keyboard_pair_progress(p);
    project_physical(&ux, &frame);
    assert_row_text(&frame, 1u, "KEYBOARD ERROR");
    assert_row_text(&frame, 2u, "STARTING SEARCH");
    assert_row_text(&frame, 3u, "ERROR F0");
    assert_row_text(&frame, 4u, "RETRY OR POWER CYCLE");
}

int main(void)
{
    test_other_devices_status_tracks_keyboard();
    test_pair_keyboard_row_cyan_but_selection_white();
    test_pair_keyboard_pin_projection();
    test_pair_keyboard_success_body_is_cyan();
    test_keyboard_saved_back_returns_other_options();
    test_pair_progress_preserves_geometry_and_error_stage();
    return 0;
}
