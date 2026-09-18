#include <assert.h>
#include <string.h>
#include "blu2usb/classic_hid/classic_hid.h"
#include "blu2usb/keyboard_transport/keyboard_transport.h"
#include "blu2usb/hid_aggregator/hid_aggregator.h"
#include "blu2usb/renderer/renderer.h"

static void test_ownership(void)
{
    blu2usb_hid_aggregator_t aggregator;
    blu2usb_hid_aggregator_init(&aggregator);
    blu2usb_canonical_keyboard_event_t synthetic = {
        .source = {BLU2USB_HID_SOURCE_SYNTHETIC_REMAP, 1},
        .type = BLU2USB_KEYBOARD_EVENT_KEY,
        .data.key = {.key = BLU2USB_KEY_ESCAPE, .pressed = true}
    };
    assert(blu2usb_hid_aggregator_apply_keyboard(&aggregator, &synthetic));
    blu2usb_keyboard_snapshot_t physical = {.modifiers=2, .keys={BLU2USB_KEY_ESCAPE, BLU2USB_KEY_A}};
    blu2usb_hid_aggregator_apply_keyboard_snapshot(&aggregator, &physical);
    blu2usb_hid_output_state_t output;
    blu2usb_hid_aggregator_snapshot(&aggregator, &output);
    assert(blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_A));
    assert(output.modifiers == 2);
    const blu2usb_keyboard_snapshot_t released = {0};
    blu2usb_hid_aggregator_apply_keyboard_snapshot(&aggregator, &released);
    blu2usb_hid_aggregator_snapshot(&aggregator, &output);
    assert(!blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_A));
    assert(blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_ESCAPE));
    assert(!output.modifiers);
    // Reverse release order: synthetic disconnect must preserve physical Escape.
    blu2usb_hid_aggregator_apply_keyboard_snapshot(&aggregator, &physical);
    assert(blu2usb_hid_aggregator_release_source(&aggregator, synthetic.source));
    blu2usb_hid_aggregator_snapshot(&aggregator, &output);
    assert(blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_ESCAPE));
}

static blu2usb_ux_command_t press(blu2usb_ux_model_t *ux, blu2usb_control_t control)
{
    assert(blu2usb_ux_input(ux, control, true).kind == BLU2USB_UX_COMMAND_NONE);
    return blu2usb_ux_input(ux, control, false);
}

static void test_ui(void)
{
    blu2usb_ux_model_t ux;
    blu2usb_ux_init(&ux);
    ux.screen = BLU2USB_SCREEN_OTHER_OPTIONS;
    assert(press(&ux, BLU2USB_CONTROL_JOY_PRESS).kind == BLU2USB_UX_COMMAND_PAIR_KEYBOARD);
    assert(ux.screen == BLU2USB_SCREEN_PAIR_KEYBOARD);
    blu2usb_ux_keyboard_status(&ux, BLU2USB_KEYBOARD_READY);
    assert(ux.screen == BLU2USB_SCREEN_KEYBOARD_SAVED);
    blu2usb_ui_frame_t frame;
    blu2usb_ui_project(&ux, &frame);
    blu2usb_ui_enforce_applied_visual_contract(&ux, &frame);
    assert(frame.cells[2][0].character == 'K');
    assert(frame.cells[2][0].tone == BLU2USB_UI_TONE_CURRENT);
    blu2usb_ux_keyboard_status(&ux, BLU2USB_KEYBOARD_ERROR);
    assert(ux.screen == BLU2USB_SCREEN_PAIR_KEYBOARD);
    assert(press(&ux, BLU2USB_CONTROL_KEY_B).kind == BLU2USB_UX_COMMAND_CANCEL_KEYBOARD);
    assert(ux.screen == BLU2USB_SCREEN_OTHER_OPTIONS);
    ux.screen = BLU2USB_SCREEN_PAIR_KEYBOARD_HELP;
    ux.return_screen = BLU2USB_SCREEN_PAIR_KEYBOARD;
    blu2usb_ux_keyboard_status(&ux, BLU2USB_KEYBOARD_READY);
    assert(ux.screen == BLU2USB_SCREEN_PAIR_KEYBOARD_HELP);
    press(&ux, BLU2USB_CONTROL_KEY_X);
    assert(ux.screen == BLU2USB_SCREEN_KEYBOARD_SAVED);
}

int main(void)
{
    test_ownership();
    test_ui();
    blu2usb_bt_runtime_message_t message = {.channel=BLU2USB_KEYBOARD_RUNTIME_CHANNEL,
        .type=BLU2USB_KEYBOARD_MESSAGE_RELEASE};
    blu2usb_keyboard_snapshot_t snapshot;
    memset(&snapshot, 0xff, sizeof(snapshot));
    assert(blu2usb_keyboard_transport_decode(&message, &snapshot));
    assert(!snapshot.modifiers && !snapshot.keys[0]);
    message.channel=99;
    assert(!blu2usb_keyboard_transport_decode(&message, &snapshot));
    return 0;
}
