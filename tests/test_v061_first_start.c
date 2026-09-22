#include <assert.h>
#include <string.h>

#include "blu2usb/renderer/renderer.h"
#include "blu2usb/ux_model/ux_model.h"

static blu2usb_ux_command_t tap(
    blu2usb_ux_model_t *ux,
    blu2usb_control_t control)
{
    (void)blu2usb_ux_input(ux, control, true);
    return blu2usb_ux_input(ux, control, false);
}

static void row_text(
    const blu2usb_ui_frame_t *frame,
    unsigned row,
    char out[BLU2USB_RENDERER_TEXT_COLS + 1u])
{
    size_t length = BLU2USB_RENDERER_TEXT_COLS;
    while (length > 0u && frame->cells[row][length - 1u].character == ' ')
        --length;

    for (size_t i = 0u; i < length; ++i)
        out[i] = frame->cells[row][i].character;
    out[length] = '\0';
}

static void assert_row(
    const blu2usb_ui_frame_t *frame,
    unsigned row,
    const char *expected)
{
    char actual[BLU2USB_RENDERER_TEXT_COLS + 1u];
    row_text(frame, row, actual);
    assert(strcmp(actual, expected) == 0);
}

static void test_init_contract_and_first_start_opt_in(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ux_init(&ux);

    /* Preserve the v0.6 host API contract. Firmware boot opts in explicitly. */
    assert(ux.screen == BLU2USB_SCREEN_HOME);
    assert(ux.first_start_complete);

    blu2usb_ux_begin_first_start(&ux);
    assert(ux.screen == BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE);
    assert(!ux.first_start_complete);
    assert(ux.selection == 0u);
}

static void test_searching_first_layout_and_didactic_controls(void)
{
    static const char *const rows[BLU2USB_RENDERER_TEXT_ROWS] = {
        "SEARCHING FIRST MOUSE",
        "PRESS TO LEARN KEYS",
        "WHILE WAIT CONNECTION",
        "       JOY UP",
        "  JOY    JOY    JOY",
        "  LEFT  PRESS  RIGHT",
        "      JOY DOWN",
        " KEY A         KEY X",
        " KEY B         KEY Y",
    };

    blu2usb_ux_model_t ux;
    blu2usb_ui_frame_t frame;
    blu2usb_ux_init(&ux);
    blu2usb_ux_begin_first_start(&ux);

    blu2usb_ui_project(&ux, &frame);
    for (unsigned row = 0u; row < BLU2USB_RENDERER_TEXT_ROWS; ++row) {
        assert_row(&frame, row, rows[row]);
        assert(blu2usb_renderer_background_rgb565(&frame, (uint8_t)row) ==
               BLU2USB_COLOR_BLACK);
    }

    assert(frame.cells[1][0].tone == BLU2USB_UI_TONE_STATIC);
    assert(frame.cells[2][0].tone == BLU2USB_UI_TONE_STATIC);

    for (unsigned control = 0u; control < BLU2USB_CONTROL_COUNT; ++control) {
        (void)tap(&ux, (blu2usb_control_t)control);
        assert(ux.screen == BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE);
        assert(!blu2usb_interaction_is_locked(&ux.interaction));
        assert(!ux.first_start_complete);
    }

    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_KEY_Y, true);
    blu2usb_ui_project(&ux, &frame);
    assert(frame.cells[8][15].tone == BLU2USB_UI_TONE_EMPHASIZED);
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_KEY_Y, false);
    assert(!blu2usb_interaction_is_locked(&ux.interaction));
}

static void test_connected_feedback_lock_unlock_to_home(void)
{
    static const char *const rows[BLU2USB_RENDERER_TEXT_ROWS] = {
        "FIRST MOUSE CONNECTED",
        "       JOY UP",
        "  JOY    JOY    JOY",
        "  LEFT  PRESS  RIGHT",
        "      JOY DOWN",
        " KEY A         KEY X",
        " KEY B         KEY Y",
        "",
        " KEY Y: LOCK",
    };

    blu2usb_ux_model_t ux;
    blu2usb_ui_frame_t frame;
    blu2usb_ux_init(&ux);
    blu2usb_ux_begin_first_start(&ux);

    blu2usb_ux_first_mouse_connected(&ux);
    assert(ux.screen == BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED);
    assert(!ux.first_start_complete);

    blu2usb_ui_project(&ux, &frame);
    for (unsigned row = 0u; row < BLU2USB_RENDERER_TEXT_ROWS; ++row)
        assert_row(&frame, row, rows[row]);

    for (unsigned row = 0u; row < 8u; ++row)
        assert(blu2usb_renderer_background_rgb565(&frame, (uint8_t)row) ==
               BLU2USB_COLOR_BLACK);
    assert(blu2usb_renderer_background_rgb565(&frame, 8u) ==
           BLU2USB_COLOR_DARK_MAGENTA);

    const blu2usb_control_t inert[] = {
        BLU2USB_CONTROL_JOY_UP,
        BLU2USB_CONTROL_JOY_DOWN,
        BLU2USB_CONTROL_JOY_LEFT,
        BLU2USB_CONTROL_JOY_RIGHT,
        BLU2USB_CONTROL_JOY_PRESS,
        BLU2USB_CONTROL_KEY_A,
        BLU2USB_CONTROL_KEY_B,
        BLU2USB_CONTROL_KEY_X,
    };
    for (size_t i = 0u; i < sizeof(inert) / sizeof(inert[0]); ++i) {
        (void)tap(&ux, inert[i]);
        assert(ux.screen == BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED);
        assert(!blu2usb_interaction_is_locked(&ux.interaction));
    }

    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_KEY_Y, true);
    blu2usb_ui_project(&ux, &frame);
    assert(frame.cells[6][15].tone == BLU2USB_UI_TONE_EMPHASIZED);
    assert(frame.cells[8][1].tone == BLU2USB_UI_TONE_EMPHASIZED);
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_KEY_Y, false);

    assert(blu2usb_interaction_is_locked(&ux.interaction));
    assert(ux.screen == BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED);

    /* First complete interaction is consumed by unlock and resolves HOME. */
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_JOY_DOWN, true);
    assert(ux.screen == BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED);
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_JOY_DOWN, false);

    assert(!blu2usb_interaction_is_locked(&ux.interaction));
    assert(ux.first_start_complete);
    assert(ux.screen == BLU2USB_SCREEN_HOME);
    assert(ux.selection == 0u);

    /* Once completed, startup transport notifications do not hijack HOME. */
    blu2usb_ux_first_mouse_disconnected(&ux);
    assert(ux.screen == BLU2USB_SCREEN_HOME);
    blu2usb_ux_first_mouse_connected(&ux);
    assert(ux.screen == BLU2USB_SCREEN_HOME);
}

static void test_disconnect_before_home_returns_to_search(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ux_init(&ux);
    blu2usb_ux_begin_first_start(&ux);

    blu2usb_ux_first_mouse_connected(&ux);
    assert(ux.screen == BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED);

    blu2usb_ux_first_mouse_disconnected(&ux);
    assert(ux.screen == BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE);
    assert(!ux.first_start_complete);

    blu2usb_ux_first_mouse_connected(&ux);
    assert(ux.screen == BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED);
}

static void test_disconnect_while_locked_does_not_open_empty_home(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ux_init(&ux);
    blu2usb_ux_begin_first_start(&ux);
    blu2usb_ux_first_mouse_connected(&ux);

    (void)tap(&ux, BLU2USB_CONTROL_KEY_Y);
    assert(blu2usb_interaction_is_locked(&ux.interaction));

    blu2usb_ux_first_mouse_disconnected(&ux);
    assert(ux.screen == BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE);
    assert(!ux.first_start_complete);

    (void)tap(&ux, BLU2USB_CONTROL_KEY_A);
    assert(!blu2usb_interaction_is_locked(&ux.interaction));
    assert(ux.screen == BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE);
    assert(!ux.first_start_complete);
}

int main(void)
{
    test_init_contract_and_first_start_opt_in();
    test_searching_first_layout_and_didactic_controls();
    test_connected_feedback_lock_unlock_to_home();
    test_disconnect_before_home_returns_to_search();
    test_disconnect_while_locked_does_not_open_empty_home();
    return 0;
}
