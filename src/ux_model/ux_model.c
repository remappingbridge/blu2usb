#include "blu2usb/ux_model/ux_model.h"

#define DYN(row) ((uint16_t)(1u << (row)))
#define EMPTY ""

static const blu2usb_screen_template_t screens[BLU2USB_SCREEN_COUNT] = {
    [BLU2USB_SCREEN_HOME] = {{"HOME"," STATUS"," MOUSE OPTIONS"," OTHER OPTIONS"," LEARN THE KEYS",EMPTY,"JOY UP / DOWN: SELECT","JOY PRESS: ACCESS","KEY Y: LOCK / UNLOCK"},0},
    [BLU2USB_SCREEN_MOUSE_STATUS] = {{"MOUSE STATUS","MOUSE NOT CONNECTED","PROFILE: PASSTHROUGH","FWD: AUTO HIDPP","BACK: AUTO STD",EMPTY,"JOY RIGHT\\LEFT: PAGE","KEY B: BACK","KEY X: MOUSE HELP"},DYN(1)|DYN(2)|DYN(3)|DYN(4)},
    [BLU2USB_SCREEN_OTHER_DEVICES_STATUS] = {{"OTHER DEVICES STATUS","KEYBOARD","CONNECTED","COMPOSITE","NOT CONNECTED",EMPTY,"JOY RIGHT\\LEFT: PAGE","KEY B: BACK","KEY X: DEVICES HELP"},DYN(1)|DYN(2)|DYN(3)|DYN(4)},
    [BLU2USB_SCREEN_MOUSE_HELP] = {{"MOUSE HELP",EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,"ANY KEY: BACK"},0x00fe},
    [BLU2USB_SCREEN_DEVICES_HELP] = {{"DEVICES HELP","KEYBOARD IS DIFFERENT","FROM COMPOSITE.","COMPOSITE IS TOUCHPAD","AND KEYBOARD EMBEDDED","TOGETHER AND IT PAIRS","ITS OWN BLUETOOTH.",EMPTY,"ANY KEY: BACK"},0},
    [BLU2USB_SCREEN_MOUSE_OPTIONS] = {{"MOUSE OPTIONS"," PAIR MOUSE"," PASSTHROUGH"," DEFAULT REMAP"," ESCAPE REMAP"," CUSTOM REMAP",EMPTY,"JOY PRESS: ACCESS","KEY B: BACK"},0},
    [BLU2USB_SCREEN_PAIR_MOUSE] = {{"PAIR MOUSE","SEARCHING BLE HID","TARGET MOUSE","AUTO SEARCH ACTIVE","FOUND 0 HID",EMPTY,"KEY A: RETRY ON ERROR","KEY B: CANCEL","KEY X: HELP"},DYN(1)|DYN(2)|DYN(3)|DYN(4)},
    [BLU2USB_SCREEN_PAIR_MOUSE_HELP] = {{"PAIR MOUSE HELP",EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,"ANY KEY: BACK"},0x00fe},
    [BLU2USB_SCREEN_MOUSE_SAVED] = {{"MOUSE PAIRED","MOUSE CONNECTED","READY TO USE",EMPTY,EMPTY,EMPTY,EMPTY,"KEY B: BACK","KEY Y: LOCK"},0x007e},
    [BLU2USB_SCREEN_APPLY_PASSTHROUGH] = {{"APPLY PASSTHROUGH","ORIGINAL MOUSE","BUTTONS POSITION","ARE NOT ACTIVE",EMPTY,EMPTY,"KEY A: APPLY","KEY B: CANCEL","KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_PASSTHROUGH_APPLIED] = {{"PASSTHROUGH APPLIED","ORIGINAL MOUSE","BUTTONS POSITION","ARE ACTIVE NOW",EMPTY,EMPTY,EMPTY,"KEY B: BACK","KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_APPLY_DEFAULT] = {{"APPLY DEFAULT REMAP","FORWARD IS LEFT","LEFT IS FORWARD","BACKWARD IS RIGHT","RIGHT IS BACKWARD",EMPTY,"KEY A: APPLY","KEY B: CANCEL","KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_DEFAULT_APPLIED] = {{"DEFAULT REMAP APPLIED","FORWARD IS LEFT","LEFT IS FORWARD","BACKWARD IS RIGHT","RIGHT IS BACKWARD",EMPTY,EMPTY,"KEY B: BACK","KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_APPLY_ESCAPE] = {{"APPLY ESCAPE","FORWARD IS LEFT","BACKWARD IS RIGHT","LEFT IS ESCAPE","RIGHT IS BACKWARD","MIDDLE IS FORWARD",EMPTY,"KEY A: APPLY","KEY B: CANCEL"},0},
    [BLU2USB_SCREEN_ESCAPE_APPLIED] = {{"ESCAPE APPLIED","FORWARD IS LEFT","BACKWARD IS RIGHT","LEFT IS ESCAPE","RIGHT IS BACKWARD","MIDDLE IS FORWARD",EMPTY,"KEY B: BACK","KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_EDIT_CUSTOM] = {{"EDIT CUSTOM REMAP"," LEFT IS LEFT"," RIGHT IS RIGHT"," MIDDLE IS MIDDLE"," FORWARD IS FORWARD"," BACKWARD IS BACKWARD",EMPTY,"JOY PRESS: ACCESS","KEY A: APPLY CUSTOM"},DYN(1)|DYN(2)|DYN(3)|DYN(4)|DYN(5)},
    [BLU2USB_SCREEN_CUSTOM_APPLIED] = {{"CUSTOM APPLIED",EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,"KEY B: BACK","KEY Y: LOCK"},DYN(1)|DYN(2)|DYN(3)|DYN(4)|DYN(5)},
    [BLU2USB_SCREEN_LEFT_WILL_BECOME] = {{"LEFT WILL BECOME"," LEFT"," RIGHT"," MIDDLE"," BACKWARD"," FORWARD"," ESCAPE",EMPTY,"KEY A: APPLY AND BACK"},0},
    [BLU2USB_SCREEN_RIGHT_WILL_BECOME] = {{"RIGHT WILL BECOME"," LEFT"," RIGHT"," MIDDLE"," BACKWARD"," FORWARD"," ESCAPE",EMPTY,"KEY A: APPLY AND BACK"},0},
    [BLU2USB_SCREEN_MIDDLE_WILL_BECOME] = {{"MIDDLE WILL BECOME"," LEFT"," RIGHT"," MIDDLE"," BACKWARD"," FORWARD"," ESCAPE",EMPTY,"KEY A: APPLY AND BACK"},0},
    [BLU2USB_SCREEN_FORWARD_WILL_BECOME] = {{"FORWARD WILL BECOME"," LEFT"," RIGHT"," MIDDLE"," BACKWARD"," FORWARD"," ESCAPE",EMPTY,"KEY A: APPLY AND BACK"},0},
    [BLU2USB_SCREEN_BACKWARD_WILL_BECOME] = {{"BACKWARD WILL BECOME"," LEFT"," RIGHT"," MIDDLE"," BACKWARD"," FORWARD"," ESCAPE",EMPTY,"KEY A: APPLY AND BACK"},0},
    [BLU2USB_SCREEN_OTHER_OPTIONS] = {{"OTHER OPTIONS"," PAIR KEYBOARD"," PAIR COMPOSITE"," SAVED DEVICES",EMPTY,EMPTY,"JOY UP\\DOWN: SELECT","JOY PRESS: ACCESS","KEY B: BACK"},0},
    [BLU2USB_SCREEN_OTHER_OPTIONS_HELP] = {{"OTHER OPTIONS HELP","KEYBOARD IS DIFFERENT","FROM COMPOSITE.","COMPOSITE IS TOUCHPAD","AND KEYBOARD EMBEDDED","TOGETHER AND IT PAIRS","ITS OWN BLUETOOTH.",EMPTY,"ANY KEY: BACK"},0},
    [BLU2USB_SCREEN_PAIR_KEYBOARD] = {{"PAIR KEYBOARD","SEARCHING KEYBOARD","TARGET KEYBOARD","AUTO SEARCH ACTIVE","FOUND 0 HID",EMPTY,"KEY A: RETRY ON ERROR","KEY B: CANCEL","KEY X: HELP"},DYN(1)|DYN(2)|DYN(3)|DYN(4)},
    [BLU2USB_SCREEN_PAIR_KEYBOARD_HELP] = {{"PAIR KEYBOARD HELP",EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,"ANY KEY: BACK"},0x00fe},
    [BLU2USB_SCREEN_KEYBOARD_SAVED] = {{"KEYBOARD SAVED","TYPE KEYBOARD","SAVED DEVICES UPDATED",EMPTY,EMPTY,EMPTY,EMPTY,"KEY B: BACK","KEY Y: LOCK"},0x007e},
    [BLU2USB_SCREEN_PAIR_COMPOSITE] = {{"PAIR COMPOSITE","SEARCHING BLE HID","TARGET COMPOSITE","AUTO SEARCH ACTIVE","FOUND 0 HID",EMPTY,"KEY A: RETRY ON ERROR","KEY B: CANCEL","KEY X: HELP"},DYN(1)|DYN(2)|DYN(3)|DYN(4)},
    [BLU2USB_SCREEN_PAIR_COMPOSITE_HELP] = {{"PAIR COMPOSITE HELP",EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,"ANY KEY: BACK"},0x00fe},
    [BLU2USB_SCREEN_COMPOSITE_SAVED] = {{"COMPOSITE SAVED","TYPE COMPOSITE","SAVED DEVICES UPDATED",EMPTY,EMPTY,EMPTY,EMPTY,"KEY B: BACK","KEY Y: LOCK"},0x007e},
    [BLU2USB_SCREEN_SAVED_DEVICES] = {{"1-4 OF 6 SAVED"," BKB-3G"," OFFICE MOUSE"," TRAVEL KEYBOARD"," GENERIC MOUSE",EMPTY,"JOY UP\\DOWN: SELECT","JOY PRESS: ACCESS","JOY RIGHT\\LEFT: PAGE"},0x003f},
    [BLU2USB_SCREEN_DEVICE_DETAILS_MOUSE] = {{"DEVICE DETAILS","LOGITECH LIFT","TYPE: MOUSE","STATUS: CONNECTED","PROFILE: DEFAULT"," REMOVE DEVICE",EMPTY,"JOY PRESS: ACCESS","KEY B: BACK"},DYN(1)|DYN(2)|DYN(3)|DYN(4)},
    [BLU2USB_SCREEN_DEVICE_DETAILS_KEYBOARD] = {{"DEVICE DETAILS","BKB-3G","TYPE: KEYBOARD","STATUS: SAVED"," REMOVE DEVICE",EMPTY,"JOY PRESS: ACCESS","KEY B: BACK","KEY Y: LOCK"},DYN(1)|DYN(2)|DYN(3)},
    [BLU2USB_SCREEN_DEVICE_DETAILS_COMPOSITE] = {{"DEVICE DETAILS","DESK COMPOSITE","TYPE: COMPOSITE","STATUS: SAVED"," REMOVE DEVICE",EMPTY,"JOY PRESS: ACCESS","KEY B: BACK","KEY Y: LOCK"},DYN(1)|DYN(2)|DYN(3)},
    [BLU2USB_SCREEN_REMOVE_DEVICE] = {{"REMOVE DEVICE","BKB-3G","PAIRING AND MAPPINGS","WILL BE DELETED",EMPTY,EMPTY,"KEY A: REMOVE","KEY B: CANCEL","KEY Y: LOCK"},DYN(1)},
    [BLU2USB_SCREEN_LEARN_KEYS] = {{"PRESS TO LEARN A KEY","      JOY UP","JOY    JOY    JOY","LEFT  PRESS  RIGHT","     JOY DOWN","               KEY A","LOCK SCREEN    KEY B"," AND UNLOCK    KEY X","  OPEN HOME -> KEY Y"},0},
    [BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE] = {{"SEARCHING FIRST MOUSE","PRESS TO LEARN KEYS","WHILE WAIT CONNECTION","       JOY UP","  JOY    JOY    JOY","  LEFT  PRESS  RIGHT","      JOY DOWN"," KEY A         KEY X"," KEY B         KEY Y"},0},
    [BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED] = {{"FIRST MOUSE CONNECTED","       JOY UP","  JOY    JOY    JOY","  LEFT  PRESS  RIGHT","      JOY DOWN"," KEY A         KEY X"," KEY B         KEY Y",EMPTY," KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_HOME_SEARCHING] = {{"SEARCHING SAVED MOUSE"," SAVED DEVICES"," PAIR NEW MOUSE"," LEARN THE KEYS",EMPTY,"KEY B: CANCEL SEARCH","JOY UP / DOWN: SELECT","JOY PRESS: ACCESS","KEY X: HELP"},0},
    [BLU2USB_SCREEN_HOME_SEARCHING_HELP] = {{"HOME SEARCHING HELP","THE MATCHING ATTEMPT","TOOK PLACE ONLY FOR","DEVICES ALREADY SAVED","IN THE PREFERENCES,","BUT NOT FOR DEVICES","THAT WERE NOT SAVED.",EMPTY,"ANY KEY: BACK"},0},
    [BLU2USB_SCREEN_HOME_RETRY] = {{"DEVICE NOT FOUND"," SAVED DEVICES"," PAIR NEW MOUSE"," LEARN THE KEYS",EMPTY,"KEY A: RETRY SEARCH","JOY UP / DOWN: SELECT","JOY PRESS: ACCESS","KEY X: HELP"},0},
    [BLU2USB_SCREEN_HOME_RETRY_HELP] = {{"HOME RETRY HELP","THE MATCHING ATTEMPT","TOOK PLACE ONLY FOR","DEVICES ALREADY SAVED","IN THE PREFERENCES,","BUT NOT FOR DEVICES","THAT WERE NOT SAVED.",EMPTY,"ANY KEY: BACK"},0},
    [BLU2USB_SCREEN_HOME_CONNECTED] = {{"MOUSE"," PASSTHROUGH"," SAVED DEVICES"," PAIR NEW MOUSE"," LEARN THE KEYS",EMPTY,"JOY UP / DOWN: SELECT","JOY PRESS: ACCESS","KEY X: HELP TO REMOVE"},DYN(1)},
    [BLU2USB_SCREEN_HOME_CONNECTED_HELP] = {{"REMOVE CONNECTED HELP","TO DISCONNECT THE","CURRENTLY CONNECTED","MOUSE NAVIGATE TO:","STEP 1. SAVED DEVICES","STEP 2. REMOVE DEVICE","STEP 3. KEY A: REMOVE",EMPTY,"ANY KEY: BACK"},0},
};

static blu2usb_ux_command_t no_command(void) {
    blu2usb_ux_command_t cmd = {BLU2USB_UX_COMMAND_NONE, BLU2USB_MOUSE_SOURCE_LEFT, BLU2USB_MOUSE_TARGET_LEFT};
    return cmd;
}

static bool is_help(blu2usb_screen_id_t screen) {
    return screen == BLU2USB_SCREEN_MOUSE_HELP || screen == BLU2USB_SCREEN_DEVICES_HELP ||
           screen == BLU2USB_SCREEN_PAIR_MOUSE_HELP || screen == BLU2USB_SCREEN_OTHER_OPTIONS_HELP ||
           screen == BLU2USB_SCREEN_PAIR_KEYBOARD_HELP || screen == BLU2USB_SCREEN_PAIR_COMPOSITE_HELP ||
           screen == BLU2USB_SCREEN_HOME_SEARCHING_HELP ||
           screen == BLU2USB_SCREEN_HOME_RETRY_HELP ||
           screen == BLU2USB_SCREEN_HOME_CONNECTED_HELP;
}

static bool is_will_become(blu2usb_screen_id_t screen) {
    return screen >= BLU2USB_SCREEN_LEFT_WILL_BECOME && screen <= BLU2USB_SCREEN_BACKWARD_WILL_BECOME;
}

static bool lock_allowed(blu2usb_screen_id_t screen) {
    return !is_help(screen) &&
           screen != BLU2USB_SCREEN_LEARN_KEYS &&
           screen != BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE &&
           screen != BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED;
}

static void enter(blu2usb_ux_model_t *ux, blu2usb_screen_id_t screen) {
    ux->screen = screen;
    ux->selection = 0;
}

static unsigned wrap_prev(unsigned value, unsigned count) { return count ? (value + count - 1u) % count : 0u; }
static unsigned wrap_next(unsigned value, unsigned count) { return count ? (value + 1u) % count : 0u; }

static blu2usb_screen_id_t back_target(const blu2usb_ux_model_t *ux) {
    switch (ux->screen) {
    case BLU2USB_SCREEN_HOME: return BLU2USB_SCREEN_HOME;
    case BLU2USB_SCREEN_MOUSE_STATUS:
    case BLU2USB_SCREEN_OTHER_DEVICES_STATUS: return BLU2USB_SCREEN_HOME;
    case BLU2USB_SCREEN_MOUSE_HELP:
    case BLU2USB_SCREEN_DEVICES_HELP:
    case BLU2USB_SCREEN_PAIR_MOUSE_HELP:
    case BLU2USB_SCREEN_OTHER_OPTIONS_HELP:
    case BLU2USB_SCREEN_PAIR_KEYBOARD_HELP:
    case BLU2USB_SCREEN_PAIR_COMPOSITE_HELP: return ux->return_screen;
    case BLU2USB_SCREEN_MOUSE_OPTIONS:
        return ux->home_v062_enabled ? ux->return_screen : BLU2USB_SCREEN_HOME;
    case BLU2USB_SCREEN_PAIR_MOUSE:
        return ux->home_v062_enabled ? ux->return_screen : BLU2USB_SCREEN_MOUSE_OPTIONS;
    case BLU2USB_SCREEN_MOUSE_SAVED: return BLU2USB_SCREEN_MOUSE_OPTIONS;
    case BLU2USB_SCREEN_APPLY_PASSTHROUGH:
    case BLU2USB_SCREEN_APPLY_DEFAULT:
    case BLU2USB_SCREEN_APPLY_ESCAPE:
    case BLU2USB_SCREEN_EDIT_CUSTOM:
    case BLU2USB_SCREEN_CUSTOM_APPLIED:
    case BLU2USB_SCREEN_PASSTHROUGH_APPLIED:
    case BLU2USB_SCREEN_DEFAULT_APPLIED:
    case BLU2USB_SCREEN_ESCAPE_APPLIED: return BLU2USB_SCREEN_MOUSE_OPTIONS;
    case BLU2USB_SCREEN_LEFT_WILL_BECOME:
    case BLU2USB_SCREEN_RIGHT_WILL_BECOME:
    case BLU2USB_SCREEN_MIDDLE_WILL_BECOME:
    case BLU2USB_SCREEN_FORWARD_WILL_BECOME:
    case BLU2USB_SCREEN_BACKWARD_WILL_BECOME: return BLU2USB_SCREEN_EDIT_CUSTOM;
    case BLU2USB_SCREEN_OTHER_OPTIONS: return BLU2USB_SCREEN_HOME;
    case BLU2USB_SCREEN_PAIR_KEYBOARD: return BLU2USB_SCREEN_OTHER_OPTIONS;
    case BLU2USB_SCREEN_KEYBOARD_SAVED: return BLU2USB_SCREEN_PAIR_KEYBOARD;
    case BLU2USB_SCREEN_PAIR_COMPOSITE: return BLU2USB_SCREEN_OTHER_OPTIONS;
    case BLU2USB_SCREEN_COMPOSITE_SAVED: return BLU2USB_SCREEN_PAIR_COMPOSITE;
    case BLU2USB_SCREEN_SAVED_DEVICES:
        return ux->home_v062_enabled ? ux->return_screen : BLU2USB_SCREEN_OTHER_OPTIONS;
    case BLU2USB_SCREEN_DEVICE_DETAILS_MOUSE:
    case BLU2USB_SCREEN_DEVICE_DETAILS_KEYBOARD:
    case BLU2USB_SCREEN_DEVICE_DETAILS_COMPOSITE: return BLU2USB_SCREEN_SAVED_DEVICES;
    case BLU2USB_SCREEN_REMOVE_DEVICE: return ux->return_screen;
    case BLU2USB_SCREEN_LEARN_KEYS: return BLU2USB_SCREEN_HOME;
    case BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE:
        return BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE;
    case BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED:
        return BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED;
    case BLU2USB_SCREEN_HOME_SEARCHING:
    case BLU2USB_SCREEN_HOME_RETRY:
    case BLU2USB_SCREEN_HOME_CONNECTED:
        return ux->screen;
    case BLU2USB_SCREEN_HOME_SEARCHING_HELP:
    case BLU2USB_SCREEN_HOME_RETRY_HELP:
    case BLU2USB_SCREEN_HOME_CONNECTED_HELP:
        return ux->return_screen;
    default: return BLU2USB_SCREEN_HOME;
    }
}

unsigned blu2usb_ux_option_count(const blu2usb_ux_model_t *ux) {
    switch (ux->screen) {
    case BLU2USB_SCREEN_HOME: return 4;
    case BLU2USB_SCREEN_HOME_SEARCHING:
    case BLU2USB_SCREEN_HOME_RETRY: return 3;
    case BLU2USB_SCREEN_HOME_CONNECTED: return 4;
    case BLU2USB_SCREEN_MOUSE_OPTIONS: return 5;
    case BLU2USB_SCREEN_OTHER_OPTIONS: return 3;
    case BLU2USB_SCREEN_EDIT_CUSTOM: return 5;
    case BLU2USB_SCREEN_LEFT_WILL_BECOME:
    case BLU2USB_SCREEN_RIGHT_WILL_BECOME:
    case BLU2USB_SCREEN_MIDDLE_WILL_BECOME:
    case BLU2USB_SCREEN_FORWARD_WILL_BECOME:
    case BLU2USB_SCREEN_BACKWARD_WILL_BECOME: return BLU2USB_MOUSE_TARGET_COUNT;
    case BLU2USB_SCREEN_SAVED_DEVICES: {
        unsigned start = ux->saved_page * 4u;
        unsigned remain = ux->saved_device_count > start ? ux->saved_device_count - start : 0u;
        return remain > 4u ? 4u : remain;
    }
    case BLU2USB_SCREEN_DEVICE_DETAILS_MOUSE:
    case BLU2USB_SCREEN_DEVICE_DETAILS_KEYBOARD:
    case BLU2USB_SCREEN_DEVICE_DETAILS_COMPOSITE: return 1;
    default: return 0;
    }
}

void blu2usb_ux_init(blu2usb_ux_model_t *ux) {
    blu2usb_interaction_init(&ux->interaction);
    ux->screen = BLU2USB_SCREEN_HOME;
    ux->return_screen = BLU2USB_SCREEN_HOME;
    ux->selection = 0;
    ux->status_page = 0;
    ux->saved_page = 0;
    ux->saved_pages = 1;
    ux->saved_device_count = 0;
    ux->active_profile = BLU2USB_MOUSE_PROFILE_PASSTHROUGH;
    ux->custom_dirty = false;
    ux->first_start_complete = true;
    ux->home_v062_enabled = false;
    ux->has_saved_mouse = false;
    ux->saved_search_failed = false;
    ux->custom_source = BLU2USB_MOUSE_SOURCE_LEFT;
    ux->custom_targets[BLU2USB_MOUSE_SOURCE_LEFT] = BLU2USB_MOUSE_TARGET_LEFT;
    ux->custom_targets[BLU2USB_MOUSE_SOURCE_RIGHT] = BLU2USB_MOUSE_TARGET_RIGHT;
    ux->custom_targets[BLU2USB_MOUSE_SOURCE_MIDDLE] = BLU2USB_MOUSE_TARGET_MIDDLE;
    ux->custom_targets[BLU2USB_MOUSE_SOURCE_FORWARD] = BLU2USB_MOUSE_TARGET_FORWARD;
    ux->custom_targets[BLU2USB_MOUSE_SOURCE_BACKWARD] = BLU2USB_MOUSE_TARGET_BACKWARD;
}

void blu2usb_ux_set_saved_device_count(blu2usb_ux_model_t *ux, unsigned count) {
    ux->saved_device_count = count;
    ux->saved_pages = count == 0 ? 1u : (count + 3u) / 4u;
    if (ux->saved_page >= ux->saved_pages) ux->saved_page = ux->saved_pages - 1u;
    if (ux->selection >= blu2usb_ux_option_count(ux)) ux->selection = 0;
}

void blu2usb_ux_set_custom_target(blu2usb_ux_model_t *ux, blu2usb_mouse_source_t source, blu2usb_mouse_target_t target) {
    if ((unsigned)source < BLU2USB_MOUSE_SOURCE_COUNT && (unsigned)target < BLU2USB_MOUSE_TARGET_COUNT) {
        if (ux->custom_targets[source] != target) ux->custom_dirty = true;
        ux->custom_targets[source] = target;
        if (is_will_become(ux->screen) && ux->custom_source == source) ux->selection = (unsigned)target;
    }
}

const blu2usb_screen_template_t *blu2usb_ux_screen_template(blu2usb_screen_id_t screen) {
    if ((unsigned)screen >= BLU2USB_SCREEN_COUNT) return 0;
    return &screens[screen];
}

static blu2usb_screen_id_t custom_screen_for(unsigned selection) {
    static const blu2usb_screen_id_t ids[5] = {
        BLU2USB_SCREEN_LEFT_WILL_BECOME, BLU2USB_SCREEN_RIGHT_WILL_BECOME,
        BLU2USB_SCREEN_MIDDLE_WILL_BECOME, BLU2USB_SCREEN_FORWARD_WILL_BECOME,
        BLU2USB_SCREEN_BACKWARD_WILL_BECOME
    };
    return ids[selection < 5 ? selection : 0];
}

static blu2usb_mouse_source_t custom_source_for(unsigned selection) {
    static const blu2usb_mouse_source_t ids[5] = {
        BLU2USB_MOUSE_SOURCE_LEFT, BLU2USB_MOUSE_SOURCE_RIGHT, BLU2USB_MOUSE_SOURCE_MIDDLE,
        BLU2USB_MOUSE_SOURCE_FORWARD, BLU2USB_MOUSE_SOURCE_BACKWARD
    };
    return ids[selection < 5 ? selection : 0];
}

static blu2usb_screen_id_t example_saved_device_details_for(unsigned index) {
    static const blu2usb_screen_id_t ids[6] = {
        BLU2USB_SCREEN_DEVICE_DETAILS_KEYBOARD,
        BLU2USB_SCREEN_DEVICE_DETAILS_MOUSE,
        BLU2USB_SCREEN_DEVICE_DETAILS_KEYBOARD,
        BLU2USB_SCREEN_DEVICE_DETAILS_MOUSE,
        BLU2USB_SCREEN_DEVICE_DETAILS_MOUSE,
        BLU2USB_SCREEN_DEVICE_DETAILS_KEYBOARD
    };
    return index < 6u ? ids[index] : BLU2USB_SCREEN_DEVICE_DETAILS_COMPOSITE;
}

static blu2usb_ux_command_t enter_home_resolved(blu2usb_ux_model_t *ux, bool start_saved_search)
{
    blu2usb_ux_command_t cmd = no_command();

    if (blu2usb_ux_mouse_connected()) {
        ux->saved_search_failed = false;
        enter(ux, BLU2USB_SCREEN_HOME_CONNECTED);
        return cmd;
    }

    if (!ux->has_saved_mouse) {
        enter(ux, BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE);
        return cmd;
    }

    if (ux->saved_search_failed && !start_saved_search) {
        enter(ux, BLU2USB_SCREEN_HOME_RETRY);
        return cmd;
    }

    ux->saved_search_failed = false;
    enter(ux, BLU2USB_SCREEN_HOME_SEARCHING);
    if (start_saved_search)
        cmd.kind = BLU2USB_UX_COMMAND_RETRY_SAVED_SEARCH;
    return cmd;
}

static bool is_v062_home(blu2usb_screen_id_t screen)
{
    return screen == BLU2USB_SCREEN_HOME_SEARCHING ||
           screen == BLU2USB_SCREEN_HOME_RETRY ||
           screen == BLU2USB_SCREEN_HOME_CONNECTED;
}

blu2usb_ux_command_t blu2usb_ux_input(blu2usb_ux_model_t *ux, blu2usb_control_t control, bool pressed) {
    blu2usb_ux_command_t cmd = no_command();
    blu2usb_interaction_event_t event = blu2usb_interaction_input(&ux->interaction, control, pressed);
    if (event.kind == BLU2USB_INTERACTION_NONE) return cmd;
    if (event.kind == BLU2USB_INTERACTION_UNLOCK) {
        if (!ux->first_start_complete) {
            if (ux->screen == BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED) {
                ux->first_start_complete = true;
                ux->has_saved_mouse = true;
                ux->saved_search_failed = false;
                enter(ux, ux->home_v062_enabled
                    ? BLU2USB_SCREEN_HOME_CONNECTED
                    : BLU2USB_SCREEN_HOME);
            } else {
                enter(ux, BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE);
            }
            return cmd;
        }

        if (ux->home_v062_enabled)
            return enter_home_resolved(ux, true);

        enter(ux, BLU2USB_SCREEN_HOME);
        return cmd;
    }

    if (ux->screen == BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE)
        return cmd;

    if (ux->screen == BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED) {
        if (control == BLU2USB_CONTROL_KEY_Y)
            blu2usb_interaction_lock(&ux->interaction);
        return cmd;
    }

    if (ux->screen == BLU2USB_SCREEN_HOME_SEARCHING && control == BLU2USB_CONTROL_KEY_B) {
        ux->saved_search_failed = true;
        enter(ux, BLU2USB_SCREEN_HOME_RETRY);
        cmd.kind = BLU2USB_UX_COMMAND_CANCEL_SAVED_SEARCH;
        return cmd;
    }

    if (ux->screen == BLU2USB_SCREEN_HOME_RETRY && control == BLU2USB_CONTROL_KEY_A) {
        ux->saved_search_failed = false;
        enter(ux, BLU2USB_SCREEN_HOME_SEARCHING);
        cmd.kind = BLU2USB_UX_COMMAND_RETRY_SAVED_SEARCH;
        return cmd;
    }

    if (is_v062_home(ux->screen) && control == BLU2USB_CONTROL_KEY_X) {
        if (ux->screen == BLU2USB_SCREEN_HOME_SEARCHING) {
            ux->saved_search_failed = true;
            ux->return_screen = BLU2USB_SCREEN_HOME_RETRY;
            enter(ux, BLU2USB_SCREEN_HOME_SEARCHING_HELP);
            cmd.kind = BLU2USB_UX_COMMAND_CANCEL_SAVED_SEARCH;
        } else if (ux->screen == BLU2USB_SCREEN_HOME_RETRY) {
            ux->return_screen = BLU2USB_SCREEN_HOME_RETRY;
            enter(ux, BLU2USB_SCREEN_HOME_RETRY_HELP);
        } else {
            ux->return_screen = BLU2USB_SCREEN_HOME_CONNECTED;
            enter(ux, BLU2USB_SCREEN_HOME_CONNECTED_HELP);
        }
        return cmd;
    }

    if (is_help(ux->screen)) {
        enter(ux, ux->return_screen);
        return cmd;
    }

    if (ux->screen == BLU2USB_SCREEN_LEARN_KEYS) {
        if (control == BLU2USB_CONTROL_KEY_Y) blu2usb_interaction_lock(&ux->interaction);
        return cmd;
    }

    if (control == BLU2USB_CONTROL_KEY_Y && lock_allowed(ux->screen)) {
        if (ux->screen == BLU2USB_SCREEN_HOME_SEARCHING)
            cmd.kind = BLU2USB_UX_COMMAND_CANCEL_SAVED_SEARCH;
        blu2usb_interaction_lock(&ux->interaction);
        return cmd;
    }

    if (control == BLU2USB_CONTROL_KEY_B) {
        if (is_v062_home(ux->screen))
            return cmd;

        if (ux->screen != BLU2USB_SCREEN_HOME) {
            const blu2usb_screen_id_t target = back_target(ux);
            if (ux->home_v062_enabled &&
                (target == BLU2USB_SCREEN_HOME || is_v062_home(target))) {
                const bool restart_saved =
                    ux->screen == BLU2USB_SCREEN_PAIR_MOUSE;
                return enter_home_resolved(ux, restart_saved);
            }
            enter(ux, target);
        }
        return cmd;
    }

    unsigned count = blu2usb_ux_option_count(ux);
    if (count && control == BLU2USB_CONTROL_JOY_UP) { ux->selection = wrap_prev(ux->selection, count); return cmd; }
    if (count && control == BLU2USB_CONTROL_JOY_DOWN) { ux->selection = wrap_next(ux->selection, count); return cmd; }

    if ((ux->screen == BLU2USB_SCREEN_HOME_SEARCHING ||
         ux->screen == BLU2USB_SCREEN_HOME_RETRY) &&
        control == BLU2USB_CONTROL_JOY_PRESS) {
        const blu2usb_screen_id_t owner = ux->screen;
        const unsigned selected = ux->selection;
        static const blu2usb_screen_id_t dest[3] = {
            BLU2USB_SCREEN_SAVED_DEVICES,
            BLU2USB_SCREEN_PAIR_MOUSE,
            BLU2USB_SCREEN_LEARN_KEYS
        };
        ux->return_screen = owner;
        enter(ux, dest[selected]);

        if (selected == 1u)
            cmd.kind = BLU2USB_UX_COMMAND_PAIR_MOUSE;
        return cmd;
    }

    if (ux->screen == BLU2USB_SCREEN_HOME_CONNECTED &&
        control == BLU2USB_CONTROL_JOY_PRESS) {
        const unsigned selected = ux->selection;
        ux->return_screen = BLU2USB_SCREEN_HOME_CONNECTED;
        if (selected == 0u) enter(ux, BLU2USB_SCREEN_MOUSE_OPTIONS);
        else if (selected == 1u) enter(ux, BLU2USB_SCREEN_SAVED_DEVICES);
        else if (selected == 2u) {
            enter(ux, BLU2USB_SCREEN_PAIR_MOUSE);
            cmd.kind = BLU2USB_UX_COMMAND_PAIR_MOUSE;
        } else enter(ux, BLU2USB_SCREEN_LEARN_KEYS);
        return cmd;
    }

    if (ux->screen == BLU2USB_SCREEN_HOME && control == BLU2USB_CONTROL_JOY_PRESS) {
        static const blu2usb_screen_id_t dest[4] = {BLU2USB_SCREEN_MOUSE_STATUS, BLU2USB_SCREEN_MOUSE_OPTIONS, BLU2USB_SCREEN_OTHER_OPTIONS, BLU2USB_SCREEN_LEARN_KEYS};
        enter(ux, dest[ux->selection]);
        return cmd;
    }

    if ((ux->screen == BLU2USB_SCREEN_MOUSE_STATUS || ux->screen == BLU2USB_SCREEN_OTHER_DEVICES_STATUS) &&
        (control == BLU2USB_CONTROL_JOY_LEFT || control == BLU2USB_CONTROL_JOY_RIGHT)) {
        ux->status_page = ux->status_page ? 0u : 1u;
        enter(ux, ux->status_page ? BLU2USB_SCREEN_OTHER_DEVICES_STATUS : BLU2USB_SCREEN_MOUSE_STATUS);
        return cmd;
    }
    if (ux->screen == BLU2USB_SCREEN_MOUSE_STATUS && control == BLU2USB_CONTROL_KEY_X) {
        ux->return_screen = ux->screen; enter(ux, BLU2USB_SCREEN_MOUSE_HELP); return cmd;
    }
    if (ux->screen == BLU2USB_SCREEN_OTHER_DEVICES_STATUS && control == BLU2USB_CONTROL_KEY_X) {
        ux->return_screen = ux->screen; enter(ux, BLU2USB_SCREEN_DEVICES_HELP); return cmd;
    }

    if (ux->screen == BLU2USB_SCREEN_MOUSE_OPTIONS && control == BLU2USB_CONTROL_JOY_PRESS) {
        const unsigned selected = ux->selection;
        switch (selected) {
        case 0:
            ux->return_screen = BLU2USB_SCREEN_MOUSE_OPTIONS;
            if (blu2usb_ux_mouse_connected()) {
                enter(ux, BLU2USB_SCREEN_MOUSE_SAVED);
            } else {
                enter(ux, BLU2USB_SCREEN_PAIR_MOUSE);
                cmd.kind = BLU2USB_UX_COMMAND_PAIR_MOUSE;
            }
            break;
        case 1:
            enter(ux, ux->active_profile == BLU2USB_MOUSE_PROFILE_PASSTHROUGH
                      ? BLU2USB_SCREEN_PASSTHROUGH_APPLIED
                      : BLU2USB_SCREEN_APPLY_PASSTHROUGH);
            break;
        case 2:
            enter(ux, ux->active_profile == BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP
                      ? BLU2USB_SCREEN_DEFAULT_APPLIED
                      : BLU2USB_SCREEN_APPLY_DEFAULT);
            break;
        case 3:
            enter(ux, ux->active_profile == BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP
                      ? BLU2USB_SCREEN_ESCAPE_APPLIED
                      : BLU2USB_SCREEN_APPLY_ESCAPE);
            break;
        case 4:
            if (ux->active_profile != BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP) ux->custom_dirty = true;
            enter(ux, BLU2USB_SCREEN_EDIT_CUSTOM);
            break;
        default:
            break;
        }
        return cmd;
    }

    if (ux->screen == BLU2USB_SCREEN_OTHER_OPTIONS && control == BLU2USB_CONTROL_JOY_PRESS) {
        unsigned selected = ux->selection;
        static const blu2usb_screen_id_t dest[3] = {BLU2USB_SCREEN_PAIR_KEYBOARD,BLU2USB_SCREEN_PAIR_COMPOSITE,BLU2USB_SCREEN_SAVED_DEVICES};
        if (selected == 2u) ux->return_screen = BLU2USB_SCREEN_OTHER_OPTIONS;
        enter(ux, dest[selected]);
        if (selected == 0) cmd.kind = BLU2USB_UX_COMMAND_PAIR_KEYBOARD;
        else if (selected == 1) cmd.kind = BLU2USB_UX_COMMAND_PAIR_COMPOSITE;
        return cmd;
    }

    if (ux->screen == BLU2USB_SCREEN_PAIR_MOUSE || ux->screen == BLU2USB_SCREEN_PAIR_KEYBOARD || ux->screen == BLU2USB_SCREEN_PAIR_COMPOSITE) {
        if (control == BLU2USB_CONTROL_KEY_A) { cmd.kind = BLU2USB_UX_COMMAND_RETRY; return cmd; }
        if (control == BLU2USB_CONTROL_KEY_X) {
            ux->return_screen = ux->screen;
            enter(ux, ux->screen == BLU2USB_SCREEN_PAIR_MOUSE ? BLU2USB_SCREEN_PAIR_MOUSE_HELP :
                      ux->screen == BLU2USB_SCREEN_PAIR_KEYBOARD ? BLU2USB_SCREEN_PAIR_KEYBOARD_HELP : BLU2USB_SCREEN_PAIR_COMPOSITE_HELP);
            return cmd;
        }
    }

    if (ux->screen == BLU2USB_SCREEN_EDIT_CUSTOM) {
        if (control == BLU2USB_CONTROL_JOY_PRESS) {
            unsigned selected = ux->selection;
            ux->custom_source = custom_source_for(selected);
            enter(ux, custom_screen_for(selected));
            ux->selection = (unsigned)ux->custom_targets[ux->custom_source];
            return cmd;
        }
        if (control == BLU2USB_CONTROL_KEY_A) {
            cmd.kind = BLU2USB_UX_COMMAND_APPLY_CUSTOM;
            return cmd;
        }
    }

    if (is_will_become(ux->screen) && control == BLU2USB_CONTROL_KEY_A) {
        cmd.kind = BLU2USB_UX_COMMAND_CUSTOM_SET_TARGET;
        cmd.source = ux->custom_source;
        cmd.target = (blu2usb_mouse_target_t)ux->selection;
        enter(ux, BLU2USB_SCREEN_EDIT_CUSTOM);
        return cmd;
    }

    if (ux->screen == BLU2USB_SCREEN_SAVED_DEVICES &&
        (control == BLU2USB_CONTROL_JOY_LEFT || control == BLU2USB_CONTROL_JOY_RIGHT) && ux->saved_pages) {
        ux->saved_page = control == BLU2USB_CONTROL_JOY_LEFT ? wrap_prev(ux->saved_page, ux->saved_pages) : wrap_next(ux->saved_page, ux->saved_pages);
        ux->selection = 0;
        return cmd;
    }

    if (ux->screen == BLU2USB_SCREEN_SAVED_DEVICES && control == BLU2USB_CONTROL_JOY_PRESS && count) {
        const unsigned absolute_index = ux->saved_page * 4u + ux->selection;
        enter(ux, example_saved_device_details_for(absolute_index));
        return cmd;
    }

    if ((ux->screen == BLU2USB_SCREEN_DEVICE_DETAILS_MOUSE || ux->screen == BLU2USB_SCREEN_DEVICE_DETAILS_KEYBOARD || ux->screen == BLU2USB_SCREEN_DEVICE_DETAILS_COMPOSITE) &&
        control == BLU2USB_CONTROL_JOY_PRESS) {
        ux->return_screen = ux->screen;
        enter(ux, BLU2USB_SCREEN_REMOVE_DEVICE);
        return cmd;
    }

    if (ux->screen == BLU2USB_SCREEN_APPLY_PASSTHROUGH && control == BLU2USB_CONTROL_KEY_A) {
        cmd.kind = BLU2USB_UX_COMMAND_APPLY_PASSTHROUGH;
    } else if (ux->screen == BLU2USB_SCREEN_APPLY_DEFAULT && control == BLU2USB_CONTROL_KEY_A) {
        cmd.kind = BLU2USB_UX_COMMAND_APPLY_DEFAULT;
    } else if (ux->screen == BLU2USB_SCREEN_APPLY_ESCAPE && control == BLU2USB_CONTROL_KEY_A) {
        cmd.kind = BLU2USB_UX_COMMAND_APPLY_ESCAPE;
    } else if (ux->screen == BLU2USB_SCREEN_REMOVE_DEVICE && control == BLU2USB_CONTROL_KEY_A) {
        cmd.kind = BLU2USB_UX_COMMAND_REMOVE_DEVICE;
    }

    return cmd;
}

void blu2usb_ux_begin_first_start(blu2usb_ux_model_t *ux)
{
    if (ux == NULL) return;
    ux->first_start_complete = false;
    ux->home_v062_enabled = true;
    ux->has_saved_mouse = false;
    ux->saved_search_failed = false;
    enter(ux, BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE);
}

void blu2usb_ux_first_mouse_connected(blu2usb_ux_model_t *ux)
{
    if (ux == NULL || ux->first_start_complete) return;
    ux->has_saved_mouse = true;
    enter(ux, BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED);
}

void blu2usb_ux_first_mouse_disconnected(blu2usb_ux_model_t *ux)
{
    if (ux == NULL || ux->first_start_complete) return;
    enter(ux, BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE);
}

void blu2usb_ux_home_mouse_connected(blu2usb_ux_model_t *ux)
{
    if (ux == NULL) return;
    ux->has_saved_mouse = true;
    ux->saved_search_failed = false;

    if (!ux->first_start_complete) {
        enter(ux, BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED);
        return;
    }

    if (!ux->home_v062_enabled) return;

    if (ux->screen == BLU2USB_SCREEN_HOME_SEARCHING ||
        ux->screen == BLU2USB_SCREEN_HOME_RETRY) {
        enter(ux, BLU2USB_SCREEN_HOME_CONNECTED);
    } else if (ux->screen == BLU2USB_SCREEN_HOME_SEARCHING_HELP ||
               ux->screen == BLU2USB_SCREEN_HOME_RETRY_HELP) {
        ux->return_screen = BLU2USB_SCREEN_HOME_CONNECTED;
    }
}

void blu2usb_ux_home_mouse_disconnected(blu2usb_ux_model_t *ux)
{
    if (ux == NULL) return;

    if (!ux->first_start_complete) {
        enter(ux, BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE);
        return;
    }

    if (!ux->home_v062_enabled) return;

    if (ux->screen == BLU2USB_SCREEN_HOME_CONNECTED) {
        ux->saved_search_failed = false;
        enter(ux, BLU2USB_SCREEN_HOME_SEARCHING);
    } else if (ux->screen == BLU2USB_SCREEN_HOME_CONNECTED_HELP) {
        ux->saved_search_failed = false;
        ux->return_screen = BLU2USB_SCREEN_HOME_SEARCHING;
    }
}

void blu2usb_ux_home_saved_search_timeout(blu2usb_ux_model_t *ux)
{
    if (ux == NULL || !ux->home_v062_enabled || !ux->first_start_complete)
        return;

    ux->saved_search_failed = true;
    if (ux->screen == BLU2USB_SCREEN_HOME_SEARCHING)
        enter(ux, BLU2USB_SCREEN_HOME_RETRY);
}

uint16_t blu2usb_ux_learn_white_span_mask(const blu2usb_ux_model_t *ux) {
    if (ux->screen != BLU2USB_SCREEN_LEARN_KEYS) return 0;
    uint16_t mask = 0;
    for (unsigned c = 0; c < BLU2USB_CONTROL_COUNT; ++c) {
        if (blu2usb_interaction_is_pressed(&ux->interaction, (blu2usb_control_t)c)) {
            if (c == BLU2USB_CONTROL_KEY_Y) mask |= (uint16_t)(1u << 9) | (uint16_t)(1u << 10) | (uint16_t)(1u << 11);
            else mask |= (uint16_t)(1u << c);
        }
    }
    return mask;
}