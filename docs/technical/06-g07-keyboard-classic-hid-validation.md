# BLU2USB-G07 — Keyboard transport facade and Bluetooth Classic HID

## Gate identity

This is the current **G07** for `blu2usb-RP2350`. The original roadmap placed Logitech Lift HID++ in G07 and Keyboard transport in G08. The Lift HID++ scope was completed and physically accepted as part of current G06, so the Keyboard transport gate advances to current G07.

Base is the exact physically accepted G06 SHA `7eee024ad4ee726c5a85ffa2f32b9f47187878af`.

The hardware target is Raspberry Pi Pico 2 W / RP2350 (`PICO_BOARD=pico2_w`), matching the physically proven BKB-3G POC.

## Scope

G07 adds a logical Keyboard transport facade and the first concrete adapter: Bluetooth Classic HID Host for the physically proven Goldentec GT T1 / BKB-3G family.

Product-facing semantics remain transport-neutral:

- the screen and application command are `PAIR KEYBOARD`;
- no user choice named Classic, BR/EDR or BLE is introduced;
- future BLE HOGP Keyboard support may sit behind the same facade without changing the USB, canonical HID or UX command contracts.

The fixed G04 USB identity remains authoritative. Remote keyboard descriptors/report IDs are never exposed to USB and Bluetooth changes never re-enumerate USB.

## Adapter contract

The Classic adapter:

- runs in the same single core-0 CYW43/BTstack runtime already accepted for BLE Mouse;
- performs Bluetooth Classic inquiry and remote-name resolution;
- recognizes the proven BKB-3G names `Bluetooth keyboard 3.0` and `BKB-3G`;
- accepts outgoing or incoming Classic HID Host connections;
- requests the exact `HID_PROTOCOL_MODE_REPORT` mode proven by the BKB-3G POC/integration;
- classifies the received HID descriptor as Keyboard only when Keyboard/Keypad usage page `0x07` is present;
- parses HID reports using BTstack's HID parser and emits only canonical Keyboard key/modifier events;
- uses source identity `BLU2USB_HID_SOURCE_KEYBOARD`;
- releases every held physical Keyboard key/modifier on disconnect or runtime overflow;
- contains no TinyUSB descriptor/report ownership and no UI code.

BLE and Classic are compiled into one shared BTstack runtime image. Protocol adapters receive the BTstack headers/feature defines but do not independently materialize duplicate HCI/L2CAP/base objects.

## BTstack coexistence sizing

The physically proven `picow-mouse-remapper` PICO-08 implementation established the minimum shape required for simultaneous BLE Mouse + Classic HID Keyboard. G07 must not regress to the earlier BLE-only limits.

The production `btstack_config.h` therefore freezes at least:

- `MAX_NR_HCI_CONNECTIONS 2` — one BLE Mouse ACL plus one Classic Keyboard ACL;
- `MAX_NR_HID_HOST_CONNECTIONS 1` — one Classic HID Host session;
- `MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES 2`;
- `NVM_NUM_LINK_KEYS 16` — Classic link keys must be storable instead of the previous zero-slot configuration;
- `MAX_NR_L2CAP_SERVICES 3`;
- Classic enhanced retransmission support and BLE+Classic cross-transport key derivation remain enabled when their respective transports are compiled.

These are runtime requirements, not build-only conveniences. A build that silently returns to one HCI connection or zero Classic link-key slots is a G07 regression even if CI still compiles.

## Shared-radio discovery arbitration

The CYW43 is shared by BLE HOGP Mouse and BR/EDR Classic HID Keyboard. `PAIR KEYBOARD` therefore has explicit discovery priority while the Classic transaction is active:

- if BLE is only scanning for a Mouse, that LE scan is stopped before Classic inquiry begins;
- if BLE is attempting an outgoing bonded reconnect, that initiation is cancelled and Classic waits until the controller is quiescent;
- an already-ready BLE Mouse session is **not disconnected**; the Classic inquiry/connection runs alongside the existing Mouse ACL, as proven by the original PICO-08 coexistence implementation;
- if `gap_inquiry_start()` temporarily reports controller busy, the pairing transaction remains active and automatically retries from the BTstack run-loop every 50 ms instead of consuming the request and freezing the UI on `SEARCHING KEYBOARD`;
- after Keyboard success or cancellation, BLE discovery/reconnect resumes automatically when no Mouse is already ready;
- HCI disconnect events are ownership-filtered: a Classic Keyboard disconnect must never be interpreted as the BLE Mouse disconnect, and vice versa.

The application layer remains transport-neutral and does not call raw GAP primitives.

## Pairing contract without terminal

Physical acceptance must not require UART, serial console or USB CDC.

The proven BKB-3G can use either:

- SSP passkey notification: a six-digit PIN must be typed on the BKB-3G followed by Enter; or
- legacy PIN fallback: `0000`.

When BTstack provides a pairing code, `PAIR KEYBOARD` dynamically replaces its search body with:

```text
PAIR KEYBOARD
TYPE PIN ON KEYBOARD
PIN: 123456
THEN PRESS ENTER
PAIRING IN PROGRESS

KEY A: RETRY ON ERROR
KEY B: CANCEL
KEY X: HELP
```

`123456` is only an example. The live PIN is cyan. The wording remains transport-neutral.

## Canonical ownership / fixed USB

Physical Keyboard input and synthetic remap-generated Escape share the existing canonical ownership aggregator.

Therefore:

- physical Keyboard key release cannot release a synthetic key owned by remap;
- synthetic Escape release cannot release the same key while a physical Keyboard still owns it;
- disconnect of Keyboard releases only the physical Keyboard source;
- Mouse/remap ownership remains active through Keyboard disconnect;
- USB Keyboard remains the same fixed interface 1 from boot;
- no `tud_disconnect()` / `tud_connect()` or descriptor swap is permitted.

## UX live state

While Keyboard is connected:

- `OTHER DEVICES STATUS` shows `KEYBOARD` / `CONNECTED`, with `CONNECTED` cyan;
- `PAIR KEYBOARD` in `OTHER OPTIONS` is cyan while unselected and white while selected;
- successful pairing opens `KEYBOARD SAVED`, whose positive body is cyan;
- `KEY B` from `KEYBOARD SAVED` returns directly to `OTHER OPTIONS`, never back into the pairing transaction.

When Keyboard disconnects, status returns to `NOT CONNECTED` in ordinary status color and the `PAIR KEYBOARD` current marker is removed.

G07 does not require the later persistent saved-device registry or autonomous preferred Keyboard reconnect across Pico power cycles. Those remain later persistence/coordinator work. Reconnection within the running firmware and a new Pair Keyboard transaction must remain usable.

## Physical acceptance scenarios

### G07-01 — G06 regression and fixed identity

Flash the final G07 UF2 and power-cycle. Confirm Learn/HAT behavior, the previously accepted Mouse connection/remap path and fixed `BLU2USB Mouse + Keyboard` USB identity remain functional. Bluetooth activity must not cause USB re-enumeration.

### G07-02 — Pair BKB-3G with Mouse absent

Keep the BLE Mouse powered off/unavailable. Navigate `HOME -> OTHER OPTIONS -> PAIR KEYBOARD`. Put the BKB-3G in pairing mode on the desired channel (`FN+1`, `FN+2` or `FN+3` until its pairing LED blinks).

Expected: BLE discovery is quiesced and the Pico repeatedly maintains a real Classic inquiry until the BKB-3G is found or the operator cancels. The screen must not remain falsely frozen on `SEARCHING KEYBOARD` after an inquiry-start failure. If a PIN appears on the LCD, type that exact PIN on the BKB-3G and press Enter. Pairing ends on `KEYBOARD SAVED` with positive body cyan. Press `KEY B` once and confirm the UI returns directly to `OTHER OPTIONS`.

### G07-03 — Pair BKB-3G while Mouse is already connected

First connect the accepted BLE Mouse and verify normal movement. Then enter `PAIR KEYBOARD` and put the BKB-3G into pairing mode.

Expected: the existing Mouse connection is preserved while Classic inquiry/pairing proceeds. Keyboard reaches `KEYBOARD SAVED`; Mouse remains usable before, during and after Keyboard pairing.

### G07-04 — Representative typing

Open an ordinary host text editor/input. Verify letters, numbers, Space, Enter and Backspace. No terminal/serial observation is part of acceptance.

### G07-05 — Modifiers and hold/release

Verify at least Shift+letter and Ctrl/Alt combination in an ordinary host GUI. Hold a normal key and release it; no key may remain stuck.

### G07-06 — Physical Keyboard + synthetic Escape coexistence

Use a Mouse profile/Custom mapping that emits synthetic Escape. While a physical Keyboard key is held, trigger and release synthetic Escape, and reverse the order. Where practical, hold physical Escape and trigger mapped Escape so both own the same target.

Expected: releasing either owner alone never prematurely releases the other.

### G07-07 — Keyboard disconnect while held / Mouse ownership isolation

With Mouse and Keyboard connected, hold a Keyboard key and turn off/disconnect the BKB-3G.

Expected: host Keyboard state is released immediately; no stuck key remains. The BLE Mouse must remain logically connected and usable. A Classic disconnect event must not clear the Mouse connection state or current Mouse profile.

### G07-08 — Pair/reconnect recovery in the same boot

After a Keyboard disconnect, return to `PAIR KEYBOARD` and make the keyboard available again. The UI/HAT must remain responsive and the Keyboard must become usable again. Autonomous persisted Keyboard reconnect after a Pico power cycle is not required by G07.

### G07-09 — Lock while Keyboard is active

Lock the LCD with Key Y while Keyboard and Mouse are connected. Type and use the Mouse while locked.

Expected: forwarding continues. The first HAT interaction only unlocks and returns HOME; it does not execute its normal action.

### G07-10 — Keyboard live UX colors and cancellation/help responsiveness

With the Keyboard connected, verify `OTHER DEVICES STATUS` reports `CONNECTED` cyan. In `OTHER OPTIONS`, `PAIR KEYBOARD` is cyan while unselected, white while selected, and returns to cyan after selection moves away.

Then disconnect the Keyboard, start `PAIR KEYBOARD` with the target unavailable, open/close Help and cancel with Key B. HAT remains responsive, BLE Mouse discovery/reconnect resumes after cancellation, and cancellation returns one logical page without freezing Mouse/USB forwarding.

## Gate close

G07 is accepted only when final-head CI is green and all applicable G07-01 through G07-10 scenarios pass on the exact production UF2. Keep its PR draft and do not merge automatically.
