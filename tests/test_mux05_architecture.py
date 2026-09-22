#!/usr/bin/env python3
from pathlib import Path
import sys

root=Path(sys.argv[1] if len(sys.argv)>1 else ".").resolve()
cmake=(root/"CMakeLists.txt").read_text()
cfg=(root/"config/mux05/btstack_config.h").read_text()
pico=(root/"src/ble_hogp/ble_hogp_pico.c").read_text()
roles=(root/"src/ble_hogp/session_roles.c").read_text()
app=(root/"src/app/main.c").read_text()

for token in (
    "src/ble_hogp/session_roles.c",
    "config/mux05",
    "pico_cyw43_arch_threadsafe_background",
):
    assert token in cmake, token

for token in (
    "#define MAX_NR_GATT_CLIENTS 2",
    "#define MAX_NR_HCI_CONNECTIONS 2",
    "#define MAX_NR_HIDS_CLIENTS 2",
    "#define ENABLE_LE_PERIPHERAL",
    "#define ENABLE_LE_PRIVACY_ADDRESS_RESOLUTION",
):
    assert token in cfg, token

for token in (
    "BLE_HOGP_PAIR_NEW_TIMEOUT_MS 15000u",
    "g_candidate_connection_handle",
    "handle_candidate_gatt_event",
    "BLU2USB_BLE_HOGP_MESSAGE_PROVISIONAL_READY",
    "SM_EVENT_REENCRYPTION_COMPLETE",
    "blu2usb_ble_hogp_parser_has_mouse",
    "blu2usb_ble_hogp_session_can_forward",
    "blu2usb_ble_hogp_pair_new_start",
    "blu2usb_ble_hogp_pair_new_cancel",
    "blu2usb_ble_hogp_pair_new_commit",
    "cyw43_arch_async_context",
):
    assert token in pico, token

for token in (
    "BLU2USB_BLE_HOGP_ROLE_AUTHORITATIVE",
    "BLU2USB_BLE_HOGP_ROLE_PROVISIONAL",
    "BLU2USB_BLE_HOGP_ROLE_RETIRING",
    "active_generation",
    "blu2usb_ble_hogp_session_abort_commit",
):
    assert token in roles, token

candidate_start=pico.rindex("static void handle_candidate_gatt_event")
candidate_end=pico.index("static void hci_packet_handler", candidate_start)
candidate=pico[candidate_start:candidate_end]
assert "g_candidate_state != BLE_HOGP_CANDIDATE_PROMOTED" in candidate
assert "blu2usb_ble_hogp_parser_parse_report" in candidate
assert candidate.index("g_candidate_state != BLE_HOGP_CANDIDATE_PROMOTED") < candidate.index("blu2usb_ble_hogp_parser_parse_report")

release_mouse=app.index("blu2usb_hid_aggregator_release_source")
commit=app.rindex("blu2usb_ble_hogp_pair_new_commit")
assert release_mouse < commit
for token in (
    "CURRENT MOUSE ACTIVE",
    "NEW MOUSE QUALIFIED",
    "KEY A: COMMIT",
    "KEY B: CANCEL",
    "USB MUST STAY LIVE",
):
    assert token in app, token

for forbidden in ("tud_disconnect(", "tud_connect("):
    assert forbidden not in app.lower()

print("MUX-05 transport risk architecture: OK")
