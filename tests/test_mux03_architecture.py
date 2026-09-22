#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()


def fail(message: str) -> None:
    raise AssertionError(message)


def main() -> int:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    graph = (ROOT / "cmake/Blu2UsbModules.cmake").read_text(encoding="utf-8")

    for token in (
        "src/ux_model/ux_v1.c",
        "src/renderer/renderer_v1.c",
    ):
        if token not in cmake:
            fail(f"MUX-03 CMake integration missing: {token}")

    if 'set(BLU2USB_DEPS_ux_model "domain;interaction;device_registry")' not in graph:
        fail("UX model must depend only on domain/interaction/device_registry")

    files = (
        ROOT / "include/blu2usb/ux_model/ux_v1.h",
        ROOT / "src/ux_model/ux_v1.c",
        ROOT / "include/blu2usb/renderer/renderer_v1.h",
        ROOT / "src/renderer/renderer_v1.c",
    )
    for path in files:
        if not path.is_file():
            fail(f"MUX-03 file missing: {path.relative_to(ROOT)}")

    combined = "\n".join(path.read_text(encoding="utf-8") for path in files)
    for pattern in (
        r"mouse_ui/",
        r"mouse-ui",
        r"mouse_core",
        r"mouse-core",
        r"ui_core",
        r"ui-core",
        r"btstack",
        r"cyw43",
        r"tinyusb",
        r"tusb\.h",
        r"ble_hogp",
        r"storage\.h",
        r"pico/",
        r"hardware/",
    ):
        if re.search(pattern, combined, re.I):
            fail(f"UI v1 leaks forbidden implementation dependency: {pattern}")

    header = files[0].read_text(encoding="utf-8")
    if "BLU2USB_UI_V1_SCREEN_COUNT" not in header:
        fail("MUX-03 30-screen enum missing")
    if "BLU2USB_UI_V1_LEARN_THE_KEYS" not in header:
        fail("MUX-03 learn-the-keys screen missing")
    for prohibited in ("PAIR_KEYBOARD", "PAIR_COMPOSITE", "OTHER_DEVICES"):
        if prohibited in header:
            fail(f"Mouse-only UI exposes prohibited screen: {prohibited}")

    source = files[1].read_text(encoding="utf-8")
    for token in (
        "BLU2USB_UI_V1_SEARCHING_FIRST",
        "BLU2USB_UI_V1_FIRST_MOUSE_CONNECTED",
        "BLU2USB_UI_V1_HOME_SEARCHING",
        "BLU2USB_UI_V1_HOME_RETRY",
        "BLU2USB_UI_V1_PAIR_NEW",
        "BLU2USB_UI_V1_HOME_CONNECTED",
        "BLU2USB_UI_V1_REMAPPER_OPTIONS",
        "BLU2USB_UI_V1_SAVED_DEVICES",
        "BLU2USB_UI_V1_REMOVE_THIS",
    ):
        if token not in source:
            fail(f"MUX-03 navigation state missing: {token}")

    renderer = files[3].read_text(encoding="utf-8")
    for literal in (
        "KEY B: BACK",
        "KEY Y: LOCK",
        "STANDARD REMAP",
        "REMOVE CONNECTED HELP",
        "PRESS TO LEARN KEYS",
    ):
        if literal not in renderer:
            fail(f"MUX-03 frozen UI literal missing: {literal}")

    if "GO TO HOME" in renderer:
        fail("deprecated GO TO HOME hint reintroduced")

    golden = ROOT / "tests/goldens/mux03/screens.txt"
    if not golden.is_file():
        fail("MUX-03 screen golden missing")
    screen_ids = []
    for line in golden.read_text(encoding="utf-8").splitlines():
        if not line:
            continue
        screen_id = line.split("|", 1)[0]
        if not screen_ids or screen_ids[-1] != screen_id:
            screen_ids.append(screen_id)
    if len(screen_ids) != 30:
        fail(f"MUX-03 golden must contain exactly 30 screens, got {len(screen_ids)}")

    print("MUX-03 architecture contract: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
