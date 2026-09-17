# BLU2USB-G05 validation — canonical HID and source ownership

## Objective

Introduce the transport-independent canonical HID boundary and source-aware ownership/refcount aggregation required before Bluetooth adapters are implemented in G06+.

The G04 fixed USB identity, LCD layout and HAT interaction behavior remain unchanged.

## Canonical HID contract

- A source identity is `{kind, instance}` and distinguishes Mouse, Keyboard, Composite and Synthetic Remap output.
- Mouse canonical events are button, relative movement and wheel/pan.
- Keyboard canonical events are logical key and modifier press/release.
- Logical keyboard keys use USB HID Keyboard/Keypad Usage IDs only as stable internal key identities; transport report structure never enters the domain API.
- Up to 16 simultaneously tracked persistent ownership sources are supported.

## Ownership contract

- Persistent Mouse buttons, Keyboard keys and modifiers are owned per source.
- Aggregate output remains pressed while at least one source owns the logical control.
- Duplicate press/release from the same source is idempotent.
- Releasing/disconnecting one source removes only that source's persistent ownership.
- Synthetic Remap Keyboard ownership can overlap physical Keyboard ownership without causing an early release.
- Relative Mouse movement and wheel input are transient, accumulate across valid sources and are consumed separately from persistent ownership.
- Relative-only events do not consume persistent ownership slots.

## Automated scenarios

1. **G05-A01 — Source validation:** valid canonical source kinds are accepted; invalid/unknown kinds are rejected.
2. **G05-A02 — Shared Mouse ownership:** Mouse and Composite hold Left simultaneously; releasing either source leaves Left held until both release.
3. **G05-A03 — Mouse idempotence:** duplicate press/release from one source does not double-count or underflow ownership references.
4. **G05-A04 — Shared Keyboard key:** Keyboard and Composite hold the same key; source-selective release preserves the remaining owner.
5. **G05-A05 — Shared modifier:** Keyboard and Composite hold Left Shift; source-selective release preserves the remaining owner.
6. **G05-A06 — Disconnect isolation:** releasing one Mouse source clears only its unique buttons and preserves controls shared with another source.
7. **G05-A07 — Synthetic coexistence:** Synthetic Remap and physical Keyboard can hold Escape simultaneously; removing synthetic ownership does not release the physical key.
8. **G05-A08 — Relative aggregation:** movement and wheel deltas from multiple sources sum correctly, survive snapshot and are cleared only by take-output.
9. **G05-A09 — Source capacity:** all 16 persistent ownership slots can be used; a 17th persistent source is rejected while full.
10. **G05-A10 — Relative events while full:** a relative-only Mouse event still succeeds when all persistent ownership slots are occupied.
11. **G05-A11 — Slot reuse:** after one source is released, the freed slot can be reused by a new source.
12. **G05-A12 — Architecture boundary:** canonical HID and aggregator contain no BTstack, TinyUSB, Pico SDK/CYW43, GPIO/SPI, HID-host or transport report-layout dependencies.
13. **G05-A13 — Regression build:** all G01-G04 host tests remain green and Pico 2 W G05 still produces a non-empty production UF2.

## Physical validation

None required for G05. This gate introduces no new LCD, HAT, USB identity or end-user runtime behavior. The target Pico 2 W firmware is still cross-built as regression evidence, but flashing it is not an acceptance requirement for this gate.

## Gate close

G05 is accepted when all automated scenarios pass on the final G05 head and the production Pico 2 W UF2 is generated. Do not merge automatically.
