#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "btstack.h"
#include "blu2usb/classic_hid/classic_hid.h"
#include "blu2usb/keyboard_transport/keyboard_transport.h"
#include "fixtures/bkb3g_descriptor.h"

// Real production adapter/parser and pinned BTstack headers/parser/utility code.
// Only controller operations and the run-loop clock/queue are deterministic doubles.
static btstack_packet_handler_t callback;
static btstack_timer_source_t *timer;
static btstack_context_callback_registration_t *pending;
static uint32_t clock_ms;
static unsigned scans, bonds, connects, disconnects, declines;
static bool old_acl_present;
static const bd_addr_t target = {0x20,0x20,0x01,0x60,0x0b,0x94};
void hci_dump_log(int level, const char *format, ...) { (void)level; (void)format; }

uint32_t btstack_run_loop_get_time_ms(void) { return clock_ms; }
void btstack_run_loop_set_timer_handler(btstack_timer_source_t *ts, void (*fn)(btstack_timer_source_t *))
{ ts->process=fn; }
void btstack_run_loop_set_timer(btstack_timer_source_t *ts, uint32_t ms) { ts->timeout=clock_ms+ms; }
void btstack_run_loop_add_timer(btstack_timer_source_t *ts) { timer=ts; }
void btstack_run_loop_execute_on_main_thread(btstack_context_callback_registration_t *cb)
{ assert(!pending); pending=cb; }
void hci_add_event_handler(btstack_packet_callback_registration_t *r) { callback=r->callback; }
void hid_host_register_packet_handler(btstack_packet_handler_t cb) { assert(cb==callback || !callback); callback=cb; }
void hid_host_init(uint8_t *storage, uint16_t len) { assert(storage && len==512); }
void gap_set_default_link_policy_settings(uint16_t p) { (void)p; }
void hci_set_master_slave_policy(uint8_t p) { assert(p==HCI_ROLE_MASTER); }
void hci_set_inquiry_mode(inquiry_mode_t p) { assert(p==INQUIRY_MODE_RSSI_AND_EIR); }
void gap_set_bondable_mode(int p) { assert(p==1); }
void gap_ssp_set_io_capability(int p) { assert(p==SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT); }
void gap_ssp_set_authentication_requirement(int p)
{ assert(p==SSP_IO_AUTHREQ_MITM_PROTECTION_NOT_REQUIRED_GENERAL_BONDING); }
void gap_ssp_set_auto_accept(int p) { assert(p==1); }
void gap_set_local_name(const char *name) { assert(name); }
void gap_discoverable_control(uint8_t p) { assert(p==1); }
int gap_inquiry_start(uint8_t duration) { assert(duration==5); ++scans; return 0; }
int gap_inquiry_stop(void) { return 0; }
int gap_remote_name_request(const bd_addr_t addr, uint8_t mode, uint16_t offset)
{ assert(bd_addr_cmp(addr,target)==0); (void)mode;(void)offset; return 0; }
int gap_dedicated_bonding(bd_addr_t addr, int mitm)
{ assert(bd_addr_cmp(addr,target)==0 && mitm==0); ++bonds; return 0; }
uint8_t hid_host_connect(bd_addr_t addr, hid_protocol_mode_t mode, uint16_t *cid)
{
    assert(!old_acl_present); // catches the original synchronous-start defect
    assert(bd_addr_cmp(addr,target)==0 && mode==HID_PROTOCOL_MODE_REPORT);
    ++connects; *cid=1; return 0;
}
void hid_host_disconnect(uint16_t cid) { assert(cid==1); ++disconnects; }
uint8_t hid_host_accept_connection(uint16_t cid, hid_protocol_mode_t mode)
{ assert(cid==1 && mode==HID_PROTOCOL_MODE_REPORT); return 0; }
uint8_t hid_host_decline_connection(uint16_t cid) { (void)cid; ++declines; return 0; }
uint8_t gap_disconnect(hci_con_handle_t handle) { assert(handle==11); ++disconnects; return 0; }
int gap_pin_code_response(const bd_addr_t addr, const char *pin)
{ assert(bd_addr_cmp(addr,target)==0 && strcmp(pin,"0000")==0); return 0; }
int gap_ssp_confirmation_response(const bd_addr_t addr) { assert(bd_addr_cmp(addr,target)==0); return 0; }
const uint8_t *hid_descriptor_storage_get_descriptor_data(uint16_t cid)
{ assert(cid==1); return accepted_descriptor; }
uint16_t hid_descriptor_storage_get_descriptor_len(uint16_t cid)
{ assert(cid==1); return sizeof(accepted_descriptor); }

static void tick(unsigned elapsed)
{ clock_ms+=elapsed; assert(timer && timer->process); timer->process(timer); }
static void emit(uint8_t *event, unsigned length)
{ event[1]=(uint8_t)(length-2u); callback(HCI_EVENT_PACKET,0,event,(uint16_t)length); }
static void state_working(void)
{ uint8_t event[]={BTSTACK_EVENT_STATE,1,HCI_STATE_WORKING}; emit(event,sizeof(event)); }
static void inquiry_complete(void)
{ uint8_t event[]={GAP_EVENT_INQUIRY_COMPLETE,1,0}; emit(event,sizeof(event)); }
static void disconnect_event(uint16_t handle)
{
    uint8_t event[]={HCI_EVENT_DISCONNECTION_COMPLETE,4,0,0,0,0x16};
    little_endian_store_16(event,3,handle); emit(event,sizeof(event));
}
static void hid(uint8_t subevent, uint16_t cid, uint8_t status)
{
    uint8_t event[15]={HCI_EVENT_HID_META,13,subevent,0,0,status};
    little_endian_store_16(event,3,cid); emit(event,sizeof(event));
}
static void prepare_bond(void)
{
    blu2usb_classic_hid_request_pair(); tick(20);
    assert(scans==0 && blu2usb_classic_hid_status()==BLU2USB_KEYBOARD_WAITING);
    state_working(); tick(20); assert(scans==1);
    uint8_t found[27]={GAP_EVENT_INQUIRY_RESULT};
    reverse_bd_addr(target,found+2); found[8]=1;
    emit(found,sizeof(found)); inquiry_complete();
    assert(blu2usb_classic_hid_status()==BLU2USB_KEYBOARD_READING_NAME);
    uint8_t name[40]={HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE};
    reverse_bd_addr(target,name+3); strcpy((char *)name+9,"Bluetooth keyboard 3.0");
    emit(name,sizeof(name)); assert(bonds==1);
    uint8_t acl[13]={HCI_EVENT_CONNECTION_COMPLETE,11,0,11,0};
    reverse_bd_addr(target,acl+5); emit(acl,sizeof(acl));
    old_acl_present=true;
}
static void bond_complete(uint8_t status)
{
    uint8_t event[9]={GAP_EVENT_DEDICATED_BONDING_COMPLETED,7,status};
    reverse_bd_addr(target,event+3); emit(event,sizeof(event));
}
static void drain_deferred(void)
{
    assert(pending); disconnect_event(11); old_acl_present=false;
    btstack_context_callback_registration_t *cb=pending; pending=NULL;
    cb->callback(cb->context);
}
static void ready(void)
{
    prepare_bond(); bond_complete(0);
    assert(connects==0 && pending);
    bond_complete(0); // duplicate completion is harmless
    drain_deferred(); assert(connects==1);
    hid(HID_SUBEVENT_CONNECTION_OPENED,99,0); // foreign CID cannot connect us
    assert(blu2usb_classic_hid_status()==BLU2USB_KEYBOARD_CONNECTING);
    hid(HID_SUBEVENT_SNIFF_SUBRATING_PARAMS,1,0);
    hid(HID_SUBEVENT_CONNECTION_OPENED,1,0);
    assert(blu2usb_classic_hid_status()!=BLU2USB_KEYBOARD_READY);
    hid(HID_SUBEVENT_DESCRIPTOR_AVAILABLE,1,0);
    assert(blu2usb_classic_hid_status()==BLU2USB_KEYBOARD_READY);
}
static void input(uint8_t key)
{
    uint8_t event[17]={HCI_EVENT_HID_META,15,HID_SUBEVENT_REPORT,1,0,10,0,
                       0xa1,1,0,0,key,0,0,0,0,0};
    emit(event,sizeof(event));
}
static void test_parser(void)
{
    assert(sizeof(accepted_descriptor)==263);
    const uint8_t keys[]={4,0,0x16,0,7,0};
    for (unsigned i=0;i<sizeof(keys);++i) {
        uint8_t report[]={0xa1,1,0,0,keys[i],0,0,0,0,0};
        blu2usb_keyboard_snapshot_t snapshot;
        assert(blu2usb_classic_hid_parse(accepted_descriptor,sizeof(accepted_descriptor),
                                        report,sizeof(report),&snapshot));
        assert(snapshot.keys[0]==keys[i] && snapshot.modifiers==0);
        assert(!blu2usb_classic_hid_parse(accepted_descriptor,sizeof(accepted_descriptor),
                                         report,sizeof(report)-1,&snapshot));
    }
    const uint8_t battery[]={0xa1,3,0x98,0x20,0x20,1,0x60,0xb,0x94};
    blu2usb_keyboard_snapshot_t snapshot;
    assert(!blu2usb_classic_hid_parse(accepted_descriptor,sizeof(accepted_descriptor),
                                     battery,sizeof(battery),&snapshot));
    const uint8_t shifted[]={0xa1,1,2,0,4,0,0,0,0,0};
    assert(blu2usb_classic_hid_parse(accepted_descriptor,sizeof(accepted_descriptor),
                                    shifted,sizeof(shifted),&snapshot));
    assert(snapshot.modifiers==2 && snapshot.keys[0]==4);
}

int main(int argc, char **argv)
{
    assert(argc==2);
    test_parser();
    blu2usb_bt_runtime_reset();
    blu2usb_classic_hid_setup();
    if (!strcmp(argv[1],"success")) {
        ready();
        disconnect_event(99); assert(blu2usb_classic_hid_status()==BLU2USB_KEYBOARD_READY);
        const uint8_t keys[]={4,0,0x16,0,7,0};
        for (unsigned i=0;i<sizeof(keys);++i) input(keys[i]);
        for (unsigned i=0;i<sizeof(keys);++i) {
            blu2usb_bt_runtime_message_t message;
            blu2usb_keyboard_snapshot_t snapshot;
            assert(blu2usb_bt_runtime_poll(&message));
            assert(blu2usb_keyboard_transport_decode(&message,&snapshot));
            assert(snapshot.keys[0]==keys[i]);
        }
        hid(HID_SUBEVENT_CONNECTION_CLOSED,1,0);
        assert(blu2usb_classic_hid_status()!=BLU2USB_KEYBOARD_READY);
        tick(1000); assert(connects==2 && bonds==1); // reconnect reuses credentials
    } else if (!strcmp(argv[1],"cancel")) {
        prepare_bond(); bond_complete(0);
        blu2usb_classic_hid_cancel_pair(); tick(20);
        drain_deferred(); assert(connects==0);
        tick(20); assert(blu2usb_classic_hid_status()==BLU2USB_KEYBOARD_IDLE);
    } else if (!strcmp(argv[1],"bond-failure")) {
        prepare_bond(); bond_complete(ERROR_CODE_INSUFFICIENT_SECURITY);
        disconnect_event(11); tick(20);
        assert(connects==0 && !pending && blu2usb_classic_hid_status()==BLU2USB_KEYBOARD_ERROR);
    } else if (!strcmp(argv[1],"timeout")) {
        state_working(); blu2usb_classic_hid_request_pair(); tick(20);
        tick(90000); inquiry_complete(); tick(20);
        assert(blu2usb_classic_hid_status()==BLU2USB_KEYBOARD_ERROR);
        const unsigned previous=scans; tick(90000); assert(scans==previous);
        blu2usb_classic_hid_request_pair(); tick(20); assert(scans==previous+1);
    } else if (!strcmp(argv[1],"overflow")) {
        ready();
        for (unsigned i=0;i<BLU2USB_BT_RUNTIME_QUEUE_CAPACITY;++i)
            assert(blu2usb_bt_runtime_publish(99,1,NULL,0));
        input(4);
        assert(blu2usb_bt_runtime_take_overflow());
        assert(blu2usb_classic_hid_status()==BLU2USB_KEYBOARD_ERROR);
    } else assert(!"unknown scenario");
    puts("PASS: production Classic adapter + accepted descriptor/reports");
}
