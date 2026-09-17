#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
technical = (root / 'docs/technical/06-g07-keyboard-classic-hid-validation.md').read_text(encoding='utf-8')
ux = (root / 'docs/ux/02-g07-keyboard-live-contract.md').read_text(encoding='utf-8')

for token in (
    'PAIR KEYBOARD',
    'Bluetooth keyboard 3.0',
    'BKB-3G',
    'PIN: 123456',
    'BLU2USB_HID_SOURCE_KEYBOARD',
    'synthetic Escape',
    'G07-10',
    'do not merge automatically',
):
    assert token in technical, f'missing G07 technical contract: {token}'

for token in (
    'TYPE PIN ON KEYBOARD',
    'THEN PRESS ENTER',
    'PIN: 123456',
    'selection has global priority',
    'NOT CONNECTED',
    'one-logical-page Back',
):
    assert token in ux, f'missing G07 UX contract: {token}'

for forbidden in ('PAIR CLASSIC', 'SEARCHING CLASSIC', 'SELECT CLASSIC'):
    assert forbidden not in ux, f'transport leaked into product UX: {forbidden}'

print('BLU2USB-G07 documentation contract: OK')
