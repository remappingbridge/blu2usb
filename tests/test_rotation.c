#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blu2usb/usb_hid/usb_hid.h"
#include "blu2usb/hid_aggregator/hid_aggregator.h"
#include "blu2usb/remap/remap.h"
#include "blu2usb/profiles/profiles.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

static void test_vectors(void)
{
    /* Every representable small vector, not only cardinal directions. */
    for (int x = -127; x <= 127; ++x) {
        for (int y = -63; y <= 63; ++y) {
            blu2usb_usb_mouse_report_t r;
            blu2usb_usb_hid_build_rotated_mouse_report(&r, 0x15, x, y, -3, 2);
            CHECK(r.x == -2*y && r.y == x);
            CHECK(r.x * r.x + 4*r.y * r.y == 4*(x * x + y * y));
            CHECK(r.buttons == 0x15 && r.wheel == -3 && r.pan == 2);
        }
    }

    /* Closed circle sampled on integer points: rotate every segment and
     * verify each accumulated position, not merely the final zero sum. */
    const int points[][2] = {{5,0},{4,3},{3,4},{0,5},{-3,4},{-4,3},
        {-5,0},{-4,-3},{-3,-4},{0,-5},{3,-4},{4,-3},{5,0}};
    int out_x = 0, out_y = 0;
    for (size_t i = 1; i < sizeof(points)/sizeof(points[0]); ++i) {
        blu2usb_usb_mouse_report_t r;
        blu2usb_usb_hid_build_rotated_mouse_report(&r, 0,
            points[i][0] - points[i-1][0], points[i][1] - points[i-1][1], 0, 0);
        out_x += r.x;
        out_y += r.y;
        CHECK(out_x == -2*points[i][1]);
        CHECK(out_y == points[i][0] - 5);
    }
}

static void test_pending_limits(void)
{
    blu2usb_usb_mouse_report_t r;
    blu2usb_usb_hid_build_rotated_mouse_report(&r, 0, INT32_MIN, INT32_MIN, INT32_MAX, INT32_MIN);
    CHECK(r.x == 126 && r.y == -128 && r.wheel == 127 && r.pan == -128);
    blu2usb_usb_hid_build_rotated_mouse_report(&r, 0, INT32_MAX, INT32_MAX, 0, 0);
    CHECK(r.x == -128 && r.y == 127);
}

static void test_pipeline(int16_t x, int16_t y, blu2usb_mouse_profile_kind_t kind)
{
    blu2usb_profiles_t profiles;
    blu2usb_profiles_init(&profiles);
    blu2usb_mouse_profile_config_t profile;
    blu2usb_profiles_configure_active(&profiles, &profile);
    profile.kind = kind;
    blu2usb_remap_t remap;
    blu2usb_remap_init(&remap);
    blu2usb_remap_set_profile(&remap, &profile);
    blu2usb_hid_aggregator_t agg;
    blu2usb_hid_aggregator_init(&agg);
    blu2usb_canonical_mouse_event_t event = {0};
    event.source = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1);
    event.type = BLU2USB_MOUSE_EVENT_MOVE;
    event.data.move.dx = x;
    event.data.move.dy = y;
    blu2usb_remap_result_t mapped;
    CHECK(blu2usb_remap_process_mouse(&remap, &event, &mapped));
    CHECK(mapped.has_mouse && !mapped.has_keyboard);
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &mapped.mouse));
    event.type = BLU2USB_MOUSE_EVENT_WHEEL;
    event.data.wheel.vertical = 300;
    event.data.wheel.horizontal = -260;
    CHECK(blu2usb_hid_aggregator_apply_mouse(&agg, &event));

    int sum_x = 0, sum_y = 0, sum_wheel = 0, sum_pan = 0;
    unsigned chunks = 0;
    for (;;) {
        blu2usb_hid_output_state_t pending;
        blu2usb_hid_aggregator_snapshot(&agg, &pending);
        if (!pending.dx && !pending.dy && !pending.wheel_vertical && !pending.wheel_horizontal) break;
        CHECK(++chunks < 1024);
        blu2usb_usb_mouse_report_t r, retry;
        blu2usb_usb_hid_build_rotated_mouse_report(&r, pending.mouse_buttons,
            pending.dx, pending.dy, pending.wheel_vertical, pending.wheel_horizontal);
        /* A USB-busy attempt must not change pending motion or rotate twice. */
        blu2usb_usb_hid_build_rotated_mouse_report(&retry, pending.mouse_buttons,
            pending.dx, pending.dy, pending.wheel_vertical, pending.wheel_horizontal);
        CHECK(memcmp(&r, &retry, sizeof(r)) == 0);
        CHECK(r.x % 2 == 0);
        CHECK(blu2usb_hid_aggregator_consume_relative(&agg,
            r.y, -(int32_t)r.x / 2, r.wheel, r.pan));
        sum_x += r.x; sum_y += r.y; sum_wheel += r.wheel; sum_pan += r.pan;
    }
    CHECK(sum_x == -2*(int32_t)y && sum_y == x);
    CHECK(sum_wheel == 300 && sum_pan == -260);
}

int main(void)
{
    test_vectors();
    test_pending_limits();
    const int16_t values[] = {INT16_MIN, -300, -128, -127, -65, -64, -63, -1, 0, 1, 63, 64, 65, 127, 128, 300, INT16_MAX};
    const blu2usb_mouse_profile_kind_t kinds[] = {
        BLU2USB_MOUSE_PROFILE_PASSTHROUGH, BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP,
        BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP, BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP};
    for (size_t k = 0; k < sizeof(kinds)/sizeof(kinds[0]); ++k)
        for (size_t x = 0; x < sizeof(values)/sizeof(values[0]); ++x)
            for (size_t y = 0; y < sizeof(values)/sizeof(values[0]); ++y)
                test_pipeline(values[x], values[y], kinds[k]);
    puts("rotation 90 horizontal 2x: vectors, ellipse, all profiles, chunk remainders and limits OK");
    return 0;
}
