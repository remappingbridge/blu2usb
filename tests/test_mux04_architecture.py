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
        "src/storage/storage.c",
        "src/storage/product_state.c",
        "src/storage/storage_pico.c",
        "target_link_libraries(blu2usb_storage PUBLIC",
        "blu2usb_device_registry",
        "blu2usb_profiles",
    ):
        if token not in cmake:
            fail(f"MUX-04 build integration missing: {token}")

    if 'set(BLU2USB_DEPS_storage "domain;device_registry;profiles")' not in graph:
        fail("storage dependency graph must own registry/profile serialization")

    storage_h = (
        ROOT / "include/blu2usb/storage/storage.h"
    ).read_text(encoding="utf-8")
    for token in (
        "#define BLU2USB_STORAGE_MAX_PAYLOAD_SIZE 1280u",
        "#define BLU2USB_STORAGE_RECORD_SIZE 1536u",
        "#define BLU2USB_STORAGE_LEGACY_RECORD_SIZE 80u",
    ):
        if token not in storage_h:
            fail(f"MUX-04 storage envelope contract missing: {token}")

    product_h = (
        ROOT / "include/blu2usb/storage/product_state.h"
    ).read_text(encoding="utf-8")
    for token in (
        "#define BLU2USB_PRODUCT_SERIALIZED_SIZE 1184u",
        "legacy_profile_pending",
        "legacy_profile_kind",
        "blu2usb_product_state_restore",
    ):
        if token not in product_h:
            fail(f"MUX-04 product schema contract missing: {token}")

    core_files = (
        ROOT / "src/storage/storage.c",
        ROOT / "src/storage/product_state.c",
    )
    combined = "\n".join(p.read_text(encoding="utf-8") for p in core_files)

    for pattern in (
        r"btstack",
        r"cyw43",
        r"tinyusb",
        r"tusb\.h",
        r"ble_hogp",
        r"pico/",
        r"hardware/",
    ):
        if re.search(pattern, combined, re.I):
            fail(f"host storage core leaks transport/HAL dependency: {pattern}")

    pico = (
        ROOT / "src/storage/storage_pico.c"
    ).read_text(encoding="utf-8").lower()

    for token in (
        "blu2usb_storage_slot_count 2u",
        "flash_safe_execute",
        "flash_range_erase",
        "flash_range_program",
        "blu2usb_product_storage_offset",
        "static uint8_t g_records",
        "static uint8_t g_encoded",
        "blu2usb_storage_record_size % flash_page_size",
    ):
        if token not in pico:
            fail(f"MUX-04 Pico storage safety behavior missing: {token}")

    if "btstack" in pico:
        fail("product storage must stay separate from BT credential storage")

    product = (
        ROOT / "src/storage/product_state.c"
    ).read_text(encoding="utf-8")

    for token in (
        "BLU2USB_SAVED_MOUSE_CAPACITY",
        "blu2usb_device_registry_clear_authoritative",
        "BLU2USB_PRODUCT_RESTORE_G06_MIGRATED",
        "blu2usb_profiles_restore",
    ):
        if token not in product:
            fail(f"MUX-04 persistence/migration behavior missing: {token}")

    print("MUX-04 architecture contract: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
