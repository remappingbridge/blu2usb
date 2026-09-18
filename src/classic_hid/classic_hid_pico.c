#include "blu2usb/classic_hid/classic_hid.h"
#include "blu2usb/bt_runtime/bt_runtime.h"
#include "btstack.h"
#include <stdatomic.h>
#include <string.h>

// Authoritative source: POC b04aaf1, physically accepted on 2026-09-18.
// See docs/technical/06-g07-keyboard-solution.md before changing this sequence.
#define MAX_DEVICES 20u
#define DESCRIPTOR_BYTES 512u
#define SERVICE_MS 20u
#define SEARCH_MS 90000u
#define PHASE_MS 30000u

typedef enum { IDLE, INQUIRY, NAME, BOND, DEFERRED_HID, HID, READY, STOPPING, FAILED } phase_t;
typedef struct {
    bd_addr_t address;
    uint8_t repetition;
    uint16_t clock;
    bool resolved;
} candidate_t;

static phase_t g_phase;
static candidate_t g_candidates[MAX_DEVICES];
static unsigned g_count, g_name_index;
static bd_addr_t g_target;
static bool g_have_target, g_working, g_inquiry, g_name_pending, g_bond_pending;
static bool g_opened, g_descriptor, g_restart, g_cancelled;
static uint16_t g_cid;
static hci_con_handle_t g_handle = HCI_CON_HANDLE_INVALID;
static uint8_t g_descriptor_storage[DESCRIPTOR_BYTES];
static uint32_t g_deadline, g_search_deadline;
static unsigned g_reconnects;
static bool g_reconnecting;
static bool g_reconnect_pending;
static uint32_t g_reconnect_at;
static btstack_packet_callback_registration_t g_hci_registration;
static btstack_context_callback_registration_t g_deferred_start;
static btstack_timer_source_t g_service_timer;
static atomic_uint g_command = ATOMIC_VAR_INIT(0u);
static atomic_uint g_status = ATOMIC_VAR_INIT(BLU2USB_KEYBOARD_IDLE);

void blu2usb_classic_hid_request_pair(void)
{ atomic_store_explicit(&g_command, 1u, memory_order_release); }
void blu2usb_classic_hid_cancel_pair(void)
{ atomic_store_explicit(&g_command, 2u, memory_order_release); }
blu2usb_keyboard_status_t blu2usb_classic_hid_status(void)
{ return (blu2usb_keyboard_status_t)atomic_load_explicit(&g_status, memory_order_acquire); }

static void status(blu2usb_keyboard_status_t value)
{ atomic_store_explicit(&g_status, value, memory_order_release); }
static uint32_t now(void) { return btstack_run_loop_get_time_ms(); }
static bool elapsed(uint32_t deadline) { return (int32_t)(now() - deadline) >= 0; }
static void release_keys(void)
{ (void)blu2usb_bt_runtime_publish(BLU2USB_KEYBOARD_RUNTIME_CHANNEL,
                                  BLU2USB_KEYBOARD_MESSAGE_RELEASE, NULL, 0u); }
static bool drained(void)
{ return !g_inquiry && !g_name_pending && !g_bond_pending && !g_cid &&
         g_handle == HCI_CON_HANDLE_INVALID; }
static bool matches(const char *name)
{ return strcmp(name, "Bluetooth keyboard 3.0") == 0 || strcmp(name, "BKB-3G") == 0; }

static void stop_attempt(bool cancelled)
{
    g_phase = STOPPING;
    g_cancelled = cancelled;
    g_reconnect_pending = false;
    g_opened = g_descriptor = false;
    status(cancelled ? BLU2USB_KEYBOARD_STOPPING : BLU2USB_KEYBOARD_ERROR);
    release_keys();
    if (g_inquiry) (void)gap_inquiry_stop();
    if (g_cid) hid_host_disconnect(g_cid);
    if (g_handle != HCI_CON_HANDLE_INVALID) (void)gap_disconnect(g_handle);
    // In-flight name/page/bond operations are allowed to return their terminal
    // event before any new attempt. Never reuse their state on a blind retry.
}

static void start_inquiry(void)
{
    g_phase = INQUIRY;
    g_count = 0;
    g_inquiry = true;
    status(BLU2USB_KEYBOARD_SEARCHING);
    if (gap_inquiry_start(5u) != ERROR_CODE_SUCCESS) {
        g_inquiry = false;
        stop_attempt(false);
    }
}

static void begin_pair(void)
{
    g_restart = false;
    g_cancelled = false;
    g_reconnects = 0;
    g_reconnecting = false;
    g_have_target = false;
    g_search_deadline = now() + SEARCH_MS;
    start_inquiry();
}

static void connect_target(void)
{
    g_phase = HID;
    g_opened = g_descriptor = false;
    g_deadline = now() + PHASE_MS;
    status(BLU2USB_KEYBOARD_CONNECTING);
    g_cid = 0;
    const uint8_t result = hid_host_connect(g_target, HID_PROTOCOL_MODE_REPORT, &g_cid);
    if (result != ERROR_CODE_SUCCESS) { stop_attempt(false); return; }
}

static void start_hid_after_bonding(void *context)
{
    UNUSED(context);
    if (g_phase != DEFERRED_HID) return;
    connect_target();
}

static void bond_target(const bd_addr_t address)
{
    memcpy(g_target, address, sizeof(g_target));
    g_have_target = true;
    if (g_inquiry) (void)gap_inquiry_stop();
    g_phase = BOND;
    g_bond_pending = true;
    g_deadline = now() + PHASE_MS;
    status(BLU2USB_KEYBOARD_PAIRING);
    // ACCEPTED invariant: Level 2, no MITM requirement. Do not change to 1.
    if (gap_dedicated_bonding(g_target, 0) != ERROR_CODE_SUCCESS) {
        g_bond_pending = false;
        stop_attempt(false);
    }
}

static void next_name(void)
{
    if (elapsed(g_search_deadline)) { stop_attempt(false); return; }
    for (unsigned i = 0; i < g_count; ++i) {
        if (g_candidates[i].resolved) continue;
        g_candidates[i].resolved = true;
        g_name_index = i;
        g_name_pending = true;
        g_phase = NAME;
        status(BLU2USB_KEYBOARD_READING_NAME);
        if (gap_remote_name_request(g_candidates[i].address, g_candidates[i].repetition,
                                     g_candidates[i].clock | 0x8000u) == ERROR_CODE_SUCCESS)
            return;
        g_name_pending = false;
    }
    start_inquiry();
}

static void found(uint8_t *packet)
{
    if (g_phase != INQUIRY) return;
    bd_addr_t address;
    gap_event_inquiry_result_get_bd_addr(packet, address);
    for (unsigned i = 0; i < g_count; ++i)
        if (bd_addr_cmp(address, g_candidates[i].address) == 0) return;
    if (gap_event_inquiry_result_get_name_available(packet)) {
        char name[249];
        unsigned length = gap_event_inquiry_result_get_name_len(packet);
        if (length >= sizeof(name)) length = sizeof(name) - 1u;
        memcpy(name, gap_event_inquiry_result_get_name(packet), length);
        name[length] = 0;
        if (matches(name)) { bond_target(address); return; }
    }
    if (g_count == MAX_DEVICES) return;
    candidate_t *candidate = &g_candidates[g_count++];
    memcpy(candidate->address, address, sizeof(address));
    candidate->repetition = gap_event_inquiry_result_get_page_scan_repetition_mode(packet);
    candidate->clock = gap_event_inquiry_result_get_clock_offset(packet);
    candidate->resolved = gap_event_inquiry_result_get_name_available(packet) != 0;
}

static void update_ready(void)
{
    if (g_phase != HID || !g_opened || !g_descriptor) return;
    g_phase = READY;
    g_reconnects = 0;
    g_reconnecting = false;
    status(BLU2USB_KEYBOARD_READY);
}

static void hid_event(uint8_t *packet)
{
    const uint8_t event = hci_event_hid_meta_get_subevent_code(packet);
    if (event == HID_SUBEVENT_INCOMING_CONNECTION) {
        bd_addr_t address;
        hid_subevent_incoming_connection_get_address(packet, address);
        const uint16_t cid = hid_subevent_incoming_connection_get_hid_cid(packet);
        if (!g_have_target || bd_addr_cmp(address, g_target) != 0 ||
            g_phase == STOPPING || g_phase == BOND || (g_cid && g_cid != cid)) {
            (void)hid_host_decline_connection(cid);
            return;
        }
        g_cid = cid;
        g_handle = hid_subevent_incoming_connection_get_handle(packet);
        g_reconnect_pending = false;
        g_phase = HID;
        g_opened = g_descriptor = false;
        g_deadline = now() + PHASE_MS;
        status(BLU2USB_KEYBOARD_CONNECTING);
        if (hid_host_accept_connection(cid, HID_PROTOCOL_MODE_REPORT) != ERROR_CODE_SUCCESS)
            stop_attempt(false);
        return;
    }
    // All used HID subevents carry hid_cid at offset 3 in pinned BTstack.
    if (!g_cid || little_endian_read_16(packet, 3) != g_cid) return;
    switch (event) {
    case HID_SUBEVENT_CONNECTION_OPENED:
        if (hid_subevent_connection_opened_get_status(packet) != ERROR_CODE_SUCCESS) {
            g_cid = 0;
            stop_attempt(false);
        } else if (g_phase != HID) {
            hid_host_disconnect(g_cid);
        } else {
            g_opened = true;
            update_ready();
        }
        break;
    case HID_SUBEVENT_DESCRIPTOR_AVAILABLE: {
        if (g_phase != HID) break;
        if (hid_subevent_descriptor_available_get_status(packet) != ERROR_CODE_SUCCESS ||
            !hid_descriptor_storage_get_descriptor_data(g_cid) ||
            !hid_descriptor_storage_get_descriptor_len(g_cid)) {
            stop_attempt(false);
            break;
        }
        g_descriptor = true;
        update_ready();
        break;
    }
    case HID_SUBEVENT_REPORT: {
        if (g_phase != READY) break;
        blu2usb_keyboard_snapshot_t snapshot;
        if (blu2usb_classic_hid_parse(hid_descriptor_storage_get_descriptor_data(g_cid),
            hid_descriptor_storage_get_descriptor_len(g_cid),
            hid_subevent_report_get_report(packet), hid_subevent_report_get_report_len(packet),
            &snapshot)) {
            if (!blu2usb_bt_runtime_publish(BLU2USB_KEYBOARD_RUNTIME_CHANNEL,
                BLU2USB_KEYBOARD_MESSAGE_SNAPSHOT, &snapshot, sizeof(snapshot)))
                stop_attempt(false);
        }
        break;
    }
    case HID_SUBEVENT_CONNECTION_CLOSED: {
        const bool was_ready = g_phase == READY;
        g_cid = 0;
        g_opened = g_descriptor = false;
        release_keys();
        if (was_ready) {
            // Keep the bonded target in RAM; bounded reconnect without rebonding.
            g_phase = FAILED;
            status(BLU2USB_KEYBOARD_CONNECTING);
            g_reconnect_pending = true;
            g_reconnecting = true;
            g_reconnect_at = now() + 1000u;
        }
        break;
    }
    case HID_SUBEVENT_SNIFF_SUBRATING_PARAMS:
        break; // Informational, never an error.
    default: break;
    }
}

static void packet_handler(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    if (type != HCI_EVENT_PACKET || size < 2u) return;
    bd_addr_t address;
    switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) g_working = true;
        break;
    case GAP_EVENT_INQUIRY_RESULT: found(packet); break;
    case GAP_EVENT_INQUIRY_COMPLETE:
        g_inquiry = false;
        if (g_phase == INQUIRY) next_name();
        break;
    case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
        if (!g_name_pending || size < 9u) break;
        reverse_bd_addr(packet + 3, address);
        if (bd_addr_cmp(address, g_candidates[g_name_index].address) != 0) break;
        g_name_pending = false;
        if (g_phase != NAME) break;
        if (packet[2] == ERROR_CODE_SUCCESS) {
            char name[249];
            unsigned length = size - 9u;
            if (length >= sizeof(name)) length = sizeof(name) - 1u;
            memcpy(name, packet + 9, length);
            name[length] = 0;
            if (matches(name)) { bond_target(address); break; }
        }
        next_name();
        break;
    case GAP_EVENT_DEDICATED_BONDING_COMPLETED:
        gap_event_dedicated_bonding_completed_get_address(packet, address);
        if (!g_have_target || bd_addr_cmp(address, g_target) != 0 || !g_bond_pending) break;
        g_bond_pending = false;
        if (g_phase != BOND) break;
        if (gap_event_dedicated_bonding_completed_get_status(packet) != ERROR_CODE_SUCCESS) {
            stop_attempt(false); break;
        }
        // CRITICAL: the old ACL still exists *inside* this event. Never call
        // hid_host_connect here. The queued callback runs after HCI cleanup.
        g_phase = DEFERRED_HID;
        g_deferred_start.callback = start_hid_after_bonding;
        g_deferred_start.context = NULL;
        btstack_run_loop_execute_on_main_thread(&g_deferred_start);
        break;
    case HCI_EVENT_CONNECTION_COMPLETE:
        hci_event_connection_complete_get_bd_addr(packet, address);
        if (!g_have_target || bd_addr_cmp(address, g_target) != 0) break;
        if (hci_event_connection_complete_get_status(packet) != ERROR_CODE_SUCCESS) break;
        g_handle = hci_event_connection_complete_get_connection_handle(packet);
        if (g_phase == STOPPING) (void)gap_disconnect(g_handle);
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        if (g_handle == HCI_CON_HANDLE_INVALID ||
            hci_event_disconnection_complete_get_connection_handle(packet) != g_handle ||
            hci_event_disconnection_complete_get_status(packet) != ERROR_CODE_SUCCESS) break;
        g_handle = HCI_CON_HANDLE_INVALID;
        // HID owns its CID lifetime and emits the profile close/failure event.
        break;
    case HCI_EVENT_PIN_CODE_REQUEST:
        hci_event_pin_code_request_get_bd_addr(packet, address);
        if (g_have_target && bd_addr_cmp(address, g_target) == 0 && g_phase == BOND)
            gap_pin_code_response(address, "0000");
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        hci_event_user_confirmation_request_get_bd_addr(packet, address);
        if (g_have_target && bd_addr_cmp(address, g_target) == 0 &&
            (g_phase == BOND || g_phase == HID)) gap_ssp_confirmation_response(address);
        break;
    case HCI_EVENT_HID_META: hid_event(packet); break;
    default: break;
    }
}

static void service(btstack_timer_source_t *timer)
{
    UNUSED(timer);
    const unsigned command = atomic_exchange_explicit(&g_command, 0u, memory_order_acq_rel);
    if (command == 1u && g_phase != READY) {
        g_restart = true;
        if (!drained()) stop_attempt(true);
    } else if (command == 2u && g_phase != READY) {
        g_restart = false;
        g_reconnecting = false;
        stop_attempt(true);
    }
    if (g_restart && g_working && drained()) begin_pair();
    else if (g_restart && !g_working) status(BLU2USB_KEYBOARD_WAITING);
    else if (g_phase == STOPPING && drained()) {
        g_phase = g_cancelled ? IDLE : FAILED;
        status(g_cancelled ? BLU2USB_KEYBOARD_IDLE : BLU2USB_KEYBOARD_ERROR);
        if (g_reconnecting && !g_cancelled && g_reconnects < 3u) {
            g_reconnect_pending = true;
            g_reconnect_at = now() + (1000u << g_reconnects);
            status(BLU2USB_KEYBOARD_CONNECTING);
        }
    }
    if ((g_phase == INQUIRY || g_phase == NAME) && elapsed(g_search_deadline))
        stop_attempt(false);
    if ((g_phase == BOND || g_phase == HID || g_phase == DEFERRED_HID) && elapsed(g_deadline))
        stop_attempt(false);
    if (g_reconnect_pending && elapsed(g_reconnect_at) && !g_cid && !g_bond_pending) {
        g_reconnect_pending = false;
        if (++g_reconnects <= 3u) connect_target();
        else status(BLU2USB_KEYBOARD_ERROR);
    }
    btstack_run_loop_set_timer(&g_service_timer, SERVICE_MS);
    btstack_run_loop_add_timer(&g_service_timer);
}

void blu2usb_classic_hid_setup(void)
{
    g_phase = IDLE;
    g_handle = HCI_CON_HANDLE_INVALID;
    hid_host_init(g_descriptor_storage, sizeof(g_descriptor_storage));
    hid_host_register_packet_handler(packet_handler);
    gap_set_default_link_policy_settings(
        LM_LINK_POLICY_ENABLE_SNIFF_MODE | LM_LINK_POLICY_ENABLE_ROLE_SWITCH);
    hci_set_master_slave_policy(HCI_ROLE_MASTER);
    hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);
    gap_set_bondable_mode(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    gap_ssp_set_authentication_requirement(
        SSP_IO_AUTHREQ_MITM_PROTECTION_NOT_REQUIRED_GENERAL_BONDING);
    gap_ssp_set_auto_accept(1);
    gap_set_local_name("blu2usb Keyboard Host");
    gap_discoverable_control(1);
    g_hci_registration.callback = packet_handler;
    hci_add_event_handler(&g_hci_registration);
    btstack_run_loop_set_timer_handler(&g_service_timer, service);
    btstack_run_loop_set_timer(&g_service_timer, SERVICE_MS);
    btstack_run_loop_add_timer(&g_service_timer);
}
