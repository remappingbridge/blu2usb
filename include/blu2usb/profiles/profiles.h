#ifndef BLU2USB_PROFILES_PROFILES_H
#define BLU2USB_PROFILES_PROFILES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blu2usb/domain/profile.h"

#define BLU2USB_PROFILE_SERIALIZED_SIZE 14u

typedef struct {
    blu2usb_mouse_profile_kind_t active_kind;
    blu2usb_mouse_target_t custom_targets[BLU2USB_MOUSE_SOURCE_COUNT];
    blu2usb_mouse_target_t draft_targets[BLU2USB_MOUSE_SOURCE_COUNT];
    bool draft_valid;
} blu2usb_profiles_t;

void blu2usb_profiles_init(blu2usb_profiles_t *profiles);
void blu2usb_profiles_configure_active(const blu2usb_profiles_t *profiles,
                                       blu2usb_mouse_profile_config_t *config);
bool blu2usb_profiles_build_preset(blu2usb_mouse_profile_kind_t kind,
                                    const blu2usb_profiles_t *profiles,
                                    blu2usb_mouse_profile_config_t *config);
void blu2usb_profiles_activate(blu2usb_profiles_t *profiles,
                               const blu2usb_mouse_profile_config_t *config);
bool blu2usb_profiles_draft_set(blu2usb_profiles_t *profiles,
                                 blu2usb_mouse_source_t source,
                                 blu2usb_mouse_target_t target);
bool blu2usb_profiles_custom_candidate(const blu2usb_profiles_t *profiles,
                                        blu2usb_mouse_profile_config_t *config);
blu2usb_mouse_target_t blu2usb_profiles_target_for_source(
    const blu2usb_mouse_profile_config_t *config,
    blu2usb_mouse_source_t source);
bool blu2usb_profiles_requires_forward_held_fix(
    const blu2usb_mouse_profile_config_t *config);
bool blu2usb_profiles_serialize(const blu2usb_profiles_t *profiles,
                                uint8_t out[BLU2USB_PROFILE_SERIALIZED_SIZE]);
bool blu2usb_profiles_restore(blu2usb_profiles_t *profiles,
                              const uint8_t data[BLU2USB_PROFILE_SERIALIZED_SIZE]);
const char *blu2usb_profiles_target_name(blu2usb_mouse_target_t target);

#endif
