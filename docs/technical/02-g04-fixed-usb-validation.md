# BLU2USB-G04 validation — fixed USB Mouse + Keyboard identity

## Objective

G04 materializes the canonical `usb_hid` module. From boot, the Pico 2 W exposes one firmware-owned USB device with two fixed HID interfaces: Mouse and Keyboard. The descriptor is independent from Bluetooth topology, pairing state, profiles, lock state and future device discovery.

This gate intentionally does not add Bluetooth input forwarding yet. It establishes the stable USB-facing contract that later gates must feed without re-enumerating the host device.

## Fixed identity

- VID: `0xCAFE`;
- PID: `0x4010`;
- device revision: `0x0100`;
- interface 0: HID Mouse;
- interface 1: HID Keyboard;
- manufacturer string: `BLU2USB`;
- product string: `BLU2USB Mouse + Keyboard`;
- no CDC, MSC, MIDI or vendor interface;
- no runtime `tud_disconnect()` / `tud_connect()` path.

The Keyboard interface exists even with no Bluetooth Keyboard connected because later remap gates need firmware-generated keyboard events such as Escape.

## Automated acceptance

CI must prove:

1. `usb_hid` is a concrete canonical module and is the only product module allowed to own TinyUSB device APIs/configuration.
2. TinyUSB is configured for exactly two HID interfaces and no debug/CDC USB interface.
3. Mouse and Keyboard descriptor identity is compile-time fixed and has no Bluetooth/runtime dependency.
4. No forced USB disconnect/reconnect path exists.
5. The canonical Mouse report remains five bytes: buttons, X, Y, wheel and pan.
6. The canonical Keyboard report remains eight bytes: modifiers, reserved byte and six keycodes.
7. Mouse and Keyboard reports can be constructed in host tests without any Bluetooth peer.
8. All G02/G03 interaction, screen and renderer tests continue to pass.
9. Pico SDK 2.2.0 cross-build for `pico2_w` produces a non-empty production UF2.

## Physical acceptance

Use the normal graphical device/input UI of the host OS plus the LCD/HAT. Serial output is neither required nor accepted as gate evidence.

1. **G04-01 — Boot/UI regression** — flash the G04 UF2 and power-cycle. The first LCD screen must be `PRESS TO LEARN A KEY`; the G03 HAT interaction remains functional.
2. **G04-02 — Corrected Learn title** — confirm the title is exactly `PRESS TO LEARN A KEY`.
3. **G04-03 — Corrected Help key** — navigate to every pairing page that exposes Help and confirm the visible label is `KEY X: HELP`; no `KEY C: HELP` label may exist.
4. **G04-04 — Fixed Mouse function** — with no Bluetooth peer connected, inspect the host OS device/input UI and confirm BLU2USB exposes a Mouse HID function.
5. **G04-05 — Fixed Keyboard function** — with no Bluetooth peer connected, confirm BLU2USB also exposes a Keyboard HID function at the same time.
6. **G04-06 — Stable reconnect identity** — unplug and reconnect the Pico 2 W normally. Confirm the same BLU2USB Mouse + Keyboard identity returns; no alternate USB personality appears.
7. **G04-07 — Keyboard without peer** — with no Bluetooth Keyboard paired or connected, confirm the USB Keyboard function remains present.
8. **G04-08 — Mouse without peer** — with no Bluetooth Mouse paired or connected, confirm the USB Mouse function remains present.
9. **G04-09 — Interaction regression** — on HOME and another menu, hold and release Joy Up/Down/Press. Selection/action must still execute only on release.
10. **G04-10 — Lock regression** — lock with Key Y release, then unlock with one complete HAT interaction. The first interaction is consumed and HOME returns normally.

## Gate boundary

G04 does not yet forward live Bluetooth HID reports. Therefore a Bluetooth connect/disconnect re-enumeration test is deferred until the first gate that enables a real Bluetooth peer. At that gate the fixed USB identity must remain unchanged while peers connect and disconnect.

G04 is physically accepted only when every applicable scenario above passes on the target Pico 2 W and host OS.
