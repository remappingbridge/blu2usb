#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blu2usb/bt_runtime/bt_runtime.h"
#include "blu2usb/classic_hid/classic_hid.h"
#include "blu2usb/hid_aggregator/hid_aggregator.h"
#include "blu2usb/keyboard_transport/keyboard_transport.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "CHECK failed %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

typedef struct {
    blu2usb_hid_aggregator_t *aggregator;
    unsigned count;
} emit_context_t;

static bool apply_event(void *context, const blu2usb_canonical_keyboard_event_t *event)
{
    emit_context_t *ctx = (emit_context_t *)context;
    ++ctx->count;
    return blu2usb_hid_aggregator_apply_keyboard(ctx->aggregator, event);
}

static void test_state_diff_and_synthetic_coexistence(void)
{
    blu2usb_hid_aggregator_t aggregator;
    blu2usb_hid_aggregator_init(&aggregator);

    blu2usb_classic_hid_keyboard_state_t previous;
    blu2usb_classic_hid_keyboard_state_t next;
    blu2usb_classic_hid_keyboard_state_clear(&previous);
    blu2usb_classic_hid_keyboard_state_clear(&next);
    CHECK(blu2usb_classic_hid_keyboard_state_set_modifier(
        &next, BLU2USB_MOD_LEFT_SHIFT, true));
    CHECK(blu2usb_classic_hid_keyboard_state_set_key(&next, BLU2USB_KEY_A, true));

    emit_context_t ctx = {&aggregator, 0u};
    const blu2usb_hid_source_t physical =
        blu2usb_hid_source_make(BLU2USB_HID_SOURCE_KEYBOARD, 1u);
    CHECK(blu2usb_classic_hid_emit_state_diff(
        &previous, &next, physical, apply_event, &ctx));
    CHECK(ctx.count == 2u);

    blu2usb_hid_output_state_t output;
    blu2usb_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_A));
    CHECK((output.modifiers & (1u << BLU2USB_MOD_LEFT_SHIFT)) != 0u);

    const blu2usb_hid_source_t synthetic =
        blu2usb_hid_source_make(BLU2USB_HID_SOURCE_SYNTHETIC_REMAP, 1u);
    blu2usb_canonical_keyboard_event_t escape = {0};
    escape.source = synthetic;
    escape.type = BLU2USB_KEYBOARD_EVENT_KEY;
    escape.data.key.key = BLU2USB_KEY_ESCAPE;
    escape.data.key.pressed = true;
    CHECK(blu2usb_hid_aggregator_apply_keyboard(&aggregator, &escape));

    blu2usb_classic_hid_keyboard_state_clear(&next);
    ctx.count = 0u;
    CHECK(blu2usb_classic_hid_emit_state_diff(
        &previous, &next, physical, apply_event, &ctx));
    CHECK(ctx.count == 2u);

    blu2usb_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(!blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_A));
    CHECK(blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_ESCAPE));
    CHECK(output.modifiers == 0u);

    CHECK(blu2usb_hid_aggregator_release_source(&aggregator, physical));
    blu2usb_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_ESCAPE));

    escape.data.key.pressed = false;
    CHECK(blu2usb_hid_aggregator_apply_keyboard(&aggregator, &escape));
    blu2usb_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(!blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_ESCAPE));
}

static void test_keyboard_transport_decodes_classic_messages(void)
{
    blu2usb_bt_runtime_message_t message;
    memset(&message, 0, sizeof(message));
    message.channel = BLU2USB_CLASSIC_HID_RUNTIME_CHANNEL;

    message.type = BLU2USB_CLASSIC_HID_MESSAGE_CONNECTED;
    blu2usb_keyboard_transport_event_t event;
    CHECK(blu2usb_keyboard_transport_decode_runtime_message(&message, &event));
    CHECK(event.type == BLU2USB_KEYBOARD_TRANSPORT_EVENT_CONNECTED);

    blu2usb_canonical_keyboard_event_t key = {0};
    key.source = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_KEYBOARD, 1u);
    key.type = BLU2USB_KEYBOARD_EVENT_KEY;
    key.data.key.key = BLU2USB_KEY_ENTER;
    key.data.key.pressed = true;
    message.type = BLU2USB_CLASSIC_HID_MESSAGE_KEYBOARD;
    message.length = sizeof(key);
    memcpy(message.payload, &key, sizeof(key));
    CHECK(blu2usb_keyboard_transport_decode_runtime_message(&message, &event));
    CHECK(event.type == BLU2USB_KEYBOARD_TRANSPORT_EVENT_KEYBOARD);
    CHECK(event.keyboard.data.key.key == BLU2USB_KEY_ENTER);
    CHECK(event.keyboard.data.key.pressed);

    blu2usb_classic_hid_pair_code_t code = {123456u, 6u};
    message.type = BLU2USB_CLASSIC_HID_MESSAGE_PAIR_CODE;
    message.length = sizeof(code);
    memcpy(message.payload, &code, sizeof(code));
    CHECK(blu2usb_keyboard_transport_decode_runtime_message(&message, &event));
    CHECK(event.type == BLU2USB_KEYBOARD_TRANSPORT_EVENT_PAIR_CODE);
    CHECK(event.pair_code.value == 123456u);
    CHECK(event.pair_code.digits == 6u);

    code.value = 0u;
    code.digits = 4u;
    memcpy(message.payload, &code, sizeof(code));
    CHECK(blu2usb_keyboard_transport_decode_runtime_message(&message, &event));
    CHECK(event.pair_code.digits == 4u);
    blu2usb_keyboard_pair_progress_t progress = {0};
    progress.phase = BLU2USB_KEYBOARD_PAIR_SEARCHING;
    progress.attempt = 3;
    message.type = BLU2USB_CLASSIC_HID_MESSAGE_PROGRESS;
    message.length = sizeof(progress);
    memcpy(message.payload, &progress, sizeof(progress));
    CHECK(blu2usb_keyboard_transport_decode_runtime_message(&message, &event));
    CHECK(event.type == BLU2USB_KEYBOARD_TRANSPORT_EVENT_PROGRESS);
    CHECK(event.progress.attempt == 3);
    --message.length;
    CHECK(!blu2usb_keyboard_transport_decode_runtime_message(&message, &event));
    message.length = sizeof(progress);
    progress.phase = 255;
    memcpy(message.payload, &progress, sizeof(progress));
    CHECK(!blu2usb_keyboard_transport_decode_runtime_message(&message, &event));
}

int main(void)
{
    CHECK(!blu2usb_keyboard_transport_progress_expired(90099u, 100u));
    CHECK(blu2usb_keyboard_transport_progress_expired(90100u, 100u));
    CHECK(!blu2usb_keyboard_transport_progress_expired(50u, UINT32_MAX - 50u));
    CHECK(blu2usb_keyboard_transport_progress_expired(90000u, UINT32_MAX - 50u));
    test_state_diff_and_synthetic_coexistence();
    test_keyboard_transport_decodes_classic_messages();
    puts("G07 Keyboard transport tests passed");
    return 0;
}
