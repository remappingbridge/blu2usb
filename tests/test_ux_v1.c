#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blu2usb/renderer/renderer.h"
#include "blu2usb/profiles/mice.h"
#include "blu2usb/storage/storage.h"
#include "blu2usb/ble_hogp/ble_hogp.h"
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);exit(1);} }while(0)
static blu2usb_ux_command_t tap(blu2usb_ux_model_t *u,blu2usb_control_t c) {
    blu2usb_ux_command_t cmd=blu2usb_ux_input(u,c,true);CHECK(cmd.kind==0);return blu2usb_ux_input(u,c,false);
}
static void navigation(void) {
    blu2usb_ux_model_t u;blu2usb_ux_set_mouse_connected(false);blu2usb_ux_init(&u);
    CHECK(u.screen==BLU2USB_SCREEN_SEARCHING_FIRST);
    for(unsigned c=0;c<9;c++){tap(&u,(blu2usb_control_t)c);CHECK(u.screen==BLU2USB_SCREEN_SEARCHING_FIRST);CHECK(!u.interaction.locked);}
    blu2usb_ux_set_saved_device_count(&u,1);blu2usb_ux_connection_changed(&u,true);CHECK(u.screen==BLU2USB_SCREEN_MOUSE_SAVED);
    tap(&u,BLU2USB_CONTROL_KEY_B);CHECK(u.screen==BLU2USB_SCREEN_MOUSE_SAVED);
    tap(&u,BLU2USB_CONTROL_KEY_Y);CHECK(u.interaction.locked);
    tap(&u,BLU2USB_CONTROL_KEY_A);CHECK(!u.interaction.locked && u.screen==BLU2USB_SCREEN_HOME);
    tap(&u,BLU2USB_CONTROL_JOY_PRESS);CHECK(u.screen==BLU2USB_SCREEN_MOUSE_OPTIONS);
    tap(&u,BLU2USB_CONTROL_JOY_PRESS);CHECK(u.screen==BLU2USB_SCREEN_PASSTHROUGH_APPLIED);
    blu2usb_ux_connection_changed(&u,false);CHECK(u.screen==BLU2USB_SCREEN_APPLY_PASSTHROUGH);
    CHECK(tap(&u,BLU2USB_CONTROL_KEY_A).kind==BLU2USB_UX_COMMAND_APPLY_PASSTHROUGH);CHECK(u.screen==BLU2USB_SCREEN_APPLY_PASSTHROUGH);
    tap(&u,BLU2USB_CONTROL_KEY_B);tap(&u,BLU2USB_CONTROL_KEY_B);CHECK(u.screen==BLU2USB_SCREEN_HOME_SEARCHING);
    tap(&u,BLU2USB_CONTROL_KEY_X);CHECK(u.screen==BLU2USB_SCREEN_HOME_SEARCHING_HELP);
    tap(&u,BLU2USB_CONTROL_KEY_Y);CHECK(!u.interaction.locked && u.screen==BLU2USB_SCREEN_HOME_RETRY);
    tap(&u,BLU2USB_CONTROL_KEY_A);CHECK(u.screen==BLU2USB_SCREEN_HOME_SEARCHING);
    blu2usb_ux_search_expired(&u);CHECK(u.screen==BLU2USB_SCREEN_HOME_RETRY);
    u.selection=1;tap(&u,BLU2USB_CONTROL_JOY_PRESS);CHECK(u.screen==BLU2USB_SCREEN_PAIR_MOUSE);
    tap(&u,BLU2USB_CONTROL_KEY_X);tap(&u,BLU2USB_CONTROL_KEY_A);CHECK(u.screen==BLU2USB_SCREEN_RETRY_PAIR_NEW);
    tap(&u,BLU2USB_CONTROL_KEY_A);CHECK(u.screen==BLU2USB_SCREEN_PAIR_MOUSE);
    tap(&u,BLU2USB_CONTROL_KEY_Y);CHECK(u.interaction.locked);tap(&u,BLU2USB_CONTROL_JOY_PRESS);CHECK(u.screen==BLU2USB_SCREEN_HOME_SEARCHING);
    u.screen=BLU2USB_SCREEN_LEARN_KEYS;tap(&u,BLU2USB_CONTROL_KEY_B);CHECK(u.screen==BLU2USB_SCREEN_LEARN_KEYS);
    for(unsigned profile=0;profile<3;profile++) {
        blu2usb_ux_set_mouse_connected(true);blu2usb_ux_profile_applied(&u,(blu2usb_mouse_profile_kind_t)profile);
        blu2usb_ux_connection_changed(&u,false);
        const blu2usb_screen_id_t inactive[]={BLU2USB_SCREEN_APPLY_PASSTHROUGH,BLU2USB_SCREEN_APPLY_DEFAULT,BLU2USB_SCREEN_APPLY_ESCAPE};
        CHECK(u.screen==inactive[profile]);
    }
    u.screen=BLU2USB_SCREEN_EDIT_CUSTOM;u.selection=0;tap(&u,BLU2USB_CONTROL_JOY_PRESS);u.selection=3;
    blu2usb_ux_command_t cmd=tap(&u,BLU2USB_CONTROL_KEY_A);CHECK(cmd.target==BLU2USB_MOUSE_TARGET_ESCAPE);
    CHECK(u.screen==BLU2USB_SCREEN_LEFT_WILL_BECOME);blu2usb_ux_set_custom_target(&u,cmd.source,cmd.target);CHECK(u.screen==BLU2USB_SCREEN_EDIT_CUSTOM && u.custom_dirty);
}
static void rendering(void) {
    CHECK(BLU2USB_SCREEN_COUNT==30);
    for(unsigned s=0;s<BLU2USB_SCREEN_COUNT;s++) {
        const blu2usb_screen_template_t *t=blu2usb_ux_screen_template((blu2usb_screen_id_t)s);CHECK(t);
        for(unsigned r=0;r<9;r++){CHECK(t->rows[r]);CHECK(strlen(t->rows[r])<=21);CHECK(!strstr(t->rows[r],"PAIR KEYBOARD"));CHECK(!strstr(t->rows[r],"PAIR COMPOSITE"));}
    }
    blu2usb_ux_model_t u;blu2usb_ux_init(&u);blu2usb_ui_frame_t f;
    u.screen=BLU2USB_SCREEN_SEARCHING_FIRST;blu2usb_ui_project(&u,&f);CHECK(f.learn_background && f.cells[1][0].tone==BLU2USB_UI_TONE_STATIC);
    u.screen=BLU2USB_SCREEN_MOUSE_SAVED;blu2usb_ui_project(&u,&f);CHECK(!f.learn_background && f.hint_start_row==8);
    u.screen=BLU2USB_SCREEN_ESCAPE_APPLIED;blu2usb_ui_project(&u,&f);CHECK(f.cells[6][0].character==' ');CHECK(f.hint_start_row==7);
    u.screen=BLU2USB_SCREEN_HOME;u.selection=1;strcpy(u.mouse_title,"LIFT MOUSE");blu2usb_ui_project(&u,&f);blu2usb_ui_enforce_applied_visual_contract(&u,&f);CHECK(f.cells[1][1].tone==BLU2USB_UI_TONE_ACTIONABLE);
}
static void persistence(void) {
    blu2usb_mice_t m={0},restored;blu2usb_profiles_init(&m.profiles);
    for(unsigned i=0;i<16;i++){m.mice[i].used=true;snprintf(m.mice[i].name,32,"MOUSE %u",i);m.mice[i].profile=(blu2usb_mouse_profile_kind_t)(i%4);}
    CHECK(blu2usb_mice_count(&m)==16);CHECK(blu2usb_mice_page(&m,7,0)==7);CHECK(blu2usb_mice_page(&m,7,8)==8);
    uint8_t data[BLU2USB_MICE_BYTES],loaded[BLU2USB_MICE_BYTES],record[BLU2USB_STORAGE_RECORD_SIZE];size_t n;
    CHECK(blu2usb_mice_encode(&m,data));CHECK(blu2usb_storage_record_encode(3,data,sizeof(data),record));
    CHECK(blu2usb_storage_record_decode(record,NULL,loaded,sizeof(loaded),&n));CHECK(blu2usb_mice_decode(&restored,loaded,n));CHECK(restored.mice[15].profile==3);
    record[100]^=1;CHECK(!blu2usb_storage_record_decode(record,NULL,loaded,sizeof(loaded),&n));
    char title[22];blu2usb_mouse_title("Lift",title);CHECK(!strcmp(title,"LIFT MOUSE"));
    blu2usb_mouse_title("MOUSE GENERIC",title);CHECK(!strcmp(title,"MOUSE GENERIC"));
    blu2usb_mouse_title("ABCDEFGHIJKLMNOP",title);CHECK(!strcmp(title,"ABCDEFGHIJKLMNO MOUSE"));
    blu2usb_mouse_title("",title);CHECK(!strcmp(title,"UNKNOWN MOUSE"));
}
static void mouse_only(void) {
    const uint8_t keyboard[]={0x05,0x01,0x09,0x06,0xa1,0x01,0xc0};
    blu2usb_ble_hogp_parser_t p;
    CHECK(!blu2usb_ble_hogp_parser_configure(&p,blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE,1),keyboard,sizeof(keyboard)));
}
int main(void){navigation();rendering();persistence();mouse_only();puts("UX v1: navigation, rendering, sixteen records, mouse-only checks passed");return 0;}
