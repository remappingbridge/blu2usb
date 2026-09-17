#include <stdio.h>
#include <stdlib.h>

#include "blu2usb/hid_aggregator/hid_aggregator.h"

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        exit(1); \
    } \
} while (0)

static blu2usb_canonical_mouse_event_t mouse_button(
    blu2usb_hid_source_t source,
    blu2usb_mouse_button_t button,
    bool pressed)
{
    blu2usb_canonical_mouse_event_t event = {0};
    event.source = source;
    event.type = BLU2USB_MOUSE_EVENT_BUTTON;
    event.data.button.button = button;
    event.data.button.pressed = pressed;
    return event;
}

static blu2usb_canonical_keyboard_event_t keyboard_key(
    blu2usb_hid_source_t source,
    blu2usb_key_t key,
    bool pressed)
{
    blu2usb_canonical_keyboard_event_t event = {0};
    event.source = source;
    event.type = BLU2USB_KEYBOARD_EVENT_KEY;
    event.data.key.key = key;
    event.data.key.pressed = pressed;
    return event;
}

static blu2usb_canonical_keyboard_event_t keyboard_modifier(
    blu2usb_hid_source_t source,
    blu2usb_modifier_t modifier,
    bool pressed)
{
    blu2usb_canonical_keyboard_event_t event = {0};
    event.source = source;
    event.type = BLU2USB_KEYBOARD_EVENT_MODIFIER;
    event.data.modifier.modifier = modifier;
    event.data.modifier.pressed = pressed;
    return event;
}

static void test_source_identity_validation(void)
{
    const blu2usb_hid_source_t invalid = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_INVALID, 0u);
    const blu2usb_hid_source_t mouse = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 0u);
    const blu2usb_hid_source_t keyboard = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_KEYBOARD, 1u);
    const blu2usb_hid_source_t composite = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_COMPOSITE, 2u);
    const blu2usb_hid_source_t synthetic = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_SYNTHETIC_REMAP, 3u);
    blu2usb_hid_source_t unknown = mouse;
    unknown.kind = (uint16_t)BLU2USB_HID_SOURCE_KIND_COUNT;

    CHECK(!blu2usb_hid_source_is_valid(invalid));
    CHECK(blu2usb_hid_source_is_valid(mouse));
    CHECK(blu2usb_hid_source_is_valid(keyboard));
    CHECK(blu2usb_hid_source_is_valid(composite));
    CHECK(blu2usb_hid_source_is_valid(synthetic));
    CHECK(!blu2usb_hid_source_is_valid(unknown));
    CHECK(blu2usb_hid_source_equal(mouse, blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 0u)));
    CHECK(!blu2usb_hid_source_equal(mouse, keyboard));
}

static void test_shared_mouse_ownership_and_idempotence(void)
{
    blu2usb_hid_aggregator_t agg;
    blu2usb_hid_output_state_t output;
    blu2usb_hid_aggregator_init(&agg);
    const blu2usb_hid_source_t mouse = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);
    const blu2usb_hid_source_t composite = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_COMPOSITE, 1u);

    blu2usb_canonical_mouse_event_t event = mouse_button(mouse, BLU2USB_MOUSE_BUTTON_LEFT, true);
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &event));
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &event));
    CHECK(agg.mouse_button_refs[BLU2USB_MOUSE_BUTTON_LEFT] == 1u);

    event = mouse_button(composite, BLU2USB_MOUSE_BUTTON_LEFT, true);
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &event));
    CHECK(agg.mouse_button_refs[BLU2USB_MOUSE_BUTTON_LEFT] == 2u);

    event = mouse_button(mouse, BLU2USB_MOUSE_BUTTON_LEFT, false);
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &event));
    blu2usb_hid_aggregator_snapshot(&agg, &output);
    CHECK(blu2usb_hid_output_mouse_button_is_down(&output, BLU2USB_MOUSE_BUTTON_LEFT));

    event = mouse_button(composite, BLU2USB_MOUSE_BUTTON_LEFT, false);
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &event));
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &event));
    CHECK(agg.mouse_button_refs[BLU2USB_MOUSE_BUTTON_LEFT] == 0u);
    blu2usb_hid_aggregator_snapshot(&agg, &output);
    CHECK(!blu2usb_hid_output_mouse_button_is_down(&output, BLU2USB_MOUSE_BUTTON_LEFT));
}

static void test_keyboard_key_and_modifier_ownership(void)
{
    blu2usb_hid_aggregator_t agg;
    blu2usb_hid_output_state_t output;
    blu2usb_hid_aggregator_init(&agg);
    const blu2usb_hid_source_t keyboard = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_KEYBOARD, 1u);
    const blu2usb_hid_source_t composite = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_COMPOSITE, 1u);

    blu2usb_canonical_keyboard_event_t event = keyboard_key(keyboard, BLU2USB_KEY_A, true);
    CHECK(blu2usb_hid_aggregator_apply_keyboard(&agg, &event));
    CHECK(blu2usb_hid_aggregator_apply_keyboard(&agg, &event));
    CHECK(agg.key_refs[BLU2USB_KEY_A] == 1u);

    event = keyboard_key(composite, BLU2USB_KEY_A, true);
    CHECK(blu2usb_hid_aggregator_apply_keyboard(&agg, &event));
    CHECK(agg.key_refs[BLU2USB_KEY_A] == 2u);

    event = keyboard_modifier(keyboard, BLU2USB_MOD_LEFT_SHIFT, true);
    CHECK(blu2usb_hid_aggregator_apply_keyboard(&agg, &event));
    event = keyboard_modifier(composite, BLU2USB_MOD_LEFT_SHIFT, true);
    CHECK(blu2usb_hid_aggregator_apply_keyboard(&agg, &event));
    CHECK(agg.modifier_refs[BLU2USB_MOD_LEFT_SHIFT] == 2u);

    CHECK(blu2usb_hid_aggregator_release_source(&agg, keyboard));
    blu2usb_hid_aggregator_snapshot(&agg, &output);
    CHECK(blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_A));
    CHECK((output.modifiers & (uint8_t)(1u << BLU2USB_MOD_LEFT_SHIFT)) != 0u);

    CHECK(blu2usb_hid_aggregator_release_source(&agg, composite));
    blu2usb_hid_aggregator_snapshot(&agg, &output);
    CHECK(!blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_A));
    CHECK(output.modifiers == 0u);
}

static void test_disconnect_isolation(void)
{
    blu2usb_hid_aggregator_t agg;
    blu2usb_hid_output_state_t output;
    blu2usb_hid_aggregator_init(&agg);
    const blu2usb_hid_source_t mouse = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 7u);
    const blu2usb_hid_source_t composite = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_COMPOSITE, 9u);

    blu2usb_canonical_mouse_event_t event = mouse_button(mouse, BLU2USB_MOUSE_BUTTON_LEFT, true);
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &event));
    event = mouse_button(mouse, BLU2USB_MOUSE_BUTTON_RIGHT, true);
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &event));
    event = mouse_button(composite, BLU2USB_MOUSE_BUTTON_LEFT, true);
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &event));

    CHECK(blu2usb_hid_aggregator_release_source(&agg, mouse));
    blu2usb_hid_aggregator_snapshot(&agg, &output);
    CHECK(blu2usb_hid_output_mouse_button_is_down(&output, BLU2USB_MOUSE_BUTTON_LEFT));
    CHECK(!blu2usb_hid_output_mouse_button_is_down(&output, BLU2USB_MOUSE_BUTTON_RIGHT));
    CHECK(blu2usb_hid_aggregator_release_source(&agg, mouse));
}

static void test_synthetic_coexists_with_physical_keyboard(void)
{
    blu2usb_hid_aggregator_t agg;
    blu2usb_hid_output_state_t output;
    blu2usb_hid_aggregator_init(&agg);
    const blu2usb_hid_source_t physical = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_KEYBOARD, 1u);
    const blu2usb_hid_source_t synthetic = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_SYNTHETIC_REMAP, 1u);

    blu2usb_canonical_keyboard_event_t event = keyboard_key(physical, BLU2USB_KEY_ESCAPE, true);
    CHECK(blu2usb_hid_aggregator_apply_keyboard(&agg, &event));
    event = keyboard_key(synthetic, BLU2USB_KEY_ESCAPE, true);
    CHECK(blu2usb_hid_aggregator_apply_keyboard(&agg, &event));

    CHECK(blu2usb_hid_aggregator_release_source(&agg, synthetic));
    blu2usb_hid_aggregator_snapshot(&agg, &output);
    CHECK(blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_ESCAPE));

    event = keyboard_key(physical, BLU2USB_KEY_ESCAPE, false);
    CHECK(blu2usb_hid_aggregator_apply_keyboard(&agg, &event));
    blu2usb_hid_aggregator_snapshot(&agg, &output);
    CHECK(!blu2usb_hid_output_key_is_down(&output, BLU2USB_KEY_ESCAPE));
}

static void test_relative_aggregation_and_consumption(void)
{
    blu2usb_hid_aggregator_t agg;
    blu2usb_hid_output_state_t output;
    blu2usb_hid_aggregator_init(&agg);
    const blu2usb_hid_source_t mouse = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);
    const blu2usb_hid_source_t composite = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_COMPOSITE, 1u);

    blu2usb_canonical_mouse_event_t move = {0};
    move.source = mouse;
    move.type = BLU2USB_MOUSE_EVENT_MOVE;
    move.data.move.dx = 12;
    move.data.move.dy = -3;
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &move));
    move.source = composite;
    move.data.move.dx = -2;
    move.data.move.dy = 8;
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &move));

    blu2usb_canonical_mouse_event_t wheel = {0};
    wheel.source = mouse;
    wheel.type = BLU2USB_MOUSE_EVENT_WHEEL;
    wheel.data.wheel.vertical = 2;
    wheel.data.wheel.horizontal = -1;
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &wheel));

    blu2usb_hid_aggregator_snapshot(&agg, &output);
    CHECK(output.dx == 10 && output.dy == 5);
    CHECK(output.wheel_vertical == 2 && output.wheel_horizontal == -1);

    blu2usb_hid_aggregator_take_output(&agg, &output);
    CHECK(output.dx == 10 && output.dy == 5);
    CHECK(output.wheel_vertical == 2 && output.wheel_horizontal == -1);
    blu2usb_hid_aggregator_snapshot(&agg, &output);
    CHECK(output.dx == 0 && output.dy == 0);
    CHECK(output.wheel_vertical == 0 && output.wheel_horizontal == 0);
}

static void test_capacity_relative_events_and_slot_reuse(void)
{
    blu2usb_hid_aggregator_t agg;
    blu2usb_hid_output_state_t output;
    blu2usb_hid_aggregator_init(&agg);

    for (uint16_t instance = 0u; instance < BLU2USB_HID_AGGREGATOR_MAX_SOURCES; ++instance) {
        const blu2usb_hid_source_t source = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, instance);
        blu2usb_canonical_mouse_event_t event = mouse_button(source, BLU2USB_MOUSE_BUTTON_LEFT, true);
        CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &event));
    }

    const blu2usb_hid_source_t overflow = blu2usb_hid_source_make(
        BLU2USB_HID_SOURCE_MOUSE, BLU2USB_HID_AGGREGATOR_MAX_SOURCES);
    blu2usb_canonical_mouse_event_t event = mouse_button(overflow, BLU2USB_MOUSE_BUTTON_RIGHT, true);
    CHECK(!blu2usb_hid_aggregator_apply_mouse(&agg, &event));

    blu2usb_canonical_mouse_event_t move = {0};
    move.source = overflow;
    move.type = BLU2USB_MOUSE_EVENT_MOVE;
    move.data.move.dx = 4;
    move.data.move.dy = -6;
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &move));
    blu2usb_hid_aggregator_snapshot(&agg, &output);
    CHECK(output.dx == 4 && output.dy == -6);

    const blu2usb_hid_source_t released = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 3u);
    CHECK(blu2usb_hid_aggregator_release_source(&agg, released));
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &event));
}

static void test_invalid_events_are_rejected(void)
{
    blu2usb_hid_aggregator_t agg;
    blu2usb_hid_aggregator_init(&agg);
    const blu2usb_hid_source_t invalid = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_INVALID, 0u);
    const blu2usb_hid_source_t mouse_source = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);

    blu2usb_canonical_mouse_event_t mouse = mouse_button(invalid, BLU2USB_MOUSE_BUTTON_LEFT, true);
    CHECK(!blu2usb_hid_aggregator_apply_mouse(&agg, &mouse));
    blu2usb_canonical_keyboard_event_t key = keyboard_key(invalid, BLU2USB_KEY_A, true);
    CHECK(!blu2usb_hid_aggregator_apply_keyboard(&agg, &key));
    CHECK(!blu2usb_hid_aggregator_release_source(&agg, invalid));

    mouse = mouse_button(mouse_source, (blu2usb_mouse_button_t)BLU2USB_MOUSE_BUTTON_COUNT, true);
    CHECK(!blu2usb_hid_aggregator_apply_mouse(&agg, &mouse));

    key = keyboard_modifier(mouse_source, (blu2usb_modifier_t)BLU2USB_MOD_COUNT, true);
    CHECK(!blu2usb_hid_aggregator_apply_keyboard(&agg, &key));
}

int main(void)
{
    test_source_identity_validation();
    test_shared_mouse_ownership_and_idempotence();
    test_keyboard_key_and_modifier_ownership();
    test_disconnect_isolation();
    test_synthetic_coexists_with_physical_keyboard();
    test_relative_aggregation_and_consumption();
    test_capacity_relative_events_and_slot_reuse();
    test_invalid_events_are_rejected();
    puts("BLU2USB-G05 canonical HID ownership: OK");
    return 0;
}
