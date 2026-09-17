#include "blu2usb/classic_hid/classic_hid.h"

#include <stdatomic.h>
#include <string.h>

#include "blu2usb/ble_hogp/ble_hogp.h"
#include "btstack.h"

#define CLASSIC_HID_TARGET_NAME "Bluetooth keyboard 3.0"
#define CLASSIC_HID_TARGET_ALIAS "BKB-3G"
#define CLASSIC_HID_INQUIRY_DURATION_1280MS 5u
#define CLASSIC_HID_MAX_DISCOVERED_DEVICES 20u
#define CLASSIC_HID_DESCRIPTOR_STORAGE_SIZE 1024u
#define CLASSIC_HID_COMMAND_SERVICE_MS 50u

typedef enum {
    CLASSIC_HID_NAME_UNKNOWN = 0,
    CLASSIC_HID_NAME_REQUESTED,
    CLASSIC_HID_NAME_RESOLVED,
} classic_hid_name_state_t;

typedef struct {
    bd_addr_t address;
    uint8_t page_scan_repetition_mode;
    uint16_t clock_offset;
    classic_hid_name_state_t name_state;
} classic_hid_discovered_device_t;

typedef enum {
    CLASSIC_HID_STATE_WAITING_FOR_STACK = 0,
    CLASSIC_HID_STATE_IDLE,
    CLASSIC_HID_STATE_INQUIRY,
    CLASSIC_HID_STATE_RESOLVING_NAMES,
    CLASSIC_HID_STATE_CONNECTING,
    CLASSIC_HID_STATE_READY,
} classic_hid_state_t;

static classic_hid_state_t g_state;
static bool g_stack_working;
static bool g_pairing_active;
static bool g_restart_after_close;
static bool g_resume_ble_after_close;
static bool g_descriptor_available;
static bool g_registered;
static uint16_t g_hid_cid;
static uint8_t g_descriptor_storage[CLASSIC_HID_DESCRIPTOR_STORAGE_SIZE];
static bd_addr_t g_target_address;
static classic_hid_discovered_device_t g_devices[CLASSIC_HID_MAX_DISCOVERED_DEVICES];
static unsigned g_device_count;
static blu2usb_classic_hid_keyboard_state_t g_keyboard_state;
static btstack_packet_callback_registration_t g_hci_registration;
static btstack_timer_source_t g_command_timer;
static atomic_bool g_pair_requested = ATOMIC_VAR_INIT(false);
static atomic_bool g_retry_requested = ATOMIC_VAR_INIT(false);
static atomic_bool g_cancel_requested = ATOMIC_VAR_INIT(false);

static bool start_inquiry(void);
static void request_next_remote_name(void);

static bool publish_status(blu2usb_classic_hid_message_type_t type)
{
    return blu2usb_bt_runtime_publish(BLU2USB_CLASSIC_HID_RUNTIME_CHANNEL,
                                       (uint16_t)type, NULL, 0u);
}

static bool publish_keyboard_event(void *context,
                                   const blu2usb_canonical_keyboard_event_t *event)
{
    (void)context;
    return event != NULL && blu2usb_bt_runtime_publish(
        BLU2USB_CLASSIC_HID_RUNTIME_CHANNEL,
        BLU2USB_CLASSIC_HID_MESSAGE_KEYBOARD,
        event, (uint16_t)sizeof(*event));
}

static void publish_pair_code(uint32_t value, uint8_t digits)
{
    blu2usb_classic_hid_pair_code_t code;
    code.value = value;
    code.digits = digits;
    (void)blu2usb_bt_runtime_publish(
        BLU2USB_CLASSIC_HID_RUNTIME_CHANNEL,
        BLU2USB_CLASSIC_HID_MESSAGE_PAIR_CODE,
        &code, (uint16_t)sizeof(code));
}

static bool target_name_matches(const char *name)
{
    return name != NULL &&
        (strcmp(name, CLASSIC_HID_TARGET_NAME) == 0 ||
         strcmp(name, CLASSIC_HID_TARGET_ALIAS) == 0);
}

static int device_index_for_address(const bd_addr_t address)
{
    for (unsigned i = 0u; i < g_device_count; ++i)
        if (bd_addr_cmp(address, g_devices[i].address) == 0) return (int)i;
    return -1;
}

static void release_all_keyboard_state(void)
{
    blu2usb_classic_hid_keyboard_state_t empty;
    blu2usb_classic_hid_keyboard_state_clear(&empty);
    const blu2usb_hid_source_t source =
        blu2usb_hid_source_make(BLU2USB_HID_SOURCE_KEYBOARD, 1u);
    (void)blu2usb_classic_hid_emit_state_diff(
        &g_keyboard_state, &empty, source, publish_keyboard_event, NULL);
    blu2usb_classic_hid_keyboard_state_clear(&g_keyboard_state);
}

static void connect_target(const bd_addr_t address)
{
    memcpy(g_target_address, address, sizeof(bd_addr_t));
    g_state = CLASSIC_HID_STATE_CONNECTING;
    (void)gap_inquiry_stop();

    uint16_t cid = 0u;
    const uint8_t status = hid_host_connect(
        g_target_address,
        HID_PROTOCOL_MODE_REPORT,
        &cid);
    if (status != ERROR_CODE_SUCCESS) {
        g_hid_cid = 0u;
        g_state = CLASSIC_HID_STATE_IDLE;
        return;
    }
    g_hid_cid = cid;
}

static bool start_inquiry(void)
{
    if (!g_stack_working || g_state == CLASSIC_HID_STATE_READY) return false;

    /* The CYW43 cannot reliably accept a BR/EDR inquiry while the BLE HOGP
     * adapter owns an LE scan / outgoing reconnect procedure. Quiesce only
     * that discovery work. A Mouse that is already READY remains connected. */
    if (!blu2usb_ble_hogp_pico_pause_discovery_for_classic()) {
        g_pairing_active = true;
        g_state = CLASSIC_HID_STATE_IDLE;
        return false;
    }

    g_device_count = 0u;
    memset(g_devices, 0, sizeof(g_devices));
    g_descriptor_available = false;
    g_pairing_active = true;
    g_state = CLASSIC_HID_STATE_INQUIRY;
    if (gap_inquiry_start(CLASSIC_HID_INQUIRY_DURATION_1280MS) != ERROR_CODE_SUCCESS) {
        /* Do not consume/freeze the transaction. The command timer retries
         * while pairing remains active, which covers short controller-busy
         * windows after LE scan/connect cancellation. */
        g_state = CLASSIC_HID_STATE_IDLE;
        return false;
    }
    return true;
}

static void request_next_remote_name(void)
{
    g_state = CLASSIC_HID_STATE_RESOLVING_NAMES;
    for (unsigned i = 0u; i < g_device_count; ++i) {
        if (g_devices[i].name_state != CLASSIC_HID_NAME_UNKNOWN) continue;
        g_devices[i].name_state = CLASSIC_HID_NAME_REQUESTED;
        const int status = gap_remote_name_request(
            g_devices[i].address,
            g_devices[i].page_scan_repetition_mode,
            (uint16_t)(g_devices[i].clock_offset | 0x8000u));
        if (status == ERROR_CODE_SUCCESS) return;
        g_devices[i].name_state = CLASSIC_HID_NAME_RESOLVED;
    }
    if (g_pairing_active) {
        g_state = CLASSIC_HID_STATE_IDLE;
        (void)start_inquiry();
    } else {
        g_state = CLASSIC_HID_STATE_IDLE;
        blu2usb_ble_hogp_pico_resume_discovery_after_classic();
    }
}

static void handle_inquiry_result(uint8_t *packet)
{
    if (g_state != CLASSIC_HID_STATE_INQUIRY) return;

    bd_addr_t address;
    gap_event_inquiry_result_get_bd_addr(packet, address);
    if (device_index_for_address(address) >= 0) return;

    if (gap_event_inquiry_result_get_name_available(packet)) {
        char name[249];
        uint8_t length = gap_event_inquiry_result_get_name_len(packet);
        if ((size_t)length >= sizeof(name)) length = (uint8_t)(sizeof(name) - 1u);
        memcpy(name, gap_event_inquiry_result_get_name(packet), length);
        name[length] = '\0';
        if (target_name_matches(name)) {
            connect_target(address);
            return;
        }
    }

    if (g_device_count >= CLASSIC_HID_MAX_DISCOVERED_DEVICES) return;
    classic_hid_discovered_device_t *device = &g_devices[g_device_count++];
    memcpy(device->address, address, sizeof(bd_addr_t));
    device->page_scan_repetition_mode =
        gap_event_inquiry_result_get_page_scan_repetition_mode(packet);
    device->clock_offset = gap_event_inquiry_result_get_clock_offset(packet);
    device->name_state = gap_event_inquiry_result_get_name_available(packet)
        ? CLASSIC_HID_NAME_RESOLVED
        : CLASSIC_HID_NAME_UNKNOWN;
}

static void handle_remote_name_complete(uint8_t *packet)
{
    if (g_state != CLASSIC_HID_STATE_RESOLVING_NAMES) return;
    bd_addr_t address;
    reverse_bd_addr(&packet[3], address);
    const int index = device_index_for_address(address);
    if (index >= 0) {
        g_devices[index].name_state = CLASSIC_HID_NAME_RESOLVED;
        if (packet[2] == ERROR_CODE_SUCCESS) {
            const char *name = (const char *)&packet[9];
            if (target_name_matches(name)) {
                connect_target(address);
                return;
            }
        }
    }
    request_next_remote_name();
}

static bool descriptor_has_keyboard(uint16_t cid)
{
    const uint8_t *descriptor = hid_descriptor_storage_get_descriptor_data(cid);
    const uint16_t descriptor_len = hid_descriptor_storage_get_descriptor_len(cid);
    if (descriptor == NULL || descriptor_len == 0u) return false;

    btstack_hid_usage_iterator_t iterator;
    btstack_hid_usage_iterator_init(
        &iterator, descriptor, descriptor_len, HID_REPORT_TYPE_INPUT);
    while (btstack_hid_usage_iterator_has_more(&iterator)) {
        btstack_hid_usage_item_t item;
        btstack_hid_usage_iterator_get_item(&iterator, &item);
        if (item.usage_page == 0x07u) return true;
    }
    return false;
}

static void handle_hid_report(uint8_t *packet)
{
    if (!g_descriptor_available || g_state != CLASSIC_HID_STATE_READY) return;
    const uint8_t *report = hid_subevent_report_get_report(packet);
    const uint16_t report_len = hid_subevent_report_get_report_len(packet);
    if (report == NULL || report_len < 2u || report[0] != 0xa1u) return;

    blu2usb_classic_hid_keyboard_state_t next;
    blu2usb_classic_hid_keyboard_state_clear(&next);

    btstack_hid_parser_t parser;
    btstack_hid_parser_init(
        &parser,
        hid_descriptor_storage_get_descriptor_data(g_hid_cid),
        hid_descriptor_storage_get_descriptor_len(g_hid_cid),
        HID_REPORT_TYPE_INPUT,
        &report[1],
        (uint16_t)(report_len - 1u));

    while (btstack_hid_parser_has_more(&parser)) {
        uint16_t usage_page = 0u;
        uint16_t usage = 0u;
        int32_t value = 0;
        btstack_hid_parser_get_field(&parser, &usage_page, &usage, &value);
        if (usage_page != 0x07u) continue;
        if (usage >= 0xe0u && usage <= 0xe7u) {
            (void)blu2usb_classic_hid_keyboard_state_set_modifier(
                &next, (blu2usb_modifier_t)(usage - 0xe0u), value != 0);
            continue;
        }
        if (usage < 0x04u || usage > 0xffu || value == 0) continue;
        (void)blu2usb_classic_hid_keyboard_state_set_key(
            &next, (blu2usb_key_t)usage, true);
    }

    const blu2usb_hid_source_t source =
        blu2usb_hid_source_make(BLU2USB_HID_SOURCE_KEYBOARD, 1u);
    (void)blu2usb_classic_hid_emit_state_diff(
        &g_keyboard_state, &next, source, publish_keyboard_event, NULL);
}

static void packet_handler(uint8_t packet_type, uint16_t channel,
                           uint8_t *packet, uint16_t size)
{
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET) return;

    bd_addr_t event_address;
    switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
            g_stack_working = true;
            if (g_state == CLASSIC_HID_STATE_WAITING_FOR_STACK)
                g_state = CLASSIC_HID_STATE_IDLE;
        }
        break;
    case GAP_EVENT_INQUIRY_RESULT:
        handle_inquiry_result(packet);
        break;
    case GAP_EVENT_INQUIRY_COMPLETE:
        if (g_state == CLASSIC_HID_STATE_INQUIRY) request_next_remote_name();
        break;
    case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
        handle_remote_name_complete(packet);
        break;
    case HCI_EVENT_PIN_CODE_REQUEST:
        hci_event_pin_code_request_get_bd_addr(packet, event_address);
        publish_pair_code(0u, 4u);
        gap_pin_code_response(event_address, "0000");
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        hci_event_user_confirmation_request_get_bd_addr(packet, event_address);
        gap_ssp_confirmation_response(event_address);
        break;
    case HCI_EVENT_USER_PASSKEY_NOTIFICATION:
        publish_pair_code(little_endian_read_32(packet, 8u), 6u);
        break;
    case HCI_EVENT_HID_META:
        switch (hci_event_hid_meta_get_subevent_code(packet)) {
        case HID_SUBEVENT_INCOMING_CONNECTION:
            g_hid_cid = hid_subevent_incoming_connection_get_hid_cid(packet);
            g_state = CLASSIC_HID_STATE_CONNECTING;
            (void)gap_inquiry_stop();
            hid_host_accept_connection(
                g_hid_cid, HID_PROTOCOL_MODE_REPORT);
            break;
        case HID_SUBEVENT_CONNECTION_OPENED: {
            const uint8_t status = hid_subevent_connection_opened_get_status(packet);
            if (status != ERROR_CODE_SUCCESS) {
                g_hid_cid = 0u;
                g_descriptor_available = false;
                if (g_pairing_active) g_state = CLASSIC_HID_STATE_IDLE;
                else {
                    g_state = CLASSIC_HID_STATE_IDLE;
                    blu2usb_ble_hogp_pico_resume_discovery_after_classic();
                }
                break;
            }
            g_hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);
            g_descriptor_available = false;
            blu2usb_classic_hid_keyboard_state_clear(&g_keyboard_state);
            g_state = CLASSIC_HID_STATE_CONNECTING;
            break;
        }
        case HID_SUBEVENT_DESCRIPTOR_AVAILABLE: {
            const uint8_t status = hid_subevent_descriptor_available_get_status(packet);
            if (status != ERROR_CODE_SUCCESS || !descriptor_has_keyboard(g_hid_cid)) {
                g_restart_after_close = g_pairing_active;
                hid_host_disconnect(g_hid_cid);
                break;
            }
            g_descriptor_available = true;
            g_pairing_active = false;
            g_restart_after_close = false;
            g_resume_ble_after_close = false;
            g_state = CLASSIC_HID_STATE_READY;
            blu2usb_ble_hogp_pico_resume_discovery_after_classic();
            (void)publish_status(BLU2USB_CLASSIC_HID_MESSAGE_CONNECTED);
            break;
        }
        case HID_SUBEVENT_REPORT:
            handle_hid_report(packet);
            break;
        case HID_SUBEVENT_CONNECTION_CLOSED: {
            const bool was_ready = g_state == CLASSIC_HID_STATE_READY;
            if (was_ready) {
                release_all_keyboard_state();
                (void)publish_status(BLU2USB_CLASSIC_HID_MESSAGE_DISCONNECTED);
            }
            g_hid_cid = 0u;
            g_descriptor_available = false;
            const bool restart = g_restart_after_close;
            const bool resume_ble = g_resume_ble_after_close;
            g_restart_after_close = false;
            g_resume_ble_after_close = false;
            if (restart && g_pairing_active) {
                g_state = CLASSIC_HID_STATE_IDLE;
            } else {
                g_state = CLASSIC_HID_STATE_IDLE;
                if (resume_ble || !g_pairing_active)
                    blu2usb_ble_hogp_pico_resume_discovery_after_classic();
            }
            break;
        }
        default:
            break;
        }
        break;
    default:
        break;
    }
}

static void service_command_requests(void)
{
    if (atomic_exchange_explicit(&g_cancel_requested, false, memory_order_acq_rel)) {
        g_pairing_active = false;
        g_restart_after_close = false;
        if (g_state == CLASSIC_HID_STATE_INQUIRY ||
            g_state == CLASSIC_HID_STATE_RESOLVING_NAMES) {
            (void)gap_inquiry_stop();
            g_state = CLASSIC_HID_STATE_IDLE;
            blu2usb_ble_hogp_pico_resume_discovery_after_classic();
        } else if (g_state == CLASSIC_HID_STATE_CONNECTING && g_hid_cid != 0u) {
            g_resume_ble_after_close = true;
            hid_host_disconnect(g_hid_cid);
        } else if (g_state != CLASSIC_HID_STATE_READY) {
            g_state = CLASSIC_HID_STATE_IDLE;
            blu2usb_ble_hogp_pico_resume_discovery_after_classic();
        }
    }

    const bool retry = atomic_exchange_explicit(
        &g_retry_requested, false, memory_order_acq_rel);
    const bool pair = atomic_exchange_explicit(
        &g_pair_requested, false, memory_order_acq_rel);
    if (!g_stack_working || g_state == CLASSIC_HID_STATE_READY) return;

    if (retry) {
        g_pairing_active = true;
        if (g_state == CLASSIC_HID_STATE_INQUIRY ||
            g_state == CLASSIC_HID_STATE_RESOLVING_NAMES)
            (void)gap_inquiry_stop();
        if (g_state == CLASSIC_HID_STATE_CONNECTING && g_hid_cid != 0u) {
            g_restart_after_close = true;
            hid_host_disconnect(g_hid_cid);
        } else {
            g_state = CLASSIC_HID_STATE_IDLE;
        }
    }

    if (pair) {
        g_pairing_active = true;
        if (g_state == CLASSIC_HID_STATE_CONNECTING && g_hid_cid != 0u) {
            g_restart_after_close = true;
            hid_host_disconnect(g_hid_cid);
        } else if (g_state != CLASSIC_HID_STATE_INQUIRY &&
                   g_state != CLASSIC_HID_STATE_RESOLVING_NAMES) {
            g_state = CLASSIC_HID_STATE_IDLE;
        }
    }

    /* Crucially, pairing remains a level-triggered transaction. If the
     * controller was busy and start_inquiry() could not start BR/EDR inquiry,
     * this timer retries every 50 ms instead of consuming the request forever. */
    if (g_pairing_active && g_state == CLASSIC_HID_STATE_IDLE)
        (void)start_inquiry();
}

static void command_timer_handler(btstack_timer_source_t *timer)
{
    (void)timer;
    service_command_requests();
    btstack_run_loop_set_timer(&g_command_timer, CLASSIC_HID_COMMAND_SERVICE_MS);
    btstack_run_loop_add_timer(&g_command_timer);
}

static void classic_hid_session_setup(void)
{
    g_state = CLASSIC_HID_STATE_WAITING_FOR_STACK;
    g_stack_working = false;
    g_pairing_active = false;
    g_restart_after_close = false;
    g_resume_ble_after_close = false;
    g_descriptor_available = false;
    g_hid_cid = 0u;
    g_device_count = 0u;
    memset(g_devices, 0, sizeof(g_devices));
    memset(g_target_address, 0, sizeof(g_target_address));
    blu2usb_classic_hid_keyboard_state_clear(&g_keyboard_state);

    hid_host_init(g_descriptor_storage, sizeof(g_descriptor_storage));
    hid_host_register_packet_handler(packet_handler);
    gap_set_default_link_policy_settings(
        LM_LINK_POLICY_ENABLE_SNIFF_MODE | LM_LINK_POLICY_ENABLE_ROLE_SWITCH);
    hci_set_master_slave_policy(HCI_ROLE_MASTER);
    hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_ONLY);
    gap_set_local_name("BLU2USB 00:00:00:00:00:00");
    gap_discoverable_control(1);

    g_hci_registration.callback = &packet_handler;
    hci_add_event_handler(&g_hci_registration);

    btstack_run_loop_set_timer_handler(&g_command_timer, command_timer_handler);
    btstack_run_loop_set_timer(&g_command_timer, CLASSIC_HID_COMMAND_SERVICE_MS);
    btstack_run_loop_add_timer(&g_command_timer);
}

bool blu2usb_classic_hid_pico_register(void)
{
    if (g_registered) return true;
    if (!blu2usb_bt_runtime_register_session_setup(classic_hid_session_setup))
        return false;
    g_registered = true;
    return true;
}

bool blu2usb_classic_hid_pico_pair_keyboard(void)
{
    atomic_store_explicit(&g_cancel_requested, false, memory_order_release);
    atomic_store_explicit(&g_pair_requested, true, memory_order_release);
    return true;
}

bool blu2usb_classic_hid_pico_retry_keyboard(void)
{
    atomic_store_explicit(&g_cancel_requested, false, memory_order_release);
    atomic_store_explicit(&g_retry_requested, true, memory_order_release);
    return true;
}

bool blu2usb_classic_hid_pico_cancel_pairing(void)
{
    atomic_store_explicit(&g_pair_requested, false, memory_order_release);
    atomic_store_explicit(&g_retry_requested, false, memory_order_release);
    atomic_store_explicit(&g_cancel_requested, true, memory_order_release);
    return true;
}
