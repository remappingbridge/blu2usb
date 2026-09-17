#include <stdbool.h>
#include "pico/stdlib.h"
#include "blu2usb/domain/version.h"
#include "blu2usb/hat/hat.h"
#include "blu2usb/renderer/renderer.h"
#include "blu2usb/renderer/st7789_pico.h"
#include "blu2usb/usb_hid/usb_hid.h"
#include "blu2usb/ux_model/ux_model.h"

static bool render_state(const blu2usb_display_hal_t *display, const blu2usb_ux_model_t *ux)
{
    blu2usb_ui_frame_t frame;
    blu2usb_ui_project(ux,&frame);
    return blu2usb_renderer_render(display,&frame);
}

int main(void)
{
    (void)blu2usb_version();

    blu2usb_ux_model_t ux;
    blu2usb_display_hal_t display;
    blu2usb_ux_init(&ux);
    ux.screen = BLU2USB_SCREEN_LEARN_KEYS;

    if (!blu2usb_usb_hid_pico_init()) {
        for (;;) tight_loop_contents();
    }

    blu2usb_hat_pico_init();
    if (!blu2usb_st7789_pico_init(&display)) {
        for (;;) {
            blu2usb_usb_hid_pico_task();
            tight_loop_contents();
        }
    }

    (void)render_state(&display,&ux);
    blu2usb_st7789_pico_set_backlight(true);

    for (;;) {
        blu2usb_usb_hid_pico_task();
        blu2usb_hat_pico_task();
        blu2usb_hat_event_t event;
        while (blu2usb_hat_pico_poll_event(&event)) {
            const bool was_locked = blu2usb_interaction_is_locked(&ux.interaction);
            (void)blu2usb_ux_input(&ux,event.control,event.pressed);
            const bool is_locked = blu2usb_interaction_is_locked(&ux.interaction);

            if (!was_locked && is_locked) {
                blu2usb_st7789_pico_set_backlight(false);
            } else if (was_locked && !is_locked) {
                (void)render_state(&display,&ux);
                blu2usb_st7789_pico_set_backlight(true);
            } else if (!is_locked) {
                (void)render_state(&display,&ux);
            }
        }
        tight_loop_contents();
    }
}
