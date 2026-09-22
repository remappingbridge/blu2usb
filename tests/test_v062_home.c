#include <assert.h>
#include <string.h>

#include "blu2usb/ble_hogp/ble_hogp.h"
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

static void init_v062_connected(blu2usb_ux_model_t *ux)
{
    blu2usb_ux_set_mouse_connected(false);
    blu2usb_ux_init(ux);
    blu2usb_ux_begin_first_start(ux);
    blu2usb_ux_set_mouse_connected(true);
    blu2usb_ux_home_mouse_connected(ux);
    assert(ux->screen == BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED);

    (void)tap(ux, BLU2USB_CONTROL_KEY_Y);
    assert(blu2usb_interaction_is_locked(&ux->interaction));
    (void)tap(ux, BLU2USB_CONTROL_KEY_A);

    assert(!blu2usb_interaction_is_locked(&ux->interaction));
    assert(ux->first_start_complete);
    assert(ux->home_v062_enabled);
    assert(ux->has_saved_mouse);
    assert(ux->screen == BLU2USB_SCREEN_HOME_CONNECTED);
}

static void test_first_start_flows_directly_to_home_connected(void)
{
    blu2usb_ux_model_t ux;
    init_v062_connected(&ux);
    assert(ux.selection == 0u);
}

static void test_home_connected_layout_profiles_and_navigation(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ui_frame_t frame;
    init_v062_connected(&ux);

    blu2usb_ui_project(&ux, &frame);
    assert_row(&frame, 0u, "MOUSE");
    assert_row(&frame, 1u, " PASSTHROUGH");
    assert_row(&frame, 2u, " SAVED DEVICES");
    assert_row(&frame, 3u, " PAIR NEW MOUSE");
    assert_row(&frame, 4u, " LEARN THE KEYS");
    assert_row(&frame, 6u, "JOY UP / DOWN: SELECT");
    assert_row(&frame, 7u, "JOY PRESS: ACCESS");
    assert_row(&frame, 8u, "KEY X: HELP TO REMOVE");

    assert(frame.cells[1][1].tone == BLU2USB_UI_TONE_EMPHASIZED);
    assert(frame.cells[2][1].tone == BLU2USB_UI_TONE_ACTIONABLE);
    assert(frame.cells[3][1].tone == BLU2USB_UI_TONE_ACTIONABLE);
    assert(frame.cells[4][1].tone == BLU2USB_UI_TONE_ACTIONABLE);

    ux.active_profile = BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP;
    blu2usb_ui_project(&ux, &frame);
    assert_row(&frame, 1u, " REMAPPED TO STANDARD");

    ux.active_profile = BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP;
    blu2usb_ui_project(&ux, &frame);
    assert_row(&frame, 1u, " REMAPPED TO ESCAPE");

    ux.active_profile = BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP;
    blu2usb_ui_project(&ux, &frame);
    assert_row(&frame, 1u, " REMAPPED TO CUSTOM");

    ux.active_profile = BLU2USB_MOUSE_PROFILE_PASSTHROUGH;
    (void)tap(&ux, BLU2USB_CONTROL_JOY_DOWN);
    assert(ux.selection == 1u);
    (void)tap(&ux, BLU2USB_CONTROL_JOY_UP);
    assert(ux.selection == 0u);
    (void)tap(&ux, BLU2USB_CONTROL_JOY_UP);
    assert(ux.selection == 3u);

    ux.selection = 0u;
    (void)tap(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_OPTIONS);
    assert(ux.return_screen == BLU2USB_SCREEN_HOME_CONNECTED);

    blu2usb_ux_command_t cmd = tap(&ux, BLU2USB_CONTROL_KEY_B);
    assert(cmd.kind == BLU2USB_UX_COMMAND_NONE);
    assert(ux.screen == BLU2USB_SCREEN_HOME_CONNECTED);
}

static void test_disconnect_search_cancel_retry_and_reconnect(void)
{
    blu2usb_ux_model_t ux;
    init_v062_connected(&ux);

    blu2usb_ux_set_mouse_connected(false);
    blu2usb_ux_home_mouse_disconnected(&ux);
    assert(ux.screen == BLU2USB_SCREEN_HOME_SEARCHING);
    assert(!ux.saved_search_failed);

    blu2usb_ux_command_t cmd = tap(&ux, BLU2USB_CONTROL_KEY_B);
    assert(cmd.kind == BLU2USB_UX_COMMAND_CANCEL_SAVED_SEARCH);
    assert(ux.screen == BLU2USB_SCREEN_HOME_RETRY);
    assert(ux.saved_search_failed);

    cmd = tap(&ux, BLU2USB_CONTROL_KEY_A);
    assert(cmd.kind == BLU2USB_UX_COMMAND_RETRY_SAVED_SEARCH);
    assert(ux.screen == BLU2USB_SCREEN_HOME_SEARCHING);
    assert(!ux.saved_search_failed);

    blu2usb_ux_home_saved_search_timeout(&ux);
    assert(ux.screen == BLU2USB_SCREEN_HOME_RETRY);
    assert(ux.saved_search_failed);

    blu2usb_ux_set_mouse_connected(true);
    blu2usb_ux_home_mouse_connected(&ux);
    assert(ux.screen == BLU2USB_SCREEN_HOME_CONNECTED);
    assert(!ux.saved_search_failed);
}

static void test_home_searching_and_retry_exact_layouts(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ui_frame_t frame;
    init_v062_connected(&ux);

    blu2usb_ux_set_mouse_connected(false);
    blu2usb_ux_home_mouse_disconnected(&ux);
    blu2usb_ui_project(&ux, &frame);

    static const char *const searching[9] = {
        "SEARCHING SAVED MOUSE",
        " SAVED DEVICES",
        " PAIR NEW MOUSE",
        " LEARN THE KEYS",
        "",
        "KEY B: CANCEL SEARCH",
        "JOY UP / DOWN: SELECT",
        "JOY PRESS: ACCESS",
        "KEY X: HELP",
    };
    for (unsigned row = 0u; row < 9u; ++row)
        assert_row(&frame, row, searching[row]);

    blu2usb_ux_home_saved_search_timeout(&ux);
    blu2usb_ui_project(&ux, &frame);

    static const char *const retry[9] = {
        "DEVICE NOT FOUND",
        " SAVED DEVICES",
        " PAIR NEW MOUSE",
        " LEARN THE KEYS",
        "",
        "KEY A: RETRY SEARCH",
        "JOY UP / DOWN: SELECT",
        "JOY PRESS: ACCESS",
        "KEY X: HELP",
    };
    for (unsigned row = 0u; row < 9u; ++row)
        assert_row(&frame, row, retry[row]);
}

static void test_search_help_cancels_to_retry(void)
{
    blu2usb_ux_model_t ux;
    init_v062_connected(&ux);

    blu2usb_ux_set_mouse_connected(false);
    blu2usb_ux_home_mouse_disconnected(&ux);

    blu2usb_ux_command_t cmd = tap(&ux, BLU2USB_CONTROL_KEY_X);
    assert(cmd.kind == BLU2USB_UX_COMMAND_CANCEL_SAVED_SEARCH);
    assert(ux.screen == BLU2USB_SCREEN_HOME_SEARCHING_HELP);
    assert(ux.return_screen == BLU2USB_SCREEN_HOME_RETRY);

    cmd = tap(&ux, BLU2USB_CONTROL_KEY_Y);
    assert(cmd.kind == BLU2USB_UX_COMMAND_NONE);
    assert(ux.screen == BLU2USB_SCREEN_HOME_RETRY);
    assert(!blu2usb_interaction_is_locked(&ux.interaction));
}

static void test_internal_page_preserves_timeout_result_on_back(void)
{
    blu2usb_ux_model_t ux;
    init_v062_connected(&ux);

    blu2usb_ux_set_mouse_connected(false);
    blu2usb_ux_home_mouse_disconnected(&ux);
    assert(ux.screen == BLU2USB_SCREEN_HOME_SEARCHING);

    (void)tap(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_SAVED_DEVICES);

    blu2usb_ux_home_saved_search_timeout(&ux);
    assert(ux.screen == BLU2USB_SCREEN_SAVED_DEVICES);
    assert(ux.saved_search_failed);

    blu2usb_ux_command_t cmd = tap(&ux, BLU2USB_CONTROL_KEY_B);
    assert(cmd.kind == BLU2USB_UX_COMMAND_NONE);
    assert(ux.screen == BLU2USB_SCREEN_HOME_RETRY);
}

static void test_pair_mouse_back_restarts_saved_search(void)
{
    blu2usb_ux_model_t ux;
    init_v062_connected(&ux);

    blu2usb_ux_set_mouse_connected(false);
    blu2usb_ux_home_mouse_disconnected(&ux);

    (void)tap(&ux, BLU2USB_CONTROL_JOY_DOWN);
    assert(ux.selection == 1u);
    blu2usb_ux_command_t cmd = tap(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(cmd.kind == BLU2USB_UX_COMMAND_PAIR_MOUSE);
    assert(ux.screen == BLU2USB_SCREEN_PAIR_MOUSE);

    cmd = tap(&ux, BLU2USB_CONTROL_KEY_B);
    assert(cmd.kind == BLU2USB_UX_COMMAND_RETRY_SAVED_SEARCH);
    assert(ux.screen == BLU2USB_SCREEN_HOME_SEARCHING);
}

static void test_lock_unlock_resolves_and_retries_saved(void)
{
    blu2usb_ux_model_t ux;
    init_v062_connected(&ux);

    blu2usb_ux_set_mouse_connected(false);
    blu2usb_ux_home_mouse_disconnected(&ux);
    blu2usb_ux_home_saved_search_timeout(&ux);
    assert(ux.screen == BLU2USB_SCREEN_HOME_RETRY);

    (void)tap(&ux, BLU2USB_CONTROL_KEY_Y);
    assert(blu2usb_interaction_is_locked(&ux.interaction));

    blu2usb_ux_command_t cmd = tap(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(!blu2usb_interaction_is_locked(&ux.interaction));
    assert(cmd.kind == BLU2USB_UX_COMMAND_RETRY_SAVED_SEARCH);
    assert(ux.screen == BLU2USB_SCREEN_HOME_SEARCHING);
}

static void test_timeout_runtime_message(void)
{
    blu2usb_bt_runtime_message_t message;
    memset(&message, 0, sizeof(message));
    message.channel = BLU2USB_BLE_HOGP_RUNTIME_CHANNEL;
    message.type = BLU2USB_BLE_HOGP_MESSAGE_SAVED_SEARCH_TIMEOUT;
    message.length = 0u;

    blu2usb_ble_hogp_event_t event;
    assert(blu2usb_ble_hogp_decode_runtime_message(&message, &event));
    assert(event.type == BLU2USB_BLE_HOGP_EVENT_SAVED_SEARCH_TIMEOUT);

    message.length = 1u;
    assert(!blu2usb_ble_hogp_decode_runtime_message(&message, &event));
}

int main(void)
{
    test_first_start_flows_directly_to_home_connected();
    test_home_connected_layout_profiles_and_navigation();
    test_disconnect_search_cancel_retry_and_reconnect();
    test_home_searching_and_retry_exact_layouts();
    test_search_help_cancels_to_retry();
    test_internal_page_preserves_timeout_result_on_back();
    test_pair_mouse_back_restarts_saved_search();
    test_lock_unlock_resolves_and_retries_saved();
    test_timeout_runtime_message();
    return 0;
}
