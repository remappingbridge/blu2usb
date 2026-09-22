#ifndef BLU2USB_RENDERER_RENDERER_V1_H
#define BLU2USB_RENDERER_RENDERER_V1_H

#include "blu2usb/renderer/renderer.h"
#include "blu2usb/ux_model/ux_v1.h"

#ifdef __cplusplus
extern "C" {
#endif

void blu2usb_ui_v1_project_frame(
    const blu2usb_ui_v1_t *ui,
    const blu2usb_product_snapshot_t *product,
    blu2usb_ui_frame_t *frame);

bool blu2usb_ui_v1_row_text(
    const blu2usb_ui_frame_t *frame,
    uint8_t row,
    char out[BLU2USB_RENDERER_TEXT_COLS + 1u]);

void blu2usb_ui_v1_format_mouse_name(
    const char *name,
    char out[BLU2USB_RENDERER_TEXT_COLS + 1u]);

#ifdef __cplusplus
}
#endif

#endif
