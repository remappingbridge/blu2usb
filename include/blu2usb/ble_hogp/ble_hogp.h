#ifndef BLU2USB_BLE_HOGP_BLE_HOGP_H
#define BLU2USB_BLE_HOGP_BLE_HOGP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blu2usb/bt_runtime/bt_runtime.h"
#include "blu2usb/domain/hid.h"

#define BLU2USB_BLE_HOGP_MAX_FIELDS 48u
#define BLU2USB_BLE_HOGP_MAX_REPORTS 16u
#define BLU2USB_BLE_HOGP_RUNTIME_CHANNEL UINT16_C(0x0501)
#define BLU2USB_BLE_HOGP_VENDOR_OUTPUT_MAX 32u

typedef enum {
    BLU2USB_BLE_HOGP_MESSAGE_CONNECTED = 1,
    BLU2USB_BLE_HOGP_MESSAGE_DISCONNECTED = 2,
    BLU2USB_BLE_HOGP_MESSAGE_MOUSE = 3,
} blu2usb_ble_hogp_message_type_t;

typedef enum {
    BLU2USB_BLE_HOGP_FIELD_BUTTON = 0,
    BLU2USB_BLE_HOGP_FIELD_X,
    BLU2USB_BLE_HOGP_FIELD_Y,
    BLU2USB_BLE_HOGP_FIELD_WHEEL,
    BLU2USB_BLE_HOGP_FIELD_PAN,
} blu2usb_ble_hogp_field_kind_t;

typedef struct {
    uint8_t report_id;
    blu2usb_ble_hogp_field_kind_t kind;
    uint16_t bit_offset;
    uint8_t bit_size;
    uint8_t button_index;
    bool signed_value;
} blu2usb_ble_hogp_field_t;

typedef struct {
    uint8_t report_id;
    uint16_t input_bits;
    uint8_t button_mask;
} blu2usb_ble_hogp_report_state_t;

typedef struct {
    blu2usb_hid_source_t source;
    blu2usb_ble_hogp_field_t fields[BLU2USB_BLE_HOGP_MAX_FIELDS];
    size_t field_count;
    blu2usb_ble_hogp_report_state_t reports[BLU2USB_BLE_HOGP_MAX_REPORTS];
    size_t report_count;
    uint8_t aggregate_buttons;
    bool configured;
} blu2usb_ble_hogp_parser_t;

typedef bool (*blu2usb_ble_hogp_emit_fn)(void *context,
                                          const blu2usb_canonical_mouse_event_t *event);

typedef struct {
    uint8_t address_type;
    uint8_t address[6];
} blu2usb_ble_hogp_peer_t;

typedef enum {
    BLU2USB_BLE_HOGP_EVENT_CONNECTED = 0,
    BLU2USB_BLE_HOGP_EVENT_DISCONNECTED,
    BLU2USB_BLE_HOGP_EVENT_MOUSE,
} blu2usb_ble_hogp_event_type_t;

typedef struct {
    blu2usb_ble_hogp_event_type_t type;
    blu2usb_ble_hogp_peer_t peer;
    blu2usb_canonical_mouse_event_t mouse;
} blu2usb_ble_hogp_event_t;

typedef bool (*blu2usb_ble_hogp_vendor_input_fn)(
    void *context, blu2usb_hid_source_t source, uint8_t report_id,
    const uint8_t *payload, size_t payload_len,
    blu2usb_ble_hogp_emit_fn emit, void *emit_context);
typedef bool (*blu2usb_ble_hogp_vendor_output_fn)(
    void *context, uint8_t *report_id, uint8_t *payload,
    uint16_t *payload_len, uint16_t payload_capacity);
typedef void (*blu2usb_ble_hogp_vendor_output_result_fn)(void *context, bool accepted);
typedef bool (*blu2usb_ble_hogp_vendor_claims_button_fn)(void *context,
                                                          blu2usb_mouse_button_t button);
typedef void (*blu2usb_ble_hogp_vendor_session_fn)(void *context, bool connected);

typedef struct {
    void *context;
    blu2usb_ble_hogp_vendor_input_fn input;
    blu2usb_ble_hogp_vendor_output_fn next_output;
    blu2usb_ble_hogp_vendor_output_result_fn output_result;
    blu2usb_ble_hogp_vendor_claims_button_fn claims_button;
    blu2usb_ble_hogp_vendor_session_fn session;
} blu2usb_ble_hogp_vendor_backend_t;

bool blu2usb_ble_hogp_parser_configure(blu2usb_ble_hogp_parser_t *parser,
                                        blu2usb_hid_source_t source,
                                        const uint8_t *descriptor,
                                        size_t descriptor_len);
bool blu2usb_ble_hogp_parser_has_mouse(const blu2usb_ble_hogp_parser_t *parser);
bool blu2usb_ble_hogp_parser_normalize_report(const blu2usb_ble_hogp_parser_t *parser,
                                               uint8_t report_id,
                                               const uint8_t *report,
                                               size_t report_len,
                                               const uint8_t **payload,
                                               size_t *payload_len);
bool blu2usb_ble_hogp_parser_parse_report(blu2usb_ble_hogp_parser_t *parser,
                                           uint8_t report_id,
                                           const uint8_t *report,
                                           size_t report_len,
                                           blu2usb_ble_hogp_emit_fn emit,
                                           void *context);
bool blu2usb_ble_hogp_decode_runtime_message(const blu2usb_bt_runtime_message_t *message,
                                              blu2usb_ble_hogp_event_t *event);
bool blu2usb_ble_hogp_register_vendor_backend(
    const blu2usb_ble_hogp_vendor_backend_t *backend);

/* Pico facade. Suppression pauses only idle/outgoing BLE discovery work so a
 * Classic inquiry can own the shared CYW43 radio. An already active Mouse
 * session is never disconnected by this control. */
void blu2usb_ble_hogp_pico_set_discovery_suppressed(bool suppressed);
bool blu2usb_ble_hogp_start(void);

#endif