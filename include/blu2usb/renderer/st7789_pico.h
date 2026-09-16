#ifndef BLU2USB_RENDERER_ST7789_PICO_H
#define BLU2USB_RENDERER_ST7789_PICO_H

#include <stdbool.h>
#include "blu2usb/renderer/renderer.h"

bool blu2usb_st7789_pico_init(blu2usb_display_hal_t *display);
void blu2usb_st7789_pico_set_backlight(bool enabled);

#endif
