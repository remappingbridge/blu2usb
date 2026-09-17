#include <stdio.h>
#include <stdlib.h>

#include "blu2usb/hid_aggregator/hid_aggregator.h"
#include "blu2usb/logitech_hidpp/logitech_hidpp.h"
#include "blu2usb/profiles/profiles.h"
#include "blu2usb/remap/remap.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "CHECK failed %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

static blu2usb_canonical_mouse_event_t button(blu2usb_mouse_button_t b, bool down)
{
    blu2usb_canonical_mouse_event_t e = {0};
    e.source = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);
    e.type = BLU2USB_MOUSE_EVENT_BUTTON;
    e.data.button.button = b;
    e.data.button.pressed = down;
    return e;
}

static void test_profiles(void)
{
    blu2usb_profiles_t p;
    blu2usb_profiles_init(&p);
    CHECK(p.active_kind == BLU2USB_MOUSE_PROFILE_PASSTHROUGH);
    blu2usb_mouse_profile_config_t c;

    CHECK(blu2usb_profiles_build_preset(BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP, &p, &c));
    CHECK(c.targets[BLU2USB_MOUSE_SOURCE_LEFT] == BLU2USB_MOUSE_TARGET_FORWARD);
    CHECK(c.targets[BLU2USB_MOUSE_SOURCE_RIGHT] == BLU2USB_MOUSE_TARGET_BACKWARD);
    CHECK(c.targets[BLU2USB_MOUSE_SOURCE_MIDDLE] == BLU2USB_MOUSE_TARGET_MIDDLE);
    CHECK(c.targets[BLU2USB_MOUSE_SOURCE_FORWARD] == BLU2USB_MOUSE_TARGET_LEFT);
    CHECK(c.targets[BLU2USB_MOUSE_SOURCE_BACKWARD] == BLU2USB_MOUSE_TARGET_RIGHT);
    CHECK(blu2usb_profiles_requires_forward_held_fix(&c));

    CHECK(blu2usb_profiles_build_preset(BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP, &p, &c));
    CHECK(c.targets[BLU2USB_MOUSE_SOURCE_LEFT] == BLU2USB_MOUSE_TARGET_ESCAPE);
    CHECK(c.targets[BLU2USB_MOUSE_SOURCE_RIGHT] == BLU2USB_MOUSE_TARGET_BACKWARD);
    CHECK(c.targets[BLU2USB_MOUSE_SOURCE_MIDDLE] == BLU2USB_MOUSE_TARGET_FORWARD);
    CHECK(c.targets[BLU2USB_MOUSE_SOURCE_FORWARD] == BLU2USB_MOUSE_TARGET_LEFT);
    CHECK(c.targets[BLU2USB_MOUSE_SOURCE_BACKWARD] == BLU2USB_MOUSE_TARGET_RIGHT);

    CHECK(blu2usb_profiles_draft_set(&p, BLU2USB_MOUSE_SOURCE_FORWARD,
                                     BLU2USB_MOUSE_TARGET_ESCAPE));
    CHECK(blu2usb_profiles_custom_candidate(&p, &c));
    CHECK(c.targets[BLU2USB_MOUSE_SOURCE_FORWARD] == BLU2USB_MOUSE_TARGET_ESCAPE);
    blu2usb_profiles_activate(&p, &c);
    CHECK(p.active_kind == BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP);

    uint8_t bytes[BLU2USB_PROFILE_SERIALIZED_SIZE];
    CHECK(blu2usb_profiles_serialize(&p, bytes));
    blu2usb_profiles_t restored;
    blu2usb_profiles_init(&restored);
    CHECK(blu2usb_profiles_restore(&restored, bytes));
    blu2usb_profiles_configure_active(&restored, &c);
    CHECK(c.targets[BLU2USB_MOUSE_SOURCE_FORWARD] == BLU2USB_MOUSE_TARGET_ESCAPE);
    bytes[7] ^= 1u;
    CHECK(!blu2usb_profiles_restore(&restored, bytes));
}

static void test_remap(void)
{
    blu2usb_profiles_t p;
    blu2usb_profiles_init(&p);
    blu2usb_mouse_profile_config_t c;
    blu2usb_remap_t remap;
    blu2usb_remap_init(&remap);
    blu2usb_hid_aggregator_t agg;
    blu2usb_hid_aggregator_init(&agg);
    blu2usb_remap_result_t r;

    CHECK(blu2usb_profiles_build_preset(BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP, &p, &c));
    blu2usb_remap_set_profile(&remap, &c);

    blu2usb_canonical_mouse_event_t e = button(BLU2USB_MOUSE_BUTTON_LEFT, true);
    CHECK(blu2usb_remap_process_mouse(&remap, &e, &r));
    CHECK(r.has_mouse && !r.has_keyboard);
    CHECK(r.mouse.data.button.button == BLU2USB_MOUSE_BUTTON_FORWARD);

    e = button(BLU2USB_MOUSE_BUTTON_FORWARD, true);
    CHECK(blu2usb_remap_process_mouse(&remap, &e, &r));
    CHECK(r.has_mouse && r.mouse.data.button.button == BLU2USB_MOUSE_BUTTON_LEFT);
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &r.mouse));
    e = button(BLU2USB_MOUSE_BUTTON_FORWARD, false);
    CHECK(blu2usb_remap_process_mouse(&remap, &e, &r));
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &r.mouse));

    CHECK(blu2usb_profiles_build_preset(BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP, &p, &c));
    blu2usb_remap_set_profile(&remap, &c);
    e = button(BLU2USB_MOUSE_BUTTON_LEFT, true);
    CHECK(blu2usb_remap_process_mouse(&remap, &e, &r));
    CHECK(!r.has_mouse && r.has_keyboard && r.keyboard.data.key.key == BLU2USB_KEY_ESCAPE);
    CHECK(blu2usb_hid_aggregator_apply_keyboard(&agg, &r.keyboard));
    blu2usb_hid_output_state_t out;
    blu2usb_hid_aggregator_snapshot(&agg, &out);
    CHECK(blu2usb_hid_output_key_is_down(&out, BLU2USB_KEY_ESCAPE));

    e = button(BLU2USB_MOUSE_BUTTON_LEFT, false);
    CHECK(blu2usb_remap_process_mouse(&remap, &e, &r));
    CHECK(blu2usb_hid_aggregator_apply_keyboard(&agg, &r.keyboard));
    blu2usb_hid_aggregator_snapshot(&agg, &out);
    CHECK(!blu2usb_hid_output_key_is_down(&out, BLU2USB_KEY_ESCAPE));
    CHECK(!blu2usb_hid_output_mouse_button_is_down(&out, BLU2USB_MOUSE_BUTTON_LEFT));
}

static void test_hidpp(void)
{
    blu2usb_logitech_hidpp_t h;
    blu2usb_logitech_hidpp_init(&h);
    blu2usb_logitech_hidpp_set_forward_desired(&h, true);
    blu2usb_logitech_hidpp_on_connect(&h);
    blu2usb_hidpp_output_t o;
    CHECK(blu2usb_logitech_hidpp_next_output(&h, &o));
    CHECK(o.kind == BLU2USB_HIDPP_OUTPUT_GET_FEATURE && o.report_id == 0x11u);
    CHECK(o.payload[3] == 0x1bu && o.payload[4] == 0x04u);
    blu2usb_logitech_hidpp_output_result(&h, o.kind, true);

    uint8_t feature[BLU2USB_HIDPP_LONG_PAYLOAD_SIZE] = {0};
    feature[0] = 0xffu; feature[1] = 0x00u; feature[2] = 0x02u; feature[3] = 0x07u;
    blu2usb_hidpp_input_result_t ir;
    CHECK(blu2usb_logitech_hidpp_process_input(&h, 0x11u, feature, sizeof(feature), &ir));
    CHECK(h.feature_index == 0x07u);
    CHECK(blu2usb_logitech_hidpp_next_output(&h, &o));
    CHECK(o.kind == BLU2USB_HIDPP_OUTPUT_SET_FORWARD_DIVERT);
    CHECK(o.payload[3] == 0x00u && o.payload[4] == 0x56u && o.payload[5] == 0x03u);
    blu2usb_logitech_hidpp_output_result(&h, o.kind, true);

    uint8_t ack[BLU2USB_HIDPP_LONG_PAYLOAD_SIZE] = {0};
    ack[0] = 0xffu; ack[1] = 0x07u; ack[2] = 0x32u;
    CHECK(blu2usb_logitech_hidpp_process_input(&h, 0x11u, ack, sizeof(ack), &ir));
    CHECK(blu2usb_logitech_hidpp_claims_forward(&h));

    uint8_t held[BLU2USB_HIDPP_LONG_PAYLOAD_SIZE] = {0};
    held[0] = 0xffu; held[1] = 0x07u; held[2] = 0x00u; held[3] = 0x00u; held[4] = 0x56u;
    CHECK(blu2usb_logitech_hidpp_process_input(&h, 0x11u, held, sizeof(held), &ir));
    CHECK(ir.held_changed && ir.forward_held);
    uint8_t released[BLU2USB_HIDPP_LONG_PAYLOAD_SIZE] = {0};
    released[0] = 0xffu; released[1] = 0x07u; released[2] = 0x00u;
    CHECK(blu2usb_logitech_hidpp_process_input(&h, 0x11u, released, sizeof(released), &ir));
    CHECK(ir.held_changed && !ir.forward_held);
}

int main(void)
{
    test_profiles();
    test_remap();
    test_hidpp();
    puts("G06 profiles/remap/HID++ tests passed");
    return 0;
}