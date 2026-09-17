/*
 * Copyright (C) 2016 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */
/* Test double for the Pico SDK 2.2.0 BTstack API. Event wire offsets follow
 * btstack_event.h (BlueKitchen; license below); no hardware is simulated. */
#ifndef G07_FAKE_BTSTACK_H
#define G07_FAKE_BTSTACK_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
typedef uint8_t bd_addr_t[6];
typedef struct btstack_timer_source { void (*process)(struct btstack_timer_source *); } btstack_timer_source_t;
typedef void (*btstack_packet_handler_t)(uint8_t,uint16_t,uint8_t*,uint16_t);
typedef struct { btstack_packet_handler_t callback; } btstack_packet_callback_registration_t;
typedef struct { int remaining; } btstack_hid_usage_iterator_t;
typedef struct { uint16_t usage_page; } btstack_hid_usage_item_t;
typedef struct { int unused; } btstack_hid_parser_t;
static inline uint16_t little_endian_read_16(const uint8_t *p, int n) { return p[n] | (p[n+1]<<8); }
static inline uint32_t little_endian_read_32(const uint8_t *p, int n) { return little_endian_read_16(p,n) | ((uint32_t)little_endian_read_16(p,n+2)<<16); }
static inline void reverse_bytes(const uint8_t *p,uint8_t *q,int n) { for(int i=0;i<n;i++) q[i]=p[n-1-i]; }
static inline void reverse_bd_addr(const uint8_t *p,uint8_t *q) { reverse_bytes(p,q,6); }
static inline int bd_addr_cmp(const uint8_t *p,const uint8_t *q) { return memcmp(p,q,6); }
#define BTSTACK_EVENT_STATE 0x60u
#define GAP_EVENT_INQUIRY_COMPLETE 0xDDu
#define GAP_EVENT_INQUIRY_RESULT 0xDCu
#define HCI_EVENT_COMMAND_STATUS 0x0Fu
#define HCI_EVENT_HID_META 0xEFu
#define HCI_EVENT_PACKET 0x04
#define HCI_EVENT_PIN_CODE_REQUEST 0x16u
#define HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE 0x07u
#define HCI_EVENT_USER_CONFIRMATION_REQUEST 0x33u
#define HCI_EVENT_USER_PASSKEY_REQUEST 0x34u
#define HCI_EVENT_USER_PASSKEY_NOTIFICATION 0x3Bu
#define HID_SUBEVENT_CONNECTION_CLOSED 0x03u
#define HID_SUBEVENT_CONNECTION_OPENED 0x02u
#define HID_SUBEVENT_DESCRIPTOR_AVAILABLE 0x0Du
#define HID_SUBEVENT_INCOMING_CONNECTION 0x01u
#define HID_SUBEVENT_REPORT 0x0Cu
#define HCI_STATE_WORKING 2
#define HCI_OPCODE_HCI_INQUIRY 0x0401
#define ERROR_CODE_SUCCESS 0
#define HID_PROTOCOL_MODE_REPORT 1
#define HID_REPORT_TYPE_INPUT 1
#define LM_LINK_POLICY_ENABLE_SNIFF_MODE 4
#define LM_LINK_POLICY_ENABLE_ROLE_SWITCH 1
#define HCI_ROLE_MASTER 0
#define INQUIRY_MODE_RSSI_AND_EIR 2
#define SSP_IO_CAPABILITY_DISPLAY_ONLY 0
static inline uint8_t btstack_event_state_get_state(const uint8_t * event){
    return event[2];
}
static inline void gap_event_inquiry_result_get_bd_addr(const uint8_t * event, bd_addr_t bd_addr){
    reverse_bytes(&event[2], bd_addr, 6);
}
static inline uint16_t gap_event_inquiry_result_get_clock_offset(const uint8_t * event){
    return little_endian_read_16(event, 12);
}
static inline const uint8_t * gap_event_inquiry_result_get_name(const uint8_t * event){
    return &event[27];
}
static inline uint8_t gap_event_inquiry_result_get_name_available(const uint8_t * event){
    return event[25];
}
static inline uint8_t gap_event_inquiry_result_get_name_len(const uint8_t * event){
    return event[26];
}
static inline uint8_t gap_event_inquiry_result_get_page_scan_repetition_mode(const uint8_t * event){
    return event[8];
}
static inline uint16_t hci_event_command_status_get_command_opcode(const uint8_t * event){
    return little_endian_read_16(event, 4);
}
static inline uint8_t hci_event_command_status_get_status(const uint8_t * event){
    return event[2];
}
static inline uint8_t hci_event_hid_meta_get_subevent_code(const uint8_t * event){
    return event[2];
}
static inline uint8_t hci_event_packet_get_type(const uint8_t * event){
    return event[0];
}
static inline void hci_event_pin_code_request_get_bd_addr(const uint8_t * event, bd_addr_t bd_addr){
    reverse_bytes(&event[2], bd_addr, 6);
}
static inline void hci_event_user_confirmation_request_get_bd_addr(const uint8_t * event, bd_addr_t bd_addr){
    reverse_bytes(&event[2], bd_addr, 6);
}
static inline uint16_t hid_subevent_connection_closed_get_hid_cid(const uint8_t * event){
    return little_endian_read_16(event, 3);
}
static inline uint16_t hid_subevent_connection_opened_get_hid_cid(const uint8_t * event){
    return little_endian_read_16(event, 3);
}
static inline uint8_t hid_subevent_connection_opened_get_status(const uint8_t * event){
    return event[5];
}
static inline uint16_t hid_subevent_descriptor_available_get_hid_cid(const uint8_t * event){
    return little_endian_read_16(event, 3);
}
static inline uint8_t hid_subevent_descriptor_available_get_status(const uint8_t * event){
    return event[5];
}
static inline uint16_t hid_subevent_incoming_connection_get_hid_cid(const uint8_t * event){
    return little_endian_read_16(event, 3);
}
static inline const uint8_t * hid_subevent_report_get_report(const uint8_t * event){
    return &event[7];
}
static inline uint16_t hid_subevent_report_get_report_len(const uint8_t * event){
    return little_endian_read_16(event, 5);
}

static inline void hid_subevent_incoming_connection_get_address(const uint8_t *event, bd_addr_t address) { reverse_bytes(&event[5],address,6); }
uint32_t btstack_run_loop_get_time_ms(void);
void btstack_run_loop_set_timer_handler(btstack_timer_source_t*,void (*)(btstack_timer_source_t*));
void btstack_run_loop_set_timer(btstack_timer_source_t*,uint32_t);
void btstack_run_loop_add_timer(btstack_timer_source_t*);
void hci_add_event_handler(btstack_packet_callback_registration_t*);
int gap_inquiry_start(uint8_t);
int gap_inquiry_stop(void);
int gap_remote_name_request(const bd_addr_t,uint8_t,uint16_t);
uint8_t hid_host_connect(bd_addr_t,int,uint16_t*);
void hid_host_disconnect(uint16_t);
uint8_t hid_host_accept_connection(uint16_t,int);
uint8_t hid_host_decline_connection(uint16_t);
void hid_host_init(uint8_t*,uint16_t);
void hid_host_register_packet_handler(btstack_packet_handler_t);
void gap_set_default_link_policy_settings(uint16_t);
void hci_set_master_slave_policy(uint8_t);
void hci_set_inquiry_mode(int);
void gap_ssp_set_io_capability(int);
void gap_set_local_name(const char*);
void gap_discoverable_control(int);
void gap_pin_code_response(bd_addr_t,const char*);
void gap_ssp_confirmation_response(bd_addr_t);
const uint8_t *hid_descriptor_storage_get_descriptor_data(uint16_t);
uint16_t hid_descriptor_storage_get_descriptor_len(uint16_t);
void btstack_hid_usage_iterator_init(btstack_hid_usage_iterator_t*,const uint8_t*,uint16_t,int);
bool btstack_hid_usage_iterator_has_more(btstack_hid_usage_iterator_t*);
void btstack_hid_usage_iterator_get_item(btstack_hid_usage_iterator_t*,btstack_hid_usage_item_t*);
void btstack_hid_parser_init(btstack_hid_parser_t*,const uint8_t*,uint16_t,int,const uint8_t*,uint16_t);
bool btstack_hid_parser_has_more(btstack_hid_parser_t*);
void btstack_hid_parser_get_field(btstack_hid_parser_t*,uint16_t*,uint16_t*,int32_t*);
static inline uint16_t hid_subevent_report_get_hid_cid(const uint8_t *p) {return little_endian_read_16(p,3);}
#endif
/* Shared bootstrap seams. Only the platform is replaced by this fixture. */
#define IO_CAPABILITY_NO_INPUT_NO_OUTPUT 3
#define SM_AUTHREQ_SECURE_CONNECTION 8
#define SM_AUTHREQ_BONDING 1
#define HCI_POWER_ON 1
void l2cap_init(void);
void sm_init(void);
void sm_set_io_capabilities(int);
void sm_set_authentication_requirements(int);
void gatt_client_init(void);
void att_server_init(const unsigned char *,void *,void *);
void hci_power_control(int);
void btstack_run_loop_execute(void);
