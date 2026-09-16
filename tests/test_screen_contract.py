#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
doc = (root / 'docs/ux/01-screen-layouts.md').read_text(encoding='utf-8')
blocks = re.findall(r'```text\n(.*?)\n```', doc, re.S)
assert blocks, 'no canonical text blocks found'
for block in blocks:
    rows = block.splitlines()
    assert len(rows) == 9, f'{rows[0] if rows else "<empty>"}: expected 9 rows, got {len(rows)}'
    for i, row in enumerate(rows, 1):
        assert len(row) <= 21, f'{rows[0]} row {i} exceeds 21 chars: {len(row)} {row!r}'

learn = next(b.splitlines() for b in blocks if b.splitlines()[0] == 'PRESS TO LEAR A KEY')
assert learn[1].index('J') == 6
assert [m.start() for m in re.finditer('JOY', learn[2])] == [0, 7, 14]
assert learn[3].index('LEFT') == 0 and learn[3].index('PRESS') == 6 and learn[3].index('RIGHT') == 13
assert learn[4].index('J') == 5
assert all(learn[i].index('K') == 14 for i in (5,6,7))
assert learn[6].index('L') == 0
assert learn[7].index('A') == 1
assert learn[8].index('O') == 2

pair_keyboard = next(b.splitlines() for b in blocks if b.splitlines()[0] == 'PAIR KEYBOARD')
assert pair_keyboard[1] == 'SEARCHING KEYBOARD'
assert 'BLE' not in pair_keyboard[1] and 'CLASSIC' not in pair_keyboard[1]

for title in ('LEFT WILL BECOME','RIGHT WILL BECOME','MIDDLE WILL BECOME','FORWARD WILL BECOME','BACKWARD WILL BECOME'):
    rows = next(b.splitlines() for b in blocks if b.splitlines()[0] == title)
    assert rows[1:7] == [' LEFT',' RIGHT',' MIDDLE',' BACKWARD',' FORWARD',' ESCAPE']

print('BLU2USB-G02 screen contract: OK')
