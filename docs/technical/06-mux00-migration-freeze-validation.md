# MUX-00 acceptance record

Status: **IMPLEMENTED — HUMAN ACCEPTANCE PENDING**

Branch: `gate/mux-00-migration-contract-freeze`

Base planning branch: `planning/mouse-ui-v1-native-rebuild`

Implementation ancestry ultimately starts from accepted G06:
`7eee024ad4ee726c5a85ffa2f32b9f47187878af`.

## Deliverables completed

- [x] exact G06 implementation baseline frozen;
- [x] exact Mouse UI Layout 1.0 commit frozen;
- [x] source/blob provenance recorded;
- [x] precedence between executable literals and narrative behavior frozen;
- [x] all 30 canonical screen IDs inventoried;
- [x] all 36 G06 screen concepts mapped to new screen/fold/remove decisions;
- [x] Default -> Standard visible terminology frozen;
- [x] FIRST/SAVED/NEW search purposes and timings frozen;
- [x] one-authoritative-Mouse invariant frozen;
- [x] saved registry capacity 16 frozen;
- [x] per-Mouse confirmed profile + global Custom relationship frozen;
- [x] Back/Lock/Help exceptions frozen;
- [x] Keyboard/Composite Mouse-UI scope removal frozen;
- [x] G01-G06 technical inheritance recorded;
- [x] mouse-ui/mouse-core/UI-Core implementation dependency explicitly prohibited;
- [x] Pair New live+candidate physical feasibility deferred explicitly to MUX-05 stop gate;
- [x] literal discrepancy in `help-home-connected` resolved by accepted executable golden precedence.

## Verification performed

The frozen `mouse-ui` source at commit
`5f269e9625ae0d02a85b5d39eb87026edc448068` was inspected directly.

The executable screen table contains exactly **30** entries.

A row-by-row comparison between `docs/spec/01-screen-reference.md` screen blocks and
`tests/goldens/mui-04/screens.txt` found:

- dynamic-example differences on `home-connected`, `saved-devices`, and `remove-this`;
- one true literal conflict on `help-home-connected`, resolved in favor of the accepted golden/tested implementation;
- no other literal conflict among the 30 screens.

The accepted mouse-ui product model at the frozen commit confirms:

- `MUI_SAVED_MOUSE_CAPACITY = 16`;
- FIRST = 8000 ms;
- SAVED = 8000 ms;
- Pair New = 15000 ms;
- profile kinds Passthrough, Standard, Escape, Custom;
- distinct search/operation token fields.

No production firmware file is intentionally changed by MUX-00.

## Gate close condition

MUX-00 requires human acceptance because it changes the normative Mouse-facing product contract for all following gates.

After acceptance, MUX-01 must branch from the exact accepted MUX-00 SHA and implement only the host-pure registry/product snapshot. BLE transport changes remain prohibited until their planned gates.
