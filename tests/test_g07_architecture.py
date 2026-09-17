#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
for token in (
    'BLU2USB_VERSION_STRING="0.7.0-g07"',
    'add_library(blu2usb_classic_hid STATIC src/classic_hid/classic_hid.c)',
    'src/classic_hid/classic_hid_pico.c',
    'add_library(blu2usb_keyboard_transport STATIC',
    'src/keyboard_transport/keyboard_transport.c',
    'src/keyboard_transport/keyboard_transport_pico.c',
    'pico_btstack_classic',
    'pico_multicore',
    'pico_flash',
    'PICO_CORE1_STACK_SIZE=0',
    'target_link_libraries(blu2usb_module_classic_hid INTERFACE blu2usb_classic_hid)',
    'target_link_libraries(blu2usb_module_keyboard_transport INTERFACE blu2usb_keyboard_transport)',
):
    assert token in cmake, f'missing G07 CMake contract: {token}'

classic_h = (root / 'include/blu2usb/classic_hid/classic_hid.h').read_text(encoding='utf-8')
classic = (root / 'src/classic_hid/classic_hid.c').read_text(encoding='utf-8')
classic_pico = (root / 'src/classic_hid/classic_hid_pico.c').read_text(encoding='utf-8')
ble_h = (root / 'include/blu2usb/ble_hogp/ble_hogp.h').read_text(encoding='utf-8')
ble_pico = (root / 'src/ble_hogp/ble_hogp_pico.c').read_text(encoding='utf-8')
transport_h = (root / 'include/blu2usb/keyboard_transport/keyboard_transport.h').read_text(encoding='utf-8')
transport = (root / 'src/keyboard_transport/keyboard_transport.c').read_text(encoding='utf-8')
app = (root / 'src/app/main.c').read_text(encoding='utf-8')
runtime = (root / 'src/bt_runtime/bt_runtime_pico.c').read_text(encoding='utf-8')
btstack_config = (root / 'include/btstack_config.h').read_text(encoding='utf-8')

for text, label in ((classic, 'classic core'), (transport, 'keyboard transport core')):
    lower = text.lower()
    for forbidden in ('btstack', 'cyw43', 'tinyusb', 'tusb.h', 'pico/', 'hardware/'):
        assert forbidden not in lower, f'{label} leaks transport/HAL token: {forbidden}'

for token in (
    'blu2usb_classic_hid_emit_state_diff',
    'BLU2USB_CLASSIC_HID_MESSAGE_PAIR_CODE',
):
    assert token in classic_h + classic, f'missing canonical Classic behavior: {token}'

for token in (
    'Bluetooth keyboard 3.0',
    'BKB-3G',
    'BLU2USB_HID_SOURCE_KEYBOARD',
    'gap_inquiry_start',
    'gap_remote_name_request',
    'hid_host_init',
    'hid_host_connect',
    'hid_host_accept_connection',
    'HID_PROTOCOL_MODE_REPORT',
    'HCI_EVENT_USER_PASSKEY_NOTIFICATION',
    'gap_pin_code_response',
    'btstack_hid_parser_init',
    'descriptor_has_keyboard',
    'blu2usb_bt_runtime_register_session_setup',
    'blu2usb_ble_hogp_pico_pause_discovery_for_classic',
    'blu2usb_ble_hogp_pico_resume_discovery_after_classic',
    'g_pairing_active && g_state == CLASSIC_HID_STATE_IDLE',
):
    assert token in classic_pico, f'missing Classic HID Pico behavior: {token}'
assert 'HID_PROTOCOL_MODE_REPORT_WITH_FALLBACK_TO_BOOT' not in classic_pico

for token in (
    'blu2usb_ble_hogp_pico_pause_discovery_for_classic',
    'blu2usb_ble_hogp_pico_resume_discovery_after_classic',
):
    assert token in ble_h, f'missing shared-radio BLE facade: {token}'
for token in (
    'BLE_HOGP_STATE_PAUSED_DISCOVERY',
    'g_discovery_suppressed',
    'gap_stop_scan()',
    'gap_connect_cancel()',
    'enter_paused_discovery',
    'hci_event_disconnection_complete_get_connection_handle',
    'disconnected != g_connection_handle',
):
    assert token in ble_pico, f'missing BLE discovery/ownership arbitration behavior: {token}'

for token in (
    '#define MAX_NR_HCI_CONNECTIONS 2',
    '#define MAX_NR_HID_HOST_CONNECTIONS 1',
    '#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES 2',
    '#define NVM_NUM_LINK_KEYS 16',
    '#define MAX_NR_L2CAP_SERVICES 3',
):
    assert token in btstack_config, f'BTstack not sized for BLE Mouse + Classic Keyboard: {token}'

# Classic depends on the BLE adapter only in the Pico composition layer so it
# can arbitrate shared CYW43 discovery without leaking raw GAP calls into app.
classic_pico_cmake = cmake.split(
    'target_sources(blu2usb_classic_hid PRIVATE src/classic_hid/classic_hid_pico.c)', 1
)[1].split('target_sources(blu2usb_keyboard_transport PRIVATE', 1)[0]
assert 'blu2usb_ble_hogp' in classic_pico_cmake

# Every physically proven Classic BKB-3G implementation in the reference repo
# executes BTstack on a dedicated Core1.  PICO-08 proved that the integrated
# BLE+Classic workload needs an explicit 8 KiB SRAM stack rather than relying on
# the linker-owned default Core1 stack.  Freeze that execution envelope here so
# future state-machine changes cannot silently return Classic to Core0-only
# background servicing.
for token in (
    '#define BLU2USB_BT_CORE1_STACK_SIZE_BYTES (8u * 1024u)',
    'g_bt_core1_stack',
    'multicore_launch_core1_with_stack',
    'btstack_run_loop_execute()',
    'flash_safe_execute_core_init()',
    'BLU2USB_BT_CORE1_START_DELAY_MS 500u',
    'g_primary_session_setup',
):
    assert token in runtime, f'missing proven Core1 Bluetooth runtime behavior: {token}'
assert 'initialize CYW43/BTstack on core 0' not in runtime

for forbidden in ('tud_disconnect(', 'tud_connect(', 'printf(', 'uart_', 'stdio_uart'):
    assert forbidden not in classic_pico.lower(), f'Classic adapter violates production policy: {forbidden}'

assert 'Classic' not in transport_h.split('/* Pico facade.')[0], 'logical public event API must remain transport-neutral'
for token in (
    'blu2usb_keyboard_transport_decode_runtime_message',
    'blu2usb_keyboard_transport_pico_pair',
):
    assert token in transport_h + transport, f'missing Keyboard facade: {token}'

for token in (
    'blu2usb_keyboard_transport_pico_start',
    'blu2usb_keyboard_transport_pico_pair',
    'blu2usb_keyboard_transport_decode_runtime_message',
    'BLU2USB_HID_SOURCE_KEYBOARD',
    'blu2usb_hid_aggregator_apply_keyboard',
):
    assert token in app, f'app missing G07 composition: {token}'
for forbidden in ('btstack.h', 'hid_host_', 'gap_inquiry_', 'tud_disconnect(', 'tud_connect('):
    assert forbidden not in app.lower(), f'app leaks raw transport primitive: {forbidden}'

for token in (
    'blu2usb_bt_runtime_register_session_setup',
    'g_session_setups',
    'pico_cyw43_arch_threadsafe_background',
):
    if token == 'pico_cyw43_arch_threadsafe_background':
        assert token in cmake
    else:
        assert token in runtime

ux_layout = (root / 'docs/ux/01-screen-layouts.md').read_text(encoding='utf-8')
assert 'PAIR KEYBOARD' in ux_layout
assert 'SEARCHING KEYBOARD' in ux_layout
assert 'SEARCHING CLASSIC' not in ux_layout
assert 'PAIR CLASSIC' not in ux_layout

print('BLU2USB-G07 Keyboard facade / Classic HID architecture: OK')
