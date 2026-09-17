#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blu2usb/ble_hogp/ble_hogp.h"
#include "blu2usb/bt_runtime/bt_runtime.h"
#include "blu2usb/hid_aggregator/hid_aggregator.h"
#include "blu2usb/usb_hid/usb_hid.h"

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        exit(1); \
    } \
} while (0)

typedef struct {
    blu2usb_canonical_mouse_event_t events[32];
    size_t count;
} event_sink_t;

static bool sink_emit(void *context, const blu2usb_canonical_mouse_event_t *event)
{
    event_sink_t *sink = context;
    if (sink == NULL || event == NULL || sink->count >= 32u) return false;
    sink->events[sink->count++] = *event;
    return true;
}

/* Keyboard report ID 1 followed by a five-button Mouse report ID 2. */
static const uint8_t k_composite_report_map[] = {
    0x05,0x01,0x09,0x06,0xa1,0x01,0x85,0x01,
    0x05,0x07,0x19,0xe0,0x29,0xe7,0x15,0x00,
    0x25,0x01,0x75,0x01,0x95,0x08,0x81,0x02,
    0x95,0x01,0x75,0x08,0x81,0x01,0xc0,

    0x05,0x01,0x09,0x02,0xa1,0x01,0x85,0x02,
    0x09,0x01,0xa1,0x00,
    0x05,0x09,0x19,0x01,0x29,0x05,0x15,0x00,
    0x25,0x01,0x95,0x05,0x75,0x01,0x81,0x02,
    0x95,0x01,0x75,0x03,0x81,0x01,
    0x05,0x01,0x09,0x30,0x09,0x31,0x09,0x38,
    0x15,0x81,0x25,0x7f,0x75,0x08,0x95,0x03,0x81,0x06,
    0x05,0x0c,0x0a,0x38,0x02,0x15,0x81,0x25,0x7f,
    0x75,0x08,0x95,0x01,0x81,0x06,
    0xc0,0xc0,
};

static const uint8_t k_keyboard_only_report_map[] = {
    0x05,0x01,0x09,0x06,0xa1,0x01,0x85,0x01,
    0x05,0x07,0x19,0xe0,0x29,0xe7,0x15,0x00,
    0x25,0x01,0x75,0x01,0x95,0x08,0x81,0x02,0xc0,
};

static void test_report_map_to_canonical_mouse(void)
{
    blu2usb_ble_hogp_parser_t parser;
    const blu2usb_hid_source_t source = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);
    CHECK(blu2usb_ble_hogp_parser_configure(&parser, source, k_composite_report_map, sizeof(k_composite_report_map)));
    CHECK(blu2usb_ble_hogp_parser_has_mouse(&parser));
    CHECK(parser.report_count == 2u);
    CHECK(parser.field_count == 9u);

    event_sink_t sink = {0};
    const uint8_t keyboard_report[2] = {0u, 0u};
    CHECK(blu2usb_ble_hogp_parser_parse_report(&parser, 1u, keyboard_report, sizeof(keyboard_report), sink_emit, &sink));
    CHECK(sink.count == 0u);

    const uint8_t mouse_report[5] = {0x11u, 10u, (uint8_t)-5, 1u, (uint8_t)-2};
    CHECK(blu2usb_ble_hogp_parser_parse_report(&parser, 2u, mouse_report, sizeof(mouse_report), sink_emit, &sink));
    CHECK(sink.count == 4u);
    CHECK(sink.events[0].type == BLU2USB_MOUSE_EVENT_BUTTON);
    CHECK(sink.events[0].data.button.button == BLU2USB_MOUSE_BUTTON_LEFT && sink.events[0].data.button.pressed);
    CHECK(sink.events[1].type == BLU2USB_MOUSE_EVENT_BUTTON);
    CHECK(sink.events[1].data.button.button == BLU2USB_MOUSE_BUTTON_FORWARD && sink.events[1].data.button.pressed);
    CHECK(sink.events[2].type == BLU2USB_MOUSE_EVENT_MOVE);
    CHECK(sink.events[2].data.move.dx == 10 && sink.events[2].data.move.dy == -5);
    CHECK(sink.events[3].type == BLU2USB_MOUSE_EVENT_WHEEL);
    CHECK(sink.events[3].data.wheel.vertical == 1 && sink.events[3].data.wheel.horizontal == -2);

    const size_t before_repeat = sink.count;
    CHECK(blu2usb_ble_hogp_parser_parse_report(&parser, 2u, mouse_report, sizeof(mouse_report), sink_emit, &sink));
    CHECK(sink.count == before_repeat + 2u);
    CHECK(sink.events[before_repeat].type == BLU2USB_MOUSE_EVENT_MOVE);
    CHECK(sink.events[before_repeat + 1u].type == BLU2USB_MOUSE_EVENT_WHEEL);

    const uint8_t released[5] = {0u,0u,0u,0u,0u};
    const size_t before_release = sink.count;
    CHECK(blu2usb_ble_hogp_parser_parse_report(&parser, 2u, released, sizeof(released), sink_emit, &sink));
    CHECK(sink.count == before_release + 2u);
    CHECK(!sink.events[before_release].data.button.pressed);
    CHECK(!sink.events[before_release + 1u].data.button.pressed);
}

static void test_btstack_report_id_framing(void)
{
    blu2usb_ble_hogp_parser_t parser;
    const blu2usb_hid_source_t source = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);
    CHECK(blu2usb_ble_hogp_parser_configure(&parser, source, k_composite_report_map, sizeof(k_composite_report_map)));

    const uint8_t framed[6] = {2u, 0x01u, 3u, (uint8_t)-4, 0u, 0u};
    const uint8_t *payload = NULL;
    size_t payload_len = 0u;
    CHECK(blu2usb_ble_hogp_parser_normalize_report(&parser, 2u, framed, sizeof(framed), &payload, &payload_len));
    CHECK(payload == &framed[1] && payload_len == 5u);

    event_sink_t sink = {0};
    CHECK(blu2usb_ble_hogp_parser_parse_report(&parser, 2u, framed, sizeof(framed), sink_emit, &sink));
    CHECK(sink.count == 2u);
    CHECK(sink.events[0].type == BLU2USB_MOUSE_EVENT_BUTTON);
    CHECK(sink.events[1].type == BLU2USB_MOUSE_EVENT_MOVE);
    CHECK(sink.events[1].data.move.dx == 3 && sink.events[1].data.move.dy == -4);

    uint8_t bad_prefix[6];
    memcpy(bad_prefix, framed, sizeof(framed));
    bad_prefix[0] = 7u;
    CHECK(!blu2usb_ble_hogp_parser_normalize_report(&parser, 2u, bad_prefix, sizeof(bad_prefix), &payload, &payload_len));
    CHECK(!blu2usb_ble_hogp_parser_parse_report(&parser, 2u, framed, 1u, sink_emit, &sink));
}

static void test_non_mouse_descriptor_rejected(void)
{
    blu2usb_ble_hogp_parser_t parser;
    const blu2usb_hid_source_t source = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);
    CHECK(!blu2usb_ble_hogp_parser_configure(&parser, source,
                                              k_keyboard_only_report_map,
                                              sizeof(k_keyboard_only_report_map)));
    const uint8_t malformed[] = {0x05u};
    CHECK(!blu2usb_ble_hogp_parser_configure(&parser, source, malformed, sizeof(malformed)));
}

static void test_runtime_queue_and_decode(void)
{
    blu2usb_bt_runtime_reset();
    CHECK(!blu2usb_bt_runtime_take_overflow());

    blu2usb_canonical_mouse_event_t mouse = {0};
    mouse.source = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);
    mouse.type = BLU2USB_MOUSE_EVENT_MOVE;
    mouse.data.move.dx = 3;
    mouse.data.move.dy = -4;
    CHECK(blu2usb_bt_runtime_publish(BLU2USB_BLE_HOGP_RUNTIME_CHANNEL,
                                      BLU2USB_BLE_HOGP_MESSAGE_MOUSE,
                                      &mouse,
                                      (uint16_t)sizeof(mouse)));

    blu2usb_bt_runtime_message_t message;
    CHECK(blu2usb_bt_runtime_poll(&message));
    blu2usb_ble_hogp_event_t event;
    CHECK(blu2usb_ble_hogp_decode_runtime_message(&message, &event));
    CHECK(event.type == BLU2USB_BLE_HOGP_EVENT_MOUSE);
    CHECK(event.mouse.data.move.dx == 3 && event.mouse.data.move.dy == -4);
    CHECK(!blu2usb_bt_runtime_poll(&message));

    blu2usb_bt_runtime_reset();
    for (unsigned int index = 0u; index < BLU2USB_BT_RUNTIME_QUEUE_CAPACITY; ++index) {
        CHECK(blu2usb_bt_runtime_publish(1u, 1u, NULL, 0u));
    }
    CHECK(!blu2usb_bt_runtime_publish(1u, 1u, NULL, 0u));
    CHECK(blu2usb_bt_runtime_take_overflow());
    CHECK(!blu2usb_bt_runtime_take_overflow());
}

static void test_disconnect_and_chunked_relative_output(void)
{
    blu2usb_hid_aggregator_t aggregator;
    blu2usb_hid_output_state_t output;
    blu2usb_hid_aggregator_init(&aggregator);
    const blu2usb_hid_source_t source = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);

    blu2usb_canonical_mouse_event_t event = {0};
    event.source = source;
    event.type = BLU2USB_MOUSE_EVENT_BUTTON;
    event.data.button.button = BLU2USB_MOUSE_BUTTON_LEFT;
    event.data.button.pressed = true;
    CHECK(blu2usb_hid_aggregator_apply_mouse(&aggregator, &event));
    event.type = BLU2USB_MOUSE_EVENT_MOVE;
    event.data.move.dx = 300;
    event.data.move.dy = -260;
    CHECK(blu2usb_hid_aggregator_apply_mouse(&aggregator, &event));
    event.type = BLU2USB_MOUSE_EVENT_WHEEL;
    event.data.wheel.vertical = 130;
    event.data.wheel.horizontal = -129;
    CHECK(blu2usb_hid_aggregator_apply_mouse(&aggregator, &event));

    blu2usb_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(output.mouse_buttons == 0x01u && output.dx == 300 && output.dy == -260);
    CHECK(output.wheel_vertical == 130 && output.wheel_horizontal == -129);

    CHECK(blu2usb_hid_aggregator_consume_relative(&aggregator, 127, -128, 127, -128));
    blu2usb_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(output.dx == 173 && output.dy == -132);
    CHECK(output.wheel_vertical == 3 && output.wheel_horizontal == -1);
    CHECK(!blu2usb_hid_aggregator_consume_relative(&aggregator, -1, 0, 0, 0));

    CHECK(blu2usb_hid_aggregator_release_source(&aggregator, source));
    blu2usb_hid_aggregator_snapshot(&aggregator, &output);
    CHECK(output.mouse_buttons == 0u);
    CHECK(output.dx == 173); /* relative motion is transient, not source-owned */

    blu2usb_usb_mouse_report_t report;
    blu2usb_usb_hid_build_mouse_report(&report, output.mouse_buttons, 127, -128, 3, -1);
    CHECK(report.buttons == 0u && report.x == 127 && report.y == -128 && report.wheel == 3 && report.pan == -1);
}

int main(void)
{
    test_report_map_to_canonical_mouse();
    test_btstack_report_id_framing();
    test_non_mouse_descriptor_rejected();
    test_runtime_queue_and_decode();
    test_disconnect_and_chunked_relative_output();
    puts("BLU2USB-G05 BLE HOGP Mouse passthrough: OK");
    return 0;
}
