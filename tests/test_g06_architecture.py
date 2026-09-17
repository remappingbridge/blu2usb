#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')

required_cmake = (
    'BLU2USB_VERSION_STRING="0.6.0-g06"',
    'src/profiles/profiles.c',
    'add_library(blu2usb_remap STATIC src/remap/remap.c)',
    'add_library(blu2usb_logitech_hidpp STATIC src/logitech_hidpp/logitech_hidpp.c)',
    'src/logitech_hidpp/logitech_hidpp_pico.c',
    'target_link_libraries(blu2usb_module_remap INTERFACE blu2usb_remap)',
    'target_link_libraries(blu2usb_module_logitech_hidpp INTERFACE blu2usb_logitech_hidpp)',
)
for token in required_cmake:
    assert token in cmake, f'missing G06 CMake contract: {token}'

files = {
    'profile domain': root / 'include/blu2usb/domain/profile.h',
    'profiles API': root / 'include/blu2usb/profiles/profiles.h',
    'profiles core': root / 'src/profiles/profiles.c',
    'remap API': root / 'include/blu2usb/remap/remap.h',
    'remap core': root / 'src/remap/remap.c',
    'HID++ API': root / 'include/blu2usb/logitech_hidpp/logitech_hidpp.h',
    'HID++ core': root / 'src/logitech_hidpp/logitech_hidpp.c',
    'HID++ Pico glue': root / 'src/logitech_hidpp/logitech_hidpp_pico.c',
}
for label, path in files.items():
    assert path.is_file(), f'missing {label}: {path.relative_to(root)}'

profile = files['profiles core'].read_text(encoding='utf-8')
for token in (
    'BLU2USB_MOUSE_PROFILE_PASSTHROUGH',
    'BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP',
    'BLU2USB_MOUSE_PROFILE_ESCAPE_REMAP',
    'BLU2USB_MOUSE_PROFILE_CUSTOM_REMAP',
    'BLU2USB_MOUSE_TARGET_ESCAPE',
    'blu2usb_profiles_requires_forward_held_fix',
    'blu2usb_profiles_serialize',
    'blu2usb_profiles_restore',
):
    assert token in profile, f'missing profile behavior: {token}'

remap = files['remap core'].read_text(encoding='utf-8')
for token in (
    'BLU2USB_HID_SOURCE_SYNTHETIC_REMAP',
    'BLU2USB_KEY_ESCAPE',
    'BLU2USB_MOUSE_EVENT_BUTTON',
):
    assert token in remap, f'missing canonical remap behavior: {token}'

hidpp = (files['HID++ API'].read_text(encoding='utf-8') + '\n' +
         files['HID++ core'].read_text(encoding='utf-8'))
for token in ('0x1b04u', '0x0056u', 'BLU2USB_HIDPP_OUTPUT_SET_FORWARD_DIVERT'):
    assert token in hidpp, f'missing HID++ behavior: {token}'

# Host-pure profile/remap/HID++ state machines stay independent from transport/HAL.
for key in ('profiles core', 'remap core', 'HID++ core'):
    text = files[key].read_text(encoding='utf-8').lower()
    for forbidden in ('btstack', 'cyw43', 'tinyusb', 'tusb.h', 'pico/', 'hardware/'):
        assert forbidden not in text, f'{key} leaks transport/HAL token: {forbidden}'

pico_hidpp = files['HID++ Pico glue'].read_text(encoding='utf-8')
assert 'blu2usb_ble_hogp_register_vendor_backend' in pico_hidpp
assert 'blu2usb_logitech_hidpp_claims_forward' in pico_hidpp

ble = (root / 'src/ble_hogp/ble_hogp_pico.c').read_text(encoding='utf-8')
for token in (
    'hids_client_send_write_report',
    'g_vendor_backend.claims_button',
    'g_vendor_backend.input',
    'g_vendor_backend.session',
):
    assert token in ble, f'BLE vendor composition missing: {token}'

app = (root / 'src/app/main.c').read_text(encoding='utf-8')
for token in (
    'blu2usb_remap_process_mouse',
    'blu2usb_usb_hid_pico_send_keyboard',
    'BLU2USB_UX_COMMAND_APPLY_PASSTHROUGH',
    'BLU2USB_UX_COMMAND_APPLY_DEFAULT',
    'BLU2USB_UX_COMMAND_APPLY_ESCAPE',
    'BLU2USB_UX_COMMAND_APPLY_CUSTOM',
):
    assert token in app, f'app G06 composition missing: {token}'
for forbidden in ('tud_disconnect(', 'tud_connect(', 'btstack.h', 'hardware/gpio', 'hardware/spi'):
    assert forbidden not in app.lower(), f'app leaks raw transport/HAL: {forbidden}'

print('BLU2USB-G06 profiles/remap/HID++ architecture: OK')
