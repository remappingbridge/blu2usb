#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()

EXPECTED_DEPS = {
    "domain": [],
    "hid_aggregator": ["domain"],
    "profiles": ["domain"],
    "remap": ["domain", "profiles"],
    "device_registry": ["domain"],
    "connection_coordinator": ["domain", "device_registry", "keyboard_transport", "ble_hogp"],
    "bt_runtime": ["domain"],
    "ble_hogp": ["domain", "bt_runtime"],
    "classic_hid": ["domain", "bt_runtime"],
    "keyboard_transport": ["domain", "ble_hogp", "classic_hid"],
    "logitech_hidpp": ["domain", "bt_runtime", "ble_hogp"],
    "usb_hid": ["domain", "hid_aggregator"],
    "storage": ["domain"],
    "interaction": ["domain"],
    "ux_model": ["domain", "interaction"],
    "renderer": ["ux_model"],
    "hat": ["domain"],
    "app": [
        "domain", "hid_aggregator", "profiles", "remap", "device_registry",
        "connection_coordinator", "bt_runtime", "ble_hogp", "classic_hid",
        "keyboard_transport", "logitech_hidpp", "usb_hid", "storage",
        "interaction", "ux_model", "renderer", "hat",
    ],
}

PURE_MODULES = {
    "domain", "hid_aggregator", "profiles", "remap", "device_registry",
    "interaction", "ux_model",
}

BTSTACK_ALLOWED = {"bt_runtime", "ble_hogp", "classic_hid", "logitech_hidpp"}


def fail(message: str) -> None:
    raise AssertionError(message)


def parse_module_graph() -> None:
    path = ROOT / "cmake" / "Blu2UsbModules.cmake"
    text = path.read_text(encoding="utf-8")

    match = re.search(r"set\(BLU2USB_MODULES\s+(.*?)\n\)", text, re.S)
    if not match:
        fail("BLU2USB_MODULES declaration is missing")

    modules = match.group(1).split()
    if modules != list(EXPECTED_DEPS):
        fail(f"module list/order differs from the frozen graph: {modules}")

    for module, expected in EXPECTED_DEPS.items():
        dep_match = re.search(
            rf'set\(BLU2USB_DEPS_{re.escape(module)}\s+"([^"]*)"\)', text
        )
        if not dep_match:
            fail(f"missing dependency declaration for {module}")
        actual = [item for item in dep_match.group(1).split(";") if item]
        if actual != expected:
            fail(f"dependency mismatch for {module}: expected {expected}, got {actual}")

    if "add_library(blu2usb_module_${module} INTERFACE)" not in text:
        fail("architecture facade modules must remain INTERFACE targets")


def source_files() -> list[Path]:
    roots = [ROOT / "src", ROOT / "include"]
    result: list[Path] = []
    for base in roots:
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.is_file() and path.suffix.lower() in {".c", ".h", ".cc", ".cpp", ".hpp"}:
                result.append(path)
    return result


def module_for(path: Path) -> str | None:
    rel = path.relative_to(ROOT)
    if rel == Path("include/tusb_config.h"):
        return "usb_hid"
    if rel == Path("include/btstack_config.h"):
        return "bt_runtime"
    if len(rel.parts) >= 2 and rel.parts[0] == "src":
        return rel.parts[1]
    if len(rel.parts) >= 3 and rel.parts[0] == "include" and rel.parts[1] == "blu2usb":
        return rel.parts[2]
    return None


def check_source_boundaries() -> None:
    textual_c_include = re.compile(r'^\s*#\s*include\s*[<"][^>"]+\.c[>"]', re.M)
    macro_interception = re.compile(r'^\s*#\s*define\s+(?:tud_|hids_|gap_|sm_|cyw43_)', re.M)
    pico_or_transport = re.compile(r'(?:pico/|hardware/|btstack|cyw43|tusb\.h|tinyusb)', re.I)
    btstack_token = re.compile(r'(?:btstack|cyw43)', re.I)
    tinyusb_token = re.compile(r'(?:tusb\.h|tinyusb|\btud_)', re.I)
    raw_app_token = re.compile(r'(?:btstack|cyw43|tusb\.h|\btud_|hardware/gpio|hardware/spi)', re.I)
    reenumeration_token = re.compile(r'\btud_(?:disconnect|connect)\s*\(', re.I)

    for path in source_files():
        text = path.read_text(encoding="utf-8")
        rel = path.relative_to(ROOT)
        module = module_for(path)

        if textual_c_include.search(text):
            fail(f"textual .c include prohibited: {rel}")
        if macro_interception.search(text):
            fail(f"transport/HAL macro interception prohibited: {rel}")

        if module in PURE_MODULES and pico_or_transport.search(text):
            fail(f"host-pure module leaks Pico/transport dependency: {rel}")

        if btstack_token.search(text) and module not in BTSTACK_ALLOWED:
            fail(f"BTstack/CYW43 token outside Bluetooth adapter boundary: {rel}")

        if tinyusb_token.search(text) and module != "usb_hid":
            fail(f"TinyUSB ownership leaked outside usb_hid: {rel}")

        if module == "app" and raw_app_token.search(text):
            fail(f"app contains raw transport/HAL primitive: {rel}")

        if reenumeration_token.search(text):
            fail(f"USB re-enumeration path is prohibited: {rel}")


def check_g04_usb_contract() -> None:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    required = (
        "add_library(blu2usb_usb_hid STATIC src/usb_hid/usb_hid.c)",
        "src/usb_hid/usb_hid_pico.c",
        "src/usb_hid/usb_descriptors.c",
        "tinyusb_device",
        "target_link_libraries(blu2usb_module_usb_hid INTERFACE blu2usb_usb_hid)",
    )
    for token in required:
        if token not in cmake:
            fail(f"G04 USB integration missing: {token}")

    config = (ROOT / "include" / "tusb_config.h").read_text(encoding="utf-8")
    if "#define CFG_TUD_HID 2" not in config:
        fail("TinyUSB must expose exactly two HID interfaces")
    for disabled in ("CFG_TUD_CDC 0", "CFG_TUD_MSC 0", "CFG_TUD_MIDI 0", "CFG_TUD_VENDOR 0"):
        if disabled not in config:
            fail(f"unexpected USB function enabled or unspecified: {disabled}")


def check_g05_canonical_hid_contract() -> None:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    for token in (
        "add_library(blu2usb_hid_aggregator STATIC",
        "src/hid_aggregator/hid_aggregator.c",
        "src/hid_aggregator/relative.c",
        "target_link_libraries(blu2usb_module_hid_aggregator INTERFACE blu2usb_hid_aggregator)",
        'BLU2USB_VERSION_STRING="0.5.0-g05"',
    ):
        if token not in cmake:
            fail(f"G05 canonical HID integration missing: {token}")

    domain_hid = ROOT / "include" / "blu2usb" / "domain" / "hid.h"
    aggregator_h = ROOT / "include" / "blu2usb" / "hid_aggregator" / "hid_aggregator.h"
    aggregator_c = ROOT / "src" / "hid_aggregator" / "hid_aggregator.c"
    relative_c = ROOT / "src" / "hid_aggregator" / "relative.c"
    for path in (domain_hid, aggregator_h, aggregator_c, relative_c):
        if not path.is_file():
            fail(f"G05 canonical HID file missing: {path.relative_to(ROOT)}")

    hid_text = domain_hid.read_text(encoding="utf-8")
    for token in (
        "BLU2USB_HID_SOURCE_MOUSE",
        "BLU2USB_HID_SOURCE_KEYBOARD",
        "BLU2USB_HID_SOURCE_COMPOSITE",
        "BLU2USB_HID_SOURCE_SYNTHETIC_REMAP",
    ):
        if token not in hid_text:
            fail(f"missing canonical HID source kind: {token}")

    aggregator_text = (
        aggregator_h.read_text(encoding="utf-8") + "\n" +
        aggregator_c.read_text(encoding="utf-8") + "\n" +
        relative_c.read_text(encoding="utf-8")
    )
    if "BLU2USB_HID_AGGREGATOR_MAX_SOURCES 16u" not in aggregator_text:
        fail("G05 must support 16 persistent ownership sources")

    canonical_lower = (hid_text + "\n" + aggregator_text).lower()
    for token in (
        "btstack", "cyw43", "tinyusb", "tusb.h", "tud_", "hid_host",
        "hardware/gpio", "hardware/spi", "pico/stdlib", "report_id",
        "descriptor_report", "remote_report",
    ):
        if token in canonical_lower:
            fail(f"canonical HID boundary contains forbidden transport/layout token: {token}")


def check_g05_ble_hogp_contract() -> None:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    required = (
        "add_library(blu2usb_bt_runtime STATIC src/bt_runtime/bt_runtime.c)",
        "add_library(blu2usb_ble_hogp STATIC src/ble_hogp/ble_hogp.c)",
        "src/bt_runtime/bt_runtime_pico.c",
        "src/ble_hogp/ble_hogp_pico.c",
        "pico_btstack_ble",
        "pico_btstack_cyw43",
        "pico_cyw43_arch_threadsafe_background",
        "PICO_BTSTACK_CYW43_MAX_HCI_PROCESS_LOOP_COUNT=8",
        "pico_btstack_make_gatt_header",
        "target_link_libraries(blu2usb_module_bt_runtime INTERFACE blu2usb_bt_runtime)",
        "target_link_libraries(blu2usb_module_ble_hogp INTERFACE blu2usb_ble_hogp)",
    )
    for token in required:
        if token not in cmake:
            fail(f"G05 BLE HOGP integration missing: {token}")

    combined = cmake + "\n" + "\n".join(
        path.read_text(encoding="utf-8")
        for path in source_files()
        if path.exists()
    )
    # G07 decision 0002 replaces the blanket Core1 ban with explicit ownership.
    for prohibited in ("tud_disconnect(", "tud_connect("):
        if prohibited in combined:
            fail(f"G05 runtime contains prohibited coupling/re-enumeration token: {prohibited}")

    runtime = (ROOT / "src/bt_runtime/bt_runtime_pico.c").read_text()
    for required in ("multicore_launch_core1_with_stack", "8192u", "btstack_run_loop_execute()",
                     "flash_safe_execute_core_init", "async_context_acquire_lock_blocking"):
        if required not in runtime:
            fail(f"G07 radio/flash ownership missing: {required}")
    for path in source_files():
        if "multicore_launch_core1" in path.read_text() and module_for(path) != "bt_runtime":
            fail(f"Core1 lifecycle leaked outside bt_runtime: {path}")

    for path in (
        ROOT / "include" / "blu2usb" / "bt_runtime" / "bt_runtime.h",
        ROOT / "src" / "bt_runtime" / "bt_runtime.c",
        ROOT / "src" / "bt_runtime" / "bt_runtime_pico.c",
        ROOT / "include" / "blu2usb" / "ble_hogp" / "ble_hogp.h",
        ROOT / "src" / "ble_hogp" / "ble_hogp.c",
        ROOT / "src" / "ble_hogp" / "ble_hogp_pico.c",
    ):
        if not path.is_file():
            fail(f"G05 BLE HOGP file missing: {path.relative_to(ROOT)}")

    adapter = (ROOT / "src" / "ble_hogp" / "ble_hogp.c").read_text(encoding="utf-8")
    pico = (ROOT / "src" / "ble_hogp" / "ble_hogp_pico.c").read_text(encoding="utf-8")
    for token in (
        "HID_USAGE_AC_PAN",
        "blu2usb_ble_hogp_parser_normalize_report",
        "BLU2USB_BLE_HOGP_FIELD_BUTTON",
        "BLU2USB_BLE_HOGP_FIELD_X",
        "BLU2USB_BLE_HOGP_FIELD_Y",
        "BLU2USB_BLE_HOGP_FIELD_WHEEL",
        "BLU2USB_BLE_HOGP_FIELD_PAN",
    ):
        if token not in adapter:
            fail(f"G05 Report Map parser missing capability: {token}")
    for token in (
        "ad_data_contains_uuid16",
        "appearance_is_explicit_non_mouse_hid",
        "hids_client_connect",
        "HID_PROTOCOL_MODE_REPORT",
        "sm_request_pairing",
        "disconnect_and_rescan",
    ):
        if token not in pico:
            fail(f"G05 BLE runtime missing behavior: {token}")


def check_production_debug_prohibition() -> None:
    paths = [ROOT / "CMakeLists.txt"]
    paths += list((ROOT / "cmake").rglob("*.cmake")) if (ROOT / "cmake").exists() else []
    paths += source_files()

    combined = "\n".join(path.read_text(encoding="utf-8") for path in paths if path.exists())

    prohibited = {
        "TinyUSB CDC runtime": r"\btud_cdc_",
        "debug build option": r"BLU2USB_(?:BUILD_)?DEBUG",
        "debug CDC token": r"debug[-_ ]?cdc",
        "debug UF2 token": r"debug[^\n]*\.uf2",
        "enabled Pico UART stdio": r"pico_enable_stdio_uart\s*\(\s*blu2usb_picow\s+1\s*\)",
        "enabled Pico USB stdio": r"pico_enable_stdio_usb\s*\(\s*blu2usb_picow\s+1\s*\)",
    }
    for label, pattern in prohibited.items():
        if re.search(pattern, combined, re.I):
            fail(f"production debug prohibition violated: {label}")

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    for required in (
        "pico_enable_stdio_usb(blu2usb_picow 0)",
        "pico_enable_stdio_uart(blu2usb_picow 0)",
        'PICO_BOARD STREQUAL "pico2_w"',
    ):
        if required not in cmake:
            fail(f"missing production scaffold guard: {required}")

    executable_names = re.findall(r"add_executable\s*\(\s*([A-Za-z0-9_.-]+)", cmake)
    pico_executables = [name for name in executable_names if name != "blu2usb_test_bootstrap"]
    if pico_executables != ["blu2usb_picow"]:
        fail(f"unexpected firmware executable targets: {pico_executables}")


def check_toolchain_lock() -> None:
    lock_path = ROOT / "ci" / "toolchain.env"
    values: dict[str, str] = {}
    for raw in lock_path.read_text(encoding="utf-8").splitlines():
        raw = raw.strip()
        if not raw or raw.startswith("#"):
            continue
        key, value = raw.split("=", 1)
        values[key] = value

    expected = {
        "CI_RUNNER": "ubuntu-24.04",
        "PICO_SDK_VERSION": "2.2.0",
        "ARM_GCC_PACKAGE_VERSION": "15:13.2.rel1-2",
        "ARM_GCC_UPSTREAM_VERSION": "13.2.Rel1",
    }
    if values != expected:
        fail(f"toolchain lock mismatch: expected {expected}, got {values}")


def main() -> int:
    parse_module_graph()
    check_source_boundaries()
    check_g04_usb_contract()
    check_g05_canonical_hid_contract()
    check_g05_ble_hogp_contract()
    check_production_debug_prohibition()
    check_toolchain_lock()
    print("BLU2USB-G05 architecture contract: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
