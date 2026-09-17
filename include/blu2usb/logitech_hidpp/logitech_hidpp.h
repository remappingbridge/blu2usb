#ifndef BLU2USB_LOGITECH_HIDPP_LOGITECH_HIDPP_H
#define BLU2USB_LOGITECH_HIDPP_LOGITECH_HIDPP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BLU2USB_HIDPP_LONG_PAYLOAD_SIZE 19u
#define BLU2USB_HIDPP_REPORT_ID_SHORT 0x10u
#define BLU2USB_HIDPP_REPORT_ID_LONG 0x11u
#define BLU2USB_HIDPP_REPROG_CONTROLS_V4 0x1b04u
#define BLU2USB_HIDPP_FORWARD_CID 0x0056u

typedef enum {
    BLU2USB_HIDPP_OUTPUT_NONE = 0,
    BLU2USB_HIDPP_OUTPUT_GET_FEATURE,
    BLU2USB_HIDPP_OUTPUT_SET_FORWARD_DIVERT,
} blu2usb_hidpp_output_kind_t;

typedef struct {
    blu2usb_hidpp_output_kind_t kind;
    uint8_t report_id;
    uint8_t payload[BLU2USB_HIDPP_LONG_PAYLOAD_SIZE];
    size_t payload_len;
} blu2usb_hidpp_output_t;

typedef struct {
    bool consumed;
    bool held_changed;
    bool forward_held;
} blu2usb_hidpp_input_result_t;

typedef struct {
    bool connected;
    bool forward_desired;
    bool forward_applied;
    bool forward_held;
    bool feature_failed;
    uint8_t feature_index;
    blu2usb_hidpp_output_kind_t waiting_for;
    bool pending_enable;
} blu2usb_logitech_hidpp_t;

void blu2usb_logitech_hidpp_init(blu2usb_logitech_hidpp_t *hidpp);
void blu2usb_logitech_hidpp_on_connect(blu2usb_logitech_hidpp_t *hidpp);
void blu2usb_logitech_hidpp_on_disconnect(blu2usb_logitech_hidpp_t *hidpp);
void blu2usb_logitech_hidpp_set_forward_desired(blu2usb_logitech_hidpp_t *hidpp, bool desired);
bool blu2usb_logitech_hidpp_next_output(blu2usb_logitech_hidpp_t *hidpp,
                                         blu2usb_hidpp_output_t *output);
void blu2usb_logitech_hidpp_output_result(blu2usb_logitech_hidpp_t *hidpp,
                                           blu2usb_hidpp_output_kind_t kind,
                                           bool accepted);
bool blu2usb_logitech_hidpp_process_input(blu2usb_logitech_hidpp_t *hidpp,
                                           uint8_t report_id,
                                           const uint8_t *payload,
                                           size_t payload_len,
                                           blu2usb_hidpp_input_result_t *result);
bool blu2usb_logitech_hidpp_claims_forward(const blu2usb_logitech_hidpp_t *hidpp);

bool blu2usb_logitech_hidpp_pico_start(void);
void blu2usb_logitech_hidpp_pico_set_forward_fix(bool enabled);

#endif
