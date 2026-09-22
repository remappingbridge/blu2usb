/* Executes the production G06-derived session manager with deterministic HCI/
 * SM/GATT completions. SDK event accessors are real; only hardware is mocked. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btstack.h"
#include "ble/le_device_db.h"
#include "blu2usb/ble_hogp/ble_hogp.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static btstack_packet_handler_t hci_cb,sm_cb,gatt_cb;
static btstack_timer_source_t *clock_timer;
static uint32_t now;
static unsigned disconnects,cancels,scans;
static bool bonds[16];
static const uint8_t descriptor[]={0x05,1,0x09,2,0xa1,1,0x09,1,0xa1,0,0x05,9,0x19,1,0x29,3,0x15,0,0x25,1,0x95,3,0x75,1,0x81,2,0x95,1,0x75,5,0x81,1,0x05,1,0x09,0x30,0x09,0x31,0x15,0x81,0x25,0x7f,0x75,8,0x95,2,0x81,6,0xc0,0xc0};
void *cyw43_arch_async_context(void){return NULL;}
void async_context_acquire_lock_blocking(void *p){(void)p;}
void async_context_release_lock(void *p){(void)p;}
bool blu2usb_bt_runtime_start(blu2usb_bt_runtime_session_setup_fn setup){setup();return true;}
void hci_add_event_handler(btstack_packet_callback_registration_t *r){hci_cb=r->callback;}
void sm_add_event_handler(btstack_packet_callback_registration_t *r){sm_cb=r->callback;}
void btstack_run_loop_set_timer_handler(btstack_timer_source_t *t,void (*p)(btstack_timer_source_t *)){t->process=p;}
void btstack_run_loop_set_timer(btstack_timer_source_t *t,uint32_t delay){(void)t;(void)delay;}
void btstack_run_loop_add_timer(btstack_timer_source_t *t){clock_timer=t;}
uint32_t btstack_run_loop_get_time_ms(void){return now;}
void gap_set_scan_parameters(uint8_t t,uint16_t i,uint16_t w){(void)t;(void)i;(void)w;}
void gap_start_scan(void){scans++;}void gap_stop_scan(void){}
uint8_t gap_connect(const bd_addr_t a,bd_addr_type_t t){(void)a;(void)t;return 0;}
uint8_t gap_connect_with_whitelist(void){return 0;}
uint8_t gap_connect_cancel(void){cancels++;return 0;}
uint8_t gap_disconnect(hci_con_handle_t h){(void)h;disconnects++;return 0;}
uint8_t gap_whitelist_clear(void){return 0;}
uint8_t gap_whitelist_add(bd_addr_type_t t,const bd_addr_t a){(void)t;(void)a;return 0;}
uint8_t gap_load_resolving_list_from_le_device_db(void){return 0;}
void le_device_db_info(int i,int *t,bd_addr_t a,sm_key_t k){*t=bonds[i]?0:BD_ADDR_TYPE_UNKNOWN;memset(a,i,6);memset(k,0,16);}
void le_device_db_remove(int i){bonds[i]=false;}
int sm_le_device_index(hci_con_handle_t h){return h-1;}
void sm_request_pairing(hci_con_handle_t h){(void)h;}
void sm_just_works_confirm(hci_con_handle_t h){(void)h;}
void sm_numeric_comparison_confirm(hci_con_handle_t h){(void)h;}
void hids_client_init(uint8_t *p,uint16_t n){(void)p;(void)n;}
uint8_t hids_client_connect(hci_con_handle_t h,btstack_packet_handler_t cb,hid_protocol_mode_t m,uint16_t *cid){(void)m;gatt_cb=cb;*cid=h;return 0;}
uint8_t hids_client_send_write_report(uint16_t c,uint8_t r,hid_report_type_t t,const uint8_t *p,uint8_t n){(void)c;(void)r;(void)t;(void)p;(void)n;return 0;}
const uint8_t *hids_client_descriptor_storage_get_descriptor_data(uint16_t c,uint8_t i){(void)c;(void)i;return descriptor;}
uint16_t hids_client_descriptor_storage_get_descriptor_len(uint16_t c,uint8_t i){(void)c;(void)i;return sizeof(descriptor);}
uint8_t gatt_client_read_value_of_characteristics_by_uuid16(btstack_packet_handler_t cb,hci_con_handle_t h,uint16_t a,uint16_t b,uint16_t uuid){(void)cb;(void)h;(void)a;(void)b;(void)uuid;return 1;}
static void event(btstack_packet_handler_t cb,uint8_t *p,unsigned n){cb(HCI_EVENT_PACKET,0,p,(uint16_t)n);}
static void advertise(unsigned address){
    uint8_t p[16]={GAP_EVENT_ADVERTISING_REPORT,14,0,0};memset(p+4,address,6);p[11]=4;p[12]=3;p[13]=3;p[14]=0x12;p[15]=0x18;event(hci_cb,p,sizeof(p));
}
static void connect_event(unsigned handle){
    uint8_t p[34]={HCI_EVENT_META_GAP,32,GAP_SUBEVENT_LE_CONNECTION_COMPLETE,0};p[4]=handle;memset(p+8,handle,6);event(hci_cb,p,sizeof(p));
}
static void qualify(unsigned handle){
    connect_event(handle);bonds[handle-1]=true;
    uint8_t sm[16]={SM_EVENT_PAIRING_COMPLETE,14};sm[2]=handle;event(sm_cb,sm,sizeof(sm));
    uint8_t gatt[8]={HCI_EVENT_GATTSERVICE_META,6,GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED,0,0,0,1,1};gatt[3]=handle;event(gatt_cb,gatt,sizeof(gatt));
}
static void disconnected(unsigned handle){uint8_t p[]={HCI_EVENT_DISCONNECTION_COMPLETE,4,0,(uint8_t)handle,0,0};event(hci_cb,p,sizeof(p));}
static blu2usb_ble_status_t status(void){blu2usb_ble_status_t s;blu2usb_ble_hogp_status(&s);return s;}
int main(void){
    CHECK(blu2usb_ble_hogp_start());uint8_t state[]={BTSTACK_EVENT_STATE,1,HCI_STATE_WORKING};event(hci_cb,state,3);
    blu2usb_ble_hogp_search(BLU2USB_SEARCH_FIRST,0);CHECK(scans);
    advertise(1);qualify(1);CHECK(status().candidate_ready && status().live_bond==-1);
    CHECK(blu2usb_ble_hogp_accept(0));CHECK(status().live_bond==0);
    blu2usb_ble_hogp_search(BLU2USB_SEARCH_NEW,1);advertise(2);qualify(2);
    CHECK(status().live_bond==0 && status().candidate_ready);
    blu2usb_ble_hogp_search(BLU2USB_SEARCH_NONE,1);CHECK(!blu2usb_ble_hogp_accept(1));CHECK(status().live_bond==0);
    disconnected(2);CHECK(!bonds[1]);CHECK(status().live_bond==0);
    blu2usb_ble_hogp_search(BLU2USB_SEARCH_NEW,1);advertise(2);qualify(2);CHECK(blu2usb_ble_hogp_accept(1));CHECK(status().live_bond==1);
    disconnected(1);CHECK(status().live_bond==1); /* late old disconnect cannot clear replacement */
    blu2usb_ble_hogp_search(BLU2USB_SEARCH_NEW,3);now+=15001;clock_timer->process(clock_timer);CHECK(status().expired);CHECK(status().live_bond==1);
    blu2usb_ble_hogp_search(BLU2USB_SEARCH_NEW,3);advertise(3);blu2usb_ble_hogp_search(BLU2USB_SEARCH_NONE,3);CHECK(cancels);
    connect_event(3);CHECK(!status().candidate_ready && status().live_bond==1);disconnected(3);
    blu2usb_ble_hogp_forget(1);CHECK(!bonds[1] && status().live_bond==-1);disconnected(2);CHECK(disconnects>=4);
    blu2usb_ble_hogp_search(BLU2USB_SEARCH_SAVED,1);now+=8001;clock_timer->process(clock_timer);CHECK(status().expired);
    puts("production radio lifecycle: provisional handoff, cancel/late completion, timeout, removal passed");return 0;
}
