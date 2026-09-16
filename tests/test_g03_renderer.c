#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "blu2usb/hat/hat.h"
#include "blu2usb/renderer/renderer.h"
#include "blu2usb/ux_model/ux_model.h"

typedef struct { unsigned fills; unsigned writes; uint16_t colors[4]; } fake_display_t;
static bool fake_fill(void *ctx,uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t color){fake_display_t *f=ctx;(void)y;(void)h;assert(x==0u&&w==BLU2USB_RENDERER_WIDTH);if(f->fills<4u)f->colors[f->fills]=color;f->fills++;return true;}
static bool fake_write(void *ctx,uint16_t x,uint16_t y,uint16_t w,uint16_t h,const uint16_t *pixels){fake_display_t *f=ctx;assert(x<BLU2USB_RENDERER_WIDTH&&y<BLU2USB_RENDERER_HEIGHT);assert(w==BLU2USB_RENDERER_GLYPH_WIDTH&&h==BLU2USB_RENDERER_GLYPH_HEIGHT&&pixels!=0);f->writes++;return true;}
static void assert_text(const blu2usb_ui_frame_t *frame,unsigned row,unsigned col,const char *text){for(size_t i=0;text[i];++i)assert(frame->cells[row][col+i].character==text[i]);}
static void send(blu2usb_ux_model_t *ux,blu2usb_control_t c,bool pressed){(void)blu2usb_ux_input(ux,c,pressed);}

static void test_geometry_colors_and_hat(void)
{
    assert(BLU2USB_RENDERER_TEXT_ROWS==9u&&BLU2USB_RENDERER_TEXT_COLS==21u);
    assert(BLU2USB_RENDERER_TEXT_X+20u*BLU2USB_RENDERER_CHAR_ADVANCE+BLU2USB_RENDERER_GLYPH_WIDTH<=BLU2USB_RENDERER_WIDTH);
    assert(BLU2USB_RENDERER_TEXT_Y+8u*BLU2USB_RENDERER_LINE_ADVANCE+BLU2USB_RENDERER_GLYPH_HEIGHT<=BLU2USB_RENDERER_HEIGHT);
    assert(blu2usb_renderer_tone_rgb565(BLU2USB_UI_TONE_TITLE)==BLU2USB_COLOR_MAGENTA);
    assert(blu2usb_renderer_tone_rgb565(BLU2USB_UI_TONE_STATIC)==BLU2USB_COLOR_OFF_WHITE_YELLOW);
    assert(blu2usb_renderer_tone_rgb565(BLU2USB_UI_TONE_ACTIONABLE)==BLU2USB_COLOR_LIGHT_GRAY);
    assert(blu2usb_renderer_tone_rgb565(BLU2USB_UI_TONE_EMPHASIZED)==BLU2USB_COLOR_WHITE);
    assert(blu2usb_renderer_tone_rgb565(BLU2USB_UI_TONE_CURRENT)==BLU2USB_COLOR_CYAN);
    blu2usb_control_t c=BLU2USB_CONTROL_COUNT;
    assert(blu2usb_hat_control_for_pin(2,&c)&&c==BLU2USB_CONTROL_JOY_UP);
    assert(blu2usb_hat_control_for_pin(3,&c)&&c==BLU2USB_CONTROL_JOY_PRESS);
    assert(blu2usb_hat_control_for_pin(15,&c)&&c==BLU2USB_CONTROL_KEY_A);
    assert(blu2usb_hat_control_for_pin(16,&c)&&c==BLU2USB_CONTROL_JOY_LEFT);
    assert(blu2usb_hat_control_for_pin(17,&c)&&c==BLU2USB_CONTROL_KEY_B);
    assert(blu2usb_hat_control_for_pin(18,&c)&&c==BLU2USB_CONTROL_JOY_DOWN);
    assert(blu2usb_hat_control_for_pin(19,&c)&&c==BLU2USB_CONTROL_KEY_X);
    assert(blu2usb_hat_control_for_pin(20,&c)&&c==BLU2USB_CONTROL_JOY_RIGHT);
    assert(blu2usb_hat_control_for_pin(21,&c)&&c==BLU2USB_CONTROL_KEY_Y);
}

static void test_learn_projection_and_feedback(void)
{
    blu2usb_ux_model_t ux; blu2usb_ui_frame_t frame; blu2usb_ux_init(&ux); ux.screen=BLU2USB_SCREEN_LEARN_KEYS;
    blu2usb_ui_project(&ux,&frame);
    assert(frame.learn_background&&frame.hint_start_row==9u);
    assert_text(&frame,0,0,"PRESS TO LEAR A KEY");
    assert(frame.cells[0][0].tone==BLU2USB_UI_TONE_TITLE);
    assert(frame.cells[1][6].tone==BLU2USB_UI_TONE_ACTIONABLE);
    send(&ux,BLU2USB_CONTROL_JOY_UP,true); blu2usb_ui_project(&ux,&frame);
    assert(frame.cells[1][6].tone==BLU2USB_UI_TONE_EMPHASIZED&&frame.cells[1][11].tone==BLU2USB_UI_TONE_EMPHASIZED);
    send(&ux,BLU2USB_CONTROL_JOY_UP,false); send(&ux,BLU2USB_CONTROL_KEY_Y,true); blu2usb_ui_project(&ux,&frame);
    assert(frame.cells[6][0].tone==BLU2USB_UI_TONE_EMPHASIZED);
    assert(frame.cells[7][1].tone==BLU2USB_UI_TONE_EMPHASIZED);
    assert(frame.cells[8][2].tone==BLU2USB_UI_TONE_EMPHASIZED);
    send(&ux,BLU2USB_CONTROL_KEY_Y,false); assert(blu2usb_interaction_is_locked(&ux.interaction));
    send(&ux,BLU2USB_CONTROL_KEY_A,true); send(&ux,BLU2USB_CONTROL_KEY_A,false);
    assert(!blu2usb_interaction_is_locked(&ux.interaction)&&ux.screen==BLU2USB_SCREEN_HOME);
}

static void test_home_regions_selection_and_render(void)
{
    blu2usb_ux_model_t ux; blu2usb_ui_frame_t frame; blu2usb_ux_init(&ux); blu2usb_ui_project(&ux,&frame);
    assert(!frame.learn_background&&frame.hint_start_row==6u);
    assert_text(&frame,0,0,"HOME"); assert(frame.cells[1][1].tone==BLU2USB_UI_TONE_EMPHASIZED);
    assert(blu2usb_renderer_background_rgb565(&frame,5)==BLU2USB_COLOR_BLACK);
    assert(blu2usb_renderer_background_rgb565(&frame,6)==BLU2USB_COLOR_DARK_MAGENTA);
    send(&ux,BLU2USB_CONTROL_JOY_DOWN,true); blu2usb_ui_project(&ux,&frame); assert(frame.cells[1][1].tone==BLU2USB_UI_TONE_EMPHASIZED);
    send(&ux,BLU2USB_CONTROL_JOY_DOWN,false); blu2usb_ui_project(&ux,&frame); assert(ux.selection==1u&&frame.cells[2][1].tone==BLU2USB_UI_TONE_EMPHASIZED);
    fake_display_t fake={0}; const blu2usb_display_hal_t display={&fake,fake_fill,fake_write}; assert(blu2usb_renderer_render(&display,&frame));
    assert(fake.fills==2u&&fake.colors[0]==BLU2USB_COLOR_BLACK&&fake.colors[1]==BLU2USB_COLOR_DARK_MAGENTA&&fake.writes>0u);
}

int main(void){test_geometry_colors_and_hat();test_learn_projection_and_feedback();test_home_regions_selection_and_render();return 0;}
