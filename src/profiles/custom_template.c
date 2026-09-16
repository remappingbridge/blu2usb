#include "blu2usb/profiles/custom_template.h"

static const blu2usb_mouse_target_t passthrough[BLU2USB_MOUSE_SOURCE_COUNT] = {
    BLU2USB_MOUSE_TARGET_LEFT,
    BLU2USB_MOUSE_TARGET_RIGHT,
    BLU2USB_MOUSE_TARGET_MIDDLE,
    BLU2USB_MOUSE_TARGET_FORWARD,
    BLU2USB_MOUSE_TARGET_BACKWARD,
};

void blu2usb_custom_template_editor_init(blu2usb_custom_template_editor_t *editor) {
    for (unsigned i = 0; i < BLU2USB_MOUSE_SOURCE_COUNT; ++i) {
        editor->committed.target[i] = passthrough[i];
        editor->draft.target[i] = passthrough[i];
    }
    editor->editing = false;
    editor->dirty = false;
}

void blu2usb_custom_template_begin(blu2usb_custom_template_editor_t *editor) {
    editor->draft = editor->committed;
    editor->editing = true;
    editor->dirty = false;
}

bool blu2usb_custom_template_set_draft(blu2usb_custom_template_editor_t *editor, blu2usb_mouse_source_t source, blu2usb_mouse_target_t target) {
    if (!editor->editing || (unsigned)source >= BLU2USB_MOUSE_SOURCE_COUNT || (unsigned)target >= BLU2USB_MOUSE_TARGET_COUNT) return false;
    editor->draft.target[source] = target;
    editor->dirty = true;
    return true;
}

void blu2usb_custom_template_commit(blu2usb_custom_template_editor_t *editor) {
    if (editor->editing) editor->committed = editor->draft;
    editor->editing = false;
    editor->dirty = false;
}

void blu2usb_custom_template_cancel(blu2usb_custom_template_editor_t *editor) {
    editor->draft = editor->committed;
    editor->editing = false;
    editor->dirty = false;
}
