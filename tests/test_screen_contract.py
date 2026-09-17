#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
doc = (root / 'docs/ux/01-screen-layouts.md').read_text(encoding='utf-8')
visual = (root / 'docs/ux/00-interaction-visual-contract.md').read_text(encoding='utf-8')
blocks = re.findall(r'```text\n(.*?)\n```', doc, re.S)
assert blocks, 'no canonical text blocks found'
for block_text in blocks:
    rows = block_text.splitlines()
    assert len(rows) == 9, f'{rows[0] if rows else "<empty>"}: expected 9 rows, got {len(rows)}'
    for i, row in enumerate(rows, 1):
        assert len(row) <= 21, f'{rows[0]} row {i} exceeds 21 chars: {len(row)} {row!r}'

learn = next(b.splitlines() for b in blocks if b.splitlines()[0] == 'PRESS TO LEARN A KEY')
assert learn[1].index('J') == 6
assert [m.start() for m in re.finditer('JOY', learn[2])] == [0, 7, 14]
assert learn[3].index('LEFT') == 0 and learn[3].index('PRESS') == 6 and learn[3].index('RIGHT') == 13
assert learn[4].index('J') == 5
assert all(learn[i].index('KEY') == 15 for i in (5, 6, 7))
assert learn[6].index('L') == 0
assert learn[7].index('A') == 1
assert learn[8].index('O') == 2

pair_mouse = next(b.splitlines() for b in blocks if b.splitlines()[0] == 'PAIR MOUSE')
assert pair_mouse[6:] == ['KEY A: RETRY ON ERROR', 'KEY B: CANCEL', 'KEY X: HELP']

mouse_paired = next(b.splitlines() for b in blocks if b.splitlines()[0] == 'MOUSE PAIRED')
assert mouse_paired[1:3] == ['MOUSE CONNECTED', 'READY TO USE']
assert mouse_paired[7:] == ['KEY B: BACK', 'KEY Y: LOCK']

mouse_status_blocks = [b.splitlines() for b in blocks if b.splitlines()[0] == 'MOUSE STATUS']
assert any(rows[1] == 'MOUSE NOT CONNECTED' and rows[2] == 'PROFILE: PASSTHROUGH' for rows in mouse_status_blocks)
assert any(rows[1] == 'MOUSE CONNECTED' and rows[2] == 'PROFILE: DEFAULT' for rows in mouse_status_blocks)

pair_keyboard = next(b.splitlines() for b in blocks if b.splitlines()[0] == 'PAIR KEYBOARD')
assert pair_keyboard[1] == 'SEARCHING KEYBOARD'
assert 'BLE' not in pair_keyboard[1] and 'CLASSIC' not in pair_keyboard[1]

for title in ('LEFT WILL BECOME', 'RIGHT WILL BECOME', 'MIDDLE WILL BECOME', 'FORWARD WILL BECOME', 'BACKWARD WILL BECOME'):
    rows = next(b.splitlines() for b in blocks if b.splitlines()[0] == title)
    assert rows[1:7] == [' LEFT', ' RIGHT', ' MIDDLE', ' BACKWARD', ' FORWARD', ' ESCAPE']

def block(title):
    return next(b.splitlines() for b in blocks if b.splitlines()[0] == title)

default_apply = block('APPLY DEFAULT REMAP')
assert default_apply[1:5] == [
    'FORWARD IS LEFT',
    'LEFT IS FORWARD',
    'BACKWARD IS RIGHT',
    'RIGHT IS BACKWARD',
]
assert default_apply[6:] == ['KEY A: APPLY', 'KEY B: CANCEL', 'KEY Y: LOCK']

default_done = block('DEFAULT REMAP APPLIED')
assert default_done[1:5] == default_apply[1:5]
assert default_done[7:] == ['KEY B: BACK', 'KEY Y: LOCK']

escape_apply = block('APPLY ESCAPE')
assert escape_apply[1:6] == [
    'FORWARD IS LEFT',
    'BACKWARD IS RIGHT',
    'LEFT IS ESCAPE',
    'RIGHT IS BACKWARD',
    'MIDDLE IS FORWARD',
]
assert escape_apply[7:] == ['KEY A: APPLY', 'KEY B: CANCEL']

escape_done = block('ESCAPE APPLIED')
assert escape_done[1:6] == escape_apply[1:6]
assert escape_done[7:] == ['KEY B: BACK', 'KEY Y: LOCK']

passthrough_done = block('PASSTHROUGH APPLIED')
assert passthrough_done[7:] == ['KEY B: BACK', 'KEY Y: LOCK']

custom_done = block('CUSTOM APPLIED')
assert custom_done[7:] == ['KEY B: BACK', 'KEY Y: LOCK']
assert custom_done[1].startswith('LEFT IS ')

assert 'KEY C: HELP' not in doc
assert 'GO TO HOME' not in doc
assert 'current/applied/success/connected active state: cyan' in visual
assert 'Profile success pages are not an extra navigation level.' in visual
assert '`DEFAULT REMAP`: Forward→Left, Left→Forward, Backward→Right, Right→Backward, Middle→Middle;' in visual
assert '`ESCAPE REMAP`: Forward→Left, Backward→Right, Left→Escape, Right→Backward, Middle→Forward.' in visual
assert '`MOUSE STATUS` row 1 is exactly `MOUSE CONNECTED` and is cyan;' in visual
assert '`PAIR MOUSE` in `MOUSE OPTIONS` is cyan while unselected, and becomes white while selected;' in visual
assert 'Successful application opens a dedicated `CUSTOM APPLIED` page.' in visual

print('BLU2USB inherited screen/profile/live-connection contract: OK')