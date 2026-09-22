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

    required_cmake = (
        "add_library(blu2usb_device_registry STATIC",
        "src/device_registry/device_registry.c",
        "src/device_registry/product_snapshot.c",
        "target_link_libraries(blu2usb_module_device_registry INTERFACE blu2usb_device_registry)",
    )
    for token in required_cmake:
        if token not in cmake:
            fail(f"MUX-01 device_registry CMake integration missing: {token}")

    files = (
        ROOT / "include/blu2usb/domain/device.h",
        ROOT / "include/blu2usb/device_registry/device_registry.h",
        ROOT / "include/blu2usb/device_registry/product_snapshot.h",
        ROOT / "src/device_registry/device_registry.c",
        ROOT / "src/device_registry/product_snapshot.c",
    )
    for path in files:
        if not path.is_file():
            fail(f"MUX-01 file missing: {path.relative_to(ROOT)}")

    combined = "\n".join(path.read_text(encoding="utf-8") for path in files)
    forbidden = (
        r"pico/",
        r"hardware/",
        r"btstack",
        r"cyw43",
        r"tinyusb",
        r"tusb\.h",
        r"\btud_",
        r"ble_hogp",
        r"storage\.h",
    )
    for pattern in forbidden:
        if re.search(pattern, combined, re.I):
            fail(f"host-pure registry leaks transport/storage dependency: {pattern}")

    header = (ROOT / "include/blu2usb/domain/device.h").read_text(encoding="utf-8")
    if "#define BLU2USB_SAVED_MOUSE_CAPACITY 16u" not in header:
        fail("MUX-01 capacity must remain exactly 16")

    registry = (ROOT / "src/device_registry/device_registry.c").read_text(encoding="utf-8")
    if "authoritative_id" not in registry:
        fail("MUX-01 authoritative Mouse identity is missing")

    snapshot = (ROOT / "src/device_registry/product_snapshot.c").read_text(encoding="utf-8")
    if "blu2usb_device_registry_authoritative" not in snapshot:
        fail("MUX-01 snapshot must project the authoritative Mouse first")

    print("MUX-01 architecture contract: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
