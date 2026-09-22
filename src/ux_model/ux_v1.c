#include "blu2usb/ux_model/ux_v1.h"

#include <string.h>

static blu2usb_ui_v1_intent_t no_intent(void)
{
    blu2usb_ui_v1_intent_t intent;
    memset(&intent, 0, sizeof(intent));
    intent.kind = BLU2USB_UI_V1_INTENT_NONE;
    intent.mouse_id = BLU2USB_MOUSE_ID_INVALID;
    intent.profile = BLU2USB_MOUSE_PROFILE_PASSTHROUGH;
    intent.source = BLU2USB_MOUSE_SOURCE_LEFT;
    intent.target = BLU2USB_MOUSE_TARGET_LEFT;
    return intent;
}

static const blu2usb_mouse_snapshot_t *current_mouse(
    const blu2usb_product_snapshot_t *product)
{
    if (product == NULL || !product->has_authoritative) return NULL;
    return blu2usb_product_snapshot_find(product, product->authoritative_id);
}

static blu2usb_mouse_profile_kind_t current_profile(
    const blu2usb_product_snapshot_t *product)
{
    const blu2usb_mouse_snapshot_t *mouse = current_mouse(product);
    return mouse == NULL ? BLU2USB_MOUSE_PROFILE_PASSTHROUGH : mouse->profile;
}

bool blu2usb_ui_v1_is_help(blu2usb_ui_v1_screen_t screen)
{
    switch (screen) {
    case BLU2USB_UI_V1_HOME_SEARCHING_HELP:
    case BLU2USB_UI_V1_HOME_RETRY_HELP:
    case BLU2USB_UI_V1_HELP_PAIR_NEW:
    case BLU2USB_UI_V1_HELP_RETRY_PAIR_NEW:
    case BLU2USB_UI_V1_HELP_HOME_CONNECTED:
    case BLU2USB_UI_V1_HELP_REMAPPER_OPTIONS:
    case BLU2USB_UI_V1_HELP_REMOVE_THIS:
        return true;
    default:
        return false;
    }
}

static bool instructional_screen(blu2usb_ui_v1_screen_t screen)
{
    return screen == BLU2USB_UI_V1_SEARCHING_FIRST ||
           screen == BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED ||
           screen == BLU2USB_UI_V1_LEARN_THE_KEYS;
}

unsigned blu2usb_ui_v1_option_count(const blu2usb_ui_v1_t *ui)
{
    if (ui == NULL) return 0u;

    switch (ui->screen) {
    case BLU2USB_UI_V1_HOME_CONNECTED:
    case BLU2USB_UI_V1_REMAPPER_OPTIONS:
        return 4u;
    case BLU2USB_UI_V1_HOME_SEARCHING:
    case BLU2USB_UI_V1_HOME_RETRY:
        return 3u;
    case BLU2USB_UI_V1_CUSTOM_EDIT:
        return 5u;
    case BLU2USB_UI_V1_LEFT:
    case BLU2USB_UI_V1_RIGHT:
    case BLU2USB_UI_V1_MIDDLE:
    case BLU2USB_UI_V1_FORWARD:
    case BLU2USB_UI_V1_BACKWARD:
        return 6u;
    default:
        return 0u;
    }
}

static unsigned wrap_prev(unsigned value, unsigned count)
{
    return count == 0u ? 0u : (value + count - 1u) % count;
}

static unsigned wrap_next(unsigned value, unsigned count)
{
    return count == 0u ? 0u : (value + 1u) % count;
}

static void show(blu2usb_ui_v1_t *ui, blu2usb_ui_v1_screen_t screen)
{
    ui->screen = screen;
    ui->selection = 0u;
}

static void show_selection(
    blu2usb_ui_v1_t *ui,
    blu2usb_ui_v1_screen_t screen,
    uint8_t selection)
{
    show(ui, screen);
    const unsigned count = blu2usb_ui_v1_option_count(ui);
    ui->selection = count == 0u ? 0u : (uint8_t)(selection % count);
}

static bool is_profile_active_screen(blu2usb_ui_v1_screen_t screen)
{
    return screen == BLU2USB_UI_V1_PASSTHROUGH_ACTIVE ||
           screen == BLU2USB_UI_V1_STANDARD_ACTIVE ||
           screen == BLU2USB_UI_V1_ESCAPE_ACTIVE;
}

static blu2usb_ui_v1_screen_t active_screen_for(
    blu2usb_mouse_profile_kind_t profile)
{
    switch (profile) {
    case BLU2USB_MOUSE_PROFILE_PASSTHROUGH:
        return BLU2USB_UI_V1_PASSTHROUGH_ACTIVE;
    case BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP:
        return BLU2USB_UI_V1_STANDARD_ACTIVE;
    case BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP:
        return BLU2USB_UI_V1_ESCAPE_ACTIVE;
    case BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP:
        return BLU2USB_UI_V1_CUSTOM_EDIT;
    default:
        return BLU2USB_UI_V1_PASSTHROUGH_ACTIVE;
    }
}

static blu2usb_ui_v1_screen_t inactive_screen_for(
    blu2usb_mouse_profile_kind_t profile)
{
    switch (profile) {
    case BLU2USB_MOUSE_PROFILE_PASSTHROUGH:
        return BLU2USB_UI_V1_PASSTHROUGH_NOT_ACTIVE;
    case BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP:
        return BLU2USB_UI_V1_STANDARD_NOT_ACTIVE;
    case BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP:
        return BLU2USB_UI_V1_ESCAPE_NOT_ACTIVE;
    case BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP:
        return BLU2USB_UI_V1_CUSTOM_EDIT;
    default:
        return BLU2USB_UI_V1_PASSTHROUGH_NOT_ACTIVE;
    }
}

static unsigned source_for_editor(blu2usb_ui_v1_screen_t screen)
{
    switch (screen) {
    case BLU2USB_UI_V1_LEFT: return 0u;
    case BLU2USB_UI_V1_RIGHT: return 1u;
    case BLU2USB_UI_V1_MIDDLE: return 2u;
    case BLU2USB_UI_V1_FORWARD: return 3u;
    case BLU2USB_UI_V1_BACKWARD: return 4u;
    default: return BLU2USB_MOUSE_SOURCE_COUNT;
    }
}

static blu2usb_ui_v1_screen_t editor_for_source(unsigned source)
{
    static const blu2usb_ui_v1_screen_t screens[BLU2USB_MOUSE_SOURCE_COUNT] = {
        BLU2USB_UI_V1_LEFT,
        BLU2USB_UI_V1_RIGHT,
        BLU2USB_UI_V1_MIDDLE,
        BLU2USB_UI_V1_FORWARD,
        BLU2USB_UI_V1_BACKWARD,
    };
    return source < BLU2USB_MOUSE_SOURCE_COUNT
        ? screens[source] : BLU2USB_UI_V1_CUSTOM_EDIT;
}

static blu2usb_mouse_source_t mouse_source_for(unsigned source)
{
    static const blu2usb_mouse_source_t sources[BLU2USB_MOUSE_SOURCE_COUNT] = {
        BLU2USB_MOUSE_SOURCE_LEFT,
        BLU2USB_MOUSE_SOURCE_RIGHT,
        BLU2USB_MOUSE_SOURCE_MIDDLE,
        BLU2USB_MOUSE_SOURCE_FORWARD,
        BLU2USB_MOUSE_SOURCE_BACKWARD,
    };
    return source < BLU2USB_MOUSE_SOURCE_COUNT
        ? sources[source] : BLU2USB_MOUSE_SOURCE_LEFT;
}

static const blu2usb_mouse_target_t visual_targets[6] = {
    BLU2USB_MOUSE_TARGET_LEFT,
    BLU2USB_MOUSE_TARGET_RIGHT,
    BLU2USB_MOUSE_TARGET_MIDDLE,
    BLU2USB_MOUSE_TARGET_ESCAPE,
    BLU2USB_MOUSE_TARGET_FORWARD,
    BLU2USB_MOUSE_TARGET_BACKWARD,
};

static unsigned visual_index_for_target(blu2usb_mouse_target_t target)
{
    for (unsigned i = 0u; i < 6u; ++i)
        if (visual_targets[i] == target) return i;
    return 0u;
}

static blu2usb_ui_v1_intent_t resolve_home(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    bool cancel_owned)
{
    blu2usb_ui_v1_intent_t intent = no_intent();
    intent.cancel_owned = cancel_owned;

    if (product == NULL || product->saved_count == 0u) {
        show(ui, BLU2USB_UI_V1_SEARCHING_FIRST);
        ui->page = 0u;
        ui->search_active = true;
        ui->search_purpose = BLU2USB_UI_V1_SEARCH_FIRST;
        intent.kind = BLU2USB_UI_V1_INTENT_SEARCH_FIRST;
        return intent;
    }

    if (product->has_authoritative) {
        show(ui, BLU2USB_UI_V1_HOME_CONNECTED);
        ui->search_active = false;
        return intent;
    }

    show(ui, BLU2USB_UI_V1_HOME_SEARCHING);
    ui->search_active = true;
    ui->search_purpose = BLU2USB_UI_V1_SEARCH_SAVED;
    intent.kind = BLU2USB_UI_V1_INTENT_SEARCH_SAVED;
    return intent;
}

void blu2usb_ui_v1_init(blu2usb_ui_v1_t *ui)
{
    if (ui == NULL) return;
    memset(ui, 0, sizeof(*ui));
    blu2usb_interaction_init(&ui->interaction);
    ui->screen = BLU2USB_UI_V1_SEARCHING_FIRST;
    ui->help_owner = BLU2USB_UI_V1_SEARCHING_FIRST;
    ui->remove_target_id = BLU2USB_MOUSE_ID_INVALID;
    ui->custom_targets[BLU2USB_MOUSE_SOURCE_LEFT] = BLU2USB_MOUSE_TARGET_LEFT;
    ui->custom_targets[BLU2USB_MOUSE_SOURCE_RIGHT] = BLU2USB_MOUSE_TARGET_RIGHT;
    ui->custom_targets[BLU2USB_MOUSE_SOURCE_MIDDLE] = BLU2USB_MOUSE_TARGET_MIDDLE;
    ui->custom_targets[BLU2USB_MOUSE_SOURCE_FORWARD] = BLU2USB_MOUSE_TARGET_FORWARD;
    ui->custom_targets[BLU2USB_MOUSE_SOURCE_BACKWARD] = BLU2USB_MOUSE_TARGET_BACKWARD;
}

blu2usb_ui_v1_intent_t blu2usb_ui_v1_home(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product)
{
    if (ui == NULL) return no_intent();
    return resolve_home(ui, product, false);
}

static void clear_owned(blu2usb_ui_v1_t *ui)
{
    ui->search_active = false;
    ui->pending_profile = false;
    ui->pending_remove = false;
}

static bool screen_owns_async(const blu2usb_ui_v1_t *ui)
{
    return ui->search_active || ui->pending_profile || ui->pending_remove;
}

static blu2usb_ui_v1_intent_t enter_help(blu2usb_ui_v1_t *ui)
{
    blu2usb_ui_v1_intent_t intent = no_intent();
    blu2usb_ui_v1_screen_t destination;

    switch (ui->screen) {
    case BLU2USB_UI_V1_HOME_SEARCHING:
        destination = BLU2USB_UI_V1_HOME_SEARCHING_HELP; break;
    case BLU2USB_UI_V1_HOME_RETRY:
        destination = BLU2USB_UI_V1_HOME_RETRY_HELP; break;
    case BLU2USB_UI_V1_PAIR_NEW:
        destination = BLU2USB_UI_V1_HELP_PAIR_NEW; break;
    case BLU2USB_UI_V1_RETRY_PAIR_NEW:
        destination = BLU2USB_UI_V1_HELP_RETRY_PAIR_NEW; break;
    case BLU2USB_UI_V1_HOME_CONNECTED:
        destination = BLU2USB_UI_V1_HELP_HOME_CONNECTED; break;
    case BLU2USB_UI_V1_REMAPPER_OPTIONS:
        destination = BLU2USB_UI_V1_HELP_REMAPPER_OPTIONS; break;
    case BLU2USB_UI_V1_REMOVE_THIS:
        destination = BLU2USB_UI_V1_HELP_REMOVE_THIS; break;
    default:
        return intent;
    }

    ui->help_owner = ui->screen;
    ui->help_selection = ui->selection;
    ui->help_page = ui->page;
    intent.cancel_owned = screen_owns_async(ui);
    clear_owned(ui);
    show(ui, destination);
    return intent;
}

static void return_from_help(blu2usb_ui_v1_t *ui)
{
    blu2usb_ui_v1_screen_t destination = ui->help_owner;
    if (destination == BLU2USB_UI_V1_HOME_SEARCHING)
        destination = BLU2USB_UI_V1_HOME_RETRY;
    else if (destination == BLU2USB_UI_V1_PAIR_NEW)
        destination = BLU2USB_UI_V1_RETRY_PAIR_NEW;

    show_selection(ui, destination, ui->help_selection);
    ui->page = ui->help_page;
}

static bool can_lock(
    const blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product)
{
    return ui != NULL && product != NULL &&
           product->saved_count > 0u &&
           ui->screen != BLU2USB_UI_V1_SEARCHING_FIRST &&
           !blu2usb_ui_v1_is_help(ui->screen);
}

static blu2usb_ui_v1_intent_t lock_ui(
    blu2usb_ui_v1_t *ui)
{
    blu2usb_ui_v1_intent_t intent = no_intent();
    intent.cancel_owned = screen_owns_async(ui);
    clear_owned(ui);
    blu2usb_interaction_lock(&ui->interaction);
    return intent;
}

static blu2usb_ui_v1_intent_t navigate_home_action(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    blu2usb_control_t control)
{
    blu2usb_ui_v1_intent_t intent = no_intent();

    if (ui->screen == BLU2USB_UI_V1_HOME_SEARCHING &&
        control == BLU2USB_CONTROL_KEY_B) {
        intent.cancel_owned = ui->search_active;
        ui->search_active = false;
        show(ui, BLU2USB_UI_V1_HOME_RETRY);
        return intent;
    }

    if (ui->screen == BLU2USB_UI_V1_HOME_RETRY &&
        control == BLU2USB_CONTROL_KEY_A) {
        show(ui, BLU2USB_UI_V1_HOME_SEARCHING);
        ui->search_active = true;
        ui->search_purpose = BLU2USB_UI_V1_SEARCH_SAVED;
        intent.kind = BLU2USB_UI_V1_INTENT_SEARCH_SAVED;
        return intent;
    }

    if (control != BLU2USB_CONTROL_JOY_PRESS) return intent;

    const bool connected = ui->screen == BLU2USB_UI_V1_HOME_CONNECTED;
    const unsigned selection = ui->selection;

    if (connected && selection == 0u) {
        show(ui, BLU2USB_UI_V1_REMAPPER_OPTIONS);
    } else if (selection == (connected ? 1u : 0u)) {
        ui->page = 0u;
        show(ui, BLU2USB_UI_V1_SAVED_DEVICES);
    } else if (selection == (connected ? 2u : 1u)) {
        show(ui, BLU2USB_UI_V1_PAIR_NEW);
        ui->search_active = true;
        ui->search_purpose = BLU2USB_UI_V1_SEARCH_NEW;
        intent.kind = BLU2USB_UI_V1_INTENT_SEARCH_NEW;
    } else {
        show(ui, BLU2USB_UI_V1_LEARN_THE_KEYS);
    }

    (void)product;
    return intent;
}

static blu2usb_ui_v1_intent_t handle_back(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product)
{
    blu2usb_ui_v1_intent_t intent = no_intent();

    switch (ui->screen) {
    case BLU2USB_UI_V1_SEARCHING_FIRST:
    case BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED:
    case BLU2USB_UI_V1_LEARN_THE_KEYS:
        return intent;

    case BLU2USB_UI_V1_HOME_SEARCHING:
        intent.cancel_owned = ui->search_active;
        ui->search_active = false;
        show(ui, BLU2USB_UI_V1_HOME_RETRY);
        return intent;

    case BLU2USB_UI_V1_PAIR_NEW:
    case BLU2USB_UI_V1_RETRY_PAIR_NEW:
        clear_owned(ui);
        return resolve_home(ui, product, true);

    case BLU2USB_UI_V1_REMAPPER_OPTIONS:
    case BLU2USB_UI_V1_SAVED_DEVICES:
        return resolve_home(ui, product, screen_owns_async(ui));

    case BLU2USB_UI_V1_PASSTHROUGH_ACTIVE:
    case BLU2USB_UI_V1_PASSTHROUGH_NOT_ACTIVE:
    case BLU2USB_UI_V1_STANDARD_ACTIVE:
    case BLU2USB_UI_V1_STANDARD_NOT_ACTIVE:
    case BLU2USB_UI_V1_ESCAPE_ACTIVE:
    case BLU2USB_UI_V1_ESCAPE_NOT_ACTIVE:
        intent.cancel_owned = ui->pending_profile;
        ui->pending_profile = false;
        show(ui, BLU2USB_UI_V1_REMAPPER_OPTIONS);
        return intent;

    case BLU2USB_UI_V1_CUSTOM_EDIT:
        intent.cancel_owned = ui->pending_profile;
        ui->pending_profile = false;
        show(ui, BLU2USB_UI_V1_REMAPPER_OPTIONS);
        return intent;

    case BLU2USB_UI_V1_LEFT:
    case BLU2USB_UI_V1_RIGHT:
    case BLU2USB_UI_V1_MIDDLE:
    case BLU2USB_UI_V1_FORWARD:
    case BLU2USB_UI_V1_BACKWARD: {
        const unsigned source = source_for_editor(ui->screen);
        show_selection(ui, BLU2USB_UI_V1_CUSTOM_EDIT, (uint8_t)source);
        return intent;
    }

    case BLU2USB_UI_V1_REMOVE_THIS:
        intent.cancel_owned = ui->pending_remove;
        ui->pending_remove = false;
        show(ui, BLU2USB_UI_V1_SAVED_DEVICES);
        if (product != NULL && product->saved_count > 0u &&
            ui->page >= product->saved_count)
            ui->page = (uint8_t)(product->saved_count - 1u);
        return intent;

    default:
        return intent;
    }
}

static blu2usb_ui_v1_intent_t remapper_action(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    blu2usb_control_t control)
{
    blu2usb_ui_v1_intent_t intent = no_intent();
    if (control != BLU2USB_CONTROL_JOY_PRESS) return intent;

    static const blu2usb_mouse_profile_kind_t profiles[4] = {
        BLU2USB_MOUSE_PROFILE_PASSTHROUGH,
        BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP,
        BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP,
        BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP,
    };

    const blu2usb_mouse_profile_kind_t profile = profiles[ui->selection % 4u];
    if (profile == BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP) {
        show(ui, BLU2USB_UI_V1_CUSTOM_EDIT);
        return intent;
    }

    show(ui, current_mouse(product) != NULL && current_profile(product) == profile
        ? active_screen_for(profile) : inactive_screen_for(profile));
    return intent;
}

static blu2usb_ui_v1_intent_t profile_action(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    blu2usb_control_t control)
{
    blu2usb_ui_v1_intent_t intent = no_intent();
    if (control != BLU2USB_CONTROL_KEY_A || current_mouse(product) == NULL ||
        ui->pending_profile)
        return intent;

    blu2usb_mouse_profile_kind_t profile;
    switch (ui->screen) {
    case BLU2USB_UI_V1_PASSTHROUGH_NOT_ACTIVE:
        profile = BLU2USB_MOUSE_PROFILE_PASSTHROUGH; break;
    case BLU2USB_UI_V1_STANDARD_NOT_ACTIVE:
        profile = BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP; break;
    case BLU2USB_UI_V1_ESCAPE_NOT_ACTIVE:
        profile = BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP; break;
    default:
        return intent;
    }

    ui->pending_profile = true;
    ui->pending_profile_kind = profile;
    intent.kind = BLU2USB_UI_V1_INTENT_APPLY_PROFILE;
    intent.mouse_id = product->authoritative_id;
    intent.profile = profile;
    return intent;
}

static blu2usb_ui_v1_intent_t custom_action(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    blu2usb_control_t control)
{
    blu2usb_ui_v1_intent_t intent = no_intent();

    if (control == BLU2USB_CONTROL_JOY_PRESS) {
        const unsigned source = ui->selection % BLU2USB_MOUSE_SOURCE_COUNT;
        const blu2usb_mouse_source_t mouse_source = mouse_source_for(source);
        show(ui, editor_for_source(source));
        ui->selection = (uint8_t)visual_index_for_target(
            ui->custom_targets[mouse_source]);
        return intent;
    }

    if (control == BLU2USB_CONTROL_KEY_A && current_mouse(product) != NULL &&
        !ui->pending_profile &&
        (current_profile(product) != BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP ||
         ui->custom_dirty)) {
        ui->pending_profile = true;
        ui->pending_profile_kind = BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP;
        intent.kind = BLU2USB_UI_V1_INTENT_APPLY_CUSTOM;
        intent.mouse_id = product->authoritative_id;
        intent.profile = BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP;
    }

    return intent;
}

static blu2usb_ui_v1_intent_t source_editor_action(
    blu2usb_ui_v1_t *ui,
    blu2usb_control_t control)
{
    blu2usb_ui_v1_intent_t intent = no_intent();
    if (control != BLU2USB_CONTROL_KEY_A) return intent;

    const unsigned source_index = source_for_editor(ui->screen);
    if (source_index >= BLU2USB_MOUSE_SOURCE_COUNT) return intent;

    const blu2usb_mouse_source_t source = mouse_source_for(source_index);
    const blu2usb_mouse_target_t target = visual_targets[ui->selection % 6u];

    if (ui->custom_targets[source] != target) {
        ui->custom_targets[source] = target;
        ui->custom_dirty = true;
    }

    intent.kind = BLU2USB_UI_V1_INTENT_CUSTOM_SET_TARGET;
    intent.source = source;
    intent.target = target;
    show_selection(ui, BLU2USB_UI_V1_CUSTOM_EDIT, (uint8_t)source_index);
    return intent;
}

static blu2usb_ui_v1_intent_t saved_devices_action(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    blu2usb_control_t control)
{
    blu2usb_ui_v1_intent_t intent = no_intent();
    if (product == NULL || product->saved_count == 0u)
        return resolve_home(ui, product, false);

    if (control == BLU2USB_CONTROL_JOY_LEFT) {
        ui->page = (uint8_t)wrap_prev(ui->page, (unsigned)product->saved_count);
    } else if (control == BLU2USB_CONTROL_JOY_RIGHT) {
        ui->page = (uint8_t)wrap_next(ui->page, (unsigned)product->saved_count);
    } else if (control == BLU2USB_CONTROL_JOY_PRESS) {
        const blu2usb_mouse_snapshot_t *mouse = &product->mice[ui->page];
        ui->remove_target_id = mouse->id;
        show(ui, BLU2USB_UI_V1_REMOVE_THIS);
    }
    return intent;
}

static blu2usb_ui_v1_intent_t remove_action(
    blu2usb_ui_v1_t *ui,
    blu2usb_control_t control)
{
    blu2usb_ui_v1_intent_t intent = no_intent();
    if (control != BLU2USB_CONTROL_KEY_A || ui->pending_remove ||
        !blu2usb_mouse_id_is_valid(ui->remove_target_id))
        return intent;

    ui->pending_remove = true;
    intent.kind = BLU2USB_UI_V1_INTENT_REMOVE_MOUSE;
    intent.mouse_id = ui->remove_target_id;
    return intent;
}

blu2usb_ui_v1_intent_t blu2usb_ui_v1_input(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    blu2usb_control_t control,
    bool pressed)
{
    if (ui == NULL) return no_intent();

    const blu2usb_interaction_event_t event =
        blu2usb_interaction_input(&ui->interaction, control, pressed);
    if (event.kind == BLU2USB_INTERACTION_NONE)
        return no_intent();

    if (event.kind == BLU2USB_INTERACTION_UNLOCK)
        return resolve_home(ui, product, false);

    if (blu2usb_ui_v1_is_help(ui->screen)) {
        return_from_help(ui);
        return no_intent();
    }

    if (ui->screen == BLU2USB_UI_V1_SEARCHING_FIRST)
        return no_intent();

    if (ui->screen == BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED ||
        ui->screen == BLU2USB_UI_V1_LEARN_THE_KEYS) {
        if (control == BLU2USB_CONTROL_KEY_Y && can_lock(ui, product))
            return lock_ui(ui);
        return no_intent();
    }

    if (control == BLU2USB_CONTROL_KEY_Y && can_lock(ui, product))
        return lock_ui(ui);

    if (control == BLU2USB_CONTROL_KEY_X)
        return enter_help(ui);

    if (control == BLU2USB_CONTROL_KEY_B)
        return handle_back(ui, product);

    const unsigned count = blu2usb_ui_v1_option_count(ui);
    if (count != 0u && control == BLU2USB_CONTROL_JOY_UP) {
        ui->selection = (uint8_t)wrap_prev(ui->selection, count);
        return no_intent();
    }
    if (count != 0u && control == BLU2USB_CONTROL_JOY_DOWN) {
        ui->selection = (uint8_t)wrap_next(ui->selection, count);
        return no_intent();
    }

    switch (ui->screen) {
    case BLU2USB_UI_V1_HOME_SEARCHING:
    case BLU2USB_UI_V1_HOME_RETRY:
    case BLU2USB_UI_V1_HOME_CONNECTED:
        return navigate_home_action(ui, product, control);

    case BLU2USB_UI_V1_RETRY_PAIR_NEW:
        if (control == BLU2USB_CONTROL_KEY_A) {
            show(ui, BLU2USB_UI_V1_PAIR_NEW);
            ui->search_active = true;
            ui->search_purpose = BLU2USB_UI_V1_SEARCH_NEW;
            blu2usb_ui_v1_intent_t intent = no_intent();
            intent.kind = BLU2USB_UI_V1_INTENT_SEARCH_NEW;
            return intent;
        }
        return no_intent();

    case BLU2USB_UI_V1_REMAPPER_OPTIONS:
        return remapper_action(ui, product, control);

    case BLU2USB_UI_V1_PASSTHROUGH_NOT_ACTIVE:
    case BLU2USB_UI_V1_STANDARD_NOT_ACTIVE:
    case BLU2USB_UI_V1_ESCAPE_NOT_ACTIVE:
        return profile_action(ui, product, control);

    case BLU2USB_UI_V1_CUSTOM_EDIT:
        return custom_action(ui, product, control);

    case BLU2USB_UI_V1_LEFT:
    case BLU2USB_UI_V1_RIGHT:
    case BLU2USB_UI_V1_MIDDLE:
    case BLU2USB_UI_V1_FORWARD:
    case BLU2USB_UI_V1_BACKWARD:
        return source_editor_action(ui, control);

    case BLU2USB_UI_V1_SAVED_DEVICES:
        return saved_devices_action(ui, product, control);

    case BLU2USB_UI_V1_REMOVE_THIS:
        return remove_action(ui, control);

    default:
        return no_intent();
    }
}

blu2usb_ui_v1_intent_t blu2usb_ui_v1_sync_product(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product)
{
    if (ui == NULL || product == NULL ||
        blu2usb_interaction_is_locked(&ui->interaction))
        return no_intent();

    if (ui->screen == BLU2USB_UI_V1_HOME_CONNECTED &&
        !product->has_authoritative)
        return resolve_home(ui, product, false);

    if (is_profile_active_screen(ui->screen) && !product->has_authoritative) {
        if (ui->screen == BLU2USB_UI_V1_PASSTHROUGH_ACTIVE)
            show(ui, BLU2USB_UI_V1_PASSTHROUGH_NOT_ACTIVE);
        else if (ui->screen == BLU2USB_UI_V1_STANDARD_ACTIVE)
            show(ui, BLU2USB_UI_V1_STANDARD_NOT_ACTIVE);
        else if (ui->screen == BLU2USB_UI_V1_ESCAPE_ACTIVE)
            show(ui, BLU2USB_UI_V1_ESCAPE_NOT_ACTIVE);
    }

    if (ui->screen == BLU2USB_UI_V1_SAVED_DEVICES) {
        if (product->saved_count == 0u)
            return resolve_home(ui, product, false);
        if (ui->page >= product->saved_count)
            ui->page = (uint8_t)(product->saved_count - 1u);
    }

    if (ui->screen == BLU2USB_UI_V1_REMOVE_THIS &&
        blu2usb_product_snapshot_find(product, ui->remove_target_id) == NULL) {
        ui->pending_remove = false;
        if (product->saved_count == 0u)
            return resolve_home(ui, product, false);
        show(ui, BLU2USB_UI_V1_SAVED_DEVICES);
        if (ui->page >= product->saved_count)
            ui->page = (uint8_t)(product->saved_count - 1u);
    }

    return no_intent();
}

blu2usb_ui_v1_intent_t blu2usb_ui_v1_search_result(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    blu2usb_ui_v1_search_purpose_t purpose,
    blu2usb_ui_v1_search_result_t result)
{
    if (ui == NULL || !ui->search_active || ui->search_purpose != purpose)
        return no_intent();

    ui->search_active = false;

    if (result == BLU2USB_UI_V1_SEARCH_FOUND) {
        if (purpose == BLU2USB_UI_V1_SEARCH_FIRST) {
            show(ui, BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED);
        } else if (purpose == BLU2USB_UI_V1_SEARCH_SAVED) {
            show(ui, product != NULL && product->has_authoritative
                ? BLU2USB_UI_V1_HOME_CONNECTED
                : BLU2USB_UI_V1_HOME_RETRY);
        } else {
            show(ui, product != NULL && product->has_authoritative
                ? BLU2USB_UI_V1_HOME_CONNECTED
                : BLU2USB_UI_V1_RETRY_PAIR_NEW);
        }
        return no_intent();
    }

    if (purpose == BLU2USB_UI_V1_SEARCH_FIRST) {
        show(ui, BLU2USB_UI_V1_SEARCHING_FIRST);
        ui->search_active = true;
        ui->search_purpose = BLU2USB_UI_V1_SEARCH_FIRST;
        blu2usb_ui_v1_intent_t intent = no_intent();
        intent.kind = BLU2USB_UI_V1_INTENT_SEARCH_FIRST;
        return intent;
    }

    show(ui, purpose == BLU2USB_UI_V1_SEARCH_SAVED
        ? BLU2USB_UI_V1_HOME_RETRY
        : BLU2USB_UI_V1_RETRY_PAIR_NEW);
    return no_intent();
}

void blu2usb_ui_v1_profile_result(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    bool success)
{
    if (ui == NULL || !ui->pending_profile) return;

    const blu2usb_mouse_profile_kind_t requested = ui->pending_profile_kind;
    ui->pending_profile = false;

    if (!success || current_mouse(product) == NULL ||
        current_profile(product) != requested)
        return;

    if (requested == BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP) {
        ui->custom_dirty = false;
        show_selection(ui, BLU2USB_UI_V1_CUSTOM_EDIT, ui->selection);
    } else {
        show(ui, active_screen_for(requested));
    }
}

blu2usb_ui_v1_intent_t blu2usb_ui_v1_remove_result(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    bool success)
{
    if (ui == NULL || !ui->pending_remove) return no_intent();

    ui->pending_remove = false;
    if (!success) return no_intent();

    ui->remove_target_id = BLU2USB_MOUSE_ID_INVALID;
    if (product == NULL || product->saved_count == 0u)
        return resolve_home(ui, product, false);

    if (ui->page >= product->saved_count)
        ui->page = (uint8_t)(product->saved_count - 1u);
    show(ui, BLU2USB_UI_V1_SAVED_DEVICES);
    return no_intent();
}

const char *blu2usb_ui_v1_screen_name(blu2usb_ui_v1_screen_t screen)
{
    static const char *const names[BLU2USB_UI_V1_SCREEN_COUNT] = {
        "searching-first",
        "first-mouse-connected",
        "home-searching",
        "home-searching-help",
        "home-retry",
        "home-retry-help",
        "pair-new",
        "help-pair-new",
        "retry-pair-new",
        "help-retry-pair-new",
        "home-connected",
        "help-home-connected",
        "remapper-options",
        "help-remapper-options",
        "passthrough-active",
        "passthrough-not-active",
        "standard-not-active",
        "standard-active",
        "escape-not-active",
        "escape-active",
        "custom-edit",
        "left",
        "right",
        "middle",
        "forward",
        "backward",
        "saved-devices",
        "remove-this",
        "help-remove-this",
        "learn-the-keys",
    };

    return (unsigned)screen < BLU2USB_UI_V1_SCREEN_COUNT
        ? names[screen] : "invalid";
}
