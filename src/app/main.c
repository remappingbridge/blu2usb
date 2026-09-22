#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "blu2usb/ble_hogp/ble_hogp.h"
#include "blu2usb/bt_runtime/bt_runtime.h"
#include "blu2usb/domain/version.h"
#include "blu2usb/hat/hat.h"
#include "blu2usb/hid_aggregator/hid_aggregator.h"
#include "blu2usb/logitech_hidpp/logitech_hidpp.h"
#include "blu2usb/profiles/profiles.h"
#include "blu2usb/remap/remap.h"
#include "blu2usb/renderer/renderer.h"
#include "blu2usb/renderer/st7789_pico.h"
#include "blu2usb/storage/storage.h"
#include "blu2usb/usb_hid/usb_hid.h"
#include "blu2usb/ux_model/ux_model.h"

#define BLU2USB_RUNTIME_MESSAGES_PER_TICK 32u

static bool render_state(const blu2usb_display_hal_t *display,
                         const blu2usb_ux_model_t *ux)
{
    blu2usb_ui_frame_t frame;
    blu2usb_ui_project(ux, &frame);
    blu2usb_ui_enforce_applied_visual_contract(ux, &frame);
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
                              bool *last_valid)
{
    blu2usb_hid_output_state_t output;
    blu2usb_hid_aggregator_snapshot(aggregator, &output);
    const int8_t dx = clamp_i8(output.dx);
    const int8_t dy = clamp_i8(output.dy);
    const int8_t wheel = clamp_i8(output.wheel_vertical);
    const int8_t pan = clamp_i8(output.wheel_horizontal);
    const bool relative = dx != 0 || dy != 0 || wheel != 0 || pan != 0;
    const bool buttons_changed = !*last_valid || output.mouse_buttons != *last_buttons;
    if (!relative && !buttons_changed) return;
    blu2usb_usb_mouse_report_t report;
    blu2usb_usb_hid_build_mouse_report(&report, output.mouse_buttons, dx, dy, wheel, pan);
    if (!blu2usb_usb_hid_pico_send_mouse(&report)) return;
    (void)blu2usb_hid_aggregator_consume_relative(aggregator, dx, dy, wheel, pan);
    *last_buttons = output.mouse_buttons;
    *last_valid = true;
}

static void build_keyboard_report(const blu2usb_hid_output_state_t *output,
                                  blu2usb_usb_keyboard_report_t *report)
{
    uint8_t keys[BLU2USB_USB_HID_KEYCODE_COUNT] = {0};
    size_t out = 0u;
    for (unsigned key = 0u;
         key < BLU2USB_HID_KEY_COUNT && out < BLU2USB_USB_HID_KEYCODE_COUNT;
         ++key) {
        if ((output->key_bitmap[key >> 3u] & (uint8_t)(1u << (key & 7u))) != 0u)
            keys[out++] = (uint8_t)key;
    }
    blu2usb_usb_hid_build_keyboard_report(report, output->modifiers, keys);
}

static void service_usb_keyboard(blu2usb_hid_aggregator_t *aggregator,
                                 blu2usb_usb_keyboard_report_t *last,
                                 bool *last_valid)
{
    blu2usb_hid_output_state_t output;
    blu2usb_hid_aggregator_snapshot(aggregator, &output);
    blu2usb_usb_keyboard_report_t report;
    build_keyboard_report(&output, &report);
    if (*last_valid && memcmp(last, &report, sizeof(report)) == 0) return;
    if (!blu2usb_usb_hid_pico_send_keyboard(&report)) return;
    *last = report;
    *last_valid = true;
}

static bool same_profile_config(const blu2usb_mouse_profile_config_t *left,
                                const blu2usb_mouse_profile_config_t *right)
{
    return left != NULL && right != NULL &&
           left->kind == right->kind &&
           memcmp(left->targets, right->targets, sizeof(left->targets)) == 0;
}

static bool persist_profiles(const blu2usb_profiles_t *profiles)
{
    uint8_t payload[BLU2USB_PROFILE_SERIALIZED_SIZE];
    return blu2usb_profiles_serialize(profiles, payload) &&
           blu2usb_storage_store(payload, sizeof(payload));
}

static bool restore_profiles(blu2usb_profiles_t *profiles)
{
    uint8_t payload[BLU2USB_STORAGE_MAX_PAYLOAD_SIZE];
    size_t payload_size = 0u;
    if (!blu2usb_storage_load(payload, sizeof(payload), &payload_size) ||
        payload_size != BLU2USB_PROFILE_SERIALIZED_SIZE) return false;
    return blu2usb_profiles_restore(profiles, payload);
}

static void synchronize_ux_profiles(blu2usb_ux_model_t *ux,
                                    const blu2usb_profiles_t *profiles)
{
    if (ux == NULL || profiles == NULL) return;
    ux->active_profile = profiles->active_kind;
    ux->custom_dirty = profiles->draft_valid;
    for (unsigned source = 0u; source < BLU2USB_MOUSE_SOURCE_COUNT; ++source) {
        ux->custom_targets[source] = profiles->draft_valid
            ? profiles->draft_targets[source]
            : profiles->custom_targets[source];
    }
}

static bool apply_profile(blu2usb_profiles_t *profiles,
                          blu2usb_remap_t *remap,
                          blu2usb_hid_aggregator_t *aggregator,
                          const blu2usb_mouse_profile_config_t *config,
                          bool *mouse_valid,
                          bool *keyboard_valid)
{
    if (profiles == NULL || remap == NULL || aggregator == NULL ||
        config == NULL || mouse_valid == NULL || keyboard_valid == NULL) return false;

    const blu2usb_profiles_t previous = *profiles;
    blu2usb_profiles_activate(profiles, config);

    blu2usb_mouse_profile_config_t active;
    blu2usb_profiles_configure_active(profiles, &active);
    if (!same_profile_config(&active, config) || !persist_profiles(profiles)) {
        *profiles = previous;
        return false;
    }

    const blu2usb_hid_source_t mouse =
        blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);
    const blu2usb_hid_source_t synthetic =
        blu2usb_hid_source_make(BLU2USB_HID_SOURCE_SYNTHETIC_REMAP, 1u);
    (void)blu2usb_hid_aggregator_release_source(aggregator, mouse);
    (void)blu2usb_hid_aggregator_release_source(aggregator, synthetic);

    blu2usb_remap_set_profile(remap, &active);
    blu2usb_logitech_hidpp_pico_set_forward_fix(
        blu2usb_profiles_requires_forward_held_fix(&active));
    *mouse_valid = false;
    *keyboard_valid = false;
    return true;
}

static void confirm_applied_profile(blu2usb_ux_model_t *ux,
                                    const blu2usb_profiles_t *profiles)
{
    if (ux == NULL || profiles == NULL) return;
    synchronize_ux_profiles(ux, profiles);
    blu2usb_ux_profile_applied(ux, profiles->active_kind);
}

static void handle_ux_command(blu2usb_ux_model_t *ux,
                              blu2usb_ux_command_t command,
                              blu2usb_profiles_t *profiles,
                              blu2usb_remap_t *remap,
                              blu2usb_hid_aggregator_t *aggregator,
                              bool *mouse_valid,
                              bool *keyboard_valid)
{
    blu2usb_mouse_profile_config_t candidate;
    switch (command.kind) {
    case BLU2USB_UX_COMMAND_APPLY_PASSTHROUGH:
        if (blu2usb_profiles_build_preset(BLU2USB_MOUSE_PROFILE_PASSTHROUGH,
                                          profiles, &candidate) &&
            apply_profile(profiles, remap, aggregator, &candidate,
                          mouse_valid, keyboard_valid))
            confirm_applied_profile(ux, profiles);
        break;
    case BLU2USB_UX_COMMAND_APPLY_DEFAULT:
        if (blu2usb_profiles_build_preset(BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP,
                                          profiles, &candidate) &&
            apply_profile(profiles, remap, aggregator, &candidate,
                          mouse_valid, keyboard_valid))
            confirm_applied_profile(ux, profiles);
        break;
    case BLU2USB_UX_COMMAND_APPLY_ESCAPE:
        if (blu2usb_profiles_build_preset(BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP,
                                          profiles, &candidate) &&
            apply_profile(profiles, remap, aggregator, &candidate,
                          mouse_valid, keyboard_valid))
            confirm_applied_profile(ux, profiles);
        break;
    case BLU2USB_UX_COMMAND_CUSTOM_SET_TARGET: {
        const blu2usb_profiles_t previous = *profiles;
        if (blu2usb_profiles_draft_set(profiles, command.source, command.target) &&
            persist_profiles(profiles)) {
            blu2usb_ux_set_custom_target(ux, command.source, command.target);
        } else {
            *profiles = previous;
        }
        break;
    }
    case BLU2USB_UX_COMMAND_APPLY_CUSTOM:
        if (blu2usb_profiles_custom_candidate(profiles, &candidate) &&
            apply_profile(profiles, remap, aggregator, &candidate,
                          mouse_valid, keyboard_valid))
            confirm_applied_profile(ux, profiles);
        break;
    case BLU2USB_UX_COMMAND_RETRY_SAVED_SEARCH:
        (void)blu2usb_ble_hogp_retry_saved_search();
        break;
    case BLU2USB_UX_COMMAND_CANCEL_SAVED_SEARCH:
        blu2usb_ble_hogp_cancel_saved_search();
        break;
    case BLU2USB_UX_COMMAND_PAIR_MOUSE:
        /* v0.6.2 replaces HOME only. Stop HOME-owned saved search before
         * entering the inherited v0.6 Pair Mouse page. */
        blu2usb_ble_hogp_cancel_saved_search();
        break;
    default:
        break;
    }
}

static bool service_ble_messages(blu2usb_ux_model_t *ux,
                                 blu2usb_hid_aggregator_t *aggregator,
                                 const blu2usb_remap_t *remap,
                                 bool *mouse_valid,
                                 bool *keyboard_valid)
{
    const blu2usb_hid_source_t mouse =
        blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);
    const blu2usb_hid_source_t synthetic =
        blu2usb_hid_source_make(BLU2USB_HID_SOURCE_SYNTHETIC_REMAP, 1u);
    bool ui_changed = false;

    for (unsigned count = 0u; count < BLU2USB_RUNTIME_MESSAGES_PER_TICK; ++count) {
        blu2usb_bt_runtime_message_t message;
        if (!blu2usb_bt_runtime_poll(&message)) break;
        blu2usb_ble_hogp_event_t event;
        if (!blu2usb_ble_hogp_decode_runtime_message(&message, &event)) continue;
        switch (event.type) {
        case BLU2USB_BLE_HOGP_EVENT_CONNECTED:
            if (!blu2usb_ux_mouse_connected()) {
                blu2usb_ux_set_mouse_connected(true);
                if (ux != NULL)
                    blu2usb_ux_home_mouse_connected(ux);
                ui_changed = true;
            }
            if (ux != NULL && ux->screen == BLU2USB_SCREEN_PAIR_MOUSE) {
                ux->screen = BLU2USB_SCREEN_MOUSE_SAVED;
                ux->selection = 0u;
                ui_changed = true;
            }
            break;
        case BLU2USB_BLE_HOGP_EVENT_DISCONNECTED:
            if (blu2usb_ux_mouse_connected()) {
                blu2usb_ux_set_mouse_connected(false);
                if (ux != NULL)
                    blu2usb_ux_home_mouse_disconnected(ux);
                ui_changed = true;
            }
            (void)blu2usb_hid_aggregator_release_source(aggregator, mouse);
            (void)blu2usb_hid_aggregator_release_source(aggregator, synthetic);
            *mouse_valid = false;
            *keyboard_valid = false;
            break;
        case BLU2USB_BLE_HOGP_EVENT_SAVED_SEARCH_TIMEOUT:
            if (ux != NULL) {
                blu2usb_ux_home_saved_search_timeout(ux);
                ui_changed = true;
            }
            break;
        case BLU2USB_BLE_HOGP_EVENT_MOUSE: {
            blu2usb_remap_result_t mapped;
            if (!blu2usb_remap_process_mouse(remap, &event.mouse, &mapped)) break;
            if (mapped.has_mouse)
                (void)blu2usb_hid_aggregator_apply_mouse(aggregator, &mapped.mouse);
            if (mapped.has_keyboard)
                (void)blu2usb_hid_aggregator_apply_keyboard(aggregator, &mapped.keyboard);
            break;
        }
        }
    }
    if (blu2usb_bt_runtime_take_overflow()) {
        (void)blu2usb_hid_aggregator_release_source(aggregator, mouse);
        (void)blu2usb_hid_aggregator_release_source(aggregator, synthetic);
        *mouse_valid = false;
        *keyboard_valid = false;
    }
    return ui_changed;
}

int main(void)
{
    (void)blu2usb_version();
    blu2usb_ux_model_t ux;
    blu2usb_display_hal_t display;
    blu2usb_hid_aggregator_t aggregator;
    blu2usb_profiles_t profiles;
    blu2usb_remap_t remap;
    uint8_t last_mouse_buttons = 0u;
    bool last_mouse_valid = false;
    blu2usb_usb_keyboard_report_t last_keyboard = {0};
    bool last_keyboard_valid = false;

    blu2usb_ux_init(&ux);
    blu2usb_ux_set_mouse_connected(false);
    blu2usb_ux_begin_first_start(&ux);
    blu2usb_hid_aggregator_init(&aggregator);
    blu2usb_profiles_init(&profiles);
    (void)restore_profiles(&profiles);
    synchronize_ux_profiles(&ux, &profiles);

    blu2usb_remap_init(&remap);
    blu2usb_mouse_profile_config_t boot_profile;
    blu2usb_profiles_configure_active(&profiles, &boot_profile);
    blu2usb_remap_set_profile(&remap, &boot_profile);

    if (!blu2usb_usb_hid_pico_init()) for (;;) tight_loop_contents();
    blu2usb_hat_pico_init();
    if (!blu2usb_st7789_pico_init(&display)) {
        for (;;) { blu2usb_usb_hid_pico_task(); tight_loop_contents(); }
    }
    (void)render_state(&display, &ux);
    blu2usb_st7789_pico_set_backlight(true);

    (void)blu2usb_logitech_hidpp_pico_start();
    blu2usb_logitech_hidpp_pico_set_forward_fix(
        blu2usb_profiles_requires_forward_held_fix(&boot_profile));
    (void)blu2usb_ble_hogp_start();

    for (;;) {
        blu2usb_usb_hid_pico_task();
        const bool ble_ui_changed =
            service_ble_messages(&ux, &aggregator, &remap,
                                 &last_mouse_valid, &last_keyboard_valid);
        if (ble_ui_changed && !blu2usb_interaction_is_locked(&ux.interaction))
            (void)render_state(&display, &ux);
        service_usb_mouse(&aggregator, &last_mouse_buttons, &last_mouse_valid);
        service_usb_keyboard(&aggregator, &last_keyboard, &last_keyboard_valid);

        blu2usb_hat_pico_task();
        blu2usb_hat_event_t event;
        while (blu2usb_hat_pico_poll_event(&event)) {
            const bool was_locked = blu2usb_interaction_is_locked(&ux.interaction);
            const bool first_start_was_complete = ux.first_start_complete;
            const blu2usb_ux_command_t command =
                blu2usb_ux_input(&ux, event.control, event.pressed);

            if (!first_start_was_complete && ux.first_start_complete)
                blu2usb_ble_hogp_set_saved_search_mode(true);

            handle_ux_command(&ux, command, &profiles, &remap, &aggregator,
                              &last_mouse_valid, &last_keyboard_valid);
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
