#include <assert.h>
#include <string.h>
#include "blu2usb/ux_model/ux_model.h"
#include "blu2usb/profiles/custom_template.h"

static void press_release(blu2usb_ux_model_t *ux, blu2usb_control_t c) {
    (void)blu2usb_ux_input(ux, c, true);
    (void)blu2usb_ux_input(ux, c, false);
}

static blu2usb_ux_command_t press_release_cmd(blu2usb_ux_model_t *ux, blu2usb_control_t c) {
    (void)blu2usb_ux_input(ux, c, true);
    return blu2usb_ux_input(ux, c, false);
}

static void test_action_on_release_and_wrap(void) {
    blu2usb_ux_model_t ux;
    blu2usb_ux_init(&ux);
    assert(ux.screen == BLU2USB_SCREEN_HOME);
    assert(ux.selection == 0);
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_JOY_DOWN, true);
    assert(ux.selection == 0);
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_JOY_DOWN, false);
    assert(ux.selection == 1);
    ux.selection = 0;
    press_release(&ux, BLU2USB_CONTROL_JOY_UP);
    assert(ux.selection == 3);
    press_release(&ux, BLU2USB_CONTROL_JOY_DOWN);
    assert(ux.selection == 0);
}

static void test_status_pagination_and_help(void) {
    blu2usb_ux_model_t ux;
    blu2usb_ux_init(&ux);
    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_STATUS);
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_JOY_LEFT, true);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_STATUS);
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_JOY_LEFT, false);
    assert(ux.screen == BLU2USB_SCREEN_OTHER_DEVICES_STATUS);
    press_release(&ux, BLU2USB_CONTROL_JOY_RIGHT);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_STATUS);
    press_release(&ux, BLU2USB_CONTROL_KEY_X);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_HELP);
    press_release(&ux, BLU2USB_CONTROL_KEY_Y);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_STATUS);
    assert(!blu2usb_interaction_is_locked(&ux.interaction));
}

static void test_learn_title_positions_press_and_unlock(void) {
    blu2usb_ux_model_t ux;
    blu2usb_ux_init(&ux);
    ux.selection = 3;
    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_LEARN_KEYS);
    const blu2usb_screen_template_t *t = blu2usb_ux_screen_template(BLU2USB_SCREEN_LEARN_KEYS);
    assert(strcmp(t->rows[0], "PRESS TO LEARN A KEY") == 0);
    assert(t->rows[1][6] == 'J');
    assert(t->rows[2][0] == 'J' && t->rows[2][7] == 'J' && t->rows[2][14] == 'J');
    assert(t->rows[3][0] == 'L' && t->rows[3][6] == 'P' && t->rows[3][13] == 'R');
    assert(t->rows[4][5] == 'J');
    assert(t->rows[5][15] == 'K' && t->rows[6][15] == 'K' && t->rows[7][15] == 'K');
    assert(t->rows[6][0] == 'L');
    assert(t->rows[7][1] == 'A');
    assert(t->rows[8][2] == 'O');
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_JOY_LEFT, true);
    assert((blu2usb_ux_learn_white_span_mask(&ux) & (1u << BLU2USB_CONTROL_JOY_LEFT)) != 0);
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_JOY_LEFT, false);
    assert(blu2usb_ux_learn_white_span_mask(&ux) == 0);
    assert(ux.screen == BLU2USB_SCREEN_LEARN_KEYS);
    press_release(&ux, BLU2USB_CONTROL_KEY_Y);
    assert(blu2usb_interaction_is_locked(&ux.interaction));
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_JOY_DOWN, true);
    assert(ux.screen == BLU2USB_SCREEN_LEARN_KEYS);
    (void)blu2usb_ux_input(&ux, BLU2USB_CONTROL_JOY_DOWN, false);
    assert(!blu2usb_interaction_is_locked(&ux.interaction));
    assert(ux.screen == BLU2USB_SCREEN_HOME);
    assert(ux.selection == 0);
}

static void test_transport_neutral_keyboard(void) {
    const blu2usb_screen_template_t *t = blu2usb_ux_screen_template(BLU2USB_SCREEN_PAIR_KEYBOARD);
    assert(strcmp(t->rows[1], "SEARCHING KEYBOARD") == 0);
    assert(strstr(t->rows[1], "BLE") == 0);
    assert(strstr(t->rows[1], "CLASSIC") == 0);
}

static void test_custom_editor_offline_and_escape_target(void) {
    blu2usb_ux_model_t ux;
    blu2usb_ux_init(&ux);
    ux.selection = 1;
    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_MOUSE_OPTIONS);
    ux.selection = 4;
    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_EDIT_CUSTOM);
    assert(blu2usb_ux_option_count(&ux) == 5);
    blu2usb_ux_set_custom_target(&ux, BLU2USB_MOUSE_SOURCE_FORWARD, BLU2USB_MOUSE_TARGET_ESCAPE);
    ux.selection = 3;
    press_release(&ux, BLU2USB_CONTROL_JOY_PRESS);
    assert(ux.screen == BLU2USB_SCREEN_FORWARD_WILL_BECOME);
    assert(ux.selection == BLU2USB_MOUSE_TARGET_ESCAPE);
    assert(blu2usb_ux_option_count(&ux) == 6);
    const blu2usb_screen_template_t *t = blu2usb_ux_screen_template(ux.screen);
    assert(strcmp(t->rows[1], " LEFT") == 0);
    assert(strcmp(t->rows[2], " RIGHT") == 0);
    assert(strcmp(t->rows[3], " MIDDLE") == 0);
    assert(strcmp(t->rows[4], " BACKWARD") == 0);
    assert(strcmp(t->rows[5], " FORWARD") == 0);
    assert(strcmp(t->rows[6], " ESCAPE") == 0);
    blu2usb_ux_command_t cmd = press_release_cmd(&ux, BLU2USB_CONTROL_KEY_A);
    assert(cmd.kind == BLU2USB_UX_COMMAND_CUSTOM_SET_TARGET);
    assert(cmd.source == BLU2USB_MOUSE_SOURCE_FORWARD);
    assert(cmd.target == BLU2USB_MOUSE_TARGET_ESCAPE);
    assert(ux.screen == BLU2USB_SCREEN_EDIT_CUSTOM);
}

static void test_custom_template_transaction(void) {
    blu2usb_custom_template_editor_t e;
    blu2usb_custom_template_editor_init(&e);
    assert(e.committed.target[BLU2USB_MOUSE_SOURCE_LEFT] == BLU2USB_MOUSE_TARGET_LEFT);
    assert(e.committed.target[BLU2USB_MOUSE_SOURCE_FORWARD] == BLU2USB_MOUSE_TARGET_FORWARD);
    blu2usb_custom_template_begin(&e);
    assert(blu2usb_custom_template_set_draft(&e, BLU2USB_MOUSE_SOURCE_LEFT, BLU2USB_MOUSE_TARGET_ESCAPE));
    assert(e.draft.target[BLU2USB_MOUSE_SOURCE_LEFT] == BLU2USB_MOUSE_TARGET_ESCAPE);
    assert(e.committed.target[BLU2USB_MOUSE_SOURCE_LEFT] == BLU2USB_MOUSE_TARGET_LEFT);
    blu2usb_custom_template_commit(&e);
    assert(e.committed.target[BLU2USB_MOUSE_SOURCE_LEFT] == BLU2USB_MOUSE_TARGET_ESCAPE);
    blu2usb_custom_template_begin(&e);
    assert(blu2usb_custom_template_set_draft(&e, BLU2USB_MOUSE_SOURCE_RIGHT, BLU2USB_MOUSE_TARGET_FORWARD));
    blu2usb_custom_template_cancel(&e);
    assert(e.committed.target[BLU2USB_MOUSE_SOURCE_RIGHT] == BLU2USB_MOUSE_TARGET_RIGHT);
}

static void test_saved_pagination_wrap(void) {
    blu2usb_ux_model_t ux;
    blu2usb_ux_init(&ux);
    ux.screen = BLU2USB_SCREEN_SAVED_DEVICES;
    blu2usb_ux_set_saved_device_count(&ux, 6);
    assert(ux.saved_pages == 2 && ux.saved_page == 0);
    press_release(&ux, BLU2USB_CONTROL_JOY_LEFT);
    assert(ux.saved_page == 1);
    assert(blu2usb_ux_option_count(&ux) == 2);
    press_release(&ux, BLU2USB_CONTROL_JOY_RIGHT);
    assert(ux.saved_page == 0);
    assert(blu2usb_ux_option_count(&ux) == 4);
}

static void test_all_templates_are_9x21(void) {
    for (unsigned s = 0; s < BLU2USB_SCREEN_COUNT; ++s) {
        const blu2usb_screen_template_t *t = blu2usb_ux_screen_template((blu2usb_screen_id_t)s);
        assert(t != 0);
        for (unsigned r = 0; r < 9; ++r) {
            assert(t->rows[r] != 0);
            assert(strlen(t->rows[r]) <= 21);
        }
    }
}

int main(void) {
    test_action_on_release_and_wrap();
    test_status_pagination_and_help();
    test_learn_title_positions_press_and_unlock();
    test_transport_neutral_keyboard();
    test_custom_editor_offline_and_escape_target();
    test_custom_template_transaction();
    test_saved_pagination_wrap();
    test_all_templates_are_9x21();
    return 0;
}
