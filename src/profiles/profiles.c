#include "blu2usb/profiles/profiles.h"

#include <string.h>

#define PROFILE_SCHEMA_VERSION 2u
#define PROFILE_ACTIVE_OFFSET 1u
#define PROFILE_DRAFT_VALID_OFFSET 2u
#define PROFILE_CUSTOM_OFFSET 3u
#define PROFILE_DRAFT_OFFSET (PROFILE_CUSTOM_OFFSET + BLU2USB_MOUSE_SOURCE_COUNT)

static bool source_valid(blu2usb_mouse_source_t source)
{
    return (unsigned)source < BLU2USB_MOUSE_SOURCE_COUNT;
}

static bool target_valid(blu2usb_mouse_target_t target)
{
    return (unsigned)target < BLU2USB_MOUSE_TARGET_COUNT;
}

static blu2usb_mouse_target_t identity_target(blu2usb_mouse_source_t source)
{
    switch (source) {
    case BLU2USB_MOUSE_SOURCE_LEFT: return BLU2USB_MOUSE_TARGET_LEFT;
    case BLU2USB_MOUSE_SOURCE_RIGHT: return BLU2USB_MOUSE_TARGET_RIGHT;
    case BLU2USB_MOUSE_SOURCE_MIDDLE: return BLU2USB_MOUSE_TARGET_MIDDLE;
    case BLU2USB_MOUSE_SOURCE_FORWARD: return BLU2USB_MOUSE_TARGET_FORWARD;
    case BLU2USB_MOUSE_SOURCE_BACKWARD: return BLU2USB_MOUSE_TARGET_BACKWARD;
    default: return BLU2USB_MOUSE_TARGET_LEFT;
    }
}

static void identity_targets(blu2usb_mouse_target_t targets[BLU2USB_MOUSE_SOURCE_COUNT])
{
    for (unsigned source = 0u; source < BLU2USB_MOUSE_SOURCE_COUNT; ++source)
        targets[source] = identity_target((blu2usb_mouse_source_t)source);
}

void blu2usb_profiles_init(blu2usb_profiles_t *profiles)
{
    if (profiles == NULL) return;
    memset(profiles, 0, sizeof(*profiles));
    profiles->active_kind = BLU2USB_MOUSE_PROFILE_PASSTHROUGH;
    identity_targets(profiles->custom_targets);
    identity_targets(profiles->draft_targets);
}

static void fill_targets(blu2usb_mouse_profile_kind_t kind,
                         const blu2usb_profiles_t *profiles,
                         blu2usb_mouse_target_t targets[BLU2USB_MOUSE_SOURCE_COUNT])
{
    identity_targets(targets);
    switch (kind) {
    case BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP:
        targets[BLU2USB_MOUSE_SOURCE_LEFT] = BLU2USB_MOUSE_TARGET_FORWARD;
        targets[BLU2USB_MOUSE_SOURCE_RIGHT] = BLU2USB_MOUSE_TARGET_BACKWARD;
        targets[BLU2USB_MOUSE_SOURCE_FORWARD] = BLU2USB_MOUSE_TARGET_LEFT;
        targets[BLU2USB_MOUSE_SOURCE_BACKWARD] = BLU2USB_MOUSE_TARGET_RIGHT;
        break;
    case BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP:
        targets[BLU2USB_MOUSE_SOURCE_LEFT] = BLU2USB_MOUSE_TARGET_ESCAPE;
        targets[BLU2USB_MOUSE_SOURCE_RIGHT] = BLU2USB_MOUSE_TARGET_BACKWARD;
        targets[BLU2USB_MOUSE_SOURCE_MIDDLE] = BLU2USB_MOUSE_TARGET_FORWARD;
        targets[BLU2USB_MOUSE_SOURCE_FORWARD] = BLU2USB_MOUSE_TARGET_LEFT;
        targets[BLU2USB_MOUSE_SOURCE_BACKWARD] = BLU2USB_MOUSE_TARGET_RIGHT;
        break;
    case BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP:
        if (profiles != NULL)
            memcpy(targets, profiles->custom_targets, sizeof(profiles->custom_targets));
        break;
    case BLU2USB_MOUSE_PROFILE_PASSTHROUGH:
    default:
        break;
    }
}

void blu2usb_profiles_configure_active(const blu2usb_profiles_t *profiles,
                                       blu2usb_mouse_profile_config_t *config)
{
    if (profiles == NULL || config == NULL) return;
    config->kind = profiles->active_kind;
    fill_targets(config->kind, profiles, config->targets);
}

bool blu2usb_profiles_build_preset(blu2usb_mouse_profile_kind_t kind,
                                    const blu2usb_profiles_t *profiles,
                                    blu2usb_mouse_profile_config_t *config)
{
    if (config == NULL || kind < BLU2USB_MOUSE_PROFILE_PASSTHROUGH ||
        kind > BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP) return false;
    config->kind = kind;
    fill_targets(kind, profiles, config->targets);
    return true;
}

void blu2usb_profiles_activate(blu2usb_profiles_t *profiles,
                               const blu2usb_mouse_profile_config_t *config)
{
    if (profiles == NULL || config == NULL ||
        config->kind < BLU2USB_MOUSE_PROFILE_PASSTHROUGH ||
        config->kind > BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP) return;
    if (config->kind == BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP) {
        for (unsigned i = 0u; i < BLU2USB_MOUSE_SOURCE_COUNT; ++i)
            if (!target_valid(config->targets[i])) return;
        memcpy(profiles->custom_targets, config->targets, sizeof(profiles->custom_targets));
        memcpy(profiles->draft_targets, config->targets, sizeof(profiles->draft_targets));
        profiles->draft_valid = false;
    }
    profiles->active_kind = config->kind;
}

bool blu2usb_profiles_draft_set(blu2usb_profiles_t *profiles,
                                 blu2usb_mouse_source_t source,
                                 blu2usb_mouse_target_t target)
{
    if (profiles == NULL || !source_valid(source) || !target_valid(target)) return false;
    if (!profiles->draft_valid) {
        memcpy(profiles->draft_targets, profiles->custom_targets, sizeof(profiles->draft_targets));
        profiles->draft_valid = true;
    }
    profiles->draft_targets[source] = target;
    return true;
}

bool blu2usb_profiles_custom_candidate(const blu2usb_profiles_t *profiles,
                                        blu2usb_mouse_profile_config_t *config)
{
    if (profiles == NULL || config == NULL) return false;
    config->kind = BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP;
    memcpy(config->targets,
           profiles->draft_valid ? profiles->draft_targets : profiles->custom_targets,
           sizeof(config->targets));
    return true;
}

blu2usb_mouse_target_t blu2usb_profiles_target_for_source(
    const blu2usb_mouse_profile_config_t *config,
    blu2usb_mouse_source_t source)
{
    if (config == NULL || !source_valid(source)) return BLU2USB_MOUSE_TARGET_LEFT;
    return config->targets[source];
}

bool blu2usb_profiles_requires_forward_held_fix(const blu2usb_mouse_profile_config_t *config)
{
    return config != NULL &&
        blu2usb_profiles_target_for_source(config, BLU2USB_MOUSE_SOURCE_FORWARD) !=
            BLU2USB_MOUSE_TARGET_FORWARD;
}

static uint8_t checksum(const uint8_t *data)
{
    uint8_t value = 0x5au;
    for (size_t i = 0u; i < BLU2USB_PROFILE_SERIALIZED_SIZE - 1u; ++i) {
        value = (uint8_t)((value << 1u) | (value >> 7u));
        value ^= data[i];
    }
    return value;
}

bool blu2usb_profiles_serialize(const blu2usb_profiles_t *profiles,
                                uint8_t out[BLU2USB_PROFILE_SERIALIZED_SIZE])
{
    if (profiles == NULL || out == NULL ||
        profiles->active_kind > BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP) return false;

    memset(out, 0, BLU2USB_PROFILE_SERIALIZED_SIZE);
    out[0] = PROFILE_SCHEMA_VERSION;
    out[PROFILE_ACTIVE_OFFSET] = (uint8_t)profiles->active_kind;
    out[PROFILE_DRAFT_VALID_OFFSET] = profiles->draft_valid ? 1u : 0u;

    for (size_t i = 0u; i < BLU2USB_MOUSE_SOURCE_COUNT; ++i) {
        const blu2usb_mouse_target_t custom = profiles->custom_targets[i];
        const blu2usb_mouse_target_t draft =
            profiles->draft_valid ? profiles->draft_targets[i] : custom;
        if (!target_valid(custom) || !target_valid(draft)) return false;
        out[PROFILE_CUSTOM_OFFSET + i] = (uint8_t)custom;
        out[PROFILE_DRAFT_OFFSET + i] = (uint8_t)draft;
    }

    out[BLU2USB_PROFILE_SERIALIZED_SIZE - 1u] = checksum(out);
    return true;
}

bool blu2usb_profiles_restore(blu2usb_profiles_t *profiles,
                              const uint8_t data[BLU2USB_PROFILE_SERIALIZED_SIZE])
{
    if (profiles == NULL || data == NULL || data[0] != PROFILE_SCHEMA_VERSION ||
        data[PROFILE_ACTIVE_OFFSET] > BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP ||
        data[PROFILE_DRAFT_VALID_OFFSET] > 1u ||
        data[BLU2USB_PROFILE_SERIALIZED_SIZE - 1u] != checksum(data)) return false;

    blu2usb_profiles_t candidate;
    blu2usb_profiles_init(&candidate);
    candidate.active_kind =
        (blu2usb_mouse_profile_kind_t)data[PROFILE_ACTIVE_OFFSET];
    candidate.draft_valid = data[PROFILE_DRAFT_VALID_OFFSET] != 0u;

    for (size_t i = 0u; i < BLU2USB_MOUSE_SOURCE_COUNT; ++i) {
        const blu2usb_mouse_target_t custom =
            (blu2usb_mouse_target_t)data[PROFILE_CUSTOM_OFFSET + i];
        const blu2usb_mouse_target_t draft =
            (blu2usb_mouse_target_t)data[PROFILE_DRAFT_OFFSET + i];
        if (!target_valid(custom) || !target_valid(draft)) return false;
        candidate.custom_targets[i] = custom;
        candidate.draft_targets[i] = candidate.draft_valid ? draft : custom;
    }

    *profiles = candidate;
    return true;
}

const char *blu2usb_profiles_target_name(blu2usb_mouse_target_t target)
{
    switch (target) {
    case BLU2USB_MOUSE_TARGET_LEFT: return "LEFT";
    case BLU2USB_MOUSE_TARGET_RIGHT: return "RIGHT";
    case BLU2USB_MOUSE_TARGET_MIDDLE: return "MIDDLE";
    case BLU2USB_MOUSE_TARGET_BACKWARD: return "BACKWARD";
    case BLU2USB_MOUSE_TARGET_FORWARD: return "FORWARD";
    case BLU2USB_MOUSE_TARGET_ESCAPE: return "ESCAPE";
    default: return "?";
    }
}
