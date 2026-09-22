#include "blu2usb/ble_hogp/ble_hogp.h"
#include <string.h>
#include "btstack.h"
#include "ble/le_device_db.h"
#include "pico/cyw43_arch.h"
#define BLE_APPEARANCE_HID_GENERIC 960u
#define BLE_APPEARANCE_HID_MOUSE 962u
#define BLE_APPEARANCE_HID_LAST 1023u
#define BLE_HOGP_STATE_READY 5
/* G06 parser and HID++ backend remain authoritative. The second slot is a
 * provisional mouse: it cannot emit USB events before application commit. */
typedef struct {
    hci_con_handle_t handle;
    uint16_t cid;
    int state, bond;
    bool abandon;
    bd_addr_t address;
    bd_addr_type_t type;
    char name[32];
    blu2usb_ble_hogp_parser_t parser;
} session_t;
static session_t sessions[2];
static session_t *live, *candidate;
static bool started, connecting, cancelling, scanning, expired;
static blu2usb_search_t mode;
static uint16_t saved_mask;
static uint32_t deadline;
static struct {bool used;bd_addr_t address;bd_addr_type_t type;} rejected[16];
static unsigned rejected_next;
static bool is_rejected(const bd_addr_t address,bd_addr_type_t type) {
    for(unsigned i=0;i<16;i++)if(rejected[i].used && rejected[i].type==type && !memcmp(rejected[i].address,address,6))return true;
    return false;
}
static void reject(session_t *s) {
    if(!s || is_rejected(s->address,s->type))return;
    rejected[rejected_next].used=true;rejected[rejected_next].type=s->type;memcpy(rejected[rejected_next].address,s->address,6);
    rejected_next=(rejected_next+1)%16;
}
static btstack_packet_callback_registration_t hci_registration,sm_registration;
static btstack_timer_source_t timer;
static uint8_t descriptors[4096];
static blu2usb_ble_hogp_vendor_backend_t g_vendor_backend;
static bool g_vendor_registered;
#define g_state (live ? live->state : 0)
#define g_hids_cid (live ? live->cid : 0)
static void gatt_event(uint8_t,uint16_t,uint8_t *,uint16_t);
static void resume_search(void);
bool blu2usb_ble_hogp_register_vendor_backend(
    const blu2usb_ble_hogp_vendor_backend_t *backend)
{
    if (backend == NULL || g_vendor_registered || backend->input == NULL ||
        backend->next_output == NULL || backend->output_result == NULL ||
        backend->claims_button == NULL || backend->session == NULL) return false;
    g_vendor_backend = *backend;
    g_vendor_registered = true;
    return true;
}

static bool publish_status(blu2usb_ble_hogp_message_type_t type)
{
    return blu2usb_bt_runtime_publish(BLU2USB_BLE_HOGP_RUNTIME_CHANNEL,
                                       (uint16_t)type, NULL, 0u);
}

static bool publish_runtime_mouse_event(void *context,
                                        const blu2usb_canonical_mouse_event_t *event)
{
    (void)context;
    return event != NULL && blu2usb_bt_runtime_publish(
        BLU2USB_BLE_HOGP_RUNTIME_CHANNEL, BLU2USB_BLE_HOGP_MESSAGE_MOUSE,
        event, (uint16_t)sizeof(*event));
}

static bool publish_mouse_event(void *context,
                                const blu2usb_canonical_mouse_event_t *event)
{
    (void)context;
    if (event != NULL && event->type == BLU2USB_MOUSE_EVENT_BUTTON &&
        g_vendor_registered && g_vendor_backend.claims_button(
            g_vendor_backend.context, event->data.button.button)) {
        return true;
    }
    return publish_runtime_mouse_event(NULL, event);
}


static bool advertisement_has_hid_service(const uint8_t *packet)
{
    const uint8_t *data = gap_event_advertising_report_get_data(packet);
    const uint8_t length = gap_event_advertising_report_get_data_length(packet);
    return ad_data_contains_uuid16(length, data,
        ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE);
}

static uint16_t advertisement_appearance(const uint8_t *packet)
{
    const uint8_t *data = gap_event_advertising_report_get_data(packet);
    const uint8_t length = gap_event_advertising_report_get_data_length(packet);
    ad_context_t context;
    for (ad_iterator_init(&context, length, (uint8_t *)data);
         ad_iterator_has_more(&context); ad_iterator_next(&context)) {
        if (ad_iterator_get_data_type(&context) == BLUETOOTH_DATA_TYPE_APPEARANCE &&
            ad_iterator_get_data_len(&context) >= 2u)
            return little_endian_read_16(ad_iterator_get_data(&context), 0u);
    }
    return 0u;
}

static bool appearance_is_explicit_non_mouse_hid(uint16_t appearance)
{
    return appearance >= BLE_APPEARANCE_HID_GENERIC &&
        appearance <= BLE_APPEARANCE_HID_LAST &&
        appearance != BLE_APPEARANCE_HID_GENERIC &&
        appearance != BLE_APPEARANCE_HID_MOUSE;
}


static void service_vendor_output(void)
{
    if (!g_vendor_registered || g_state != BLE_HOGP_STATE_READY || g_hids_cid == 0u) return;
    uint8_t report_id = 0u;
    uint8_t payload[BLU2USB_BLE_HOGP_VENDOR_OUTPUT_MAX] = {0};
    uint16_t payload_len = 0u;
    if (!g_vendor_backend.next_output(g_vendor_backend.context, &report_id,
        payload, &payload_len, (uint16_t)sizeof(payload))) return;
    const bool valid = report_id != 0u && payload_len > 0u && payload_len <= sizeof(payload);
    const uint8_t status = valid ? hids_client_send_write_report(
        g_hids_cid, report_id, HID_REPORT_TYPE_OUTPUT, payload, (uint8_t)payload_len)
        : ERROR_CODE_PARAMETER_OUT_OF_MANDATORY_RANGE;
    g_vendor_backend.output_result(g_vendor_backend.context,
                                   status == ERROR_CODE_SUCCESS);
}

static session_t *by_handle(hci_con_handle_t h) {
    for(unsigned i=0;i<2;i++)if(sessions[i].handle==h)return &sessions[i];
    return NULL;
}
static session_t *by_cid(uint16_t c) {
    for(unsigned i=0;i<2;i++)if(c && sessions[i].cid==c)return &sessions[i];
    return NULL;
}
static void clear_session(session_t *s) { memset(s,0,sizeof(*s));s->handle=HCI_CON_HANDLE_INVALID;s->bond=-1; }
static bool saved(int bond) {return bond>=0 && bond<16 && (saved_mask&(1u<<bond));}
static void stop_search(void) {
    gap_stop_scan();scanning=false;
    if(connecting) {cancelling=true;(void)gap_connect_cancel();}
    if(candidate) {
        candidate->abandon=true;
        if(candidate->handle!=HCI_CON_HANDLE_INVALID) gap_disconnect(candidate->handle);
        else if(!connecting) {clear_session(candidate);candidate=NULL;}
    }
}
static void drop(session_t *s) {
    if(!s)return;
    if(s==candidate && !s->abandon)reject(s);
    s->abandon=true;
    if(s==live) {
        if(g_vendor_registered)g_vendor_backend.session(g_vendor_backend.context,false);
        live=NULL;publish_status(BLU2USB_BLE_HOGP_MESSAGE_DISCONNECTED);
    }
    if(s->handle!=HCI_CON_HANDLE_INVALID)gap_disconnect(s->handle);
}
static void scan(void) {
    if(!started || mode==BLU2USB_SEARCH_NONE || connecting || candidate)return;
    gap_set_scan_parameters(1,48,48);gap_start_scan();scanning=true;
}
static void resume_search(void) {
    if(!started || mode==BLU2USB_SEARCH_NONE || connecting || candidate)return;
    if(mode==BLU2USB_SEARCH_SAVED && !live) {
        gap_whitelist_clear();gap_load_resolving_list_from_le_device_db();unsigned n=0;
        for(int i=0;i<16;i++)if(saved(i)) {int type;bd_addr_t addr;sm_key_t irk;le_device_db_info(i,&type,addr,irk);
            if(type!=BD_ADDR_TYPE_UNKNOWN && gap_whitelist_add((bd_addr_type_t)type,addr)==ERROR_CODE_SUCCESS)n++; }
        if(n)for(unsigned i=0;i<2;i++)if(sessions[i].handle==HCI_CON_HANDLE_INVALID && &sessions[i]!=live) {
            candidate=&sessions[i];clear_session(candidate);candidate->state=1;
            if(gap_connect_with_whitelist()==ERROR_CODE_SUCCESS){connecting=true;return;}
            candidate=NULL;break;
        }
        /* Saved search never falls back to accepting unknown mice. */
    }
    scan();
}
static void gatt_event(uint8_t packet_type,uint16_t channel,uint8_t *packet,uint16_t size) {
    (void)packet_type;(void)channel;(void)size;
    if(hci_event_packet_get_type(packet)!=HCI_EVENT_GATTSERVICE_META)return;
    uint8_t sub=hci_event_gattservice_meta_get_subevent_code(packet);
    session_t *s=NULL;
    if(sub==GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED) s=by_cid(gattservice_subevent_hid_service_connected_get_hids_cid(packet));
    else if(sub==GATTSERVICE_SUBEVENT_HID_SERVICE_DISCONNECTED) s=by_cid(gattservice_subevent_hid_service_disconnected_get_hids_cid(packet));
    else if(sub==GATTSERVICE_SUBEVENT_HID_REPORT) s=by_cid(gattservice_subevent_hid_report_get_hids_cid(packet));
    if(!s || s->abandon)return;
    if(sub==GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED) {
        if(gattservice_subevent_hid_service_connected_get_status(packet)!=ERROR_CODE_SUCCESS){drop(s);return;}
        const uint8_t *descriptor=hids_client_descriptor_storage_get_descriptor_data(s->cid,0);
        uint16_t len=hids_client_descriptor_storage_get_descriptor_len(s->cid,0);
        blu2usb_hid_source_t source=blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE,1);
        if(!descriptor || !len || !blu2usb_ble_hogp_parser_configure(&s->parser,source,descriptor,len) ||
            !blu2usb_ble_hogp_parser_has_mouse(&s->parser) || s->parser.contains_keyboard ||
            gattservice_subevent_hid_service_connected_get_num_instances(packet)!=1) {drop(s);return;}
        s->bond=sm_le_device_index(s->handle);
        if(s->bond<0 || s->bond>=16 || mode==BLU2USB_SEARCH_NONE ||
            (mode==BLU2USB_SEARCH_NEW && saved(s->bond)) ||
            (mode==BLU2USB_SEARCH_SAVED && !saved(s->bond))) {drop(s);return;}
        s->state=BLE_HOGP_STATE_READY;
    } else if(sub==GATTSERVICE_SUBEVENT_HID_SERVICE_DISCONNECTED) drop(s);
    else if(s==live && s->state==BLE_HOGP_STATE_READY) {
        const uint8_t report_id=gattservice_subevent_hid_report_get_report_id(packet);
        const uint8_t *raw=gattservice_subevent_hid_report_get_report(packet);
        uint16_t raw_len=gattservice_subevent_hid_report_get_report_len(packet);
        const uint8_t *payload=NULL;size_t payload_len=0;
        if(!blu2usb_ble_hogp_parser_normalize_report(&s->parser,report_id,raw,raw_len,&payload,&payload_len)){drop(s);return;}
        blu2usb_hid_source_t source=blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE,1);
        bool consumed=g_vendor_registered && g_vendor_backend.input(g_vendor_backend.context,source,report_id,payload,payload_len,publish_runtime_mouse_event,NULL);
        if(!consumed && !blu2usb_ble_hogp_parser_parse_report(&s->parser,report_id,payload,payload_len,publish_mouse_event,NULL))drop(s);
        service_vendor_output();
    }
}
static void name_from_ad(session_t *s,const uint8_t *packet) {
    ad_context_t ctx;
    for(ad_iterator_init(&ctx,gap_event_advertising_report_get_data_length(packet),(uint8_t *)gap_event_advertising_report_get_data(packet));ad_iterator_has_more(&ctx);ad_iterator_next(&ctx)) {
        uint8_t type=ad_iterator_get_data_type(&ctx);
        if(type==BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME || type==BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME) {
            unsigned len=ad_iterator_get_data_len(&ctx);if(len>31)len=31;memcpy(s->name,ad_iterator_get_data(&ctx),len);s->name[len]=0;
        }
    }
}
static void hci_event(uint8_t type,uint16_t channel,uint8_t *packet,uint16_t size) {
    (void)channel;(void)size;if(type!=HCI_EVENT_PACKET)return;
    switch(hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
        if(btstack_event_state_get_state(packet)==HCI_STATE_WORKING){started=true;resume_search();}break;
    case GAP_EVENT_ADVERTISING_REPORT: {
        if(!scanning || mode==BLU2USB_SEARCH_NONE || !advertisement_has_hid_service(packet) || appearance_is_explicit_non_mouse_hid(advertisement_appearance(packet)))break;
        bd_addr_t addr;gap_event_advertising_report_get_address(packet,addr);
        bd_addr_type_t at=gap_event_advertising_report_get_address_type(packet);
        if(is_rejected(addr,at))break;
        if(live && live->type==at && !memcmp(live->address,addr,6))break;
        for(unsigned i=0;i<2;i++)if(sessions[i].handle==HCI_CON_HANDLE_INVALID && &sessions[i]!=live){candidate=&sessions[i];break;}
        if(!candidate)break;
        clear_session(candidate);candidate->type=at;memcpy(candidate->address,addr,6);name_from_ad(candidate,packet);
        gap_stop_scan();scanning=false;candidate->state=1;
        if(gap_connect(addr,at)==ERROR_CODE_SUCCESS)connecting=true;else {candidate=NULL;resume_search();}
        break;
    }
    case HCI_EVENT_META_GAP:
        if(hci_event_gap_meta_get_subevent_code(packet)==GAP_SUBEVENT_LE_CONNECTION_COMPLETE && connecting) {
            connecting=false;
            if(gap_subevent_le_connection_complete_get_status(packet)!=ERROR_CODE_SUCCESS) {
                if(candidate)clear_session(candidate);
                candidate=NULL;cancelling=false;resume_search();break;
            }
            if(!candidate)break;
            candidate->handle=gap_subevent_le_connection_complete_get_connection_handle(packet);
            candidate->type=gap_subevent_le_connection_complete_get_peer_address_type(packet);
            gap_subevent_le_connection_complete_get_peer_address(packet,candidate->address);
            if(cancelling || candidate->abandon || mode==BLU2USB_SEARCH_NONE){cancelling=false;drop(candidate);break;}
            candidate->state=2;sm_request_pairing(candidate->handle);
        }break;
    case HCI_EVENT_DISCONNECTION_COMPLETE: {
        session_t *s=by_handle(hci_event_disconnection_complete_get_connection_handle(packet));if(!s)break;
        bool was_live=s==live;
        if(was_live){if(g_vendor_registered)g_vendor_backend.session(g_vendor_backend.context,false);live=NULL;publish_status(BLU2USB_BLE_HOGP_MESSAGE_DISCONNECTED);}
        if(s==candidate){
            int bond=sm_le_device_index(s->handle);if(bond<0)bond=s->bond;
            if(bond>=0 && !saved(bond))le_device_db_remove(bond);
            candidate=NULL;
        }
        clear_session(s);resume_search();break;
    }
    default:break;
    }
}
static void connect_hids(session_t *s) {
    s->state=4;
    if(hids_client_connect(s->handle,gatt_event,HID_PROTOCOL_MODE_REPORT,&s->cid)!=ERROR_CODE_SUCCESS)drop(s);
}
static void name_event(uint8_t type,uint16_t channel,uint8_t *packet,uint16_t size) {
    (void)channel;(void)size;if(type!=HCI_EVENT_PACKET)return;
    if(hci_event_packet_get_type(packet)==GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT) {
        session_t *s=by_handle(gatt_event_characteristic_value_query_result_get_handle(packet));
        if(!s || s->abandon || s->state!=3)return;
        unsigned n=gatt_event_characteristic_value_query_result_get_value_length(packet);if(n>31)n=31;
        if(n){memcpy(s->name,gatt_event_characteristic_value_query_result_get_value(packet),n);s->name[n]=0;}
    } else if(hci_event_packet_get_type(packet)==GATT_EVENT_QUERY_COMPLETE) {
        session_t *s=by_handle(gatt_event_query_complete_get_handle(packet));
        if(s && !s->abandon && s->state==3)connect_hids(s);
    }
}
static void sm_event(uint8_t type,uint16_t channel,uint8_t *packet,uint16_t size) {
    (void)channel;(void)size;if(type!=HCI_EVENT_PACKET)return;
    hci_con_handle_t handle=HCI_CON_HANDLE_INVALID;uint8_t status=1;
    switch(hci_event_packet_get_type(packet)) {
    case SM_EVENT_JUST_WORKS_REQUEST:sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));return;
    case SM_EVENT_NUMERIC_COMPARISON_REQUEST:sm_numeric_comparison_confirm(sm_event_numeric_comparison_request_get_handle(packet));return;
    case SM_EVENT_PAIRING_COMPLETE:handle=sm_event_pairing_complete_get_handle(packet);status=sm_event_pairing_complete_get_status(packet);break;
    case SM_EVENT_REENCRYPTION_COMPLETE:handle=sm_event_reencryption_complete_get_handle(packet);status=sm_event_reencryption_complete_get_status(packet);break;
    default:return;
    }
    session_t *s=by_handle(handle);if(!s || s->abandon || s->state!=2)return;
    s->bond=sm_le_device_index(handle);
    if(status!=ERROR_CODE_SUCCESS || (mode==BLU2USB_SEARCH_NEW && saved(s->bond)) || (mode==BLU2USB_SEARCH_SAVED && !saved(s->bond))) {drop(s);return;}
    s->state=3;
    if(gatt_client_read_value_of_characteristics_by_uuid16(name_event,handle,1,0xffff,0x2a00)!=ERROR_CODE_SUCCESS)connect_hids(s);
}
static void tick(btstack_timer_source_t *t) {
    if(mode!=BLU2USB_SEARCH_NONE && (int32_t)(btstack_run_loop_get_time_ms()-deadline)>=0) {
        blu2usb_search_t previous=mode;mode=BLU2USB_SEARCH_NONE;stop_search();
        if(previous==BLU2USB_SEARCH_FIRST){mode=previous;deadline=btstack_run_loop_get_time_ms()+8000;resume_search();}
        else expired=true;
    }
    service_vendor_output();btstack_run_loop_set_timer(t,20);btstack_run_loop_add_timer(t);
}
static void setup(void) {
    for(unsigned i=0;i<2;i++)clear_session(&sessions[i]);
    hids_client_init(descriptors,sizeof(descriptors));hci_registration.callback=hci_event;hci_add_event_handler(&hci_registration);
    sm_registration.callback=sm_event;sm_add_event_handler(&sm_registration);
    btstack_run_loop_set_timer_handler(&timer,tick);btstack_run_loop_set_timer(&timer,20);btstack_run_loop_add_timer(&timer);
}
bool blu2usb_ble_hogp_start(void){return blu2usb_bt_runtime_start(setup);}
void blu2usb_ble_hogp_search(blu2usb_search_t next,uint16_t mask) {
    async_context_acquire_lock_blocking(cyw43_arch_async_context());
    mode=BLU2USB_SEARCH_NONE;stop_search();saved_mask=mask;mode=next;expired=false;
    memset(rejected,0,sizeof(rejected));rejected_next=0;
    deadline=btstack_run_loop_get_time_ms()+(next==BLU2USB_SEARCH_NEW ? 15000:8000);resume_search();
    async_context_release_lock(cyw43_arch_async_context());
}
void blu2usb_ble_hogp_status(blu2usb_ble_status_t *out) {
    async_context_acquire_lock_blocking(cyw43_arch_async_context());memset(out,0,sizeof(*out));
    out->live_bond=live ? live->bond:-1;out->candidate_bond=candidate ? candidate->bond:-1;
    out->candidate_ready=candidate && !candidate->abandon && candidate->state==BLE_HOGP_STATE_READY && mode!=BLU2USB_SEARCH_NONE;
    if(candidate)memcpy(out->candidate_name,candidate->name,32);
    out->searching=mode!=BLU2USB_SEARCH_NONE;out->expired=expired;expired=false;
    async_context_release_lock(cyw43_arch_async_context());
}
bool blu2usb_ble_hogp_accept(int bond) {
    async_context_acquire_lock_blocking(cyw43_arch_async_context());
    bool ok=candidate && !candidate->abandon && candidate->bond==bond && candidate->state==BLE_HOGP_STATE_READY && mode!=BLU2USB_SEARCH_NONE;
    if(ok) {
        blu2usb_bt_runtime_reset();
        if(live)drop(live);
        live=candidate;candidate=NULL;saved_mask|=(uint16_t)(1u<<bond);mode=BLU2USB_SEARCH_NONE;
        if(g_vendor_registered)g_vendor_backend.session(g_vendor_backend.context,true);
        publish_status(BLU2USB_BLE_HOGP_MESSAGE_CONNECTED);service_vendor_output();
    }
    async_context_release_lock(cyw43_arch_async_context());return ok;
}
void blu2usb_ble_hogp_forget(int bond) {
    if(bond<0 || bond>=16)return;
    async_context_acquire_lock_blocking(cyw43_arch_async_context());
    if(live && live->bond==bond)drop(live);
    if(candidate && candidate->bond==bond)drop(candidate);
    saved_mask&=(uint16_t)~(1u<<bond);le_device_db_remove(bond);
    async_context_release_lock(cyw43_arch_async_context());
}
uint16_t blu2usb_ble_hogp_bond_mask(void) {
    uint16_t mask=0;async_context_acquire_lock_blocking(cyw43_arch_async_context());
    for(int i=0;i<16;i++){int type;bd_addr_t address;sm_key_t irk;le_device_db_info(i,&type,address,irk);if(type!=BD_ADDR_TYPE_UNKNOWN)mask|=(uint16_t)(1u<<i);}
    async_context_release_lock(cyw43_arch_async_context());return mask;
}

void blu2usb_ble_hogp_disconnect(int bond) {
    async_context_acquire_lock_blocking(cyw43_arch_async_context());
    if(live && live->bond==bond)drop(live);
    async_context_release_lock(cyw43_arch_async_context());
}
