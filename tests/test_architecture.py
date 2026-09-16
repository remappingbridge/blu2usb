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
        "domain",
        "hid_aggregator",
        "profiles",
        "remap",
        "device_registry",
        "connection_coordinator",
        "bt_runtime",
        "ble_hogp",
        "classic_hid",
        "keyboard_transport",
        "logitech_hidpp",
        "usb_hid",
        "storage",
        "interaction",
        "ux_model",
        "renderer",
        "hat",
    ],
}

PURE_MODULES = {
    "domain",
    "hid_aggregator",
    "profiles",
    "remap",
    "device_registry",
    "interaction",
    "ux_model",
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
        fail("contract modules must remain INTERFACE scaffolds in G01")


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


def check_production_debug_prohibition() -> None:
    paths = [ROOT / "CMakeLists.txt"]
    paths += list((ROOT / "cmake").rglob("*.cmake")) if (ROOT / "cmake").exists() else []
    paths += source_files()

    combined = "\n".join(path.read_text(encoding="utf-8") for path in paths if path.exists())

    prohibited = {
        "TinyUSB CDC": r"CFG_TUD_CDC|\btud_cdc_",
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
    check_production_debug_prohibition()
    check_toolchain_lock()
    print("BLU2USB-G01 architecture contract: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
