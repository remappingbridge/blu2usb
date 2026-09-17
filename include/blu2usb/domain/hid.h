#ifndef BLU2USB_DOMAIN_HID_H
#define BLU2USB_DOMAIN_HID_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLU2USB_HID_SOURCE_INVALID = 0,
    BLU2USB_HID_SOURCE_MOUSE,
    BLU2USB_HID_SOURCE_KEYBOARD,
    BLU2USB_HID_SOURCE_COMPOSITE,
    BLU2USB_HID_SOURCE_SYNTHETIC_REMAP,
    BLU2USB_HID_SOURCE_KIND_COUNT,
} blu2usb_hid_source_kind_t;

typedef struct {
    uint16_t kind;
    uint16_t instance;
} blu2usb_hid_source_t;

static inline blu2usb_hid_source_t blu2usb_hid_source_make(
    blu2usb_hid_source_kind_t kind,
    uint16_t instance)
{
    const blu2usb_hid_source_t source = {(uint16_t)kind, instance};
    return source;
}

static inline bool blu2usb_hid_source_is_valid(blu2usb_hid_source_t source)
{
    return source.kind > (uint16_t)BLU2USB_HID_SOURCE_INVALID &&
           source.kind < (uint16_t)BLU2USB_HID_SOURCE_KIND_COUNT;
}

static inline bool blu2usb_hid_source_equal(blu2usb_hid_source_t a,
                                             blu2usb_hid_source_t b)
{
    return a.kind == b.kind && a.instance == b.instance;
}

typedef enum {
    BLU2USB_MOUSE_BUTTON_LEFT = 0,
    BLU2USB_MOUSE_BUTTON_RIGHT,
    BLU2USB_MOUSE_BUTTON_MIDDLE,
    BLU2USB_MOUSE_BUTTON_BACK,
    BLU2USB_MOUSE_BUTTON_FORWARD,
    BLU2USB_MOUSE_BUTTON_6,
    BLU2USB_MOUSE_BUTTON_7,
    BLU2USB_MOUSE_BUTTON_8,
    BLU2USB_MOUSE_BUTTON_COUNT,
} blu2usb_mouse_button_t;

typedef enum {
    BLU2USB_MOUSE_EVENT_BUTTON = 0,
    BLU2USB_MOUSE_EVENT_MOVE,
    BLU2USB_MOUSE_EVENT_WHEEL,
} blu2usb_mouse_event_type_t;

typedef struct {
    blu2usb_hid_source_t source;
    blu2usb_mouse_event_type_t type;
    union {
        struct {
            blu2usb_mouse_button_t button;
            bool pressed;
        } button;
        struct {
            int16_t dx;
            int16_t dy;
        } move;
        struct {
            int16_t vertical;
            int16_t horizontal;
        } wheel;
    } data;
} blu2usb_canonical_mouse_event_t;

/* Logical keyboard keys use USB HID Keyboard/Keypad Usage IDs as stable
 * identities only. Transport descriptor structure remains adapter-owned. */
typedef uint8_t blu2usb_key_t;

enum {
    BLU2USB_KEY_A = 0x04,
    BLU2USB_KEY_ENTER = 0x28,
    BLU2USB_KEY_ESCAPE = 0x29,
    BLU2USB_KEY_SPACE = 0x2c,
};

typedef enum {
    BLU2USB_MOD_LEFT_CTRL = 0,
    BLU2USB_MOD_LEFT_SHIFT,
    BLU2USB_MOD_LEFT_ALT,
    BLU2USB_MOD_LEFT_GUI,
    BLU2USB_MOD_RIGHT_CTRL,
    BLU2USB_MOD_RIGHT_SHIFT,
    BLU2USB_MOD_RIGHT_ALT,
    BLU2USB_MOD_RIGHT_GUI,
    BLU2USB_MOD_COUNT,
} blu2usb_modifier_t;

typedef enum {
    BLU2USB_KEYBOARD_EVENT_KEY = 0,
    BLU2USB_KEYBOARD_EVENT_MODIFIER,
} blu2usb_keyboard_event_type_t;

typedef struct {
    blu2usb_hid_source_t source;
    blu2usb_keyboard_event_type_t type;
    union {
        struct {
            blu2usb_key_t key;
            bool pressed;
        } key;
        struct {
            blu2usb_modifier_t modifier;
            bool pressed;
        } modifier;
    } data;
} blu2usb_canonical_keyboard_event_t;

#ifdef __cplusplus
}
#endif

#endif
