/* Executes the production adapter against deterministic asynchronous events. */
#include <assert.h>
#include <stdio.h>
#include "btstack.h"
#include "blu2usb/classic_hid/classic_hid.h"
#include "blu2usb/keyboard_transport/keyboard_transport.h"

static blu2usb_bt_runtime_session_setup_fn setup;
static btstack_packet_handler_t handler;
static btstack_timer_source_t *timer;
static uint32_t now;
static unsigned inquiries, names, connects, disconnects, resumes;
static bool radio_ready = true, stop_completes = true;
static uint8_t inquiry_result;
static blu2usb_keyboard_pair_progress_t latest;
static unsigned connected_events;

bool blu2usb_bt_runtime_register_session_setup(blu2usb_bt_runtime_session_setup_fn fn) { setup=fn; return true; }
uint32_t btstack_run_loop_get_time_ms(void) { return now; }
void btstack_run_loop_set_timer_handler(btstack_timer_source_t *t,void (*fn)(btstack_timer_source_t*)) { t->process=fn; }
void btstack_run_loop_set_timer(btstack_timer_source_t *t,uint32_t ms) { (void)t; (void)ms; }
void btstack_run_loop_add_timer(btstack_timer_source_t *t) { timer=t; }
void hci_add_event_handler(btstack_packet_callback_registration_t *r) { handler=r->callback; }
static void emit(uint8_t *p, uint16_t n) { handler(HCI_EVENT_PACKET,0,p,n); }
static void complete(void) { uint8_t p[]={GAP_EVENT_INQUIRY_COMPLETE,1,0}; emit(p,sizeof(p)); }
int gap_inquiry_start(uint8_t duration) { assert(duration==5); ++inquiries; return inquiry_result; }
int gap_inquiry_stop(void) { if(stop_completes) complete(); return 0; }
int gap_remote_name_request(const bd_addr_t a,uint8_t mode,uint16_t clock) { (void)a;(void)mode;(void)clock; ++names;return 0; }
uint8_t hid_host_connect(bd_addr_t a,int mode,uint16_t *cid) { (void)a;assert(mode==HID_PROTOCOL_MODE_REPORT);++connects;*cid=42;return 0; }
void hid_host_disconnect(uint16_t cid) { assert(cid==42);++disconnects; }
uint8_t hid_host_accept_connection(uint16_t cid,int mode) { (void)cid;(void)mode;return 0; }
uint8_t hid_host_decline_connection(uint16_t cid) { (void)cid;return 0; }
void hid_host_init(uint8_t *p,uint16_t n) { (void)p;assert(n==1024); }
void hid_host_register_packet_handler(btstack_packet_handler_t fn) { (void)fn; }
void gap_set_default_link_policy_settings(uint16_t n) {(void)n;}
void hci_set_master_slave_policy(uint8_t n) {(void)n;}
void hci_set_inquiry_mode(int n) {(void)n;}
void gap_ssp_set_io_capability(int n) {(void)n;}
void gap_set_local_name(const char *n) {(void)n;}
void gap_discoverable_control(int n) {(void)n;}
void gap_pin_code_response(bd_addr_t a,const char *p) {(void)a;(void)p;}
void gap_ssp_confirmation_response(bd_addr_t a) {(void)a;}
bool blu2usb_ble_hogp_pico_pause_discovery_for_classic(void) {return radio_ready;}
void blu2usb_ble_hogp_pico_resume_discovery_after_classic(void) {++resumes;}
const uint8_t *hid_descriptor_storage_get_descriptor_data(uint16_t cid) {static uint8_t d[]={5,7}; (void)cid;return d;}
uint16_t hid_descriptor_storage_get_descriptor_len(uint16_t cid) {(void)cid;return 2;}
void btstack_hid_usage_iterator_init(btstack_hid_usage_iterator_t *it,const uint8_t *p,uint16_t n,int t) {(void)p;(void)n;(void)t;it->remaining=1;}
bool btstack_hid_usage_iterator_has_more(btstack_hid_usage_iterator_t *it) {return it->remaining!=0;}
void btstack_hid_usage_iterator_get_item(btstack_hid_usage_iterator_t *it,btstack_hid_usage_item_t *item) {it->remaining=0;item->usage_page=7;}
void btstack_hid_parser_init(btstack_hid_parser_t *p,const uint8_t *d,uint16_t n,int t,const uint8_t *r,uint16_t l) {(void)p;(void)d;(void)n;(void)t;(void)r;(void)l;}
bool btstack_hid_parser_has_more(btstack_hid_parser_t *p) {(void)p;return false;}
void btstack_hid_parser_get_field(btstack_hid_parser_t *p,uint16_t *a,uint16_t *b,int32_t *c) {(void)p;(void)a;(void)b;(void)c;}

static void drain(void) {
    blu2usb_bt_runtime_message_t m;
    while(blu2usb_bt_runtime_poll(&m)) {
        blu2usb_keyboard_transport_event_t e;
        assert(blu2usb_keyboard_transport_decode_runtime_message(&m,&e));
        if(e.type==BLU2USB_KEYBOARD_TRANSPORT_EVENT_PROGRESS) latest=e.progress;
        if(e.type==BLU2USB_KEYBOARD_TRANSPORT_EVENT_CONNECTED) ++connected_events;
    }
}
static void tick(uint32_t ms) {now+=ms;timer->process(timer);drain();}
static void working(void) {uint8_t p[]={BTSTACK_EVENT_STATE,1,HCI_STATE_WORKING};emit(p,sizeof(p));}
static void status(uint8_t code) {uint8_t p[]={HCI_EVENT_COMMAND_STATUS,4,code,1,1,4};emit(p,sizeof(p));drain();}
static void result(bool named) {
    uint8_t p[64]={GAP_EVENT_INQUIRY_RESULT};p[1]=sizeof(p)-2;p[2]=1;p[8]=1;
    if(named) {p[25]=1;p[26]=6;memcpy(p+27,"BKB-3G",6);}
    emit(p,sizeof(p));drain();
}
static void hid(uint8_t sub,uint16_t cid,uint8_t code) {uint8_t p[]={HCI_EVENT_HID_META,4,sub,cid&255,cid>>8,code};emit(p,sizeof(p));drain();}
static void reset(bool ready) {
    blu2usb_classic_hid_pico_cancel_pairing(); if(timer) tick(1);
    blu2usb_bt_runtime_reset();setup(); now=100;inquiries=names=connects=disconnects=resumes=connected_events=0;
    radio_ready=stop_completes=true;inquiry_result=0;memset(&latest,0,sizeof(latest));
    if(ready) working();
}
static void pair(void) {assert(blu2usb_classic_hid_pico_pair_keyboard());tick(50);}
int main(void) {
    assert(blu2usb_classic_hid_pico_register());
    /* Reproduce API success followed by asynchronous controller rejection.
     * The old production adapter never leaves INQUIRY in this scenario. */
    reset(true); pair(); status(0x0c); tick(1050); assert(inquiries==2);
    reset(false); pair(); assert(inquiries==0);working();tick(50);assert(inquiries==1);
    assert(latest.phase==BLU2USB_KEYBOARD_PAIR_START_SEARCH);
    status(0x0c);assert(latest.phase==BLU2USB_KEYBOARD_PAIR_RETRY);assert(latest.error==0x0c);
    tick(1050);assert(inquiries==2);status(0);assert(latest.phase==BLU2USB_KEYBOARD_PAIR_SEARCHING);
    /* Cancel completion may be synchronous; it must not resolve any names. */
    result(false);blu2usb_classic_hid_pico_cancel_pairing();tick(50);assert(names==0);assert(resumes>0);
    reset(true);pair();status(0);tick(10050);assert(latest.phase==BLU2USB_KEYBOARD_PAIR_ERROR);assert(resumes>0);
    reset(true);radio_ready=false;pair();tick(15050);assert(latest.phase==BLU2USB_KEYBOARD_PAIR_ERROR);assert(inquiries==0);
    reset(false);pair();tick(15050);assert(latest.phase==BLU2USB_KEYBOARD_PAIR_ERROR);
    reset(true);pair();status(0);result(false);complete();tick(50);assert(names==1);tick(10050);
    assert(latest.phase==BLU2USB_KEYBOARD_PAIR_ERROR);pair();assert(names==1);assert(inquiries==1);
    /* Late name completion after failure drains ownership, never connects. */
    uint8_t name[32]={HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE,30,0,1};memcpy(name+9,"BKB-3G",7);emit(name,sizeof(name));assert(connects==0);
    reset(true);pair();status(0);result(true);assert(connects==1);tick(30050);assert(disconnects>0);assert(latest.phase==BLU2USB_KEYBOARD_PAIR_ERROR);
    /* A late descriptor cannot commit an aborted connection. */
    hid(HID_SUBEVENT_DESCRIPTOR_AVAILABLE,42,0);assert(connected_events==0);
    hid(HID_SUBEVENT_CONNECTION_CLOSED,99,0);pair();assert(connects==1);
    hid(HID_SUBEVENT_CONNECTION_CLOSED,42,0);pair();status(0);result(true);assert(connects==2);
    hid(HID_SUBEVENT_CONNECTION_OPENED,42,0);assert(latest.phase==BLU2USB_KEYBOARD_PAIR_SETUP);
    hid(HID_SUBEVENT_DESCRIPTOR_AVAILABLE,42,0);assert(connected_events==1);assert(resumes>0);
    tick(120000);assert(connected_events==1);
    reset(true);pair();status(0);result(true);blu2usb_classic_hid_pico_cancel_pairing();tick(50);
    hid(HID_SUBEVENT_CONNECTION_OPENED,42,0);hid(HID_SUBEVENT_DESCRIPTOR_AVAILABLE,42,0);assert(connected_events==0);
    reset(true);pair();status(0x0c);tick(1050);status(0x0c);tick(1050);status(0x0c);
    assert(latest.phase==BLU2USB_KEYBOARD_PAIR_ERROR);tick(60000);assert(inquiries==3);
    /* Cancellation before controller ACK must not mistake API acceptance for
     * an active inquiry or synchronously run remote-name resolution. */
    reset(true);pair();blu2usb_classic_hid_pico_cancel_pairing();tick(50);
    status(0);tick(50);assert(names==0);assert(connects==0);assert(inquiries==1);
    /* Missing inquiry ACK does not permit another outstanding transaction. */
    reset(true);pair();tick(10050);pair();assert(inquiries==1);
    puts("G07 production adapter asynchronous regression scenarios passed");
}
