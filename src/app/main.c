#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "blu2usb/ble_hogp/ble_hogp.h"
#include "blu2usb/bt_runtime/bt_runtime.h"
#include "blu2usb/domain/version.h"
#include "blu2usb/hat/hat.h"
#include "blu2usb/hid_aggregator/hid_aggregator.h"
#include "blu2usb/renderer/renderer.h"
#include "blu2usb/renderer/st7789_pico.h"
#include "blu2usb/usb_hid/usb_hid.h"
#include "blu2usb/ux_model/ux_model.h"

#define BLU2USB_RUNTIME_MESSAGES_PER_TICK 32u

static bool render_state(const blu2usb_display_hal_t *display, const blu2usb_ux_model_t *ux)
{
    blu2usb_ui_frame_t frame;
    blu2usb_ui_project(ux, &frame);
    return blu2usb_renderer_render(display, &frame);
}

static int8_t clamp_i8(int32_t value)
{
    if (value > INT8_MAX) return INT8_MAX;
    if (value < INT8_MIN) return INT8_MIN;
    return (int8_t)value;
}

static void service_usb_mouse(blu2usb_hid_aggregator_t *aggregator,
                              uint8_t *last_buttons,
                              bool *last_buttons_valid)
{
    blu2usb_hid_output_state_t output;
    blu2usb_hid_aggregator_snapshot(aggregator, &output);

    const int8_t dx = clamp_i8(output.dx);
    const int8_t dy = clamp_i8(output.dy);
    const int8_t wheel = clamp_i8(output.wheel_vertical);
    const int8_t pan = clamp_i8(output.wheel_horizontal);
    const bool has_relative = dx != 0 || dy != 0 || wheel != 0 || pan != 0;
    const bool buttons_changed = !*last_buttons_valid || output.mouse_buttons != *last_buttons;
    if (!has_relative && !buttons_changed) return;

    blu2usb_usb_mouse_report_t report;
    blu2usb_usb_hid_build_mouse_report(&report, output.mouse_buttons, dx, dy, wheel, pan);
    if (!blu2usb_usb_hid_pico_send_mouse(&report)) return;

    (void)blu2usb_hid_aggregator_consume_relative(aggregator, dx, dy, wheel, pan);
    *last_buttons = output.mouse_buttons;
    *last_buttons_valid = true;
}

static void service_ble_messages(blu2usb_hid_aggregator_t *aggregator)
{
    const blu2usb_hid_source_t ble_mouse_source =
        blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);

    for (unsigned int count = 0u; count < BLU2USB_RUNTIME_MESSAGES_PER_TICK; ++count) {
        blu2usb_bt_runtime_message_t message;
        if (!blu2usb_bt_runtime_poll(&message)) break;

        blu2usb_ble_hogp_event_t event;
        if (!blu2usb_ble_hogp_decode_runtime_message(&message, &event)) continue;
        switch (event.type) {
        case BLU2USB_BLE_HOGP_EVENT_CONNECTED:
            break;
        case BLU2USB_BLE_HOGP_EVENT_DISCONNECTED:
            (void)blu2usb_hid_aggregator_release_source(aggregator, ble_mouse_source);
            break;
        case BLU2USB_BLE_HOGP_EVENT_MOUSE:
            (void)blu2usb_hid_aggregator_apply_mouse(aggregator, &event.mouse);
            break;
        }
    }

    if (blu2usb_bt_runtime_take_overflow()) {
        /* A dropped transition must never leave a USB button stuck. Relative
         * deltas remain transient, but persistent BLE ownership is released. */
        (void)blu2usb_hid_aggregator_release_source(aggregator, ble_mouse_source);
    }
}

int main(void)
{
    (void)blu2usb_version();

    blu2usb_ux_model_t ux;
    blu2usb_display_hal_t display;
    blu2usb_hid_aggregator_t hid_aggregator;
    uint8_t last_mouse_buttons = 0u;
    bool last_mouse_buttons_valid = false;

    blu2usb_ux_init(&ux);
    ux.screen = BLU2USB_SCREEN_LEARN_KEYS;
    blu2usb_hid_aggregator_init(&hid_aggregator);

    if (!blu2usb_usb_hid_pico_init()) {
        for (;;) tight_loop_contents();
    }

    blu2usb_hat_pico_init();
    if (!blu2usb_st7789_pico_init(&display)) {
        for (;;) {
            blu2usb_usb_hid_pico_task();
            tight_loop_contents();
        }
    }

    (void)render_state(&display, &ux);
    blu2usb_st7789_pico_set_backlight(true);

    /* G05 scans autonomously for one BLE HOGP Mouse. Pairing/reconnect policy
     * is deliberately minimal here; persistence/coordinator ownership arrives
     * in later gates. Failure to start Bluetooth must not freeze USB/HAT/UI. */
    (void)blu2usb_ble_hogp_start();

    for (;;) {
        blu2usb_usb_hid_pico_task();
        service_ble_messages(&hid_aggregator);
        service_usb_mouse(&hid_aggregator, &last_mouse_buttons, &last_mouse_buttons_valid);

        blu2usb_hat_pico_task();
        blu2usb_hat_event_t event;
        while (blu2usb_hat_pico_poll_event(&event)) {
            const bool was_locked = blu2usb_interaction_is_locked(&ux.interaction);
            (void)blu2usb_ux_input(&ux, event.control, event.pressed);
            const bool is_locked = blu2usb_interaction_is_locked(&ux.interaction);

            if (!was_locked && is_locked) {
                blu2usb_st7789_pico_set_backlight(false);
            } else if (was_locked && !is_locked) {
                (void)render_state(&display, &ux);
                blu2usb_st7789_pico_set_backlight(true);
            } else if (!is_locked) {
                (void)render_state(&display, &ux);
            }
        }
        tight_loop_contents();
    }
}
