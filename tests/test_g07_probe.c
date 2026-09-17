/* Exercises the verbatim PICO-08 production host, not a replacement algorithm.
 * Event fixture proves control flow only; physical pairing remains unaccepted. */
#include <assert.h>
#include <stdio.h>
#include "btstack.h"
#include "btstack_tlv.h"
#include "blu2usb/classic_hid/classic_probe.h"
#include "blu2usb/ux_model/ux_model.h"

static btstack_packet_handler_t handlers[4], hid_handler;
static unsigned handler_count;
static btstack_timer_source_t *timer;
static unsigned inquiries, names, connects, disconnects, stops, pins, confirmations;
static unsigned accepted, stores;
static uint8_t inquiry_error, name_error, connect_error;
static bool saved;
static uint8_t saved_addr[6];
static void emit(uint8_t *p, uint16_t n) {
    if(p[0]==HCI_EVENT_HID_META) hid_handler(HCI_EVENT_PACKET,0,p,n);
    else for(unsigned i=0;i<handler_count;++i) handlers[i](HCI_EVENT_PACKET,0,p,n);
}
static void complete(void) {uint8_t p[]={GAP_EVENT_INQUIRY_COMPLETE,1,0};emit(p,sizeof(p));}
void btstack_run_loop_set_timer_handler(btstack_timer_source_t *t,void (*fn)(btstack_timer_source_t*)) {t->process=fn;}
void btstack_run_loop_set_timer(btstack_timer_source_t *t,uint32_t ms) {(void)t;assert(ms==50);}
void btstack_run_loop_add_timer(btstack_timer_source_t *t) {timer=t;}
void hci_add_event_handler(btstack_packet_callback_registration_t *r) {assert(handler_count<4);handlers[handler_count++]=r->callback;}
int gap_inquiry_start(uint8_t duration) {assert(duration==5);++inquiries;return inquiry_error;}
/* Active controller inquiry cancellation completes later, as on HCI. */
int gap_inquiry_stop(void) {++stops;return 0;}
int gap_remote_name_request(const bd_addr_t a,uint8_t mode,uint16_t clock) {
    assert((a[5]==1 || a[5]==2) && mode==1 && clock==0x8123);++names;return name_error;
}
uint8_t hid_host_connect(bd_addr_t a,int mode,uint16_t *cid) {assert(a[5]==1);assert(mode==HID_PROTOCOL_MODE_REPORT);++connects;*cid=42;return connect_error;}
void hid_host_disconnect(uint16_t cid) {assert(cid==42);++disconnects;}
uint8_t hid_host_accept_connection(uint16_t cid,int mode) {assert(cid==42 && mode==HID_PROTOCOL_MODE_REPORT);++accepted;return 0;}
void hid_host_init(uint8_t *p,uint16_t n) {assert(p && n==512);}
void hid_host_register_packet_handler(btstack_packet_handler_t fn) {hid_handler=fn;}
void gap_set_default_link_policy_settings(uint16_t n) {assert(n==5);}
void hci_set_master_slave_policy(uint8_t n) {assert(n==HCI_ROLE_MASTER);}
void hci_set_inquiry_mode(int n) {assert(n==INQUIRY_MODE_RSSI_AND_EIR);}
void gap_ssp_set_io_capability(int n) {assert(n==SSP_IO_CAPABILITY_DISPLAY_ONLY);}
void gap_set_local_name(const char *n) {assert(!strcmp(n,"Remapper Pico 2 W 00:00:00:00:00:00"));}
void gap_discoverable_control(int n) {assert(n==1);}
void gap_pin_code_response(bd_addr_t a,const char *p) {assert(a[5]==1 && !strcmp(p,"0000"));++pins;}
void gap_ssp_confirmation_response(bd_addr_t a) {assert(a[5]==1);++confirmations;}
const char *bd_addr_to_str(const bd_addr_t a) {(void)a;return "test-peer";}
const uint8_t *hid_descriptor_storage_get_descriptor_data(uint16_t cid) {static uint8_t d[]={5,7};assert(cid==42);return d;}
uint16_t hid_descriptor_storage_get_descriptor_len(uint16_t cid) {assert(cid==42);return 2;}
void btstack_hid_parser_init(btstack_hid_parser_t *p,const uint8_t *d,uint16_t n,int t,const uint8_t *r,uint16_t l) {(void)p;(void)d;(void)n;(void)t;(void)r;(void)l;}
bool btstack_hid_parser_has_more(btstack_hid_parser_t *p) {(void)p;return false;}
void btstack_hid_parser_get_field(btstack_hid_parser_t *p,uint16_t *page,uint16_t *usage,int32_t *v) {(void)p;*page=7;*usage=4;*v=1;}
static int get_tag(void *c,uint32_t tag,uint8_t *data,uint32_t size) {
    (void)c;assert(tag==0x4b423347 && size==6);if(!saved)return 0;memcpy(data,saved_addr,6);return 6;
}
static int store_tag(void *c,uint32_t tag,const uint8_t *data,uint32_t size) {
    (void)c;assert(tag==0x4b423347 && size==6);saved=true;memcpy(saved_addr,data,6);++stores;return 0;
}
void btstack_tlv_get_instance(const btstack_tlv_t **impl,void **context) {
    static const btstack_tlv_t tlv={get_tag,store_tag};*impl=&tlv;*context=NULL;
}
static void tick(void) {timer->process(timer);}
static void working(void) {uint8_t p[]={BTSTACK_EVENT_STATE,1,HCI_STATE_WORKING};emit(p,sizeof(p));}
static void result_addr(const char *name,uint8_t address) {
    uint8_t p[64]={GAP_EVENT_INQUIRY_RESULT};p[1]=sizeof(p)-2;p[2]=address;p[8]=1;p[12]=0x23;p[13]=1;
    if(name) {p[25]=1;p[26]=(uint8_t)strlen(name);memcpy(p+27,name,strlen(name));}emit(p,sizeof(p));
}
static void result(const char *name) {result_addr(name,1);}
static void name_complete(const char *name) {uint8_t p[64]={HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE,62,0,1};memcpy(p+9,name,strlen(name)+1);emit(p,sizeof(p));}
static void hid(uint8_t sub,uint8_t code) {uint8_t p[]={HCI_EVENT_HID_META,4,sub,42,0,code};emit(p,sizeof(p));}
static void reset(void) {
    if(timer){blu2usb_classic_probe_cancel();tick();}
    handler_count=0;saved=false;
    blu2usb_classic_probe_setup();
    inquiries=names=connects=disconnects=stops=pins=confirmations=accepted=stores=0;
    inquiry_error=name_error=connect_error=0;working();
}
static void pair(void) {blu2usb_classic_probe_pair();tick();}
static void phase(blu2usb_probe_phase_t p) {assert(blu2usb_classic_probe_snapshot().phase==p);}
int main(void) {
    blu2usb_classic_probe_shared_init();reset();pair();phase(BLU2USB_PROBE_SEARCHING);
    result(NULL);complete();assert(names==1);
    name_complete("Bluetooth keyboard 3.0");assert(connects==1);phase(BLU2USB_PROBE_CONNECTING);
    uint8_t pin[]={HCI_EVENT_PIN_CODE_REQUEST,6,1,0,0,0,0,0};emit(pin,sizeof(pin));
    assert(pins==1);phase(BLU2USB_PROBE_PIN);assert(blu2usb_classic_probe_snapshot().pin_digits==4);
    uint8_t confirm[]={HCI_EVENT_USER_CONFIRMATION_REQUEST,10,1,0,0,0,0,0,0,0,0,0};emit(confirm,sizeof(confirm));assert(confirmations==1);
    uint8_t passkey[]={HCI_EVENT_USER_PASSKEY_NOTIFICATION,10,1,0,0,0,0,0,0x40,0xe2,1,0};emit(passkey,sizeof(passkey));
    assert(blu2usb_classic_probe_snapshot().pin==123456 && blu2usb_classic_probe_snapshot().pin_digits==6);
    hid(HID_SUBEVENT_CONNECTION_OPENED,0);hid(HID_SUBEVENT_DESCRIPTOR_AVAILABLE,0);
    phase(BLU2USB_PROBE_READY);assert(stores==1);
    blu2usb_classic_probe_cancel();tick();phase(BLU2USB_PROBE_READY);assert(disconnects==0);
    hid(HID_SUBEVENT_CONNECTION_CLOSED,0);phase(BLU2USB_PROBE_IDLE);
    for(unsigned i=0;i<100;++i) { tick(); }
    assert(connects==2); /* original saved-peer reconnect */
    reset();pair();result("BKB-3G");assert(connects==1 && names==0);
    hid(HID_SUBEVENT_CONNECTION_OPENED,0x04);assert(inquiries==2);phase(BLU2USB_PROBE_SEARCHING);
    reset();pair();connect_error=0x0c;result("BKB-3G");assert(inquiries==2);phase(BLU2USB_PROBE_SEARCHING);
    /* Failed name submission advances through remaining candidates, then scans. */
    reset();pair();result(NULL);result_addr(NULL,2);name_error=0x0c;complete();
    assert(names==2 && inquiries==2);phase(BLU2USB_PROBE_SEARCHING);
    reset();pair();result("Other device");complete();assert(inquiries==2 && names==0);
    reset();inquiry_error=0x0c;pair();phase(BLU2USB_PROBE_ERROR);
    assert(!strcmp(blu2usb_classic_probe_snapshot().message,"CLASSIC SCAN FAILED"));
    reset();pair();blu2usb_classic_probe_cancel();tick();phase(BLU2USB_PROBE_IDLE);
    complete();assert(names==0 && inquiries==1);
    reset();pair();result(NULL);complete();blu2usb_classic_probe_cancel();tick();
    name_complete("BKB-3G");assert(connects==0);phase(BLU2USB_PROBE_IDLE);
    reset();hid(HID_SUBEVENT_INCOMING_CONNECTION,0);assert(accepted==1);
    reset();pair();result("BKB-3G");hid(HID_SUBEVENT_CONNECTION_OPENED,0);
    hid(HID_SUBEVENT_DESCRIPTOR_AVAILABLE,1);phase(BLU2USB_PROBE_ERROR);assert(disconnects==1);
    blu2usb_ux_model_t ux;blu2usb_ux_init(&ux);ux.screen=BLU2USB_SCREEN_OTHER_OPTIONS;ux.selection=0;
    assert(blu2usb_ux_input(&ux,BLU2USB_CONTROL_JOY_PRESS,true).kind==BLU2USB_UX_COMMAND_NONE);
    assert(blu2usb_ux_input(&ux,BLU2USB_CONTROL_JOY_PRESS,false).kind==BLU2USB_UX_COMMAND_PAIR_KEYBOARD);
    puts("PICO-08 original host: name fallback, retry, security, persistence, reconnect, cancel PASS");
}
