#include "blu2usb/logitech_hidpp/logitech_hidpp.h"
#include <string.h>

#define HIDPP_DEVICE_INDEX 0xffu
#define HIDPP_SOFTWARE_ID 0x02u
#define HIDPP_ROOT_FEATURE_INDEX 0x00u
#define HIDPP_SET_CONTROL_REPORTING_FUNCTION 3u
#define HIDPP_DIVERT_ENABLE_FLAGS 0x03u
#define HIDPP_DIVERT_DISABLE_FLAGS 0x02u

static bool report_id_is_hidpp(uint8_t id)
{
    return id == BLU2USB_HIDPP_REPORT_ID_SHORT || id == BLU2USB_HIDPP_REPORT_ID_LONG;
}

void blu2usb_logitech_hidpp_init(blu2usb_logitech_hidpp_t *hidpp)
{
    if (hidpp != NULL) memset(hidpp, 0, sizeof(*hidpp));
}

void blu2usb_logitech_hidpp_on_connect(blu2usb_logitech_hidpp_t *hidpp)
{
    if (hidpp == NULL) return;
    const bool desired = hidpp->forward_desired;
    memset(hidpp, 0, sizeof(*hidpp));
    hidpp->connected = true;
    hidpp->forward_desired = desired;
}

void blu2usb_logitech_hidpp_on_disconnect(blu2usb_logitech_hidpp_t *hidpp)
{
    if (hidpp == NULL) return;
    const bool desired = hidpp->forward_desired;
    memset(hidpp, 0, sizeof(*hidpp));
    hidpp->forward_desired = desired;
}

void blu2usb_logitech_hidpp_set_forward_desired(blu2usb_logitech_hidpp_t *hidpp, bool desired)
{
    if (hidpp == NULL || hidpp->forward_desired == desired) return;
    hidpp->forward_desired = desired;
    if (desired) hidpp->feature_failed = false;
}

static void build_get_feature(blu2usb_hidpp_output_t *output)
{
    memset(output, 0, sizeof(*output));
    output->kind = BLU2USB_HIDPP_OUTPUT_GET_FEATURE;
    output->report_id = BLU2USB_HIDPP_REPORT_ID_LONG;
    output->payload_len = BLU2USB_HIDPP_LONG_PAYLOAD_SIZE;
    output->payload[0] = HIDPP_DEVICE_INDEX;
    output->payload[1] = HIDPP_ROOT_FEATURE_INDEX;
    output->payload[2] = HIDPP_SOFTWARE_ID;
    output->payload[3] = (uint8_t)(BLU2USB_HIDPP_REPROG_CONTROLS_V4 >> 8u);
    output->payload[4] = (uint8_t)(BLU2USB_HIDPP_REPROG_CONTROLS_V4 & 0xffu);
}

static void build_set_forward(const blu2usb_logitech_hidpp_t *hidpp,
                              bool enable,
                              blu2usb_hidpp_output_t *output)
{
    memset(output, 0, sizeof(*output));
    output->kind = BLU2USB_HIDPP_OUTPUT_SET_FORWARD_DIVERT;
    output->report_id = BLU2USB_HIDPP_REPORT_ID_LONG;
    output->payload_len = BLU2USB_HIDPP_LONG_PAYLOAD_SIZE;
    output->payload[0] = HIDPP_DEVICE_INDEX;
    output->payload[1] = hidpp->feature_index;
    output->payload[2] = (uint8_t)((HIDPP_SET_CONTROL_REPORTING_FUNCTION << 4u) | HIDPP_SOFTWARE_ID);
    output->payload[3] = (uint8_t)(BLU2USB_HIDPP_FORWARD_CID >> 8u);
    output->payload[4] = (uint8_t)(BLU2USB_HIDPP_FORWARD_CID & 0xffu);
    output->payload[5] = enable ? HIDPP_DIVERT_ENABLE_FLAGS : HIDPP_DIVERT_DISABLE_FLAGS;
}

bool blu2usb_logitech_hidpp_next_output(blu2usb_logitech_hidpp_t *hidpp,
                                         blu2usb_hidpp_output_t *output)
{
    if (hidpp == NULL || output == NULL || !hidpp->connected ||
        hidpp->waiting_for != BLU2USB_HIDPP_OUTPUT_NONE) return false;
    if (hidpp->feature_index == 0u) {
        if (!hidpp->forward_desired || hidpp->feature_failed) return false;
        build_get_feature(output);
        return true;
    }
    if (hidpp->forward_applied == hidpp->forward_desired) return false;
    hidpp->pending_enable = hidpp->forward_desired;
    build_set_forward(hidpp, hidpp->pending_enable, output);
    return true;
}

void blu2usb_logitech_hidpp_output_result(blu2usb_logitech_hidpp_t *hidpp,
                                           blu2usb_hidpp_output_kind_t kind,
                                           bool accepted)
{
    if (hidpp != NULL && accepted && kind != BLU2USB_HIDPP_OUTPUT_NONE)
        hidpp->waiting_for = kind;
}

static bool error_matches(const uint8_t *payload,
                          size_t len,
                          uint8_t expected_feature,
                          uint8_t expected_function)
{
    return payload != NULL && len >= 5u && payload[1] == 0xffu &&
        payload[2] == expected_feature &&
        (uint8_t)(payload[3] >> 4u) == expected_function &&
        (uint8_t)(payload[3] & 0x0fu) == HIDPP_SOFTWARE_ID;
}

bool blu2usb_logitech_hidpp_process_input(blu2usb_logitech_hidpp_t *hidpp,
                                           uint8_t report_id,
                                           const uint8_t *payload,
                                           size_t payload_len,
                                           blu2usb_hidpp_input_result_t *result)
{
    if (result != NULL) memset(result, 0, sizeof(*result));
    if (hidpp == NULL || payload == NULL || !hidpp->connected || !report_id_is_hidpp(report_id))
        return false;
    const bool interested = hidpp->forward_desired || hidpp->forward_applied ||
        hidpp->waiting_for != BLU2USB_HIDPP_OUTPUT_NONE || hidpp->feature_index != 0u;
    if (!interested) return false;
    if (result != NULL) result->consumed = true;

    if (hidpp->waiting_for == BLU2USB_HIDPP_OUTPUT_GET_FEATURE) {
        if (error_matches(payload, payload_len, HIDPP_ROOT_FEATURE_INDEX, 0u)) {
            hidpp->feature_failed = true;
            hidpp->waiting_for = BLU2USB_HIDPP_OUTPUT_NONE;
            return true;
        }
        if (payload_len >= 4u && payload[1] == HIDPP_ROOT_FEATURE_INDEX &&
            (uint8_t)(payload[2] >> 4u) == 0u && (uint8_t)(payload[2] & 0x0fu) == HIDPP_SOFTWARE_ID) {
            hidpp->feature_index = payload[3];
            hidpp->feature_failed = hidpp->feature_index == 0u;
            hidpp->waiting_for = BLU2USB_HIDPP_OUTPUT_NONE;
            return true;
        }
    }

    if (hidpp->waiting_for == BLU2USB_HIDPP_OUTPUT_SET_FORWARD_DIVERT) {
        if (error_matches(payload, payload_len, hidpp->feature_index, HIDPP_SET_CONTROL_REPORTING_FUNCTION)) {
            if (hidpp->pending_enable) {
                hidpp->feature_failed = true;
                hidpp->forward_applied = false;
            }
            hidpp->waiting_for = BLU2USB_HIDPP_OUTPUT_NONE;
            return true;
        }
        if (payload_len >= 3u && payload[1] == hidpp->feature_index &&
            (uint8_t)(payload[2] >> 4u) == HIDPP_SET_CONTROL_REPORTING_FUNCTION &&
            (uint8_t)(payload[2] & 0x0fu) == HIDPP_SOFTWARE_ID) {
            const bool previous = hidpp->forward_held;
            hidpp->forward_applied = hidpp->pending_enable;
            if (!hidpp->forward_applied) hidpp->forward_held = false;
            hidpp->waiting_for = BLU2USB_HIDPP_OUTPUT_NONE;
            if (result != NULL && previous != hidpp->forward_held) {
                result->held_changed = true;
                result->forward_held = hidpp->forward_held;
            }
            return true;
        }
    }

    if (hidpp->feature_index != 0u && payload_len >= 3u &&
        payload[1] == hidpp->feature_index && (uint8_t)(payload[2] >> 4u) == 0u) {
        bool held = false;
        for (size_t pos = 3u; pos + 1u < payload_len; pos += 2u) {
            const uint16_t cid = (uint16_t)(((uint16_t)payload[pos] << 8u) | payload[pos + 1u]);
            if (cid == 0u) break;
            if (cid == BLU2USB_HIDPP_FORWARD_CID) held = true;
        }
        held = held && hidpp->forward_applied;
        if (held != hidpp->forward_held) {
            hidpp->forward_held = held;
            if (result != NULL) {
                result->held_changed = true;
                result->forward_held = held;
            }
        }
        return true;
    }
    return true;
}

bool blu2usb_logitech_hidpp_claims_forward(const blu2usb_logitech_hidpp_t *hidpp)
{
    return hidpp != NULL && hidpp->connected && hidpp->forward_applied;
}
