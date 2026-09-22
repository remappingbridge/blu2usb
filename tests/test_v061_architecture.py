#!/usr/bin/env python3

from pathlib import Path
import re
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()

cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
ux_h = (root / "include/blu2usb/ux_model/ux_model.h").read_text(encoding="utf-8")
ux_c = (root / "src/ux_model/ux_model.c").read_text(encoding="utf-8")
renderer = (root / "src/renderer/renderer.c").read_text(encoding="utf-8")
app = (root / "src/app/main.c").read_text(encoding="utf-8")

version = re.search(r"project\(blu2usb_picow VERSION (0\.6\.\d+)", cmake)
assert version is not None
assert tuple(map(int, version.group(1).split("."))) >= (0, 6, 1)
assert 'BLU2USB_VERSION_STRING="0.6.' in cmake

learn = ux_h.index("BLU2USB_SCREEN_LEARN_KEYS")
search = ux_h.index("BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE")
connected = ux_h.index("BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED")
count = ux_h.index("BLU2USB_SCREEN_COUNT")
assert learn < search < connected < count, "v0.6 screen IDs must stay stable; append startup screens"

for token in (
    "blu2usb_ux_begin_first_start",
    "blu2usb_ux_first_mouse_connected",
    "blu2usb_ux_first_mouse_disconnected",
    "first_start_complete",
):
    assert token in ux_h and token in ux_c

for literal in (
    "SEARCHING FIRST MOUSE",
    "PRESS TO LEARN KEYS",
    "WHILE WAIT CONNECTION",
    "FIRST MOUSE CONNECTED",
    " KEY Y: LOCK",
):
    assert literal in ux_c

for token in (
    "BLU2USB_SCREEN_SEARCHING_FIRST_MOUSE",
    "BLU2USB_SCREEN_FIRST_MOUSE_CONNECTED",
    "project_first_start_pressed",
):
    assert token in renderer

assert "blu2usb_ux_begin_first_start(&ux)" in app
assert (
    "blu2usb_ux_first_mouse_connected(ux)" in app or
    "blu2usb_ux_home_mouse_connected(ux)" in app
)
assert (
    "blu2usb_ux_first_mouse_disconnected(ux)" in app or
    "blu2usb_ux_home_mouse_disconnected(ux)" in app
)

# This point release must keep G06 transport/profile/remap/storage untouched.
for forbidden in (
    "tud_disconnect(",
    "tud_connect(",
    "btstack.h",
    "hardware/gpio",
    "hardware/spi",
):
    assert forbidden not in app.lower()

print("BLU2USB v0.6.1+ first-start UX regression contract: OK")
