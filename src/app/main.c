#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "blu2usb/ble_hogp/ble_hogp.h"
#include "blu2usb/bt_runtime/bt_runtime.h"
#include "blu2usb/bt_runtime/boot_gate.h"
#include "blu2usb/domain/version.h"
#include "blu2usb/classic_hid/classic_probe.h"
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

/* Minimal connection observation for experiment A; full Keyboard UX is stage D. */
static void project_connection_probe(const blu2usb_ux_model_t *ux, blu2usb_ui_frame_t *frame)
{
    if (ux->screen != BLU2USB_SCREEN_PAIR_KEYBOARD) return;
    const blu2usb_classic_probe_snapshot_t state = blu2usb_classic_probe_snapshot();
    static const char *const labels[] = {
        "STARTING BLUETOOTH", "READY TO PAIR", "STARTING SEARCH",
        "SEARCHING KEYBOARD", "READING DEVICE NAME", "CONNECTING KEYBOARD",
        "TYPE PIN ON KEYBOARD", "SETTING UP KEYBOARD", "KEYBOARD CONNECTED",
        "FINISHING REQUEST", "CONNECTION ERROR"
    };
    for (unsigned row = 1; row <= 4; ++row)
        for (unsigned col = 0; col < BLU2USB_RENDERER_TEXT_COLS; ++col) {
            frame->cells[row][col].character = ' ';
            frame->cells[row][col].tone = BLU2USB_UI_TONE_STATIC;
        }
    const char *label = state.phase <= BLU2USB_PROBE_ERROR ? labels[state.phase] : "CONNECTION ERROR";
    (void)blu2usb_ui_frame_set_text(frame, 1, 0, label,
        state.phase == BLU2USB_PROBE_READY ? BLU2USB_UI_TONE_CURRENT : BLU2USB_UI_TONE_STATIC);
    (void)blu2usb_ui_frame_set_text(frame, 4, 0, "PICO-08 A2", BLU2USB_UI_TONE_STATIC);
    char line[22];
    if (state.phase == BLU2USB_PROBE_PIN) {
        (void)snprintf(line, sizeof(line), "PIN: %0*u", state.pin_digits == 4 ? 4 : 6, (unsigned)state.pin);
        (void)blu2usb_ui_frame_set_text(frame, 2, 0, line, BLU2USB_UI_TONE_CURRENT);
        (void)blu2usb_ui_frame_set_text(frame, 3, 0, "THEN PRESS ENTER", BLU2USB_UI_TONE_STATIC);
    } else if (state.phase == BLU2USB_PROBE_ERROR) {
        (void)snprintf(line, sizeof(line), "%s", state.message);
        (void)blu2usb_ui_frame_set_text(frame, 2, 0, line, BLU2USB_UI_TONE_STATIC);
    } else if (state.phase == BLU2USB_PROBE_READY) {
        (void)blu2usb_ui_frame_set_text(frame, 2, 0, "CONNECTION CONFIRMED", BLU2USB_UI_TONE_CURRENT);
    } else {
        (void)snprintf(line, sizeof(line), "INQUIRY RESULTS %u", state.found);
        (void)blu2usb_ui_frame_set_text(frame, 2, 0, line, BLU2USB_UI_TONE_STATIC);
    }
}

static bool render_state(const blu2usb_display_hal_t *display,
                         const blu2usb_ux_model_t *ux)
{
    blu2usb_ui_frame_t frame;
    blu2usb_ui_project(ux, &frame);
    blu2usb_ui_enforce_applied_visual_contract(ux, &frame);
    project_connection_probe(ux, &frame);
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
    case BLU2USB_UX_COMMAND_PAIR_KEYBOARD:
        blu2usb_classic_probe_pair();
        break;
    case BLU2USB_UX_COMMAND_RETRY:
        if (ux->screen == BLU2USB_SCREEN_PAIR_KEYBOARD) blu2usb_classic_probe_pair();
        break;
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
                ui_changed = true;
            }
            (void)blu2usb_hid_aggregator_release_source(aggregator, mouse);
            (void)blu2usb_hid_aggregator_release_source(aggregator, synthetic);
            *mouse_valid = false;
            *keyboard_valid = false;
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
    ux.screen = BLU2USB_SCREEN_LEARN_KEYS;
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
    const bool lcd_ready = render_state(&display, &ux);
    blu2usb_st7789_pico_set_backlight(true);
    blu2usb_bt_boot_gate_t boot_gate = {0};
    (void)blu2usb_bt_boot_due(&boot_gate, lcd_ready, to_ms_since_boot(get_absolute_time()));
    blu2usb_classic_probe_shared_init();

    (void)blu2usb_logitech_hidpp_pico_start();
    blu2usb_logitech_hidpp_pico_set_forward_fix(
        blu2usb_profiles_requires_forward_held_fix(&boot_profile));
    blu2usb_classic_probe_snapshot_t previous_probe = blu2usb_classic_probe_snapshot();

    for (;;) {
        if (blu2usb_bt_boot_due(&boot_gate, lcd_ready, to_ms_since_boot(get_absolute_time())))
            (void)blu2usb_bt_runtime_start(blu2usb_ble_hogp_session_setup,
                blu2usb_classic_probe_setup, blu2usb_ble_hogp_session_prepare);
        blu2usb_usb_hid_pico_task();
        bool ble_ui_changed =
            service_ble_messages(&ux, &aggregator, &remap,
                                 &last_mouse_valid, &last_keyboard_valid);
        const blu2usb_classic_probe_snapshot_t probe = blu2usb_classic_probe_snapshot();
        if (probe.phase != previous_probe.phase || probe.error != previous_probe.error ||
            probe.found != previous_probe.found || probe.pin != previous_probe.pin ||
            probe.pin_digits != previous_probe.pin_digits ||
            strcmp(probe.message, previous_probe.message) != 0) {
            previous_probe = probe;
            if (ux.screen == BLU2USB_SCREEN_PAIR_KEYBOARD) ble_ui_changed = true;
        }
        if (ble_ui_changed && !blu2usb_interaction_is_locked(&ux.interaction))
            (void)render_state(&display, &ux);
        service_usb_mouse(&aggregator, &last_mouse_buttons, &last_mouse_valid);
        service_usb_keyboard(&aggregator, &last_keyboard, &last_keyboard_valid);

        blu2usb_hat_pico_task();
        blu2usb_hat_event_t event;
        while (blu2usb_hat_pico_poll_event(&event)) {
            const bool was_locked = blu2usb_interaction_is_locked(&ux.interaction);
            const blu2usb_screen_id_t previous_screen = ux.screen;
            const blu2usb_ux_command_t command =
                blu2usb_ux_input(&ux, event.control, event.pressed);
            const bool was_pair = previous_screen == BLU2USB_SCREEN_PAIR_KEYBOARD ||
                                  previous_screen == BLU2USB_SCREEN_PAIR_KEYBOARD_HELP;
            const bool now_pair = ux.screen == BLU2USB_SCREEN_PAIR_KEYBOARD ||
                                  ux.screen == BLU2USB_SCREEN_PAIR_KEYBOARD_HELP;
            if (was_pair && !now_pair) blu2usb_classic_probe_cancel();
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
