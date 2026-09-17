"""Pin vendored production source to the exact inspected historical bytes."""
import hashlib
import json
from pathlib import Path
root = Path(__file__).resolve().parents[1]
p = root / 'src/classic_hid/pico08'
manifest = json.loads((p / 'provenance.json').read_text())
assert manifest['commit'] == '0d917e58e73acf4333d7bc773186f97898ee3ffa'
for name, sha in manifest['sha256'].items():
    assert hashlib.sha256((p / name).read_bytes()).hexdigest() == sha, name
print('PICO-08 original source identity: PASS')
