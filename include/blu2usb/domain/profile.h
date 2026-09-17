#ifndef BLU2USB_DOMAIN_PROFILE_H
#define BLU2USB_DOMAIN_PROFILE_H

#define BLU2USB_MOUSE_SOURCE_COUNT 5u

typedef enum {
    BLU2USB_MOUSE_PROFILE_PASSTHROUGH = 0,
    BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP,
    BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP,
    BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP,
} blu2usb_mouse_profile_kind_t;

typedef enum {
    BLU2USB_MOUSE_SOURCE_LEFT = 0,
    BLU2USB_MOUSE_SOURCE_RIGHT,
    BLU2USB_MOUSE_SOURCE_MIDDLE,
    BLU2USB_MOUSE_SOURCE_FORWARD,
    BLU2USB_MOUSE_SOURCE_BACKWARD,
} blu2usb_mouse_source_t;

typedef enum {
    BLU2USB_MOUSE_TARGET_LEFT = 0,
    BLU2USB_MOUSE_TARGET_RIGHT,
    BLU2USB_MOUSE_TARGET_MIDDLE,
    BLU2USB_MOUSE_TARGET_BACKWARD,
    BLU2USB_MOUSE_TARGET_FORWARD,
    BLU2USB_MOUSE_TARGET_ESCAPE,
    BLU2USB_MOUSE_TARGET_COUNT,
} blu2usb_mouse_target_t;

typedef struct {
    blu2usb_mouse_target_t target[BLU2USB_MOUSE_SOURCE_COUNT];
} blu2usb_custom_template_t;

typedef struct {
    blu2usb_mouse_profile_kind_t kind;
    blu2usb_mouse_target_t targets[BLU2USB_MOUSE_SOURCE_COUNT];
} blu2usb_mouse_profile_config_t;

#endif
