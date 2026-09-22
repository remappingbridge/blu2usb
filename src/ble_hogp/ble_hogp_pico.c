#include "blu2usb/ble_hogp/ble_hogp.h"
#include "blu2usb/ble_hogp/session_roles.h"

#include <string.h>
#include "btstack.h"
#include "ble/le_device_db.h"
#include "pico/async_context.h"
#include "pico/cyw43_arch.h"

#define BLE_HOGP_DESCRIPTOR_STORAGE_SIZE 4096u
#define BLE_HOGP_REJECTED_DEVICE_CAPACITY 4u
#define BLE_APPEARANCE_HID_GENERIC 960u
#define BLE_APPEARANCE_HID_MOUSE 962u
#define BLE_APPEARANCE_HID_LAST 1023u
#define BLE_HOGP_VENDOR_SERVICE_MS 20u
#define BLE_HOGP_BONDED_RECONNECT_TIMEOUT_MS 8000u
#define BLE_HOGP_PAIR_NEW_TIMEOUT_MS 15000u

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
static blu2usb_ble_hogp_vendor_backend_t g_vendor_backend;
static bool g_vendor_registered;

typedef enum {
    BLE_HOGP_CANDIDATE_IDLE = 0,
    BLE_HOGP_CANDIDATE_SCANNING,
    BLE_HOGP_CANDIDATE_CONNECTING,
    BLE_HOGP_CANDIDATE_SECURING,
    BLE_HOGP_CANDIDATE_CONNECTING_HIDS,
    BLE_HOGP_CANDIDATE_READY,
    BLE_HOGP_CANDIDATE_DISCONNECTING,
    BLE_HOGP_CANDIDATE_PROMOTED,
} ble_hogp_candidate_state_t;

static ble_hogp_candidate_state_t g_candidate_state;
static bd_addr_t g_candidate_address;
static bd_addr_type_t g_candidate_address_type;
static hci_con_handle_t g_candidate_connection_handle = HCI_CON_HANDLE_INVALID;
static uint16_t g_candidate_hids_cid;
static blu2usb_ble_hogp_parser_t g_candidate_parser;
static btstack_timer_source_t g_candidate_timer;
static bool g_candidate_timer_active;
static bool g_candidate_delete_bond;
static bool g_candidate_resume_scan;
static bool g_candidate_promoted;
static bool g_commit_pending;
static uint32_t g_candidate_generation;
static blu2usb_ble_hogp_session_roles_t g_session_roles;

static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel,
                                     uint8_t *packet, uint16_t size);
static void handle_candidate_gatt_event(uint8_t packet_type, uint16_t channel,
                                        uint8_t *packet, uint16_t size);
static void start_scan(void);
static void reconnect_or_scan(void);
static void candidate_start_scan_locked(void);
static void candidate_clear_transport_locked(void);
static void candidate_timeout_handler(btstack_timer_source_t *timer);

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

static bool publish_provisional(
    blu2usb_ble_hogp_message_type_t type,
    uint32_t generation,
    bd_addr_type_t address_type,
    const bd_addr_t address)
{
    blu2usb_ble_hogp_provisional_event_t event;
    memset(&event, 0, sizeof(event));
    event.generation = generation;
    event.peer.address_type = (uint8_t)address_type;
    if (address != NULL)
        memcpy(event.peer.address, address, sizeof(event.peer.address));
    return blu2usb_bt_runtime_publish(
        BLU2USB_BLE_HOGP_RUNTIME_CHANNEL,
        (uint16_t)type,
        &event,
        (uint16_t)sizeof(event));
}

static bool address_is_bonded(const bd_addr_t address, bd_addr_type_t type)
{
    const int count = le_device_db_count();
    for (int index = 0; index < count; ++index) {
        int stored_type = 0;
        bd_addr_t stored_address;
        sm_key_t irk;
        memset(stored_address, 0, sizeof(stored_address));
        memset(irk, 0, sizeof(irk));
        le_device_db_info(index, &stored_type, stored_address, irk);
        if ((bd_addr_type_t)stored_type == type &&
            memcmp(stored_address, address, sizeof(bd_addr_t)) == 0)
            return true;
    }
    return false;
}

static void stop_candidate_timer(void)
{
    if (!g_candidate_timer_active) return;
    (void)btstack_run_loop_remove_timer(&g_candidate_timer);
    g_candidate_timer_active = false;
}

static void start_candidate_timer(void)
{
    stop_candidate_timer();
    btstack_run_loop_set_timer(
        &g_candidate_timer, BLE_HOGP_PAIR_NEW_TIMEOUT_MS);
    btstack_run_loop_add_timer(&g_candidate_timer);
    g_candidate_timer_active = true;
}

static void candidate_delete_bond_if_requested(void)
{
    if (!g_candidate_delete_bond) return;
    gap_delete_bonding(g_candidate_address_type, g_candidate_address);
    g_candidate_delete_bond = false;
}

static void candidate_clear_transport_locked(void)
{
    candidate_delete_bond_if_requested();
    g_candidate_connection_handle = HCI_CON_HANDLE_INVALID;
    g_candidate_hids_cid = 0u;
    memset(&g_candidate_parser, 0, sizeof(g_candidate_parser));
    memset(g_candidate_address, 0, sizeof(g_candidate_address));
    g_candidate_address_type = BD_ADDR_TYPE_UNKNOWN;
    if (!g_candidate_promoted)
        g_candidate_state = BLE_HOGP_CANDIDATE_IDLE;
}

static void candidate_start_scan_locked(void)
{
    if (!g_session_roles.new_active || g_session_roles.commit_pending)
        return;
    g_candidate_state = BLE_HOGP_CANDIDATE_SCANNING;
    gap_set_scan_parameters(0u, 48u, 48u);
    gap_start_scan();
}

static void candidate_abort_locked(
    blu2usb_ble_hogp_message_type_t event_type,
    bool delete_bond)
{
    uint8_t slot = BLU2USB_BLE_HOGP_SESSION_SLOT_NONE;
    const uint32_t generation = g_candidate_generation;
    const bd_addr_type_t peer_type = g_candidate_address_type;
    bd_addr_t peer_address;
    memcpy(peer_address, g_candidate_address, sizeof(peer_address));

    if (!blu2usb_ble_hogp_session_cancel_new(
            &g_session_roles, generation, &slot))
        return;

    (void)slot;
    stop_candidate_timer();
    g_candidate_delete_bond =
        delete_bond && peer_type != BD_ADDR_TYPE_UNKNOWN;

    if (g_candidate_state == BLE_HOGP_CANDIDATE_SCANNING) {
        gap_stop_scan();
        candidate_clear_transport_locked();
    } else if (g_candidate_state == BLE_HOGP_CANDIDATE_CONNECTING) {
        g_candidate_state = BLE_HOGP_CANDIDATE_DISCONNECTING;
        if (gap_connect_cancel() != ERROR_CODE_SUCCESS)
            candidate_clear_transport_locked();
    } else if (g_candidate_connection_handle != HCI_CON_HANDLE_INVALID) {
        g_candidate_state = BLE_HOGP_CANDIDATE_DISCONNECTING;
        gap_disconnect(g_candidate_connection_handle);
    } else {
        candidate_clear_transport_locked();
    }

    g_candidate_generation = 0u;
    (void)publish_provisional(
        event_type, generation, peer_type, peer_address);
}

static void candidate_timeout_handler(btstack_timer_source_t *timer)
{
    (void)timer;
    g_candidate_timer_active = false;
    candidate_abort_locked(
        BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_TIMEOUT, true);
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

static void reconnect_timeout_handler(btstack_timer_source_t *timer)
{
    (void)timer;
    g_reconnect_timer_active = false;
    if (g_state != BLE_HOGP_STATE_CONNECTING) return;

    g_reconnect_cancel_pending = true;
    if (gap_connect_cancel() != ERROR_CODE_SUCCESS) start_scan();
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
    if (!start_bonded_reconnect()) start_scan();
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

static void connect_candidate_hid_service(void)
{
    g_candidate_state = BLE_HOGP_CANDIDATE_CONNECTING_HIDS;
    g_candidate_hids_cid = 0u;
    const uint8_t status = hids_client_connect(
        g_candidate_connection_handle,
        &handle_candidate_gatt_event,
        HID_PROTOCOL_MODE_REPORT,
        &g_candidate_hids_cid);
    if (status != ERROR_CODE_SUCCESS) {
        candidate_abort_locked(
            BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED, true);
    }
}

static void service_vendor_output(void)
{
    if (!g_vendor_registered) return;

    uint16_t hids_cid = 0u;
    if (g_candidate_promoted &&
        g_candidate_state == BLE_HOGP_CANDIDATE_PROMOTED &&
        blu2usb_ble_hogp_session_can_forward(&g_session_roles, 1u)) {
        hids_cid = g_candidate_hids_cid;
    } else if (!g_candidate_promoted &&
               g_state == BLE_HOGP_STATE_READY &&
               blu2usb_ble_hogp_session_can_forward(&g_session_roles, 0u)) {
        hids_cid = g_hids_cid;
    }

    if (hids_cid == 0u) return;

    uint8_t report_id = 0u;
    uint8_t payload[BLU2USB_BLE_HOGP_VENDOR_OUTPUT_MAX] = {0};
    uint16_t payload_len = 0u;
    if (!g_vendor_backend.next_output(g_vendor_backend.context, &report_id,
        payload, &payload_len, (uint16_t)sizeof(payload))) return;

    const bool valid =
        report_id != 0u && payload_len > 0u && payload_len <= sizeof(payload);
    const uint8_t status = valid ? hids_client_send_write_report(
        hids_cid, report_id, HID_REPORT_TYPE_OUTPUT,
        payload, (uint8_t)payload_len)
        : ERROR_CODE_PARAMETER_OUT_OF_MANDATORY_RANGE;

    g_vendor_backend.output_result(
        g_vendor_backend.context, status == ERROR_CODE_SUCCESS);
}

static void vendor_timer_handler(btstack_timer_source_t *timer)
{
    (void)timer;
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
        if (g_session_roles.authoritative_slot ==
                BLU2USB_BLE_HOGP_SESSION_SLOT_NONE) {
            if (!blu2usb_ble_hogp_session_set_authoritative(
                    &g_session_roles, 0u, true)) {
                disconnect_and_rescan();
                return;
            }
        }
        if (g_vendor_registered)
            g_vendor_backend.session(g_vendor_backend.context, true);
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
        if (g_state != BLE_HOGP_STATE_READY ||
            !blu2usb_ble_hogp_session_can_forward(
                &g_session_roles, 0u)) break;
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

static void handle_candidate_gatt_event(
    uint8_t packet_type, uint16_t channel,
    uint8_t *packet, uint16_t size)
{
    (void)packet_type;
    (void)channel;
    (void)size;

    if (hci_event_packet_get_type(packet) != HCI_EVENT_GATTSERVICE_META)
        return;

    switch (hci_event_gattservice_meta_get_subevent_code(packet)) {
    case GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED: {
        const uint8_t status =
            gattservice_subevent_hid_service_connected_get_status(packet);
        if (status != ERROR_CODE_SUCCESS) {
            candidate_abort_locked(
                BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED, true);
            return;
        }

        const uint8_t *descriptor =
            hids_client_descriptor_storage_get_descriptor_data(
                g_candidate_hids_cid, 0u);
        const uint16_t descriptor_len =
            hids_client_descriptor_storage_get_descriptor_len(
                g_candidate_hids_cid, 0u);
        const blu2usb_hid_source_t source =
            blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);

        const bool parser_ready =
            descriptor != NULL && descriptor_len > 0u &&
            blu2usb_ble_hogp_parser_configure(
                &g_candidate_parser, source,
                descriptor, descriptor_len) &&
            blu2usb_ble_hogp_parser_has_mouse(&g_candidate_parser);

        if (!parser_ready) {
            if (descriptor != NULL && descriptor_len > 0u)
                reject_address(
                    g_candidate_address, g_candidate_address_type);
            candidate_abort_locked(
                BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED, true);
            return;
        }

        if (!blu2usb_ble_hogp_session_candidate_ready(
                &g_session_roles, 1u, g_candidate_generation)) {
            candidate_abort_locked(
                BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED, true);
            return;
        }

        g_candidate_state = BLE_HOGP_CANDIDATE_READY;
        stop_candidate_timer();

        (void)publish_provisional(
            BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_READY,
            g_candidate_generation,
            g_candidate_address_type,
            g_candidate_address);
        break;
    }

    case GATTSERVICE_SUBEVENT_HID_SERVICE_DISCONNECTED:
        if (g_candidate_state == BLE_HOGP_CANDIDATE_PROMOTED) {
            if (g_candidate_connection_handle != HCI_CON_HANDLE_INVALID)
                gap_disconnect(g_candidate_connection_handle);
        } else if (g_candidate_state != BLE_HOGP_CANDIDATE_DISCONNECTING) {
            candidate_abort_locked(
                BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED, true);
        }
        break;

    case GATTSERVICE_SUBEVENT_HID_REPORT: {
        if (g_candidate_state != BLE_HOGP_CANDIDATE_PROMOTED ||
            !blu2usb_ble_hogp_session_can_forward(
                &g_session_roles, 1u))
            break;

        const uint8_t report_id =
            gattservice_subevent_hid_report_get_report_id(packet);
        const uint8_t *raw =
            gattservice_subevent_hid_report_get_report(packet);
        const uint16_t raw_len =
            gattservice_subevent_hid_report_get_report_len(packet);

        const uint8_t *payload = NULL;
        size_t payload_len = 0u;
        if (!blu2usb_ble_hogp_parser_normalize_report(
                &g_candidate_parser, report_id,
                raw, raw_len, &payload, &payload_len)) {
            gap_disconnect(g_candidate_connection_handle);
            break;
        }

        const blu2usb_hid_source_t source =
            blu2usb_hid_source_make(BLU2USB_HID_SOURCE_MOUSE, 1u);
        bool consumed = false;

        if (g_vendor_registered)
            consumed = g_vendor_backend.input(
                g_vendor_backend.context,
                source,
                report_id,
                payload,
                payload_len,
                publish_runtime_mouse_event,
                NULL);

        if (!consumed &&
            !blu2usb_ble_hogp_parser_parse_report(
                &g_candidate_parser,
                report_id,
                payload,
                payload_len,
                publish_mouse_event,
                NULL)) {
            gap_disconnect(g_candidate_connection_handle);
            break;
        }

        service_vendor_output();
        break;
    }

    default:
        break;
    }
}

static void hci_packet_handler(uint8_t packet_type, uint16_t channel,
                               uint8_t *packet, uint16_t size)
{
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING &&
            g_state == BLE_HOGP_STATE_WAITING_FOR_STACK &&
            !g_candidate_promoted)
            reconnect_or_scan();
        break;

    case GAP_EVENT_ADVERTISING_REPORT: {
        if (!advertisement_has_hid_service(packet)) break;

        bd_addr_t address;
        gap_event_advertising_report_get_address(packet, address);
        const bd_addr_type_t type =
            gap_event_advertising_report_get_address_type(packet);
        const uint16_t appearance = advertisement_appearance(packet);

        if (address_is_rejected(address, type)) break;
        if (appearance_is_explicit_non_mouse_hid(appearance)) {
            reject_address(address, type);
            break;
        }

        if (g_candidate_state == BLE_HOGP_CANDIDATE_SCANNING) {
            /* NEW accepts only an address that is not already bonded/saved.
             * MUX-06 will replace this direct bond check with registry identity
             * eligibility, including privacy resolution. */
            if (address_is_bonded(address, type)) break;

            gap_stop_scan();
            memcpy(g_candidate_address, address, sizeof(bd_addr_t));
            g_candidate_address_type = type;
            g_candidate_state = BLE_HOGP_CANDIDATE_CONNECTING;

            if (gap_connect(
                    g_candidate_address,
                    g_candidate_address_type) != ERROR_CODE_SUCCESS) {
                candidate_abort_locked(
                    BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED,
                    false);
            }
            break;
        }

        if (g_state != BLE_HOGP_STATE_SCANNING) break;

        gap_stop_scan();
        stop_reconnect_timer();
        memcpy(g_remote_address, address, sizeof(bd_addr_t));
        g_remote_address_type = type;
        g_reconnect_cancel_pending = false;
        g_state = BLE_HOGP_STATE_CONNECTING;

        if (gap_connect(
                g_remote_address,
                g_remote_address_type) != ERROR_CODE_SUCCESS)
            start_scan();
        break;
    }

    case HCI_EVENT_META_GAP:
        if (hci_event_gap_meta_get_subevent_code(packet) !=
            GAP_SUBEVENT_LE_CONNECTION_COMPLETE)
            break;

        if (g_candidate_state == BLE_HOGP_CANDIDATE_DISCONNECTING &&
            g_candidate_connection_handle == HCI_CON_HANDLE_INVALID) {
            /* Completion after gap_connect_cancel(). The operation was already
             * invalidated/published by cancel/timeout. */
            candidate_clear_transport_locked();
            break;
        }

        if (g_candidate_state == BLE_HOGP_CANDIDATE_CONNECTING) {
            const uint8_t status =
                gap_subevent_le_connection_complete_get_status(packet);

            if (status != ERROR_CODE_SUCCESS) {
                candidate_abort_locked(
                    BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED,
                    false);
                break;
            }

            g_candidate_connection_handle =
                gap_subevent_le_connection_complete_get_connection_handle(
                    packet);
            g_candidate_state = BLE_HOGP_CANDIDATE_SECURING;
            sm_request_pairing(g_candidate_connection_handle);
            break;
        }

        if (g_state == BLE_HOGP_STATE_CONNECTING) {
            stop_reconnect_timer();
            const uint8_t status =
                gap_subevent_le_connection_complete_get_status(packet);

            if (status != ERROR_CODE_SUCCESS) {
                g_connection_handle = HCI_CON_HANDLE_INVALID;
                g_reconnect_cancel_pending = false;
                start_scan();
                break;
            }

            g_reconnect_cancel_pending = false;
            g_connection_handle =
                gap_subevent_le_connection_complete_get_connection_handle(
                    packet);
            g_state = BLE_HOGP_STATE_SECURING;
            sm_request_pairing(g_connection_handle);
        }
        break;

    case HCI_EVENT_DISCONNECTION_COMPLETE: {
        const hci_con_handle_t handle =
            hci_event_disconnection_complete_get_connection_handle(packet);

        if (handle == g_candidate_connection_handle) {
            if (g_candidate_promoted &&
                g_candidate_state == BLE_HOGP_CANDIDATE_PROMOTED) {
                if (g_vendor_registered)
                    g_vendor_backend.session(
                        g_vendor_backend.context, false);

                (void)blu2usb_ble_hogp_session_disconnected(
                    &g_session_roles, 1u);
                g_candidate_promoted = false;
                g_candidate_state = BLE_HOGP_CANDIDATE_IDLE;
                candidate_clear_transport_locked();
                (void)publish_status(
                    BLU2USB_BLE_HOGP_MESSAGE_DISCONNECTED);
                reconnect_or_scan();
                break;
            }

            if (g_candidate_state ==
                BLE_HOGP_CANDIDATE_DISCONNECTING) {
                const bool resume =
                    g_candidate_resume_scan &&
                    g_session_roles.new_active;
                g_candidate_resume_scan = false;
                candidate_clear_transport_locked();
                if (resume)
                    candidate_start_scan_locked();
                break;
            }

            if (g_session_roles.new_active) {
                const uint32_t generation = g_candidate_generation;
                stop_candidate_timer();
                (void)blu2usb_ble_hogp_session_disconnected(
                    &g_session_roles, 1u);
                g_candidate_delete_bond = true;
                candidate_clear_transport_locked();
                g_candidate_generation = 0u;
                (void)publish_provisional(
                    BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED,
                    generation,
                    g_candidate_address_type,
                    g_candidate_address);
            } else {
                candidate_clear_transport_locked();
            }
            break;
        }

        if (handle != g_connection_handle)
            break; /* stale/late disconnect from a retired session */

        if (g_commit_pending) {
            const uint32_t generation = g_candidate_generation;
            uint8_t promoted = BLU2USB_BLE_HOGP_SESSION_SLOT_NONE;

            stop_reconnect_timer();
            g_connection_handle = HCI_CON_HANDLE_INVALID;
            g_hids_cid = 0u;
            memset(&g_parser, 0, sizeof(g_parser));
            g_reconnect_after_disconnect = false;
            g_state = BLE_HOGP_STATE_WAITING_FOR_STACK;

            if (!blu2usb_ble_hogp_session_retired(
                    &g_session_roles,
                    0u,
                    generation,
                    &promoted) ||
                promoted != 1u) {
                g_commit_pending = false;
                candidate_abort_locked(
                    BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED,
                    true);
                break;
            }

            g_commit_pending = false;
            g_candidate_promoted = true;
            g_candidate_state = BLE_HOGP_CANDIDATE_PROMOTED;

            if (g_vendor_registered)
                g_vendor_backend.session(
                    g_vendor_backend.context, true);

            (void)publish_provisional(
                BLU2USB_BLE_HOGP_MESSAGE_PROMOTED,
                generation,
                g_candidate_address_type,
                g_candidate_address);
            g_candidate_generation = 0u;
            service_vendor_output();
            break;
        }

        const bool was_ready = g_state == BLE_HOGP_STATE_READY;
        const bool reconnect_bonded =
            was_ready || g_reconnect_after_disconnect;

        stop_reconnect_timer();

        if (was_ready && g_vendor_registered)
            g_vendor_backend.session(
                g_vendor_backend.context, false);

        if (was_ready)
            (void)blu2usb_ble_hogp_session_disconnected(
                &g_session_roles, 0u);

        if (g_session_roles.new_active)
            candidate_abort_locked(
                BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED,
                true);

        g_connection_handle = HCI_CON_HANDLE_INVALID;
        g_hids_cid = 0u;
        memset(&g_parser, 0, sizeof(g_parser));

        if (was_ready)
            (void)publish_status(
                BLU2USB_BLE_HOGP_MESSAGE_DISCONNECTED);

        g_reconnect_after_disconnect = false;
        if (reconnect_bonded) reconnect_or_scan();
        else start_scan();
        break;
    }

    default:
        break;
    }
}

static void sm_packet_handler(uint8_t packet_type, uint16_t channel,
                              uint8_t *packet, uint16_t size)
{
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
    case SM_EVENT_JUST_WORKS_REQUEST:
        sm_just_works_confirm(
            sm_event_just_works_request_get_handle(packet));
        break;

    case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
        sm_numeric_comparison_confirm(
            sm_event_passkey_display_number_get_handle(packet));
        break;

    case SM_EVENT_PAIRING_COMPLETE: {
        const hci_con_handle_t handle =
            sm_event_pairing_complete_get_handle(packet);
        const uint8_t status =
            sm_event_pairing_complete_get_status(packet);

        if (handle == g_candidate_connection_handle &&
            g_candidate_state == BLE_HOGP_CANDIDATE_SECURING) {
            if (status == ERROR_CODE_SUCCESS) {
                connect_candidate_hid_service();
            } else {
                candidate_abort_locked(
                    BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED,
                    true);
            }
            break;
        }

        if (handle == g_connection_handle &&
            g_state == BLE_HOGP_STATE_SECURING) {
            if (status == ERROR_CODE_SUCCESS)
                connect_hid_service();
            else
                disconnect_and_rescan();
        }
        break;
    }

    case SM_EVENT_REENCRYPTION_COMPLETE: {
        const hci_con_handle_t handle =
            sm_event_reencryption_complete_get_handle(packet);
        const uint8_t status =
            sm_event_reencryption_complete_get_status(packet);

        if (handle == g_candidate_connection_handle &&
            g_candidate_state == BLE_HOGP_CANDIDATE_SECURING) {
            /* Re-encryption proves this peer is already bonded. NEW must not
             * accept it, and its existing bond must be preserved. */
            candidate_abort_locked(
                BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED,
                false);
            break;
        }

        if (handle == g_connection_handle &&
            g_state == BLE_HOGP_STATE_SECURING) {
            if (status == ERROR_CODE_SUCCESS)
                connect_hid_service();
            else
                disconnect_and_rescan();
        }
        break;
    }

    default:
        break;
    }
}

static void ble_hogp_session_setup(void)
{
    memset(&g_parser, 0, sizeof(g_parser));
    memset(&g_candidate_parser, 0, sizeof(g_candidate_parser));
    memset(g_rejected_devices, 0, sizeof(g_rejected_devices));
    memset(g_remote_address, 0, sizeof(g_remote_address));
    memset(g_candidate_address, 0, sizeof(g_candidate_address));

    g_remote_address_type = BD_ADDR_TYPE_UNKNOWN;
    g_candidate_address_type = BD_ADDR_TYPE_UNKNOWN;
    g_rejected_next = 0u;

    g_state = BLE_HOGP_STATE_WAITING_FOR_STACK;
    g_connection_handle = HCI_CON_HANDLE_INVALID;
    g_hids_cid = 0u;

    g_candidate_state = BLE_HOGP_CANDIDATE_IDLE;
    g_candidate_connection_handle = HCI_CON_HANDLE_INVALID;
    g_candidate_hids_cid = 0u;
    g_candidate_timer_active = false;
    g_candidate_delete_bond = false;
    g_candidate_resume_scan = false;
    g_candidate_promoted = false;
    g_commit_pending = false;
    g_candidate_generation = 0u;
    blu2usb_ble_hogp_session_roles_init(&g_session_roles);

    g_reconnect_timer_active = false;
    g_reconnect_cancel_pending = false;
    g_reconnect_after_disconnect = false;

    hids_client_init(g_descriptor_storage, sizeof(g_descriptor_storage));

    g_hci_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&g_hci_registration);
    g_sm_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&g_sm_registration);

    btstack_run_loop_set_timer_handler(
        &g_reconnect_timer, reconnect_timeout_handler);
    btstack_run_loop_set_timer_handler(
        &g_candidate_timer, candidate_timeout_handler);
    btstack_run_loop_set_timer_handler(
        &g_vendor_timer, vendor_timer_handler);
    btstack_run_loop_set_timer(
        &g_vendor_timer, BLE_HOGP_VENDOR_SERVICE_MS);
    btstack_run_loop_add_timer(&g_vendor_timer);
}

bool blu2usb_ble_hogp_start(void)
{
    return blu2usb_bt_runtime_start(ble_hogp_session_setup);
}

static bool pair_new_start_locked(uint32_t *generation_out)
{
    if (generation_out == NULL || g_candidate_promoted ||
        g_candidate_state != BLE_HOGP_CANDIDATE_IDLE)
        return false;

    uint8_t slot = BLU2USB_BLE_HOGP_SESSION_SLOT_NONE;
    uint32_t generation = 0u;

    if (!blu2usb_ble_hogp_session_start_new(
            &g_session_roles, &slot, &generation) ||
        slot != 1u)
        return false;

    memset(&g_candidate_parser, 0, sizeof(g_candidate_parser));
    memset(g_candidate_address, 0, sizeof(g_candidate_address));
    g_candidate_address_type = BD_ADDR_TYPE_UNKNOWN;
    g_candidate_connection_handle = HCI_CON_HANDLE_INVALID;
    g_candidate_hids_cid = 0u;
    g_candidate_delete_bond = false;
    g_candidate_resume_scan = false;
    g_candidate_generation = generation;

    start_candidate_timer();
    candidate_start_scan_locked();
    *generation_out = generation;
    return true;
}

bool blu2usb_ble_hogp_pair_new_start(uint32_t *generation_out)
{
    async_context_t *context = cyw43_arch_async_context();
    if (context == NULL || generation_out == NULL) return false;

    async_context_acquire_lock_blocking(context);
    const bool result = pair_new_start_locked(generation_out);
    async_context_release_lock(context);
    return result;
}

bool blu2usb_ble_hogp_pair_new_cancel(uint32_t generation)
{
    async_context_t *context = cyw43_arch_async_context();
    if (context == NULL || generation == 0u) return false;

    async_context_acquire_lock_blocking(context);
    const bool valid =
        g_session_roles.new_active &&
        !g_session_roles.commit_pending &&
        g_candidate_generation == generation;

    if (valid)
        candidate_abort_locked(
            BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_CLEARED, true);

    async_context_release_lock(context);
    return valid;
}

bool blu2usb_ble_hogp_pair_new_commit(uint32_t generation)
{
    async_context_t *context = cyw43_arch_async_context();
    if (context == NULL || generation == 0u) return false;

    async_context_acquire_lock_blocking(context);

    uint8_t retiring = BLU2USB_BLE_HOGP_SESSION_SLOT_NONE;
    const bool valid =
        g_candidate_state == BLE_HOGP_CANDIDATE_READY &&
        g_candidate_generation == generation &&
        g_connection_handle != HCI_CON_HANDLE_INVALID &&
        blu2usb_ble_hogp_session_begin_commit(
            &g_session_roles, generation, &retiring) &&
        retiring == 0u;

    bool committed = valid;
    if (valid) {
        stop_candidate_timer();
        g_commit_pending = true;
        if (g_vendor_registered)
            g_vendor_backend.session(
                g_vendor_backend.context, false);
        g_state = BLE_HOGP_STATE_DISCONNECTING;

        if (gap_disconnect(g_connection_handle) != ERROR_CODE_SUCCESS) {
            g_state = BLE_HOGP_STATE_READY;
            g_commit_pending = false;
            committed = false;
            (void)blu2usb_ble_hogp_session_abort_commit(
                &g_session_roles, generation);
            if (g_vendor_registered)
                g_vendor_backend.session(
                    g_vendor_backend.context, true);
        }
    }

    async_context_release_lock(context);
    return committed;
}
