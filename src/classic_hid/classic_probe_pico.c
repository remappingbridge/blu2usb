/* Experiment A: reconstructed from PICO-08 classic_keyboard.c at 0d917e5.
 * Keep inquiry -> EIR/remote name -> REPORT HID -> descriptor explicit.
 * No facade, automatic Keyboard reconnect, BLE arbitration or USB output.
 * Differences from that reference are documented in 08/09-g07 documents.
 */
#include "blu2usb/classic_hid/classic_probe.h"
#include <stdatomic.h>
#include <string.h>
#include "btstack.h"

enum { COMMAND_NONE, COMMAND_PAIR, COMMAND_CANCEL };
enum { NAME_UNKNOWN, NAME_REQUESTED, NAME_RESOLVED };
typedef struct {
    bd_addr_t address;
    uint8_t repetition;
    uint16_t offset;
    uint8_t name_state;
} discovered_t;

static atomic_uint g_command = ATOMIC_VAR_INIT(COMMAND_NONE);
static atomic_uint g_status = ATOMIC_VAR_INIT(BLU2USB_PROBE_STARTING);
static atomic_uint g_pin = ATOMIC_VAR_INIT(0);
static bool g_working, g_pairing, g_inquiry_pending, g_inquiry_accepted;
static int g_name_index = -1;
static uint16_t g_cid;
static blu2usb_probe_phase_t g_phase;
static bd_addr_t g_target;
static discovered_t g_devices[20];
static unsigned g_count;
static uint8_t g_descriptor[1024];
static btstack_packet_callback_registration_t g_hci_handler;
static btstack_timer_source_t g_timer;

static void status(blu2usb_probe_phase_t phase, uint8_t error)
{
    g_phase = phase;
    atomic_store_explicit(&g_status, (unsigned)phase | ((unsigned)error << 8) |
                          (g_count << 16), memory_order_release);
}

blu2usb_classic_probe_snapshot_t blu2usb_classic_probe_snapshot(void)
{
    unsigned word = atomic_load_explicit(&g_status, memory_order_acquire);
    unsigned pin = atomic_load_explicit(&g_pin, memory_order_acquire);
    blu2usb_classic_probe_snapshot_t result = {0};
    result.phase = (blu2usb_probe_phase_t)(word & 255u);
    result.error = (uint8_t)(word >> 8);
    result.found = (uint16_t)(word >> 16);
    result.pin = pin & 0xffffffu;
    result.pin_digits = (uint8_t)(pin >> 24);
    return result;
}

void blu2usb_classic_probe_pair(void)
{
    atomic_store_explicit(&g_command, COMMAND_PAIR, memory_order_release);
}

void blu2usb_classic_probe_cancel(void)
{
    atomic_store_explicit(&g_command, COMMAND_CANCEL, memory_order_release);
}

static bool outstanding(void)
{
    return g_inquiry_pending || g_name_index >= 0 || g_cid != 0;
}

static bool target_name_matches(const char *name)
{
    return strcmp(name, "Bluetooth keyboard 3.0") == 0 ||
           strcmp(name, "BKB-3G") == 0;
}

static int find_address(const bd_addr_t address)
{
    for (unsigned i = 0; i < g_count; ++i)
        if (bd_addr_cmp(address, g_devices[i].address) == 0) return (int)i;
    return -1;
}

static void fail(uint8_t error)
{
    g_pairing = false;
    status(BLU2USB_PROBE_ERROR, error);
    if (g_inquiry_accepted) (void)gap_inquiry_stop();
    if (g_cid) hid_host_disconnect(g_cid);
}

static void start_inquiry(void)
{
    if (!g_working || !g_pairing || outstanding()) return;
    g_count = 0;
    memset(g_devices, 0, sizeof(g_devices));
    g_inquiry_pending = true;
    g_inquiry_accepted = false;
    status(BLU2USB_PROBE_WAIT_ACK, 0);
    /* Same duration as PICO-08. API success is submission, NOT controller ACK. */
    int result = gap_inquiry_start(5);
    if (result != ERROR_CODE_SUCCESS) {
        g_inquiry_pending = false;
        fail((uint8_t)result);
    }
}

static void next_name(void)
{
    if (!g_pairing) return;
    for (unsigned i = 0; i < g_count; ++i) {
        if (g_devices[i].name_state != NAME_UNKNOWN) continue;
        g_name_index = (int)i;
        g_devices[i].name_state = NAME_REQUESTED;
        status(BLU2USB_PROBE_READ_NAME, 0);
        int result = gap_remote_name_request(g_devices[i].address,
            g_devices[i].repetition, (uint16_t)(g_devices[i].offset | 0x8000u));
        if (result == ERROR_CODE_SUCCESS) return;
        g_devices[i].name_state = NAME_RESOLVED;
        g_name_index = -1;
        /* Surface rejection; do not conceal it behind automatic retry. */
        fail((uint8_t)result);
        return;
    }
    start_inquiry();
}

static void connect_target(const bd_addr_t address)
{
    memcpy(g_target, address, sizeof(g_target));
    /* Set intent before stop: pinned SDK may emit completion synchronously. */
    status(BLU2USB_PROBE_CONNECTING, 0);
    if (g_inquiry_accepted) (void)gap_inquiry_stop();
    uint8_t result = hid_host_connect(g_target, HID_PROTOCOL_MODE_REPORT, &g_cid);
    if (result != ERROR_CODE_SUCCESS) {
        g_cid = 0;
        fail(result);
    }
}

static void inquiry_result(uint8_t *packet)
{
    if (!g_pairing || (g_phase != BLU2USB_PROBE_SEARCHING &&
                       g_phase != BLU2USB_PROBE_WAIT_ACK)) return;
    bd_addr_t address;
    gap_event_inquiry_result_get_bd_addr(packet, address);
    if (find_address(address) >= 0) return;
    bool named = gap_event_inquiry_result_get_name_available(packet);
    if (named) {
        char name[256];
        uint8_t length = gap_event_inquiry_result_get_name_len(packet);
        memcpy(name, gap_event_inquiry_result_get_name(packet), length);
        name[length] = 0;
        if (target_name_matches(name)) { connect_target(address); return; }
    }
    if (g_count == 20) return;
    discovered_t *device = &g_devices[g_count++];
    memcpy(device->address, address, sizeof(bd_addr_t));
    device->repetition = gap_event_inquiry_result_get_page_scan_repetition_mode(packet);
    device->offset = gap_event_inquiry_result_get_clock_offset(packet);
    device->name_state = named ? NAME_RESOLVED : NAME_UNKNOWN;
    status(g_phase, 0);
}

static bool keyboard_descriptor(void)
{
    const uint8_t *data = hid_descriptor_storage_get_descriptor_data(g_cid);
    uint16_t length = hid_descriptor_storage_get_descriptor_len(g_cid);
    if (data == NULL || length == 0) return false;
    btstack_hid_usage_iterator_t iterator;
    btstack_hid_usage_iterator_init(&iterator, data, length, HID_REPORT_TYPE_INPUT);
    while (btstack_hid_usage_iterator_has_more(&iterator)) {
        btstack_hid_usage_item_t item;
        btstack_hid_usage_iterator_get_item(&iterator, &item);
        if (item.usage_page == 7) return true;
    }
    return false;
}

static void packet_handler(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    if (type != HCI_EVENT_PACKET || size < 2) return;
    bd_addr_t address;
    switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
            g_working = true;
            status(BLU2USB_PROBE_IDLE, 0);
            if (g_pairing) start_inquiry();
        }
        break;
    case HCI_EVENT_COMMAND_STATUS:
        if (hci_event_command_status_get_command_opcode(packet) != HCI_OPCODE_HCI_INQUIRY ||
            !g_inquiry_pending) break;
        if (hci_event_command_status_get_status(packet) != 0) {
            g_inquiry_pending = false;
            g_inquiry_accepted = false;
            if (g_pairing) fail(hci_event_command_status_get_status(packet));
        } else {
            g_inquiry_accepted = true;
            if (g_pairing && g_phase == BLU2USB_PROBE_WAIT_ACK)
                status(BLU2USB_PROBE_SEARCHING, 0);
            else (void)gap_inquiry_stop();
        }
        break;
    case GAP_EVENT_INQUIRY_RESULT: inquiry_result(packet); break;
    case GAP_EVENT_INQUIRY_COMPLETE:
        g_inquiry_pending = g_inquiry_accepted = false;
        if (g_pairing && (g_phase == BLU2USB_PROBE_SEARCHING ||
                          g_phase == BLU2USB_PROBE_WAIT_ACK)) {
            if (packet[2]) fail(packet[2]);
            else next_name();
        }
        break;
    case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE: {
        if (size < 10 || g_name_index < 0) break;
        reverse_bd_addr(&packet[3], address);
        int index = find_address(address);
        if (index != g_name_index) break;
        g_name_index = -1;
        g_devices[index].name_state = NAME_RESOLVED;
        if (!g_pairing) break;
        char name[249];
        unsigned length = size - 9;
        if (length > sizeof(name) - 1) length = sizeof(name) - 1;
        memcpy(name, &packet[9], length);
        name[length] = 0;
        if (packet[2] == 0 && target_name_matches(name)) connect_target(address);
        else next_name();
        break;
    }
    case HCI_EVENT_PIN_CODE_REQUEST:
        hci_event_pin_code_request_get_bd_addr(packet, address);
        if (!g_pairing || bd_addr_cmp(address, g_target) != 0) break;
        atomic_store_explicit(&g_pin, 4u << 24, memory_order_release);
        status(BLU2USB_PROBE_PIN, 0);
        gap_pin_code_response(address, "0000");
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        hci_event_user_confirmation_request_get_bd_addr(packet, address);
        if (g_pairing && bd_addr_cmp(address, g_target) == 0)
            gap_ssp_confirmation_response(address);
        break;
    case HCI_EVENT_USER_PASSKEY_NOTIFICATION:
        reverse_bd_addr(&packet[2], address);
        if (!g_pairing || bd_addr_cmp(address, g_target) != 0) break;
        atomic_store_explicit(&g_pin, (6u << 24) | little_endian_read_32(packet, 8), memory_order_release);
        status(BLU2USB_PROBE_PIN, 0);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        /* Display-only host cannot supply keyboard input: expose, don't hang. */
        reverse_bd_addr(&packet[2], address);
        if (g_pairing && bd_addr_cmp(address, g_target) == 0) fail(0xe1);
        break;
    case HCI_EVENT_HID_META:
        switch (hci_event_hid_meta_get_subevent_code(packet)) {
        case HID_SUBEVENT_INCOMING_CONNECTION:
            /* A requires an explicit Pair. Unknown incoming reconnect is later work. */
            if (!g_pairing || g_cid) {
                (void)hid_host_decline_connection(hid_subevent_incoming_connection_get_hid_cid(packet));
                break;
            }
            hid_subevent_incoming_connection_get_address(packet, g_target);
            g_cid = hid_subevent_incoming_connection_get_hid_cid(packet);
            status(BLU2USB_PROBE_CONNECTING, 0);
            if (g_inquiry_accepted) (void)gap_inquiry_stop();
            hid_host_accept_connection(g_cid, HID_PROTOCOL_MODE_REPORT);
            break;
        case HID_SUBEVENT_CONNECTION_OPENED:
            if (hid_subevent_connection_opened_get_hid_cid(packet) != g_cid) break;
            if (hid_subevent_connection_opened_get_status(packet)) {
                g_cid = 0;
                if (g_pairing) fail(hid_subevent_connection_opened_get_status(packet));
            } else if (!g_pairing) hid_host_disconnect(g_cid);
            else status(BLU2USB_PROBE_SETUP, 0);
            break;
        case HID_SUBEVENT_DESCRIPTOR_AVAILABLE:
            if (hid_subevent_descriptor_available_get_hid_cid(packet) != g_cid || !g_pairing) break;
            if (hid_subevent_descriptor_available_get_status(packet))
                fail(hid_subevent_descriptor_available_get_status(packet));
            else if (!keyboard_descriptor()) fail(0xe2);
            else { g_pairing = false; status(BLU2USB_PROBE_READY, 0); }
            break;
        case HID_SUBEVENT_CONNECTION_CLOSED:
            if (hid_subevent_connection_closed_get_hid_cid(packet) != g_cid) break;
            g_cid = 0;
            g_pairing = false;
            if (g_phase != BLU2USB_PROBE_ERROR) status(BLU2USB_PROBE_IDLE, 0);
            break;
        default: break; /* Deliberately no Keyboard reports in experiment A. */
        }
        break;
    default: break;
    }
    if (!g_pairing && !outstanding() && g_phase == BLU2USB_PROBE_DRAINING)
        status(BLU2USB_PROBE_IDLE, 0);
}

static void command_timer(btstack_timer_source_t *timer)
{
    unsigned command = atomic_exchange_explicit(&g_command, COMMAND_NONE, memory_order_acq_rel);
    if (command == COMMAND_CANCEL && g_phase != BLU2USB_PROBE_READY) {
        g_pairing = false;
        status(outstanding() ? BLU2USB_PROBE_DRAINING : BLU2USB_PROBE_IDLE, 0);
        if (g_inquiry_accepted) (void)gap_inquiry_stop();
        if (g_cid) hid_host_disconnect(g_cid);
    } else if (command == COMMAND_PAIR && g_phase != BLU2USB_PROBE_READY && !g_pairing) {
        if (outstanding()) {
            status(BLU2USB_PROBE_DRAINING, 0);
        } else {
            atomic_store_explicit(&g_pin, 0, memory_order_release);
            g_pairing = true;
            if (g_working) start_inquiry();
            else status(BLU2USB_PROBE_STARTING, 0);
        }
    }
    btstack_run_loop_set_timer(timer, 50);
    btstack_run_loop_add_timer(timer);
}

void blu2usb_classic_probe_setup(void)
{
    g_working = g_pairing = g_inquiry_pending = g_inquiry_accepted = false;
    g_cid = 0;
    g_count = 0;
    g_name_index = -1;
    status(BLU2USB_PROBE_STARTING, 0);
    /* Do not reset the command mailbox: a UI request may predate Core1 setup. */
    hid_host_init(g_descriptor, sizeof(g_descriptor));
    hid_host_register_packet_handler(packet_handler);
    gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_SNIFF_MODE | LM_LINK_POLICY_ENABLE_ROLE_SWITCH);
    hci_set_master_slave_policy(HCI_ROLE_MASTER);
    hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_ONLY);
    gap_set_local_name("BLU2USB 00:00:00:00:00:00");
    gap_discoverable_control(1);
    g_hci_handler.callback = packet_handler;
    hci_add_event_handler(&g_hci_handler);
    btstack_run_loop_set_timer_handler(&g_timer, command_timer);
    btstack_run_loop_set_timer(&g_timer, 50);
    btstack_run_loop_add_timer(&g_timer);
}
