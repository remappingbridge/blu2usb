/* Deterministic event tests of the production A adapter, not a radio simulator. */
#include <assert.h>
#include <stdio.h>
#include "btstack.h"
#include "blu2usb/classic_hid/classic_probe.h"
#include "blu2usb/ux_model/ux_model.h"

static btstack_packet_handler_t handler;
static btstack_timer_source_t *timer;
static unsigned inquiries, names, connects, disconnects, stops, declines, pins;
static uint8_t inquiry_error;
static bool keyboard = true;
static void emit(uint8_t *p, uint16_t n) { handler(HCI_EVENT_PACKET,0,p,n); }
static void complete(void) { uint8_t p[]={GAP_EVENT_INQUIRY_COMPLETE,1,0}; emit(p,sizeof(p)); }
void btstack_run_loop_set_timer_handler(btstack_timer_source_t *t,void (*fn)(btstack_timer_source_t*)) { t->process=fn; }
void btstack_run_loop_set_timer(btstack_timer_source_t *t,uint32_t ms) { (void)t; assert(ms==50); }
void btstack_run_loop_add_timer(btstack_timer_source_t *t) { timer=t; }
void hci_add_event_handler(btstack_packet_callback_registration_t *r) { handler=r->callback; }
int gap_inquiry_start(uint8_t duration) { assert(duration==5); ++inquiries; return inquiry_error; }
int gap_inquiry_stop(void) { ++stops; complete(); return 0; }
int gap_remote_name_request(const bd_addr_t a,uint8_t mode,uint16_t clock) {
    assert(a[5]==1 && mode==1 && clock==0x8123); ++names; return 0;
}
uint8_t hid_host_connect(bd_addr_t a,int mode,uint16_t *cid) { assert(a[5]==1);assert(mode==HID_PROTOCOL_MODE_REPORT);++connects;*cid=42;return 0; }
void hid_host_disconnect(uint16_t cid) { assert(cid==42);++disconnects; }
uint8_t hid_host_accept_connection(uint16_t cid,int mode) { assert(cid==42 && mode==HID_PROTOCOL_MODE_REPORT);return 0; }
uint8_t hid_host_decline_connection(uint16_t cid) { (void)cid;++declines;return 0; }
void hid_host_init(uint8_t *p,uint16_t n) { assert(p && n==1024); }
void hid_host_register_packet_handler(btstack_packet_handler_t fn) { assert(fn); }
void gap_set_default_link_policy_settings(uint16_t n) {assert(n==5);}
void hci_set_master_slave_policy(uint8_t n) {assert(n==HCI_ROLE_MASTER);}
void hci_set_inquiry_mode(int n) {assert(n==INQUIRY_MODE_RSSI_AND_EIR);}
void gap_ssp_set_io_capability(int n) {assert(n==SSP_IO_CAPABILITY_DISPLAY_ONLY);}
void gap_set_local_name(const char *n) {assert(strstr(n,"BLU2USB"));}
void gap_discoverable_control(int n) {assert(n==1);}
void gap_pin_code_response(bd_addr_t a,const char *p) {assert(a[5]==1 && !strcmp(p,"0000"));++pins;}
void gap_ssp_confirmation_response(bd_addr_t a) {assert(a[5]==1);}
const uint8_t *hid_descriptor_storage_get_descriptor_data(uint16_t cid) {static uint8_t d[]={5,7};assert(cid==42);return d;}
uint16_t hid_descriptor_storage_get_descriptor_len(uint16_t cid) {assert(cid==42);return 2;}
void btstack_hid_usage_iterator_init(btstack_hid_usage_iterator_t *it,const uint8_t *p,uint16_t n,int t) {assert(p && n && t==HID_REPORT_TYPE_INPUT);it->remaining=1;}
bool btstack_hid_usage_iterator_has_more(btstack_hid_usage_iterator_t *it) {return it->remaining!=0;}
void btstack_hid_usage_iterator_get_item(btstack_hid_usage_iterator_t *it,btstack_hid_usage_item_t *item) {it->remaining=0;item->usage_page=keyboard ? 7 : 1;}
static void tick(void) {timer->process(timer);}
static void working(void) {uint8_t p[]={BTSTACK_EVENT_STATE,1,HCI_STATE_WORKING};emit(p,sizeof(p));}
static void ack(uint8_t code) {uint8_t p[]={HCI_EVENT_COMMAND_STATUS,4,code,1,1,4};emit(p,sizeof(p));}
static void result(const char *name) {
    uint8_t p[64]={GAP_EVENT_INQUIRY_RESULT};p[1]=sizeof(p)-2;p[2]=1;p[8]=1;p[12]=0x23;p[13]=1;
    if(name) {p[25]=1;p[26]=(uint8_t)strlen(name);memcpy(p+27,name,strlen(name));}
    emit(p,sizeof(p));
}
static void name_complete(const char *name) {uint8_t p[64]={HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE,62,0,1};memcpy(p+9,name,strlen(name)+1);emit(p,sizeof(p));}
static void hid(uint8_t sub,uint16_t cid,uint8_t code) {uint8_t p[]={HCI_EVENT_HID_META,4,sub,cid&255,cid>>8,code};emit(p,sizeof(p));}
static void reset(bool ready) {
    blu2usb_classic_probe_cancel(); if(timer) tick();
    blu2usb_classic_probe_setup();
    inquiries=names=connects=disconnects=stops=declines=pins=0;inquiry_error=0;keyboard=true;
    if(ready) working();
}
static void pair(void) {blu2usb_classic_probe_pair();tick();}
static void phase(blu2usb_probe_phase_t p) {assert(blu2usb_classic_probe_snapshot().phase==p);}
int main(void) {
    /* Pair survives pre-setup and pre-WORKING ordering. */
    blu2usb_classic_probe_pair();blu2usb_classic_probe_setup();tick();assert(inquiries==0);
    working();assert(inquiries==1);phase(BLU2USB_PROBE_WAIT_ACK);
    ack(0);phase(BLU2USB_PROBE_SEARCHING);
    /* No patch-on-patch retries: API and asynchronous errors are observable. */
    reset(true);inquiry_error=0x0c;pair();phase(BLU2USB_PROBE_ERROR);tick();assert(inquiries==1);
    reset(true);pair();ack(0x0c);phase(BLU2USB_PROBE_ERROR);assert(blu2usb_classic_probe_snapshot().error==0x0c);
    for(int i=0;i<50;i++) { tick(); }
    assert(inquiries==1);
    pair();assert(inquiries==2);
    /* Cancel submitted inquiry: defer stop until real ACK, then drain. */
    reset(true);pair();blu2usb_classic_probe_cancel();tick();assert(stops==0);phase(BLU2USB_PROBE_DRAINING);
    ack(0);assert(stops==1);phase(BLU2USB_PROBE_IDLE);assert(names==0);
    /* Cancel active inquiry before synchronous completion, no recursive name. */
    reset(true);pair();ack(0);result(NULL);blu2usb_classic_probe_cancel();tick();assert(names==0);phase(BLU2USB_PROBE_IDLE);
    /* Serial remote name, correct scan parameters, and both target aliases. */
    reset(true);pair();ack(0);result(NULL);complete();assert(names==1);phase(BLU2USB_PROBE_READ_NAME);
    name_complete("Bluetooth keyboard 3.0");assert(connects==1);phase(BLU2USB_PROBE_CONNECTING);
    hid(HID_SUBEVENT_CONNECTION_OPENED,42,0);phase(BLU2USB_PROBE_SETUP);
    hid(HID_SUBEVENT_DESCRIPTOR_AVAILABLE,42,0);phase(BLU2USB_PROBE_READY);
    blu2usb_classic_probe_cancel();tick();phase(BLU2USB_PROBE_READY);assert(disconnects==0);
    hid(HID_SUBEVENT_CONNECTION_CLOSED,99,0);phase(BLU2USB_PROBE_READY);
    hid(HID_SUBEVENT_CONNECTION_CLOSED,42,0);phase(BLU2USB_PROBE_IDLE);
    reset(true);pair();ack(0);result("BKB-3G");assert(connects==1 && names==0);
    /* Late name and descriptor must not complete canceled pairing. */
    reset(true);pair();ack(0);result(NULL);complete();blu2usb_classic_probe_cancel();tick();
    pair();assert(inquiries==1);name_complete("BKB-3G");assert(connects==0);phase(BLU2USB_PROBE_IDLE);
    reset(true);pair();ack(0);result("BKB-3G");blu2usb_classic_probe_cancel();tick();
    hid(HID_SUBEVENT_DESCRIPTOR_AVAILABLE,42,0);assert(blu2usb_classic_probe_snapshot().phase!=BLU2USB_PROBE_READY);
    hid(HID_SUBEVENT_CONNECTION_CLOSED,42,0);phase(BLU2USB_PROBE_IDLE);
    /* Descriptor failure / non-keyboard never becomes a successful connection. */
    reset(true);pair();ack(0);result("BKB-3G");keyboard=false;
    hid(HID_SUBEVENT_DESCRIPTOR_AVAILABLE,42,0);phase(BLU2USB_PROBE_ERROR);assert(disconnects==1);
    /* PIN security belongs to the selected address. */
    reset(true);pair();ack(0);result("BKB-3G");
    uint8_t pin[]={HCI_EVENT_PIN_CODE_REQUEST,6,2,0,0,0,0,0};emit(pin,sizeof(pin));assert(pins==0);
    pin[2]=1;emit(pin,sizeof(pin));assert(pins==1);phase(BLU2USB_PROBE_PIN);assert(blu2usb_classic_probe_snapshot().pin_digits==4);
    uint8_t passkey[]={HCI_EVENT_USER_PASSKEY_NOTIFICATION,10,1,0,0,0,0,0,0x40,0xe2,1,0};
    emit(passkey,sizeof(passkey));assert(blu2usb_classic_probe_snapshot().pin==123456);
    assert(blu2usb_classic_probe_snapshot().pin_digits==6);
    /* Real UX release emits exactly one transport-neutral Pair command. */
    blu2usb_ux_model_t ux;blu2usb_ux_init(&ux);ux.screen=BLU2USB_SCREEN_OTHER_OPTIONS;ux.selection=0;
    assert(blu2usb_ux_input(&ux,BLU2USB_CONTROL_JOY_PRESS,true).kind==BLU2USB_UX_COMMAND_NONE);
    assert(blu2usb_ux_input(&ux,BLU2USB_CONTROL_JOY_PRESS,false).kind==BLU2USB_UX_COMMAND_PAIR_KEYBOARD);
    assert(ux.screen==BLU2USB_SCREEN_PAIR_KEYBOARD);
    puts("Experiment A: discovery, name, ACK, cancellation, security, descriptor, release PASS");
}
