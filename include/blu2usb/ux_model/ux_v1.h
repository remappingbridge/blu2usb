#ifndef BLU2USB_UX_MODEL_UX_V1_H
#define BLU2USB_UX_MODEL_UX_V1_H

#include <stdbool.h>
#include <stdint.h>

#include "blu2usb/device_registry/product_snapshot.h"
#include "blu2usb/domain/control.h"
#include "blu2usb/domain/profile.h"
#include "blu2usb/interaction/interaction.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLU2USB_UI_V1_SEARCHING_FIRST = 0,
    BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED,
    BLU2USB_UI_V1_HOME_SEARCHING,
    BLU2USB_UI_V1_HOME_SEARCHING_HELP,
    BLU2USB_UI_V1_HOME_RETRY,
    BLU2USB_UI_V1_HOME_RETRY_HELP,
    BLU2USB_UI_V1_PAIR_NEW,
    BLU2USB_UI_V1_HELP_PAIR_NEW,
    BLU2USB_UI_V1_RETRY_PAIR_NEW,
    BLU2USB_UI_V1_HELP_RETRY_PAIR_NEW,
    BLU2USB_UI_V1_HOME_CONNECTED,
    BLU2USB_UI_V1_HELP_HOME_CONNECTED,
    BLU2USB_UI_V1_REMAPPER_OPTIONS,
    BLU2USB_UI_V1_HELP_REMAPPER_OPTIONS,
    BLU2USB_UI_V1_PASSTHROUGH_ACTIVE,
    BLU2USB_UI_V1_PASSTHROUGH_NOT_ACTIVE,
    BLU2USB_UI_V1_STANDARD_NOT_ACTIVE,
    BLU2USB_UI_V1_STANDARD_ACTIVE,
    BLU2USB_UI_V1_ESCAPE_NOT_ACTIVE,
    BLU2USB_UI_V1_ESCAPE_ACTIVE,
    BLU2USB_UI_V1_CUSTOM_EDIT,
    BLU2USB_UI_V1_LEFT,
    BLU2USB_UI_V1_RIGHT,
    BLU2USB_UI_V1_MIDDLE,
    BLU2USB_UI_V1_FORWARD,
    BLU2USB_UI_V1_BACKWARD,
    BLU2USB_UI_V1_SAVED_DEVICES,
    BLU2USB_UI_V1_REMOVE_THIS,
    BLU2USB_UI_V1_HELP_REMOVE_THIS,
    BLU2USB_UI_V1_LEARN_THE_KEYS,
    BLU2USB_UI_V1_SCREEN_COUNT
} blu2usb_ui_v1_screen_t;

typedef enum {
    BLU2USB_UI_V1_INTENT_NONE = 0,
    BLU2USB_UI_V1_INTENT_SEARCH_FIRST,
    BLU2USB_UI_V1_INTENT_SEARCH_SAVED,
    BLU2USB_UI_V1_INTENT_SEARCH_NEW,
    BLU2USB_UI_V1_INTENT_APPLY_PROFILE,
    BLU2USB_UI_V1_INTENT_APPLY_CUSTOM,
    BLU2USB_UI_V1_INTENT_CUSTOM_SET_TARGET,
    BLU2USB_UI_V1_INTENT_REMOVE_MOUSE
} blu2usb_ui_v1_intent_kind_t;

typedef struct {
    bool cancel_owned;
    blu2usb_ui_v1_intent_kind_t kind;
    blu2usb_mouse_id_t mouse_id;
    blu2usb_mouse_profile_kind_t profile;
    blu2usb_mouse_source_t source;
    blu2usb_mouse_target_t target;
} blu2usb_ui_v1_intent_t;

typedef enum {
    BLU2USB_UI_V1_SEARCH_FIRST = 0,
    BLU2USB_UI_V1_SEARCH_SAVED,
    BLU2USB_UI_V1_SEARCH_NEW
} blu2usb_ui_v1_search_purpose_t;

typedef enum {
    BLU2USB_UI_V1_SEARCH_FOUND = 0,
    BLU2USB_UI_V1_SEARCH_TIMED_OUT,
    BLU2USB_UI_V1_SEARCH_FAILED,
    BLU2USB_UI_V1_SEARCH_CANCELLED
} blu2usb_ui_v1_search_result_t;

typedef struct {
    blu2usb_interaction_t interaction;
    blu2usb_ui_v1_screen_t screen;
    blu2usb_ui_v1_screen_t help_owner;
    uint8_t selection;
    uint8_t page;
    uint8_t help_selection;
    uint8_t help_page;
    bool search_active;
    blu2usb_ui_v1_search_purpose_t search_purpose;
    bool pending_profile;
    blu2usb_mouse_profile_kind_t pending_profile_kind;
    bool pending_remove;
    blu2usb_mouse_id_t remove_target_id;
    bool custom_dirty;
    blu2usb_mouse_target_t custom_targets[BLU2USB_MOUSE_SOURCE_COUNT];
} blu2usb_ui_v1_t;

void blu2usb_ui_v1_init(blu2usb_ui_v1_t *ui);

blu2usb_ui_v1_intent_t blu2usb_ui_v1_home(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product);

blu2usb_ui_v1_intent_t blu2usb_ui_v1_input(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    blu2usb_control_t control,
    bool pressed);

blu2usb_ui_v1_intent_t blu2usb_ui_v1_sync_product(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product);

blu2usb_ui_v1_intent_t blu2usb_ui_v1_search_result(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    blu2usb_ui_v1_search_purpose_t purpose,
    blu2usb_ui_v1_search_result_t result);

void blu2usb_ui_v1_profile_result(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    bool success);

void blu2usb_ui_v1_remove_result(
    blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    bool success);

const char *blu2usb_ui_v1_screen_name(blu2usb_ui_v1_screen_t screen);
unsigned blu2usb_ui_v1_option_count(const blu2usb_ui_v1_t *ui);
bool blu2usb_ui_v1_is_help(blu2usb_ui_v1_screen_t screen);

#ifdef __cplusplus
}
#endif

#endif
