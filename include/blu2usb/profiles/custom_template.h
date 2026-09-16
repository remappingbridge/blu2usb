#ifndef BLU2USB_PROFILES_CUSTOM_TEMPLATE_H
#define BLU2USB_PROFILES_CUSTOM_TEMPLATE_H

#include <stdbool.h>
#include "blu2usb/domain/profile.h"

typedef struct {
    blu2usb_custom_template_t committed;
    blu2usb_custom_template_t draft;
    bool editing;
    bool dirty;
} blu2usb_custom_template_editor_t;

void blu2usb_custom_template_editor_init(blu2usb_custom_template_editor_t *editor);
void blu2usb_custom_template_begin(blu2usb_custom_template_editor_t *editor);
bool blu2usb_custom_template_set_draft(blu2usb_custom_template_editor_t *editor, blu2usb_mouse_source_t source, blu2usb_mouse_target_t target);
void blu2usb_custom_template_commit(blu2usb_custom_template_editor_t *editor);
void blu2usb_custom_template_cancel(blu2usb_custom_template_editor_t *editor);

#endif
