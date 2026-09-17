# BLU2USB-G05 validation — BLE HOGP Mouse passthrough

## Objective

Implement one live BLE HOGP Mouse path on Pico 2 W while preserving the fixed USB Mouse + Keyboard identity from G04 and the accepted LCD/HAT interaction behavior from G03.

Remote HID report layouts remain confined to the BLE adapter. The application receives only firmware-owned canonical Mouse events, which pass through source-aware ownership aggregation before fixed USB Mouse reports are submitted.

## Runtime contract

- CYW43/BTstack is initialized on core 0 and serviced with `pico_cyw43_arch_threadsafe_background`; no dedicated Core1 Bluetooth owner is used.
- BLE scanning runs without blocking USB servicing or HAT/UI input.
- The Mouse path searches BLE advertisements exposing the HID service and rejects explicit non-Mouse HID appearances.
- Security uses bonding/Secure Connections with Just Works confirmation where applicable.
- HIDS Client uses Report Protocol and obtains the peer Report Map before accepting the device as a Mouse.
- A candidate whose Report Map does not contain a Mouse Application collection is rejected and scanning resumes.
- The parser recognizes Mouse buttons, relative X/Y, vertical wheel and Consumer AC Pan horizontal wheel.
- BTstack/HIDS reports are accepted either in canonical descriptor-sized form or with exactly one duplicated Report ID byte. Any incompatible framing is rejected rather than interpreted with shifted fields.
- Connection, security, HIDS setup and report-parse failures disconnect/rescan instead of trapping the main loop.
- Disconnect or runtime-queue overflow releases persistent ownership for the BLE Mouse source so a host button cannot remain stuck.
- Relative motion is transmitted to USB in bounded `int8` chunks and consumed only after TinyUSB accepts that report.
- Bluetooth state never changes the G04 USB descriptor and never calls forced USB disconnect/reconnect.

## Canonical ownership carried into G05

The canonical HID foundation expected before this transport gate is materialized here as well:

- source identities distinguish Mouse, Keyboard, Composite and Synthetic Remap;
- persistent Mouse buttons, Keyboard keys and modifiers are owned per source;
- aggregate state stays held until the last owner releases;
- duplicate press/release is idempotent;
- relative motion/wheel remains transient;
- up to 16 persistent ownership sources are tracked.

## Automated acceptance

CI must prove at least:

1. canonical source validation and per-source ownership/refcount semantics;
2. shared Mouse/Keyboard/modifier ownership and source-selective disconnect release;
3. synthetic Keyboard ownership can coexist with physical Keyboard ownership;
4. relative accumulation, bounded partial consumption and ownership-slot reuse;
5. a composite HID Report Map yields only Mouse events from its Mouse report while its Keyboard report is ignored by the Mouse adapter;
6. Left/Forward button transitions, signed X/Y, wheel and horizontal pan decode correctly;
7. repeated held reports do not create duplicate button ownership transitions;
8. descriptor-sized and duplicated-Report-ID HIDS framing normalize to the same canonical payload;
9. malformed/truncated framing is rejected;
10. a Keyboard-only HID Report Map is rejected as a Mouse candidate;
11. the bounded Bluetooth runtime queue detects overflow;
12. canonical HID modules remain free of BTstack, TinyUSB, Pico SDK and remote report-layout dependencies;
13. BTstack/CYW43 stays inside the approved adapter/runtime modules and the application contains no raw transport/HAL primitives;
14. the G05 build uses `pico_cyw43_arch_threadsafe_background` and contains no Core1 launch path;
15. no forced TinyUSB reconnect path exists and the fixed two-HID USB descriptor from G04 remains unchanged;
16. all prior G02-G04 host regressions pass;
17. Pico 2 W production cross-build produces a non-empty UF2.

## Physical scenarios

Use the Pico 2 W, Waveshare HAT/LCD, a BLE HOGP Mouse and the normal graphical host OS. No serial terminal is required.

1. **G05-01 — Boot and G04 regression.** Flash G05 and power-cycle. `PRESS TO LEARN A KEY` remains the first LCD page, the HAT remains usable, and the host still exposes the fixed BLU2USB Mouse + Keyboard USB functions before any Bluetooth peer connects.
2. **G05-02 — BLE Mouse discovery/pair.** Put one BLE HOGP Mouse into pairing/discoverable mode after boot. The firmware should discover, secure and accept it automatically. After pairing, moving the physical Mouse must move the host pointer through the BLU2USB USB Mouse function.
3. **G05-03 — X/Y motion and no ghost movement.** Move the BLE Mouse left, right, up, down and diagonally. Direction must be correct and the host pointer must stop when the physical Mouse stops; no persistent drift or one-byte Report-ID shift is allowed.
4. **G05-04 — Left/Right/Middle hold and drag.** Test Left, Right and Middle as click/release. Then hold Left while moving the Mouse and perform a drag. Each button must remain held for the complete physical hold and release immediately when the physical button is released.
5. **G05-05 — Wheel and horizontal pan.** Test vertical wheel. If the test Mouse exposes standard HID horizontal pan, test it too. Unsupported horizontal pan on the physical Mouse is not a failure.
6. **G05-06 — Standard Back/Forward buttons.** If the Mouse exposes Back/Forward as ordinary HID buttons, verify both press/release paths. A vendor-specific Logitech Forward behavior that requires HID++ is outside this gate and is handled in the later HID++ gate.
7. **G05-07 — Disconnect while held.** Hold a Mouse button, then power off/disconnect the BLE Mouse before releasing it. The host must not remain with that button stuck. The firmware must return to scanning without freezing USB or the UI.
8. **G05-08 — Reconnection/rescan usability.** After a disconnect, put the same Mouse back into an advertising/connectable state. The runtime must remain usable and resume a valid Mouse connection without requiring a Pico reset. Product-level preferred-device persistence is not required in G05.
9. **G05-09 — HAT responsiveness under Bluetooth activity.** During scanning, pairing, connection and after disconnect, exercise Joy Up/Down/Press and normal Back navigation. HAT actions must remain responsive and release-triggered; Bluetooth work must not visibly freeze the LCD/UI.
10. **G05-10 — Stable USB identity throughout Bluetooth state changes.** Observe the host while the BLE Mouse connects, disconnects and reconnects. The BLU2USB USB device must remain the same fixed Mouse + Keyboard identity; there must be no USB disappearance/re-enumeration caused by Bluetooth state.
11. **G05-11 — Lock does not stop Mouse forwarding.** With the BLE Mouse connected, lock the LCD with Key Y. Move/click the BLE Mouse while the display is locked. USB Mouse forwarding must continue. Unlock with one complete HAT interaction and confirm that interaction is consumed exactly as in earlier gates.
12. **G05-12 — Concurrent UI + Mouse stress.** While continuously moving the BLE Mouse and occasionally scrolling/clicking, navigate several LCD pages. Pointer input and HAT/UI behavior must remain responsive, with no stuck button, ghost motion or firmware freeze.

## Gate close

Do not close BLU2USB-G05 until automated CI is green on the final head and all applicable physical scenarios G05-01 through G05-12 pass on the target Pico 2 W. Do not merge automatically.
