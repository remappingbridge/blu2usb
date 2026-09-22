#include <stdio.h>
#include <string.h>

#include "blu2usb/renderer/renderer_v1.h"
#include "blu2usb/ux_model/ux_v1.h"

static int failures = 0;

#define CHECK(condition) do {     if (!(condition)) {         fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);         ++failures;     } } while (0)

static blu2usb_ui_v1_intent_t tap(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    blu2usb_control_t control)
{
    (void)blu2usb_ui_v1_input(ui, product, control, true);
    return blu2usb_ui_v1_input(ui, product, control, false);
}

static void build(
    const blu2usb_device_registry_t *registry,
    blu2usb_product_snapshot_t *snapshot)
{
    CHECK(blu2usb_product_snapshot_build(registry, snapshot));
}

static void add_mouse(
    blu2usb_device_registry_t *registry,
    blu2usb_mouse_id_t id,
    const char *name,
    blu2usb_mouse_profile_kind_t profile)
{
    CHECK(blu2usb_device_registry_add(registry, id, name, profile));
}

static void test_first_start_and_instructional_lock(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_product_snapshot_t product;
    blu2usb_ui_v1_t ui;

    blu2usb_device_registry_init(&registry);
    build(&registry, &product);
    blu2usb_ui_v1_init(&ui);

    blu2usb_ui_v1_intent_t intent = blu2usb_ui_v1_home(&ui, &product);
    CHECK(ui.screen == BLU2USB_UI_V1_SEARCHING_FIRST);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_SEARCH_FIRST);

    intent = tap(&ui, &product, BLU2USB_CONTROL_KEY_B);
    CHECK(ui.screen == BLU2USB_UI_V1_SEARCHING_FIRST);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_NONE);

    (void)tap(&ui, &product, BLU2USB_CONTROL_KEY_Y);
    CHECK(!blu2usb_interaction_is_locked(&ui.interaction));

    add_mouse(&registry, UINT64_C(1), "Lift",
              BLU2USB_MOUSE_PROFILE_PASSTHROUGH);
    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(1)));
    build(&registry, &product);

    intent = blu2usb_ui_v1_search_result(
        &ui, &product, BLU2USB_UI_V1_SEARCH_FIRST,
        BLU2USB_UI_V1_SEARCH_FOUND);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_NONE);
    CHECK(ui.screen == BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED);

    (void)tap(&ui, &product, BLU2USB_CONTROL_KEY_B);
    CHECK(ui.screen == BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED);

    intent = tap(&ui, &product, BLU2USB_CONTROL_KEY_Y);
    CHECK(intent.cancel_owned == false);
    CHECK(blu2usb_interaction_is_locked(&ui.interaction));

    (void)blu2usb_ui_v1_input(
        &ui, &product, BLU2USB_CONTROL_KEY_A, true);
    intent = blu2usb_ui_v1_input(
        &ui, &product, BLU2USB_CONTROL_KEY_A, false);
    CHECK(!blu2usb_interaction_is_locked(&ui.interaction));
    CHECK(ui.screen == BLU2USB_UI_V1_HOME_CONNECTED);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_NONE);
}

static void test_help_priority_and_learn_lock(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_product_snapshot_t product;
    blu2usb_ui_v1_t ui;

    blu2usb_device_registry_init(&registry);
    add_mouse(&registry, UINT64_C(1), "Lift",
              BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP);
    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(1)));
    build(&registry, &product);
    blu2usb_ui_v1_init(&ui);
    (void)blu2usb_ui_v1_home(&ui, &product);

    (void)tap(&ui, &product, BLU2USB_CONTROL_KEY_X);
    CHECK(ui.screen == BLU2USB_UI_V1_HELP_HOME_CONNECTED);

    (void)tap(&ui, &product, BLU2USB_CONTROL_KEY_Y);
    CHECK(ui.screen == BLU2USB_UI_V1_HOME_CONNECTED);
    CHECK(!blu2usb_interaction_is_locked(&ui.interaction));

    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_DOWN);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_DOWN);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_DOWN);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_PRESS);
    CHECK(ui.screen == BLU2USB_UI_V1_LEARN_THE_KEYS);

    (void)tap(&ui, &product, BLU2USB_CONTROL_KEY_B);
    CHECK(ui.screen == BLU2USB_UI_V1_LEARN_THE_KEYS);

    (void)tap(&ui, &product, BLU2USB_CONTROL_KEY_Y);
    CHECK(blu2usb_interaction_is_locked(&ui.interaction));

    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_LEFT);
    CHECK(!blu2usb_interaction_is_locked(&ui.interaction));
    CHECK(ui.screen == BLU2USB_UI_V1_HOME_CONNECTED);
}

static void test_saved_search_retry_pair_new_and_help_cancel(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_product_snapshot_t product;
    blu2usb_ui_v1_t ui;

    blu2usb_device_registry_init(&registry);
    add_mouse(&registry, UINT64_C(1), "Lift",
              BLU2USB_MOUSE_PROFILE_PASSTHROUGH);
    build(&registry, &product);
    blu2usb_ui_v1_init(&ui);

    blu2usb_ui_v1_intent_t intent = blu2usb_ui_v1_home(&ui, &product);
    CHECK(ui.screen == BLU2USB_UI_V1_HOME_SEARCHING);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_SEARCH_SAVED);

    intent = tap(&ui, &product, BLU2USB_CONTROL_KEY_B);
    CHECK(intent.cancel_owned);
    CHECK(ui.screen == BLU2USB_UI_V1_HOME_RETRY);

    intent = tap(&ui, &product, BLU2USB_CONTROL_KEY_A);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_SEARCH_SAVED);
    CHECK(ui.screen == BLU2USB_UI_V1_HOME_SEARCHING);

    intent = blu2usb_ui_v1_search_result(
        &ui, &product, BLU2USB_UI_V1_SEARCH_SAVED,
        BLU2USB_UI_V1_SEARCH_TIMED_OUT);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_NONE);
    CHECK(ui.screen == BLU2USB_UI_V1_HOME_RETRY);

    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_DOWN);
    intent = tap(&ui, &product, BLU2USB_CONTROL_JOY_PRESS);
    CHECK(ui.screen == BLU2USB_UI_V1_PAIR_NEW);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_SEARCH_NEW);

    intent = tap(&ui, &product, BLU2USB_CONTROL_KEY_X);
    CHECK(intent.cancel_owned);
    CHECK(ui.screen == BLU2USB_UI_V1_HELP_PAIR_NEW);

    (void)tap(&ui, &product, BLU2USB_CONTROL_KEY_A);
    CHECK(ui.screen == BLU2USB_UI_V1_RETRY_PAIR_NEW);

    intent = tap(&ui, &product, BLU2USB_CONTROL_KEY_B);
    CHECK(intent.cancel_owned);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_SEARCH_SAVED);
    CHECK(ui.screen == BLU2USB_UI_V1_HOME_SEARCHING);
}

static void test_profiles_custom_and_disconnect_projection(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_product_snapshot_t product;
    blu2usb_ui_v1_t ui;

    blu2usb_device_registry_init(&registry);
    add_mouse(&registry, UINT64_C(1), "Lift",
              BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP);
    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(1)));
    build(&registry, &product);
    blu2usb_ui_v1_init(&ui);
    (void)blu2usb_ui_v1_home(&ui, &product);

    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_PRESS);
    CHECK(ui.screen == BLU2USB_UI_V1_REMAPPER_OPTIONS);

    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_DOWN);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_PRESS);
    CHECK(ui.screen == BLU2USB_UI_V1_STANDARD_NOT_ACTIVE);

    blu2usb_ui_v1_intent_t intent =
        tap(&ui, &product, BLU2USB_CONTROL_KEY_A);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_APPLY_PROFILE);
    CHECK(intent.profile == BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP);
    CHECK(intent.mouse_id == UINT64_C(1));

    CHECK(blu2usb_device_registry_set_profile(
        &registry, UINT64_C(1), BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP));
    build(&registry, &product);
    blu2usb_ui_v1_profile_result(&ui, &product, true);
    CHECK(ui.screen == BLU2USB_UI_V1_STANDARD_ACTIVE);

    blu2usb_device_registry_clear_authoritative(&registry);
    build(&registry, &product);
    intent = blu2usb_ui_v1_sync_product(&ui, &product);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_NONE);
    CHECK(ui.screen == BLU2USB_UI_V1_STANDARD_NOT_ACTIVE);

    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(1)));
    build(&registry, &product);
    (void)blu2usb_ui_v1_home(&ui, &product);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_PRESS);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_DOWN);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_DOWN);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_DOWN);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_PRESS);
    CHECK(ui.screen == BLU2USB_UI_V1_CUSTOM_EDIT);

    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_PRESS);
    CHECK(ui.screen == BLU2USB_UI_V1_LEFT);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_DOWN);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_DOWN);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_DOWN);
    intent = tap(&ui, &product, BLU2USB_CONTROL_KEY_A);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_CUSTOM_SET_TARGET);
    CHECK(intent.source == BLU2USB_MOUSE_SOURCE_LEFT);
    CHECK(intent.target == BLU2USB_MOUSE_TARGET_ESCAPE);
    CHECK(ui.screen == BLU2USB_UI_V1_CUSTOM_EDIT);
    CHECK(ui.custom_targets[BLU2USB_MOUSE_SOURCE_LEFT] ==
          BLU2USB_MOUSE_TARGET_ESCAPE);
    CHECK(ui.custom_dirty);

    intent = tap(&ui, &product, BLU2USB_CONTROL_KEY_A);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_APPLY_CUSTOM);
    CHECK(intent.mouse_id == UINT64_C(1));

    CHECK(blu2usb_device_registry_set_profile(
        &registry, UINT64_C(1), BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP));
    build(&registry, &product);
    blu2usb_ui_v1_profile_result(&ui, &product, true);
    CHECK(ui.screen == BLU2USB_UI_V1_CUSTOM_EDIT);
    CHECK(!ui.custom_dirty);
}

static void test_saved_device_identity_stability_and_remove(void)
{
    blu2usb_device_registry_t registry;
    blu2usb_product_snapshot_t product;
    blu2usb_ui_v1_t ui;

    blu2usb_device_registry_init(&registry);
    add_mouse(&registry, UINT64_C(1), "Lift",
              BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP);
    add_mouse(&registry, UINT64_C(2), "Office Mouse",
              BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP);
    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(1)));
    build(&registry, &product);
    blu2usb_ui_v1_init(&ui);
    (void)blu2usb_ui_v1_home(&ui, &product);

    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_DOWN);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_PRESS);
    CHECK(ui.screen == BLU2USB_UI_V1_SAVED_DEVICES);
    CHECK(ui.page == 0u);

    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_RIGHT);
    CHECK(ui.page == 1u);
    (void)tap(&ui, &product, BLU2USB_CONTROL_JOY_PRESS);
    CHECK(ui.screen == BLU2USB_UI_V1_REMOVE_THIS);
    CHECK(ui.remove_target_id == UINT64_C(2));

    CHECK(blu2usb_device_registry_set_authoritative(&registry, UINT64_C(2)));
    build(&registry, &product);
    CHECK(product.mice[0].id == UINT64_C(2));
    CHECK(ui.remove_target_id == UINT64_C(2));

    blu2usb_ui_v1_intent_t intent =
        tap(&ui, &product, BLU2USB_CONTROL_KEY_X);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_NONE);
    CHECK(ui.screen == BLU2USB_UI_V1_HELP_REMOVE_THIS);

    (void)tap(&ui, &product, BLU2USB_CONTROL_KEY_Y);
    CHECK(ui.screen == BLU2USB_UI_V1_REMOVE_THIS);
    CHECK(!blu2usb_interaction_is_locked(&ui.interaction));

    intent = tap(&ui, &product, BLU2USB_CONTROL_KEY_A);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_REMOVE_MOUSE);
    CHECK(intent.mouse_id == UINT64_C(2));

    CHECK(blu2usb_device_registry_remove(&registry, UINT64_C(2)));
    build(&registry, &product);
    intent = blu2usb_ui_v1_remove_result(&ui, &product, true);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_NONE);
    CHECK(ui.screen == BLU2USB_UI_V1_SAVED_DEVICES);
    CHECK(product.saved_count == 1u);
    CHECK(product.mice[0].id == UINT64_C(1));

    ui.remove_target_id = UINT64_C(1);
    ui.pending_remove = true;
    CHECK(blu2usb_device_registry_remove(&registry, UINT64_C(1)));
    build(&registry, &product);
    intent = blu2usb_ui_v1_remove_result(&ui, &product, true);
    CHECK(ui.screen == BLU2USB_UI_V1_SEARCHING_FIRST);
    CHECK(intent.kind == BLU2USB_UI_V1_INTENT_SEARCH_FIRST);
}

int main(void)
{
    test_first_start_and_instructional_lock();
    test_help_priority_and_learn_lock();
    test_saved_search_retry_pair_new_and_help_cancel();
    test_profiles_custom_and_disconnect_projection();
    test_saved_device_identity_stability_and_remove();

    if (failures != 0) {
        fprintf(stderr, "%d MUX-03 navigation assertion(s) failed\n", failures);
        return 1;
    }

    puts("MUX-03 UI Layout 1.0 navigation PASS");
    return 0;
}
