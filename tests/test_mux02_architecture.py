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
        "add_library(blu2usb_connection_coordinator STATIC",
        "src/connection_coordinator/connection_coordinator.c",
        "target_link_libraries(blu2usb_connection_coordinator PUBLIC blu2usb_device_registry)",
        "target_link_libraries(blu2usb_module_connection_coordinator INTERFACE blu2usb_connection_coordinator)",
    ):
        if token not in cmake:
            fail(f"MUX-02 CMake integration missing: {token}")

    if 'set(BLU2USB_DEPS_connection_coordinator "domain;device_registry")' not in graph:
        fail("connection_coordinator must be host-pure and depend only on domain/device_registry")

    files = (
        ROOT / "include/blu2usb/connection_coordinator/connection_coordinator.h",
        ROOT / "src/connection_coordinator/connection_coordinator.c",
    )
    for path in files:
        if not path.is_file():
            fail(f"MUX-02 file missing: {path.relative_to(ROOT)}")

    combined = "\n".join(path.read_text(encoding="utf-8") for path in files)
    for pattern in (
        r"pico/",
        r"hardware/",
        r"btstack",
        r"cyw43",
        r"tinyusb",
        r"tusb\.h",
        r"\btud_",
        r"ble_hogp",
        r"storage\.h",
        r"ux_model",
        r"renderer",
    ):
        if re.search(pattern, combined, re.I):
            fail(f"host-pure coordinator leaks forbidden dependency: {pattern}")

    header = files[0].read_text(encoding="utf-8")
    for token in (
        "#define BLU2USB_SEARCH_FIRST_MS UINT64_C(8000)",
        "#define BLU2USB_SEARCH_SAVED_MS UINT64_C(8000)",
        "#define BLU2USB_SEARCH_NEW_MS UINT64_C(15000)",
        "BLU2USB_HANDOFF_FREEZE_OLD_INPUT",
        "BLU2USB_HANDOFF_RELEASE_OLD_OWNERSHIP",
        "BLU2USB_HANDOFF_RETIRE_OLD_TRANSPORT",
        "BLU2USB_HANDOFF_PERSIST_CANDIDATE",
        "BLU2USB_HANDOFF_PROMOTE_CANDIDATE",
    ):
        if token not in header:
            fail(f"MUX-02 frozen coordinator contract missing: {token}")

    source = files[1].read_text(encoding="utf-8")
    if "stale_result_count" not in source:
        fail("MUX-02 stale-result accounting missing")
    if "BLU2USB_MOUSE_PROFILE_PASSTHROUGH" not in source:
        fail("newly committed Mouse must start Passthrough")

    print("MUX-02 architecture contract: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
