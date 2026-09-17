#ifndef BLU2USB_BLE_HOGP_BLE_HOGP_H
#define BLU2USB_BLE_HOGP_BLE_HOGP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blu2usb/bt_runtime/bt_runtime.h"
#include "blu2usb/domain/hid.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLU2USB_BLE_HOGP_MAX_FIELDS 48u
#define BLU2USB_BLE_HOGP_MAX_REPORTS 16u
#define BLU2USB_BLE_HOGP_RUNTIME_CHANNEL UINT16_C(0x0501)

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

typedef bool (*blu2usb_ble_hogp_emit_fn)(void *context, const blu2usb_canonical_mouse_event_t *event);

typedef enum {
    BLU2USB_BLE_HOGP_EVENT_CONNECTED = 0,
    BLU2USB_BLE_HOGP_EVENT_DISCONNECTED,
    BLU2USB_BLE_HOGP_EVENT_MOUSE,
} blu2usb_ble_hogp_event_type_t;

typedef struct {
    blu2usb_ble_hogp_event_type_t type;
    blu2usb_canonical_mouse_event_t mouse;
} blu2usb_ble_hogp_event_t;

bool blu2usb_ble_hogp_parser_configure(blu2usb_ble_hogp_parser_t *parser,
                                        blu2usb_hid_source_t source,
                                        const uint8_t *descriptor,
                                        size_t descriptor_len);
bool blu2usb_ble_hogp_parser_has_mouse(const blu2usb_ble_hogp_parser_t *parser);

/* Accepts either canonical descriptor-sized payload or BTstack HIDS framing
 * with one duplicated Report ID byte. All other lengths are rejected. */
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

bool blu2usb_ble_hogp_start(void);

#ifdef __cplusplus
}
#endif

#endif
