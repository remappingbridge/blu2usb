#include "blu2usb/logitech_hidpp/logitech_hidpp.h"

#include <string.h>
#include "blu2usb/ble_hogp/ble_hogp.h"

static blu2usb_logitech_hidpp_t g_hidpp;
static volatile bool g_forward_desired;
static blu2usb_hidpp_output_kind_t g_last_output_kind;
static bool g_transport_failed;

static bool vendor_input(void *context, blu2usb_hid_source_t source,
                         uint8_t report_id, const uint8_t *payload,
                         size_t payload_len, blu2usb_ble_hogp_emit_fn emit,
                         void *emit_context)
{
    (void)context;
    blu2usb_hidpp_input_result_t result;
    if (!blu2usb_logitech_hidpp_process_input(&g_hidpp, report_id, payload,
                                               payload_len, &result)) return false;
    if (result.held_changed && emit != NULL) {
        blu2usb_canonical_mouse_event_t event = {0};
        event.source = source;
        event.type = BLU2USB_MOUSE_EVENT_BUTTON;
        event.data.button.button = BLU2USB_MOUSE_BUTTON_FORWARD;
        event.data.button.pressed = result.forward_held;
        (void)emit(emit_context, &event);
    }
    return result.consumed;
}

static bool vendor_next_output(void *context, uint8_t *report_id,
                               uint8_t *payload, uint16_t *payload_len,
                               uint16_t payload_capacity)
{
    (void)context;
    if (report_id == NULL || payload == NULL || payload_len == NULL || g_transport_failed)
        return false;
    blu2usb_logitech_hidpp_set_forward_desired(&g_hidpp, g_forward_desired);
    blu2usb_hidpp_output_t output;
    if (!blu2usb_logitech_hidpp_next_output(&g_hidpp, &output) ||
        output.payload_len > payload_capacity || output.payload_len > UINT16_MAX) return false;
    *report_id = output.report_id;
    *payload_len = (uint16_t)output.payload_len;
    memcpy(payload, output.payload, output.payload_len);
    g_last_output_kind = output.kind;
    return true;
}

static void vendor_output_result(void *context, bool accepted)
{
    (void)context;
    if (!accepted) { g_transport_failed = true; return; }
    blu2usb_logitech_hidpp_output_result(&g_hidpp, g_last_output_kind, true);
}

static bool vendor_claims_button(void *context, blu2usb_mouse_button_t button)
{
    (void)context;
    return button == BLU2USB_MOUSE_BUTTON_FORWARD &&
        blu2usb_logitech_hidpp_claims_forward(&g_hidpp);
}

static void vendor_session(void *context, bool connected)
{
    (void)context;
    g_transport_failed = false;
    if (connected) {
        blu2usb_logitech_hidpp_on_connect(&g_hidpp);
        blu2usb_logitech_hidpp_set_forward_desired(&g_hidpp, g_forward_desired);
    } else {
        blu2usb_logitech_hidpp_on_disconnect(&g_hidpp);
    }
    g_last_output_kind = BLU2USB_HIDPP_OUTPUT_NONE;
}

bool blu2usb_logitech_hidpp_pico_start(void)
{
    blu2usb_logitech_hidpp_init(&g_hidpp);
    g_forward_desired = false;
    g_last_output_kind = BLU2USB_HIDPP_OUTPUT_NONE;
    g_transport_failed = false;
    const blu2usb_ble_hogp_vendor_backend_t backend = {
        .context = NULL,
        .input = vendor_input,
        .next_output = vendor_next_output,
        .output_result = vendor_output_result,
        .claims_button = vendor_claims_button,
        .session = vendor_session,
    };
    return blu2usb_ble_hogp_register_vendor_backend(&backend);
}

void blu2usb_logitech_hidpp_pico_set_forward_fix(bool enabled)
{
    g_forward_desired = enabled;
}
