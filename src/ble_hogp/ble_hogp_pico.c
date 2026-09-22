#include "blu2usb/ble_hogp/ble_hogp.h"

#include <string.h>
#include "btstack.h"
#include "ble/le_device_db.h"

#define BLE_HOGP_DESCRIPTOR_STORAGE_SIZE 2048u
#define BLE_HOGP_REJECTED_DEVICE_CAPACITY 4u
#define BLE_APPEARANCE_HID_GENERIC 960u
#define BLE_APPEARANCE_HID_MOUSE 962u
#define BLE_APPEARANCE_HID_LAST 1023u
#define BLE_HOGP_VENDOR_SERVICE_MS 20u
#define BLE_HOGP_BONDED_RECONNECT_TIMEOUT_MS 8000u

_Static_assert(sizeof(blu2usb_canonical_mouse_event_t) <= BLU2USB_BT_RUNTIME_MESSAGE_PAYLOAD_SIZE,
               "canonical mouse event must fit runtime message");

typedef enum {
    BLE_HOGP_STATE_WAITING_FOR_STACK = 0,
    BLE_HOGP_STATE_SCANNING,
    BLE_HOGP_STATE_CONNECTING,
    BLE_HOGP_STATE_SECURING,
    BLE_HOGP_STATE_CONNECTING_HIDS,
    BLE_HOGP_STATE_READY,
    BLE_HOGP_STATE_DISCONNECTING,
    BLE_HOGP_STATE_IDLE,
} ble_hogp_state_t;

typedef struct {
    bool used;
    bd_addr_type_t address_type;
    bd_addr_t address;
} ble_hogp_rejected_device_t;

static ble_hogp_state_t g_state;
static bd_addr_t g_remote_address;
static bd_addr_type_t g_remote_address_type;
static hci_con_handle_t g_connection_handle = HCI_CON_HANDLE_INVALID;
static uint16_t g_hids_cid;
static uint8_t g_descriptor_storage[BLE_HOGP_DESCRIPTOR_STORAGE_SIZE];
static blu2usb_ble_hogp_parser_t g_parser;
static ble_hogp_rejected_device_t g_rejected_devices[BLE_HOGP_REJECTED_DEVICE_CAPACITY];
static size_t g_rejected_next;
static btstack_packet_callback_registration_t g_hci_registration;
static btstack_packet_callback_registration_t g_sm_registration;
static btstack_timer_source_t g_vendor_timer;
static btstack_timer_source_t g_reconnect_timer;
static bool g_reconnect_timer_active;
static bool g_reconnect_cancel_pending;
static bool g_reconnect_after_disconnect;
static volatile bool g_saved_search_mode;
static bool g_saved_timeout_pending;
static bool g_idle_after_disconnect;
static volatile bool g_request_retry_saved_search;
static volatile bool g_request_cancel_saved_search;
static volatile bool g_request_open_scan;
static blu2usb_ble_hogp_vendor_backend_t g_vendor_backend;
static bool g_vendor_registered;

static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel,
                                     uint8_t *packet, uint16_t size);
static void start_scan(void);
static void reconnect_or_scan(void);

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

static bool address_is_rejected(const bd_addr_t address, bd_addr_type_t type)
{
    for (size_t i = 0u; i < BLE_HOGP_REJECTED_DEVICE_CAPACITY; ++i)
        if (g_rejected_devices[i].used && g_rejected_devices[i].address_type == type &&
            memcmp(g_rejected_devices[i].address, address, sizeof(bd_addr_t)) == 0) return true;
    return false;
}

static void reject_address(const bd_addr_t address, bd_addr_type_t type)
{
    if (address_is_rejected(address, type)) return;
    ble_hogp_rejected_device_t *slot = &g_rejected_devices[g_rejected_next];
    slot->used = true;
    slot->address_type = type;
    memcpy(slot->address, address, sizeof(bd_addr_t));
    g_rejected_next = (g_rejected_next + 1u) % BLE_HOGP_REJECTED_DEVICE_CAPACITY;
}

static void stop_reconnect_timer(void)
{
    if (!g_reconnect_timer_active) return;
    (void)btstack_run_loop_remove_timer(&g_reconnect_timer);
    g_reconnect_timer_active = false;
}

static void start_scan(void)
{
    stop_reconnect_timer();
    g_reconnect_cancel_pending = false;
    g_state = BLE_HOGP_STATE_SCANNING;
    gap_set_scan_parameters(0u, 48u, 48u);
    gap_start_scan();
}

static void finish_saved_search(bool timed_out)
{
    stop_reconnect_timer();
    g_reconnect_cancel_pending = false;
    g_saved_timeout_pending = false;
    g_state = BLE_HOGP_STATE_IDLE;
    if (timed_out)
        (void)publish_status(BLU2USB_BLE_HOGP_MESSAGE_SAVED_SEARCH_TIMEOUT);
}

static void reconnect_timeout_handler(btstack_timer_source_t *timer)
{
    (void)timer;
    g_reconnect_timer_active = false;
    if (g_state != BLE_HOGP_STATE_CONNECTING) return;

    g_reconnect_cancel_pending = true;
    g_saved_timeout_pending = g_saved_search_mode;
    if (gap_connect_cancel() != ERROR_CODE_SUCCESS) {
        if (g_saved_search_mode) finish_saved_search(true);
        else start_scan();
    }
}

static bool start_bonded_reconnect(void)
{
    const int count = le_device_db_count();
    if (count <= 0) return false;

    stop_reconnect_timer();
    g_reconnect_cancel_pending = false;
    (void)gap_whitelist_clear();
    (void)gap_load_resolving_list_from_le_device_db();

    unsigned added = 0u;
    for (int index = 0; index < count; ++index) {
        int address_type = 0;
        bd_addr_t address;
        sm_key_t irk;
        memset(address, 0, sizeof(address));
        memset(irk, 0, sizeof(irk));
        le_device_db_info(index, &address_type, address, irk);
        if (gap_whitelist_add((bd_addr_type_t)address_type, address) != ERROR_CODE_SUCCESS)
            continue;
        if (added == 0u) {
            memcpy(g_remote_address, address, sizeof(bd_addr_t));
            g_remote_address_type = (bd_addr_type_t)address_type;
        }
        ++added;
    }

    if (added == 0u || gap_connect_with_whitelist() != ERROR_CODE_SUCCESS) return false;

    g_state = BLE_HOGP_STATE_CONNECTING;
    btstack_run_loop_set_timer(&g_reconnect_timer,
                               BLE_HOGP_BONDED_RECONNECT_TIMEOUT_MS);
    btstack_run_loop_add_timer(&g_reconnect_timer);
    g_reconnect_timer_active = true;
    return true;
}

static void reconnect_or_scan(void)
{
    if (start_bonded_reconnect()) return;
    if (g_saved_search_mode) {
        finish_saved_search(true);
        return;
    }
    start_scan();
}

static void disconnect_current(bool reconnect_bonded)
{
    const bool was_ready = g_state == BLE_HOGP_STATE_READY;
    if (was_ready && g_vendor_registered)
        g_vendor_backend.session(g_vendor_backend.context, false);
    g_reconnect_after_disconnect = reconnect_bonded;
    g_state = BLE_HOGP_STATE_DISCONNECTING;
    if (was_ready) (void)publish_status(BLU2USB_BLE_HOGP_MESSAGE_DISCONNECTED);
    if (g_connection_handle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(g_connection_handle);
    } else {
        g_reconnect_after_disconnect = false;
        if (reconnect_bonded) reconnect_or_scan();
        else start_scan();
    }
}

static void disconnect_and_rescan(void)
{
    disconnect_current(false);
}

static void connect_hid_service(void)
{
    g_state = BLE_HOGP_STATE_CONNECTING_HIDS;
    g_hids_cid = 0u;
    const uint8_t status = hids_client_connect(g_connection_handle,
        &handle_gatt_client_event, HID_PROTOCOL_MODE_REPORT, &g_hids_cid);
    if (status != ERROR_CODE_SUCCESS) disconnect_and_rescan();
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

static void service_saved_search_requests(void)
{
    if (g_request_cancel_saved_search) {
        g_request_cancel_saved_search = false;
        stop_reconnect_timer();
        g_saved_timeout_pending = false;

        if (g_state == BLE_HOGP_STATE_CONNECTING) {
            g_reconnect_cancel_pending = true;
            if (gap_connect_cancel() != ERROR_CODE_SUCCESS)
                finish_saved_search(false);
        } else if (g_state == BLE_HOGP_STATE_SECURING ||
                   g_state == BLE_HOGP_STATE_CONNECTING_HIDS) {
            g_idle_after_disconnect = true;
            g_state = BLE_HOGP_STATE_DISCONNECTING;
            if (g_connection_handle != HCI_CON_HANDLE_INVALID)
                gap_disconnect(g_connection_handle);
            else
                finish_saved_search(false);
        } else if (g_state != BLE_HOGP_STATE_READY) {
            finish_saved_search(false);
        }
    }

    if (g_request_open_scan) {
        if (g_saved_search_mode) {
            g_request_open_scan = false;
        } else if (g_state == BLE_HOGP_STATE_IDLE) {
            g_request_open_scan = false;
            start_scan();
        } else if (g_state == BLE_HOGP_STATE_SCANNING ||
                   g_state == BLE_HOGP_STATE_READY) {
            g_request_open_scan = false;
        }
    }

    if (g_request_retry_saved_search) {
        if (!g_saved_search_mode || g_state == BLE_HOGP_STATE_READY) {
            g_request_retry_saved_search = false;
        } else {
            if (g_state == BLE_HOGP_STATE_SCANNING) {
                gap_stop_scan();
                g_state = BLE_HOGP_STATE_IDLE;
            }

            if (g_state == BLE_HOGP_STATE_IDLE) {
                g_request_retry_saved_search = false;
                if (!start_bonded_reconnect())
                    finish_saved_search(true);
            }
        }
    }
}

static void vendor_timer_handler(btstack_timer_source_t *timer)
{
    (void)timer;
    service_saved_search_requests();
    service_vendor_output();
    btstack_run_loop_set_timer(&g_vendor_timer, BLE_HOGP_VENDOR_SERVICE_MS);
    btstack_run_loop_add_timer(&g_vendor_timer);
}

static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel,
                                     uint8_t *packet, uint16_t size)
{
    (void)packet_type; (void)channel; (void)size;
    if (hci_event_packet_get_type(packet) != HCI_EVENT_GATTSERVICE_META) return;

    switch (hci_event_gattservice_meta_get_subevent_code(packet)) {
    case GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED: {
        const uint8_t status = gattservice_subevent_hid_service_connected_get_status(packet);
        if (status != ERROR_CODE_SUCCESS) { disconnect_and_rescan(); return; }
        const uint8_t *descriptor = hids_client_descriptor_storage_get_descriptor_data(g_hids_cid, 0u);
        const uint16_t descriptor_len = hids_client_descriptor_storage_get_descriptor_len(g_hids_cid, 0u);
        const blu2usb_hid_source_t source = blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);
        const bool parser_ready = descriptor != NULL && descriptor_len > 0u &&
            blu2usb_ble_hogp_parser_configure(&g_parser, source, descriptor, descriptor_len) &&
            blu2usb_ble_hogp_parser_has_mouse(&g_parser);
        if (!parser_ready) {
            if (descriptor != NULL && descriptor_len > 0u)
                reject_address(g_remote_address, g_remote_address_type);
            disconnect_and_rescan();
            return;
        }
        g_state = BLE_HOGP_STATE_READY;
        g_reconnect_after_disconnect = false;
        if (g_vendor_registered) g_vendor_backend.session(g_vendor_backend.context, true);
        if (!publish_status(BLU2USB_BLE_HOGP_MESSAGE_CONNECTED)) {
            disconnect_and_rescan(); return;
        }
        service_vendor_output();
        break;
    }
    case GATTSERVICE_SUBEVENT_HID_SERVICE_DISCONNECTED:
        if (g_state != BLE_HOGP_STATE_DISCONNECTING)
            disconnect_current(g_state == BLE_HOGP_STATE_READY);
        break;
    case GATTSERVICE_SUBEVENT_HID_REPORT: {
        if (g_state != BLE_HOGP_STATE_READY) break;
        const uint8_t report_id = gattservice_subevent_hid_report_get_report_id(packet);
        const uint8_t *raw = gattservice_subevent_hid_report_get_report(packet);
        const uint16_t raw_len = gattservice_subevent_hid_report_get_report_len(packet);
        const uint8_t *payload = NULL;
        size_t payload_len = 0u;
        if (!blu2usb_ble_hogp_parser_normalize_report(&g_parser, report_id, raw,
                                                       raw_len, &payload, &payload_len)) {
            disconnect_and_rescan(); break;
        }
        const blu2usb_hid_source_t source =
            blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);
        bool consumed = false;
        if (g_vendor_registered)
            consumed = g_vendor_backend.input(g_vendor_backend.context, source,
                report_id, payload, payload_len, publish_runtime_mouse_event, NULL);
        if (!consumed && !blu2usb_ble_hogp_parser_parse_report(&g_parser, report_id,
            payload, payload_len, publish_mouse_event, NULL)) {
            disconnect_and_rescan();
        }
        service_vendor_output();
        break;
    }
    default: break;
    }
}

static void hci_packet_handler(uint8_t packet_type, uint16_t channel,
                               uint8_t *packet, uint16_t size)
{
    (void)channel; (void)size;
    if (packet_type != HCI_EVENT_PACKET) return;
    switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING &&
            g_state == BLE_HOGP_STATE_WAITING_FOR_STACK) reconnect_or_scan();
        break;
    case GAP_EVENT_ADVERTISING_REPORT: {
        if (g_state != BLE_HOGP_STATE_SCANNING || !advertisement_has_hid_service(packet)) break;
        bd_addr_t address;
        gap_event_advertising_report_get_address(packet, address);
        const bd_addr_type_t type = gap_event_advertising_report_get_address_type(packet);
        const uint16_t appearance = advertisement_appearance(packet);
        if (address_is_rejected(address, type)) break;
        if (appearance_is_explicit_non_mouse_hid(appearance)) { reject_address(address, type); break; }
        gap_stop_scan();
        stop_reconnect_timer();
        memcpy(g_remote_address, address, sizeof(bd_addr_t));
        g_remote_address_type = type;
        g_reconnect_cancel_pending = false;
        g_state = BLE_HOGP_STATE_CONNECTING;
        if (gap_connect(g_remote_address, g_remote_address_type) != ERROR_CODE_SUCCESS)
            start_scan();
        break;
    }
    case HCI_EVENT_META_GAP:
        if (hci_event_gap_meta_get_subevent_code(packet) == GAP_SUBEVENT_LE_CONNECTION_COMPLETE &&
            g_state == BLE_HOGP_STATE_CONNECTING) {
            stop_reconnect_timer();
            const uint8_t status = gap_subevent_le_connection_complete_get_status(packet);

            if (g_reconnect_cancel_pending && g_saved_search_mode) {
                const bool timed_out = g_saved_timeout_pending;
                g_reconnect_cancel_pending = false;
                g_saved_timeout_pending = false;

                if (status == ERROR_CODE_SUCCESS) {
                    g_connection_handle =
                        gap_subevent_le_connection_complete_get_connection_handle(packet);
                    g_idle_after_disconnect = true;
                    g_state = BLE_HOGP_STATE_DISCONNECTING;
                    gap_disconnect(g_connection_handle);
                    if (timed_out)
                        (void)publish_status(BLU2USB_BLE_HOGP_MESSAGE_SAVED_SEARCH_TIMEOUT);
                } else {
                    g_connection_handle = HCI_CON_HANDLE_INVALID;
                    finish_saved_search(timed_out);
                }
                break;
            }

            if (status != ERROR_CODE_SUCCESS) {
                g_connection_handle = HCI_CON_HANDLE_INVALID;
                g_reconnect_cancel_pending = false;
                if (g_saved_search_mode) finish_saved_search(true);
                else start_scan();
                break;
            }
            g_reconnect_cancel_pending = false;
            g_connection_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
            g_state = BLE_HOGP_STATE_SECURING;
            sm_request_pairing(g_connection_handle);
        }
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE: {
        const bool was_ready = g_state == BLE_HOGP_STATE_READY;
        const bool reconnect_bonded = was_ready || g_reconnect_after_disconnect;
        stop_reconnect_timer();
        if (was_ready && g_vendor_registered) g_vendor_backend.session(g_vendor_backend.context, false);
        g_connection_handle = HCI_CON_HANDLE_INVALID;
        g_hids_cid = 0u;
        memset(&g_parser, 0, sizeof(g_parser));
        if (was_ready) (void)publish_status(BLU2USB_BLE_HOGP_MESSAGE_DISCONNECTED);

        if (g_idle_after_disconnect) {
            g_idle_after_disconnect = false;
            g_reconnect_after_disconnect = false;
            finish_saved_search(false);
            break;
        }

        g_reconnect_after_disconnect = false;
        if (reconnect_bonded) reconnect_or_scan();
        else if (g_saved_search_mode) finish_saved_search(false);
        else start_scan();
        break;
    }
    default: break;
    }
}

static void sm_packet_handler(uint8_t packet_type, uint16_t channel,
                              uint8_t *packet, uint16_t size)
{
    (void)channel; (void)size;
    if (packet_type != HCI_EVENT_PACKET) return;
    bool ready = false;
    switch (hci_event_packet_get_type(packet)) {
    case SM_EVENT_JUST_WORKS_REQUEST:
        sm_just_works_confirm(sm_event_just_works_request_get_handle(packet)); break;
    case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
        sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet)); break;
    case SM_EVENT_PAIRING_COMPLETE:
        if (sm_event_pairing_complete_get_status(packet) == ERROR_CODE_SUCCESS) ready = true;
        else disconnect_and_rescan();
        break;
    case SM_EVENT_REENCRYPTION_COMPLETE:
        if (sm_event_reencryption_complete_get_status(packet) == ERROR_CODE_SUCCESS) ready = true;
        else disconnect_and_rescan();
        break;
    default: break;
    }
    if (ready && g_state == BLE_HOGP_STATE_SECURING) connect_hid_service();
}

static void ble_hogp_session_setup(void)
{
    memset(&g_parser, 0, sizeof(g_parser));
    memset(g_rejected_devices, 0, sizeof(g_rejected_devices));
    memset(g_remote_address, 0, sizeof(g_remote_address));
    g_remote_address_type = BD_ADDR_TYPE_UNKNOWN;
    g_rejected_next = 0u;
    g_state = BLE_HOGP_STATE_WAITING_FOR_STACK;
    g_connection_handle = HCI_CON_HANDLE_INVALID;
    g_hids_cid = 0u;
    g_reconnect_timer_active = false;
    g_reconnect_cancel_pending = false;
    g_reconnect_after_disconnect = false;
    g_saved_search_mode = false;
    g_saved_timeout_pending = false;
    g_idle_after_disconnect = false;
    g_request_retry_saved_search = false;
    g_request_cancel_saved_search = false;
    g_request_open_scan = false;
    hids_client_init(g_descriptor_storage, sizeof(g_descriptor_storage));
    g_hci_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&g_hci_registration);
    g_sm_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&g_sm_registration);
    btstack_run_loop_set_timer_handler(&g_reconnect_timer, reconnect_timeout_handler);
    btstack_run_loop_set_timer_handler(&g_vendor_timer, vendor_timer_handler);
    btstack_run_loop_set_timer(&g_vendor_timer, BLE_HOGP_VENDOR_SERVICE_MS);
    btstack_run_loop_add_timer(&g_vendor_timer);
}

bool blu2usb_ble_hogp_start(void)
{
    return blu2usb_bt_runtime_start(ble_hogp_session_setup);
}

void blu2usb_ble_hogp_set_saved_search_mode(bool enabled)
{
    g_saved_search_mode = enabled;
    if (!enabled)
        g_request_open_scan = true;
}

bool blu2usb_ble_hogp_retry_saved_search(void)
{
    if (!g_saved_search_mode) return false;
    g_request_retry_saved_search = true;
    return true;
}

void blu2usb_ble_hogp_cancel_saved_search(void)
{
    if (!g_saved_search_mode) return;
    g_request_cancel_saved_search = true;
}
