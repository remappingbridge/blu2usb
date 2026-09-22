#include "blu2usb/renderer/renderer.h"
#include <stdio.h>
#include <string.h>
static const char *names[]={"PASSTHROUGH","STANDARD","ESCAPE","CUSTOM"};
static void row(blu2usb_ui_frame_t *f,unsigned r,const char *s,blu2usb_ui_tone_t tone) {
    for(unsigned c=0;c<21;c++) f->cells[r][c].character=' ';
    blu2usb_ui_frame_set_text(f,r,0,s,tone);
}
void blu2usb_ui_enforce_applied_visual_contract(const blu2usb_ux_model_t *u,blu2usb_ui_frame_t *f) {
    char text[22];
    if(u->screen==BLU2USB_SCREEN_HOME) {
        row(f,0,u->mouse_title,BLU2USB_UI_TONE_TITLE);
        snprintf(text,sizeof(text),u->active_profile==0 ? " NO REMAP %s" : " REMAPPED TO %s",names[u->active_profile]);
        row(f,1,text,u->selection==0 ? BLU2USB_UI_TONE_EMPHASIZED:BLU2USB_UI_TONE_ACTIONABLE);
    }
    if(u->screen==BLU2USB_SCREEN_SAVED_DEVICES) {
        snprintf(text,sizeof(text),"%u OF %u",u->saved_page+1,u->saved_device_count); row(f,0,text,BLU2USB_UI_TONE_TITLE);
        row(f,1,u->page_title,u->page_connected ? BLU2USB_UI_TONE_CURRENT:BLU2USB_UI_TONE_STATIC);
        row(f,2,u->page_connected ? "STATUS: CONNECTED":"STATUS: DISCONNECTED",BLU2USB_UI_TONE_STATIC);
        snprintf(text,sizeof(text),"PROFILE: %s",names[u->page_profile]); row(f,3,text,BLU2USB_UI_TONE_STATIC);
    }
    if(u->screen==BLU2USB_SCREEN_REMOVE_DEVICE) row(f,1,u->remove_title,BLU2USB_UI_TONE_STATIC);
    if(u->screen==BLU2USB_SCREEN_EDIT_CUSTOM) {
        const char *sources[]={"LEFT","RIGHT","MIDDLE","FORWARD","BACKWARD"};
        const char *targets[]={"LEFT","RIGHT","MIDDLE","BACKWARD","FORWARD","ESCAPE"};
        for(unsigned i=0;i<5;i++) {
            snprintf(text,sizeof(text)," %s IS %s",sources[i],targets[u->custom_targets[i]]);
            row(f,i+1,text,u->selection==i ? BLU2USB_UI_TONE_EMPHASIZED:
                (!u->custom_dirty && u->active_profile==3 && blu2usb_ux_mouse_connected()) ? BLU2USB_UI_TONE_CURRENT:BLU2USB_UI_TONE_ACTIONABLE);
        }
    }
}
