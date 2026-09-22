#include "blu2usb/ux_model/ux_model.h"
#include <string.h>

static const blu2usb_screen_template_t screens[BLU2USB_SCREEN_COUNT] = {
    [BLU2USB_SCREEN_SEARCHING_FIRST] = {{"SEARCHING FIRST MOUSE","PRESS TO LEARN KEYS","WHILE WAIT CONNECTION","       JOY UP","  JOY    JOY    JOY","  LEFT  PRESS  RIGHT","      JOY DOWN"," KEY A         KEY X"," KEY B         KEY Y"},0},
    [BLU2USB_SCREEN_MOUSE_SAVED] = {{"FIRST MOUSE CONNECTED","       JOY UP","  JOY    JOY    JOY","  LEFT  PRESS  RIGHT","      JOY DOWN"," KEY A         KEY X"," KEY B         KEY Y",""," KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_HOME_SEARCHING] = {{"SEARCHING SAVED MOUSE"," SAVED DEVICES"," PAIR NEW MOUSE"," LEARN THE KEYS","","KEY B: CANCEL SEARCH","JOY UP / DOWN: SELECT","JOY PRESS: ACCESS","KEY X: HELP"},0},
    [BLU2USB_SCREEN_HOME_SEARCHING_HELP] = {{"HOME SEARCHING HELP","THE MATCHING ATTEMPT","TOOK PLACE ONLY FOR","DEVICES ALREADY SAVED","IN THE PREFERENCES,","BUT NOT FOR DEVICES","THAT WERE NOT SAVED.","","ANY KEY: BACK"},0},
    [BLU2USB_SCREEN_HOME_RETRY] = {{"DEVICE NOT FOUND"," SAVED DEVICES"," PAIR NEW MOUSE"," LEARN THE KEYS","","KEY A: RETRY SEARCH","JOY UP / DOWN: SELECT","JOY PRESS: ACCESS","KEY X: HELP"},0},
    [BLU2USB_SCREEN_HOME_RETRY_HELP] = {{"HOME RETRY HELP","THE MATCHING ATTEMPT","TOOK PLACE ONLY FOR","DEVICES ALREADY SAVED","IN THE PREFERENCES,","BUT NOT FOR DEVICES","THAT WERE NOT SAVED.","","ANY KEY: BACK"},0},
    [BLU2USB_SCREEN_PAIR_MOUSE] = {{"PAIR NEW MOUSE","TRYING TO CONNECT","A NEW MOUSE THAT","IS NOT LISTED","IN SAVED DEVICES","","KEY B: CANCEL","KEY X: HELP","KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_PAIR_MOUSE_HELP] = {{"PAIR NEW DEVICE HELP","TO CONNECT A SAVED","DEVICE FIRST UNPLUG","CURRENTLY CONNECTED","MOUSE AND PRESS THE","KEY B TO BACK UNTIL","SEARCHING APPEARS.","","ANY KEY: BACK"},0},
    [BLU2USB_SCREEN_RETRY_PAIR_NEW] = {{"PAIR NEW MOUSE","NO NEW MOUSE OUTSIDE","THE LIST OF SAVED","DEVICES WAS FOUND","","KEY A: RETRY NEW PAIR","KEY B: BACK TRY SAVED","KEY X: HELP","KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_HELP_RETRY_PAIR_NEW] = {{"DEVICE NOT FOUND HELP","TO CONNECT A SAVED","DEVICE FIRST UNPLUG","CURRENTLY CONNECTED","MOUSE AND PRESS THE","KEY B TO BACK UNTIL","SEARCHING APPEARS.","","ANY KEY: BACK"},0},
    [BLU2USB_SCREEN_HOME] = {{"LOGITECH LIFT"," REMAPPED TO ESCAPE"," SAVED DEVICES"," PAIR NEW MOUSE"," LEARN THE KEYS","","JOY UP / DOWN: SELECT","JOY PRESS: ACCESS","KEY X: HELP TO REMOVE"},0},
    [BLU2USB_SCREEN_HELP_HOME_CONNECTED] = {{"HOME CONNECTED HELP","TO DISCONNECT THE","CURRENTLY CONNECTED","MOUSE, NAVIGATE TO:","SAVED DEVICES >","(MOUSE PAGE) > REMOVE","DEVICE > REMOVE","","ANY KEY: BACK"},0},
    [BLU2USB_SCREEN_MOUSE_OPTIONS] = {{"MOUSE OPTIONS"," PASSTHROUGH"," STANDARD REMAP"," ESCAPE REMAP"," CUSTOM REMAP","","JOY PRESS: ACCESS","KEY B: BACK","KEY X: HELP"},0},
    [BLU2USB_SCREEN_HELP_REMAPPER_OPTIONS] = {{"REMAPPER OPTIONS HELP","CHOOSE FROM THE","OPTIONS TO CHANGE THE","FUNCTIONS OF THE","MOUSE BUTTONS.","PASSTHROUGH IS THE","DEFAULT OPTION.","","ANY KEY: BACK"},0},
    [BLU2USB_SCREEN_PASSTHROUGH_APPLIED] = {{"PASSTHROUGH ACTIVE","ORIGINAL MOUSE","BUTTONS POSITION","ARE ACTIVE NOW","","","","KEY B: BACK","KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_APPLY_PASSTHROUGH] = {{"APPLY PASSTHROUGH","ORIGINAL MOUSE","BUTTONS POSITION","ARE NOT ACTIVE","","","KEY A: APPLY","KEY B: CANCEL","KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_APPLY_DEFAULT] = {{"APPLY STANDARD REMAP","FORWARD IS LEFT","LEFT IS FORWARD","BACKWARD IS RIGHT","RIGHT IS BACKWARD","","KEY A: APPLY","KEY B: CANCEL","KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_DEFAULT_APPLIED] = {{"STANDARD REMAP ACTIVE","FORWARD IS LEFT","LEFT IS FORWARD","BACKWARD IS RIGHT","RIGHT IS BACKWARD","","","KEY B: BACK","KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_APPLY_ESCAPE] = {{"APPLY ESCAPE REMAP","FORWARD IS LEFT","BACKWARD IS RIGHT","LEFT IS ESCAPE","RIGHT IS BACKWARD","MIDDLE IS FORWARD","","KEY A: APPLY","KEY B: CANCEL"},0},
    [BLU2USB_SCREEN_ESCAPE_APPLIED] = {{"ESCAPE APPLIED ACTIVE","FORWARD IS LEFT","BACKWARD IS RIGHT","LEFT IS ESCAPE","RIGHT IS BACKWARD","MIDDLE IS FORWARD","","KEY B: BACK","KEY Y: LOCK"},0},
    [BLU2USB_SCREEN_EDIT_CUSTOM] = {{"EDIT CUSTOM REMAP"," LEFT IS LEFT"," RIGHT IS RIGHT"," MIDDLE IS MIDDLE"," FORWARD IS FORWARD"," BACKWARD IS BACKWARD","","JOY PRESS: ACCESS","KEY A: APPLY CUSTOM"},0},
    [BLU2USB_SCREEN_LEFT_WILL_BECOME] = {{"LEFT WILL BECOME"," LEFT"," RIGHT"," MIDDLE"," ESCAPE"," FORWARD"," BACKWARD","","KEY A: APPLY AND BACK"},0},
    [BLU2USB_SCREEN_RIGHT_WILL_BECOME] = {{"RIGHT WILL BECOME"," LEFT"," RIGHT"," MIDDLE"," ESCAPE"," FORWARD"," BACKWARD","","KEY A: APPLY AND BACK"},0},
    [BLU2USB_SCREEN_MIDDLE_WILL_BECOME] = {{"MIDDLE WILL BECOME"," LEFT"," RIGHT"," MIDDLE"," ESCAPE"," FORWARD"," BACKWARD","","KEY A: APPLY AND BACK"},0},
    [BLU2USB_SCREEN_FORWARD_WILL_BECOME] = {{"FORWARD WILL BECOME"," LEFT"," RIGHT"," MIDDLE"," ESCAPE"," FORWARD"," BACKWARD","","KEY A: APPLY AND BACK"},0},
    [BLU2USB_SCREEN_BACKWARD_WILL_BECOME] = {{"BACKWARD WILL BECOME"," LEFT"," RIGHT"," MIDDLE"," ESCAPE"," FORWARD"," BACKWARD","","KEY A: APPLY AND BACK"},0},
    [BLU2USB_SCREEN_SAVED_DEVICES] = {{"3 OF 4","LOGITECH LIFT","STATUS: CONNECTED","PROFILE: STANDARD"," REMOVE DEVICE","","JOY RIGHT\\LEFT: PAGE","JOY PRESS: ACCESS","KEY B: BACK"},0},
    [BLU2USB_SCREEN_REMOVE_DEVICE] = {{"REMOVE THIS MOUSE","LOGITECH LIFT","","PAIRING AND MAPPINGS","WILL BE DELETED","","KEY A: REMOVE","KEY B: CANCEL","KEY X: HELP"},0},
    [BLU2USB_SCREEN_HELP_REMOVE_THIS] = {{"REMOVE MOUSE HELP","COMPLETELY REMOVE THE","AUTOMATIC CONNECTION","WHEN TURNING ON THE","DEVICE AND DELETE ITS","BUTTON REMAPPING","PROFILE.","","ANY KEY: BACK"},0},
    [BLU2USB_SCREEN_LEARN_KEYS] = {{"PRESS TO LEARN KEYS","       JOY UP","  JOY    JOY    JOY","  LEFT  PRESS  RIGHT","      JOY DOWN"," KEY A         KEY X"," KEY B         KEY Y",""," KEY Y: LOCK"},0},
};

static bool help(blu2usb_screen_id_t s) {
    return s == BLU2USB_SCREEN_HOME_SEARCHING_HELP || s == BLU2USB_SCREEN_HOME_RETRY_HELP ||
        s == BLU2USB_SCREEN_PAIR_MOUSE_HELP || s == BLU2USB_SCREEN_HELP_RETRY_PAIR_NEW ||
        s == BLU2USB_SCREEN_HELP_HOME_CONNECTED || s == BLU2USB_SCREEN_HELP_REMAPPER_OPTIONS ||
        s == BLU2USB_SCREEN_HELP_REMOVE_THIS;
}
static bool editor(blu2usb_screen_id_t s) {
    return s >= BLU2USB_SCREEN_LEFT_WILL_BECOME && s <= BLU2USB_SCREEN_BACKWARD_WILL_BECOME;
}
static void enter(blu2usb_ux_model_t *u, blu2usb_screen_id_t s) {
    u->screen=s; u->selection=0; u->search_expired=false;
}
void blu2usb_ux_home(blu2usb_ux_model_t *u) {
    enter(u, !u->saved_device_count ? BLU2USB_SCREEN_SEARCHING_FIRST :
        blu2usb_ux_mouse_connected() ? BLU2USB_SCREEN_HOME : BLU2USB_SCREEN_HOME_SEARCHING);
}
void blu2usb_ux_init(blu2usb_ux_model_t *u) {
    memset(u,0,sizeof(*u)); blu2usb_interaction_init(&u->interaction);
    const blu2usb_mouse_target_t identity[]={0,1,2,4,3};
    memcpy(u->custom_targets,identity,sizeof(identity)); blu2usb_ux_home(u);
}
void blu2usb_ux_set_saved_device_count(blu2usb_ux_model_t *u,unsigned n) {
    u->saved_device_count=n; u->saved_pages=n;
    if (u->saved_page>=n) u->saved_page=0;
}
void blu2usb_ux_set_custom_target(blu2usb_ux_model_t *u,blu2usb_mouse_source_t s,blu2usb_mouse_target_t t) {
    if ((unsigned)s>=5 || (unsigned)t>=6) return;
    u->custom_targets[s]=t; u->custom_dirty=true; enter(u,BLU2USB_SCREEN_EDIT_CUSTOM);
}
void blu2usb_ux_search_expired(blu2usb_ux_model_t *u) {
    if (u->screen==BLU2USB_SCREEN_HOME_SEARCHING) enter(u,BLU2USB_SCREEN_HOME_RETRY);
    else if (u->screen==BLU2USB_SCREEN_PAIR_MOUSE) enter(u,BLU2USB_SCREEN_RETRY_PAIR_NEW);
}
void blu2usb_ux_connection_changed(blu2usb_ux_model_t *u,bool connected) {
    blu2usb_ux_set_mouse_connected(connected);
    if (connected) {
        if (u->screen==BLU2USB_SCREEN_SEARCHING_FIRST) enter(u,BLU2USB_SCREEN_MOUSE_SAVED);
        else if (u->screen==BLU2USB_SCREEN_HOME_SEARCHING || u->screen==BLU2USB_SCREEN_PAIR_MOUSE) blu2usb_ux_home(u);
    } else {
        if (u->screen==BLU2USB_SCREEN_HOME) blu2usb_ux_home(u);
        if (u->screen==BLU2USB_SCREEN_PASSTHROUGH_APPLIED) enter(u,BLU2USB_SCREEN_APPLY_PASSTHROUGH);
        if (u->screen==BLU2USB_SCREEN_DEFAULT_APPLIED) enter(u,BLU2USB_SCREEN_APPLY_DEFAULT);
        if (u->screen==BLU2USB_SCREEN_ESCAPE_APPLIED) enter(u,BLU2USB_SCREEN_APPLY_ESCAPE);
    }
}
const blu2usb_screen_template_t *blu2usb_ux_screen_template(blu2usb_screen_id_t s) {
    return (unsigned)s<BLU2USB_SCREEN_COUNT ? &screens[s] : NULL;
}
unsigned blu2usb_ux_option_count(const blu2usb_ux_model_t *u) {
    if (editor(u->screen)) return 6;
    switch(u->screen) {
    case BLU2USB_SCREEN_HOME: case BLU2USB_SCREEN_MOUSE_OPTIONS: return 4;
    case BLU2USB_SCREEN_HOME_SEARCHING: case BLU2USB_SCREEN_HOME_RETRY: return 3;
    case BLU2USB_SCREEN_EDIT_CUSTOM: return 5;
    case BLU2USB_SCREEN_SAVED_DEVICES: return u->saved_device_count ? 1 : 0;
    default: return 0;
    }
}
uint16_t blu2usb_ux_learn_white_span_mask(const blu2usb_ux_model_t *u) { return u->interaction.held_mask; }
blu2usb_ux_command_t blu2usb_ux_input(blu2usb_ux_model_t *u,blu2usb_control_t c,bool pressed) {
    blu2usb_ux_command_t cmd={0};
    blu2usb_interaction_event_t e=blu2usb_interaction_input(&u->interaction,c,pressed);
    if(e.kind==BLU2USB_INTERACTION_NONE) return cmd;
    if(e.kind==BLU2USB_INTERACTION_UNLOCK) { u->interaction.held_mask=0; blu2usb_ux_home(u); return cmd; }
    if(help(u->screen)) { u->interaction.held_mask=0; enter(u,u->return_screen); if(u->screen==BLU2USB_SCREEN_HOME)blu2usb_ux_home(u); return cmd; }
    if(c==BLU2USB_CONTROL_KEY_Y && u->saved_device_count) { blu2usb_interaction_lock(&u->interaction); return cmd; }
    if(u->screen==BLU2USB_SCREEN_SEARCHING_FIRST || u->screen==BLU2USB_SCREEN_MOUSE_SAVED || u->screen==BLU2USB_SCREEN_LEARN_KEYS) return cmd;
    unsigned n=blu2usb_ux_option_count(u);
    if(n && (c==BLU2USB_CONTROL_JOY_UP || c==BLU2USB_CONTROL_JOY_DOWN)) {
        u->selection=(u->selection+n+(c==BLU2USB_CONTROL_JOY_UP ? -1 : 1))%n; return cmd;
    }
    if(u->screen==BLU2USB_SCREEN_SAVED_DEVICES && u->saved_device_count && (c==BLU2USB_CONTROL_JOY_LEFT || c==BLU2USB_CONTROL_JOY_RIGHT)) {
        u->saved_page=(u->saved_page+u->saved_device_count+(c==BLU2USB_CONTROL_JOY_LEFT ? -1 : 1))%u->saved_device_count; return cmd;
    }
    if(c==BLU2USB_CONTROL_KEY_X) {
        blu2usb_screen_id_t dest=u->screen, back=u->screen;
        switch(u->screen) {
        case BLU2USB_SCREEN_HOME: dest=BLU2USB_SCREEN_HELP_HOME_CONNECTED; break;
        case BLU2USB_SCREEN_HOME_SEARCHING: dest=BLU2USB_SCREEN_HOME_SEARCHING_HELP; back=BLU2USB_SCREEN_HOME_RETRY; break;
        case BLU2USB_SCREEN_HOME_RETRY: dest=BLU2USB_SCREEN_HOME_RETRY_HELP; break;
        case BLU2USB_SCREEN_PAIR_MOUSE: dest=BLU2USB_SCREEN_PAIR_MOUSE_HELP; back=BLU2USB_SCREEN_RETRY_PAIR_NEW; break;
        case BLU2USB_SCREEN_RETRY_PAIR_NEW: dest=BLU2USB_SCREEN_HELP_RETRY_PAIR_NEW; break;
        case BLU2USB_SCREEN_MOUSE_OPTIONS: dest=BLU2USB_SCREEN_HELP_REMAPPER_OPTIONS; break;
        case BLU2USB_SCREEN_REMOVE_DEVICE: dest=BLU2USB_SCREEN_HELP_REMOVE_THIS; break;
        default: break;
        }
        if(dest!=u->screen) { u->return_screen=back; enter(u,dest); } return cmd;
    }
    if(c==BLU2USB_CONTROL_KEY_B) {
        if(editor(u->screen)) enter(u,BLU2USB_SCREEN_EDIT_CUSTOM);
        else switch(u->screen) {
        case BLU2USB_SCREEN_HOME_SEARCHING: enter(u,BLU2USB_SCREEN_HOME_RETRY); break;
        case BLU2USB_SCREEN_REMOVE_DEVICE: enter(u,BLU2USB_SCREEN_SAVED_DEVICES); break;
        case BLU2USB_SCREEN_APPLY_PASSTHROUGH: case BLU2USB_SCREEN_PASSTHROUGH_APPLIED:
        case BLU2USB_SCREEN_APPLY_DEFAULT: case BLU2USB_SCREEN_DEFAULT_APPLIED:
        case BLU2USB_SCREEN_APPLY_ESCAPE: case BLU2USB_SCREEN_ESCAPE_APPLIED:
        case BLU2USB_SCREEN_EDIT_CUSTOM: enter(u,BLU2USB_SCREEN_MOUSE_OPTIONS); break;
        default: blu2usb_ux_home(u); break;
        } return cmd;
    }
    if(c==BLU2USB_CONTROL_KEY_A) {
        static const blu2usb_mouse_target_t targets[]={0,1,2,5,4,3};
        if(editor(u->screen)) { cmd.kind=BLU2USB_UX_COMMAND_CUSTOM_SET_TARGET; cmd.source=u->custom_source; cmd.target=targets[u->selection]; return cmd; }
        switch(u->screen) {
        case BLU2USB_SCREEN_HOME_RETRY: enter(u,BLU2USB_SCREEN_HOME_SEARCHING); break;
        case BLU2USB_SCREEN_RETRY_PAIR_NEW: enter(u,BLU2USB_SCREEN_PAIR_MOUSE); break;
        case BLU2USB_SCREEN_APPLY_PASSTHROUGH: cmd.kind=BLU2USB_UX_COMMAND_APPLY_PASSTHROUGH; break;
        case BLU2USB_SCREEN_APPLY_DEFAULT: cmd.kind=BLU2USB_UX_COMMAND_APPLY_DEFAULT; break;
        case BLU2USB_SCREEN_APPLY_ESCAPE: cmd.kind=BLU2USB_UX_COMMAND_APPLY_ESCAPE; break;
        case BLU2USB_SCREEN_EDIT_CUSTOM: cmd.kind=BLU2USB_UX_COMMAND_APPLY_CUSTOM; break;
        case BLU2USB_SCREEN_REMOVE_DEVICE: cmd.kind=BLU2USB_UX_COMMAND_REMOVE_DEVICE; break;
        default: break;
        } return cmd;
    }
    if(c!=BLU2USB_CONTROL_JOY_PRESS) return cmd;
    if(u->screen==BLU2USB_SCREEN_HOME || u->screen==BLU2USB_SCREEN_HOME_SEARCHING || u->screen==BLU2USB_SCREEN_HOME_RETRY) {
        unsigned option=u->selection+(u->screen==BLU2USB_SCREEN_HOME ? 0 : 1);
        const blu2usb_screen_id_t next[]={BLU2USB_SCREEN_MOUSE_OPTIONS,BLU2USB_SCREEN_SAVED_DEVICES,BLU2USB_SCREEN_PAIR_MOUSE,BLU2USB_SCREEN_LEARN_KEYS};
        enter(u,next[option]);
    } else if(u->screen==BLU2USB_SCREEN_MOUSE_OPTIONS) {
        const blu2usb_screen_id_t inactive[]={BLU2USB_SCREEN_APPLY_PASSTHROUGH,BLU2USB_SCREEN_APPLY_DEFAULT,BLU2USB_SCREEN_APPLY_ESCAPE,BLU2USB_SCREEN_EDIT_CUSTOM};
        const blu2usb_screen_id_t active[]={BLU2USB_SCREEN_PASSTHROUGH_APPLIED,BLU2USB_SCREEN_DEFAULT_APPLIED,BLU2USB_SCREEN_ESCAPE_APPLIED,BLU2USB_SCREEN_EDIT_CUSTOM};
        unsigned i=u->selection; enter(u,blu2usb_ux_mouse_connected() && (unsigned)u->active_profile==i ? active[i]:inactive[i]);
    } else if(u->screen==BLU2USB_SCREEN_EDIT_CUSTOM) {
        u->custom_source=(blu2usb_mouse_source_t)u->selection;
        enter(u,(blu2usb_screen_id_t)(BLU2USB_SCREEN_LEFT_WILL_BECOME+u->custom_source));
    } else if(u->screen==BLU2USB_SCREEN_SAVED_DEVICES && u->saved_device_count) enter(u,BLU2USB_SCREEN_REMOVE_DEVICE);
    return cmd;
}
