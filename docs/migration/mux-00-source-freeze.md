# MUX-00 — Mouse UI Layout 1.0 source freeze

Status: **IMPLEMENTED — HUMAN ACCEPTANCE PENDING**

## Baselines

BLU2USB implementation baseline:

- repository: `remappingbridge/blu2usb`
- accepted gate: **G06**
- commit: `7eee024ad4ee726c5a85ffa2f32b9f47187878af`

Mouse UX behavior baseline:

- repository: `remappingbridge/mouse-ui`
- product baseline: **UI Layout 1.0 — FROZEN / ACCEPTED**
- inspected/frozen commit: `5f269e9625ae0d02a85b5d39eb87026edc448068`

The abandoned branch `experimental/g06-mouse-ux-v1` is not a source of code or contract authority.

## Frozen mouse-ui inputs

The following blobs are pinned as evidence for this migration:

| Path | Blob SHA | Role |
|---|---|---|
| `docs/product/ui-layout-v1.0.md` | `1923ad7c39e6c0b094bf39c8d41b810eb071c993` | business rules |
| `docs/spec/01-screen-reference.md` | `0a0705d429b4c40d5e10d9fc083160f1f46b15a9` | screen behavior/reference |
| `docs/spec/02-controls-lock-help.md` | `e5f134efa2251a157ea002c408b003a0cd8fa401` | controls, Back, Lock, Help |
| `docs/spec/03-first-start-and-pairing.md` | `79262d9891159919530537fbf0e7a4f0f5dc11eb` | FIRST/SAVED/NEW lifecycle |
| `docs/spec/04-home-and-connection.md` | `4da13e6e4ff8215e3d31362405e9991f63873185` | HOME resolver/live Mouse |
| `docs/spec/05-remapping.md` | `dd32978e782bf6623a639f68e694c0faed147c68` | profile/Custom semantics |
| `docs/spec/06-saved-devices.md` | `b55c32dff9b4068ae1426c26d7a9d08ccefa7b49` | saved registry/removal |
| `src/projector/screens.c` | `0cecfd8e802047673cb6d2d4b866c11bd6137b45` | executable 30-screen templates |
| `tests/goldens/mui-04/screens.txt` | `b09d727073cedda027ce0ecd7c44fc84aab6306d` | accepted literal rows |
| `tests/test_screen_projection.c` | `26791d309b92bd71f794a1c64aebc8eedfd54ddf` | 30-screen projection acceptance |
| `tests/test_ux_regressions.c` | `a8b79db2758027c5e6947e307754394ac1df28c2` | accepted UX regression behavior |

## Precedence used by BLU2USB

Mouse UI Layout 1.0 contains both prose and executable acceptance material. BLU2USB freezes the following precedence so migration never silently guesses:

1. **Literal screen rows and screen inventory:** accepted executable golden `tests/goldens/mui-04/screens.txt`, corroborated by `src/projector/screens.c` and `test_screen_projection.c`.
2. **Business behavior and state transitions:** `docs/product/ui-layout-v1.0.md` plus the six active spec documents.
3. **Ambiguous behavioral edge cases:** accepted executable regression tests at the frozen commit.
4. **Dynamic example values:** are examples only; names, page counters, connection status, profile and Custom rows are projected from product state.

This precedence is local to the BLU2USB migration. It does not transfer ownership of mouse-ui code into this repository.

## Discrepancy review

A mechanical comparison of the 30 golden screen rows against the first literal block in each corresponding screen section found four apparent mismatches.

Three are intentional dynamic examples:

- `home-connected` title: prose example `LOGITECH LIFT` vs golden projected `LIFT MOUSE`;
- `saved-devices`: page/name/status values differ because they are dynamic;
- `remove-this`: Mouse name is dynamic.

One is a true literal documentation/executable mismatch:

### `help-home-connected`

The accepted executable baseline is:

```text
REMOVE CONNECTED HELP
TO DISCONNECT THE
CURRENTLY CONNECTED
MOUSE NAVIGATE TO:
STEP 1. SAVED DEVICES
STEP 2. REMOVE DEVICE
STEP 3. KEY A: REMOVE

ANY KEY: BACK
```

The prose screen reference at the same commit contains different wording. For MUX implementation the block above is frozen because it is the version enforced by the accepted golden and screen-projection test.

No other literal mismatch was found in the 30-screen inventory after excluding documented dynamic fields.

## What is copied and what is not

BLU2USB consumes **behavior**, not mouse-ui implementation.

May be reexpressed natively:

- screen identities and literal rows;
- navigation/interaction rules;
- semantic colors and dynamic projection rules;
- FIRST/SAVED/NEW timing and intent;
- saved-Mouse/product invariants;
- profile terminology and visible mappings;
- operation cancellation/stale-result requirements.

Must not be imported as architecture:

- SDL2 platform code;
- mouse-ui private structs;
- mock world;
- desktop lab/scenario implementation;
- projector/renderer C source by copy;
- `mouse-core`;
- UI-Core C ABI/header;
- `ui-core-contract.json`;
- contract adapter code;
- repository-to-repository runtime dependency.

The future embedded implementation remains native to BLU2USB module boundaries.
