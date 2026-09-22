#!/usr/bin/env python3

from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()

cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
ux_h = (root / "include/blu2usb/ux_model/ux_model.h").read_text(encoding="utf-8")
ux_c = (root / "src/ux_model/ux_model.c").read_text(encoding="utf-8")
renderer = (root / "src/renderer/renderer.c").read_text(encoding="utf-8")
ble_h = (root / "include/blu2usb/ble_hogp/ble_hogp.h").read_text(encoding="utf-8")
ble_pico = (root / "src/ble_hogp/ble_hogp_pico.c").read_text(encoding="utf-8")
app = (root / "src/app/main.c").read_text(encoding="utf-8")

assert 'project(blu2usb_picow VERSION 0.6.2' in cmake
assert 'BLU2USB_VERSION_STRING="0.6.2"' in cmake

for token in (
    "BLU2USB_SCREEN_HOME_SEARCHING",
    "BLU2USB_SCREEN_HOME_RETRY",
    "BLU2USB_SCREEN_HOME_CONNECTED",
    "BLU2USB_SCREEN_HOME_SEARCHING_HELP",
    "BLU2USB_SCREEN_HOME_RETRY_HELP",
    "BLU2USB_SCREEN_HOME_CONNECTED_HELP",
):
    assert token in ux_h and token in ux_c

# Point-release screen IDs remain append-only.
first_connected = ux_h.index("BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED")
home_searching = ux_h.index("BLU2USB_SCREEN_HOME_SEARCHING")
home_retry = ux_h.index("BLU2USB_SCREEN_HOME_RETRY")
home_connected = ux_h.index("BLU2USB_SCREEN_HOME_CONNECTED")
screen_count = ux_h.index("BLU2USB_SCREEN_COUNT")
assert first_connected < home_searching < home_retry < home_connected < screen_count

for literal in (
    "SEARCHING SAVED MOUSE",
    "DEVICE NOT FOUND",
    " SAVED DEVICES",
    " PAIR NEW MOUSE",
    " LEARN THE KEYS",
    "KEY B: CANCEL SEARCH",
    "KEY A: RETRY SEARCH",
    "KEY X: HELP TO REMOVE",
):
    assert literal in ux_c

for literal in (
    " REMAPPED TO STANDARD",
    " REMAPPED TO ESCAPE",
    " REMAPPED TO CUSTOM",
):
    assert literal in renderer

assert "BLE_HOGP_BONDED_RECONNECT_TIMEOUT_MS 8000u" in ble_pico
assert "BLU2USB_BLE_HOGP_MESSAGE_SAVED_SEARCH_TIMEOUT" in ble_h
assert "g_saved_search_mode" in ble_pico
assert "finish_saved_search(true)" in ble_pico
assert "g_request_retry_saved_search" in ble_pico
assert "g_request_cancel_saved_search" in ble_pico

for token in (
    "blu2usb_ux_home_mouse_connected",
    "blu2usb_ux_home_mouse_disconnected",
    "blu2usb_ux_home_saved_search_timeout",
    "blu2usb_ble_hogp_set_saved_search_mode(true)",
    "BLU2USB_UX_COMMAND_RETRY_SAVED_SEARCH",
    "BLU2USB_UX_COMMAND_CANCEL_SAVED_SEARCH",
):
    assert token in app

# v0.6.2 still uses the accepted single-session G06 transport, not MUX.
for forbidden in (
    "session_roles",
    "PROVISIONAL",
    "mouse-ui/",
    "mouse_core",
    "device_registry",
):
    assert forbidden not in app
    assert forbidden not in ble_pico

print("BLU2USB v0.6.2 three-state HOME contract: OK")
