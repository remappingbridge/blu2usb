/* UI boundary for the byte-identical PICO-08 host. No pairing state machine. */
#include "blu2usb/classic_hid/classic_probe.h"
#include "pico08/classic_keyboard.h"
#include "pico/critical_section.h"
#include "btstack.h"
#include <string.h>

static critical_section_t observation_lock;
static btstack_packet_callback_registration_t observer;
static bool working;
static uint16_t found;
static uint32_t legacy_pin_revision;
static bool legacy_pin;

/* Observation only: never sends an HCI command or handles a transport event. */
static void observe(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    if (type != HCI_EVENT_PACKET || size < 3) return;
    const uint8_t event = hci_event_packet_get_type(packet);
    classic_keyboard_snapshot_t snapshot = {0};
    if (event == HCI_EVENT_PIN_CODE_REQUEST)
        (void)classic_keyboard_get_snapshot(&snapshot);
    critical_section_enter_blocking(&observation_lock);
    if (event == BTSTACK_EVENT_STATE)
        working = btstack_event_state_get_state(packet) == HCI_STATE_WORKING;
    else if (event == GAP_EVENT_INQUIRY_RESULT && found < UINT16_MAX)
        ++found;
    else if (event == HCI_EVENT_PIN_CODE_REQUEST) {
        legacy_pin = true;
        legacy_pin_revision = snapshot.revision;
    }
    critical_section_exit(&observation_lock);
}

void blu2usb_classic_probe_shared_init(void)
{
    /* Called once on Core0, before Core1 or UI commands. */
    classic_keyboard_shared_init();
    critical_section_init(&observation_lock);
}

void blu2usb_classic_probe_setup(void)
{
    classic_keyboard_core1_init();
    observer.callback = observe;
    hci_add_event_handler(&observer);
}

void blu2usb_classic_probe_pair(void)
{
    critical_section_enter_blocking(&observation_lock);
    found = 0;
    legacy_pin = false;
    critical_section_exit(&observation_lock);
    (void)classic_keyboard_request_pair();
}

void blu2usb_classic_probe_cancel(void)
{
    (void)classic_keyboard_request_cancel();
}

blu2usb_classic_probe_snapshot_t blu2usb_classic_probe_snapshot(void)
{
    classic_keyboard_snapshot_t original = {0};
    blu2usb_classic_probe_snapshot_t result = {0};
    (void)classic_keyboard_get_snapshot(&original);
    critical_section_enter_blocking(&observation_lock);
    const bool ready = working;
    const bool show_legacy = legacy_pin && legacy_pin_revision == original.revision;
    result.found = found;
    critical_section_exit(&observation_lock);
    static const blu2usb_probe_phase_t phases[] = {
        BLU2USB_PROBE_IDLE, BLU2USB_PROBE_SEARCHING, BLU2USB_PROBE_CONNECTING,
        BLU2USB_PROBE_READY, BLU2USB_PROBE_ERROR
    };
    result.phase = ready ? phases[original.state] : BLU2USB_PROBE_STARTING;
    memcpy(result.message, original.message, sizeof(result.message));
    if (original.state == CLASSIC_KEYBOARD_CONNECTING &&
        strncmp(original.message, "TYPE ", 5) == 0) {
        uint32_t pin = 0;
        bool valid = true;
        for (unsigned i = 5; i < 11; ++i) {
            char c = original.message[i];
            if (c < '0' || c > '9') { valid = false; break; }
            pin = pin * 10u + (unsigned)(c - '0');
        }
        if (valid) { result.phase = BLU2USB_PROBE_PIN; result.pin = pin; result.pin_digits = 6; }
    } else if (show_legacy && original.state == CLASSIC_KEYBOARD_CONNECTING) {
        result.phase = BLU2USB_PROBE_PIN;
        result.pin_digits = 4;
    }
    return result;
}
