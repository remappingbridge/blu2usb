"""Check the final build composition, not just CMake source tokens."""
import collections
import hashlib
import json
import pathlib
import subprocess
import sys

root = pathlib.Path(__file__).resolve().parents[1]
build = pathlib.Path(sys.argv[1]).resolve()
commands = json.loads((build / 'compile_commands.json').read_text())
for name in ('hci.c', 'l2cap.c', 'btstack_memory.c', 'btstack_cyw43.c', 'hid_host.c'):
    entries = [c for c in commands if pathlib.Path(c['file']).name == name]
    assert len(entries) == 1, (name, len(entries))
    command = entries[0]['command']
    assert '-DENABLE_BLE=1' in command and '-DENABLE_CLASSIC=1' in command, name
for name in ('bt_runtime_pico.c', 'ble_hogp_pico.c', 'classic_probe_pico.c'):
    entries = [c for c in commands if pathlib.Path(c['file']).name == name]
    assert len(entries) == 1, name
    assert all(flag in entries[0]['command'] for flag in ('-DENABLE_BLE=1','-DENABLE_CLASSIC=1')), name
flash_entries = [c for c in commands if c['file'].endswith('/pico_flash/flash.c')]
assert len(flash_entries) == 1, ('flash.c variants', len(flash_entries))
assert '-DLIB_PICO_MULTICORE=1' in flash_entries[0]['command']
cache = (build / 'CMakeCache.txt').read_text()
assert 'PICO_BOARD:STRING=pico2_w' in cache
elf = build / 'blu2usb_picow.elf'
nm = subprocess.check_output(['arm-none-eabi-nm', '--defined-only', str(elf)], text=True)
symbols = collections.Counter(line.split()[-1] for line in nm.splitlines() if line.split())
for name in ('hci_init','l2cap_init','btstack_cyw43_init','hid_host_init',
             'blu2usb_classic_probe_setup','blu2usb_ble_hogp_session_setup',
             'flash_safe_execute','multicore_lockout_victim_init'):
    assert symbols[name] == 1, (name, symbols[name])
uf2 = build / 'blu2usb_picow.uf2'
data = uf2.read_bytes()
assert data and len(data) % 512 == 0
sha = subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
manifest = dict(stage='G07 experiment A - connection only; physical acceptance pending',
                commit=sha, board='pico2_w', mcu='RP2350', pico_sdk='2.2.0',
                btstack='501e6d2b86e6c92bfb9c390bcf55709938e25ac1',
                compiler=subprocess.check_output(['arm-none-eabi-gcc','--version'],text=True).splitlines()[0],
                file=uf2.name, size=len(data), sha256=hashlib.sha256(data).hexdigest(),
                shared_stack_objects='one compilation each; both transport defines',
                elf_symbols='one definition for each required bootstrap/stack entrypoint')
(build / 'g07-experiment-a-evidence.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest,indent=2))
