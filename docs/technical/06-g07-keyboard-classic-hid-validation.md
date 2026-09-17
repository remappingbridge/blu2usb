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
- if `gap_inquiry_start()` temporarily reports controller busy, the pairing transaction remains active and services the transaction every 50 ms, retries rejected submissions after a 1 s backoff, and reports an error after three consecutive rejections instead of freezing the UI on `SEARCHING KEYBOARD`;
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

## 2026-09-17 investigation: distinguish software proof from physical cause

Inspected development HEAD: `9a9d96fe2ec008e786cefaceb5b5660434d7fd75`.
The only commit after the physically failing `a3cc04c9fd5f169e7d4401479a1ed1faa67fae82`
at investigation start changed memory limits; it did not add a state watchdog.
Reference: `picow-mouse-remapper`, branch `feat/pico-08-classic-keyboard-integration`,
SHA `0d917e58e73acf4333d7bc773186f97898ee3ffa`.
SDK: Pico SDK 2.2.0 (`a1438dff1d38bd9c65dbd693f0e5db4b9ae91779`),
BTstack submodule `501e6d2b86e6c92bfb9c390bcf55709938e25ac1`.

### Proven defect and evidence boundary

`gap_inquiry_start()` returning success means the host accepted/enqueued the
request. It does **not** mean the controller accepted HCI Inquiry. In the pinned
BTstack `src/hci.c`, `hci_handle_command_status()` handles a rejected
`HCI_OPCODE_HCI_INQUIRY` by returning its private inquiry state to IDLE, without
emitting `GAP_EVENT_INQUIRY_COMPLETE`. The old adapter ignored Command Status,
remained in `CLASSIC_HID_STATE_INQUIRY`, and its timer only retried in IDLE.
This is a deterministic software deadlock, reproduced by compiling the original
production adapter into the new event-driven host harness. It fails the
`inquiries == 2` assertion after injecting successful API submission followed by
controller status `0x0c`. The corrected adapter passes the same scenario.

This does **not** prove that the physical BKB-3G run actually received status
`0x0c`, or that this is its only cause. No controller capture or new physical test
was available during code investigation. The next UF2 is a correction candidate,
not physical acceptance. The old PR description's radio-contention explanation
was not established as the physical root cause by the failed firmware.

Memory sizing, report-mode selection and BLE arbitration cannot repair this
host/controller state divergence. The earlier retry inspected only the immediate
API return; it could not see an asynchronous rejection. No further memory sizing
or multicore change is made by this correction.

Additional demonstrated code defects corrected:

- Pair requests consumed while `g_stack_working` was false are now retained as an
  active transaction until WORKING or a bounded startup failure.
- Cancellation changes application state before calling `gap_inquiry_stop()`;
  that SDK function can synchronously emit completion for a queued request.
- A queued inquiry is not treated as active/cancelable until controller ACK;
  late ACK after cancellation triggers cleanup, not name discovery.
- Late name/descriptor events cannot complete an aborted pair transaction;
  name requests are correlated with their pending address and HID events with CID.
- Startup session setup and power-on are serialized under the existing async
  context lock. The shared core-0 architecture is retained.

### Full boot-to-keyboard trace and comparison with the working reference

| Stage | Current execution / old reference comparison |
|---|---|
| 1–3: boot and facade | `main` registers vendor backend, then `keyboard_transport_pico_start` registers `classic_hid_session_setup`; runtime queue reset does not clear this registration. Old code called `classic_keyboard_core1_init` from `btstack_main`. |
| 4–6: runtime | `ble_hogp_start` calls `bt_runtime_start`; `cyw43_arch_init` initializes memory, async run loop, HCI transport and TLV via SDK `btstack_cyw43_init`. Old `picow_bt_example_init` performs the same SDK initialization. |
| 7–8: services | Both paths call `l2cap_init`, `sm_init`, GATT client, ATT server and HIDS client. No separate SDP server/RFCOMM initialization is required for outgoing HID-host SDP queries. Old BLE IO capability was DISPLAY_ONLY; current BLE NO_INPUT_NO_OUTPUT preserves accepted Mouse behavior. Classic separately selects DISPLAY_ONLY in both. |
| 9–11: registration/power | BLE setup then all registered setups execute before `hci_power_control`. Classic calls `hid_host_init` and registers both HID and HCI handlers before power-on. Old registers Classic before BLE HCI handler. Both are registered before WORKING; no callback removal occurs. New critical section excludes background execution during this sequence. |
| 12–14: readiness/commands | WORKING moves Classic to IDLE. A 50 ms async timer consumes atomic requests from the HAT release path. Early pair requests are retained. Old reference uses the same timer pattern with a critical-section mailbox. |
| 15: arbitration | Current code pauses only Mouse discovery/reconnect. READY Mouse ACL is preserved. Old implementation lacks this explicit arbitration; coexistence alone does not prove arbitration was the cause. |
| 16–18: inquiry | Same 5 × 1.28 s inquiry and RSSI/EIR mode. Submission and controller ACK are now distinct product phases. Command Status errors retry after 1 s, at most three consecutive failures; results/completion are delivered to the registered HCI handler. Healthy empty inquiries continue searching. |
| 19–20: names | Same stored address, page-scan repetition mode, clock offset with valid bit and exact target names. Only the outstanding address can finish name resolution. Missing name completion reaches explicit error after 10 s. |
| 21–22: HID/authentication | Same outgoing `hid_host_connect(..., HID_PROTOCOL_MODE_REPORT, ...)` and incoming accept path. Same sniff/role-switch default policy, master role, discoverable setting, DISPLAY_ONLY SSP and legacy `0000`. PIN is shown on LCD; no terminal required. |
| 23–25: descriptor/input | Both wait for HID descriptor and parse reports. Current adapter validates Keyboard usage page, then publishes CONNECTED and canonical physical Keyboard events through the runtime/facade. UI transitions to SAVED; aggregator retains separate synthetic Escape ownership. Late events from cancelled attempts cannot commit. |
| Run loop/Core/stack | Old main allocates an 8 KiB Core1 stack and calls run-loop execute. SDK `btstack_run_loop_async_context.c` processes timers from its pending worker; the background worker does not depend on the blocking execute loop. No evidence requires moving the current runtime to Core1. No stack-size change is inferred from the old design. |
| TLV/persistence | SDK `setup_tlv` installs Classic link-key DB and LE DB when both features are compiled. Old reference also stores a peer address for autonomous reconnect; G07 does not add that later registry behavior. Product flash sectors and SDK tail sectors remain untouched and separate. |
| Compilation/coexistence | Current CMake materializes BLE + Classic base code in one runtime archive; adapter libraries consume SDK headers. Two HCI ACL slots and one HID Host slot remain. The old project links both protocols directly into one executable. |

### Bounded failure and ownership policy

| Waiting phase | Bound / recovery |
|---|---|
| Stack readiness or Mouse discovery cancellation/security setup | 15 s; explicit error and release discovery suppression. A pending LE cancel is still owned by BLE and handled by its eventual completion. No connected Mouse is disconnected. |
| Inquiry submission/ACK/completion | 10 s; explicit error, stop only an ACKed inquiry, and retain pending ownership until completion. An ACK arriving after abort is cancelled. |
| API/controller rejection | Back off 1 s, retry only after the rejection makes inquiry idle; stop after three consecutive failures and display the actual status byte. |
| Remote name | 10 s; terminate the user transaction, release Mouse discovery and retain the pending address until its completion. The pinned SDK has no GAP name-cancel API that safely resets its private state; do not fake IDLE or overwrite it. |
| HID connect / descriptor | 30 s per phase; disconnect owned HID CID, reject late descriptor, retain CID until failure/close. `hid_host_disconnect` can be a no-op during SDP with no control channel; do not falsely claim resource cleanup. Late OPENED is disconnected. |
| User PIN entry | 60 s from PIN notification. |
| Entire command service stops producing updates | Independent main-loop watch: 90 s, LCD error F2, post cancellation; HAT/USB main loop remains active. This cannot recover a completely hung CPU. |

`ERROR F0` is a phase deadline; `ERROR F1` means the previous operation still
owns a resource and cannot safely be reused; `ERROR F2` means no product update
was received by the independent watch. Other error bytes are operation status.
The error screen retains the last phase on row 2. Retry is allowed when owned
operations have drained. If the controller never completes cleanup, a full power
cycle is explicitly required; resetting the shared radio would break a working
Mouse and is not used. Normal cancellation immediately resumes Mouse discovery.

### Automated evidence

- All assertion-based host tests now compile with `-UNDEBUG`, including Release CI.
  Previously Release removed assertions from several test executables.
- Enabling those checks exposed stale G02 expected text/columns. Only the test
  expectations were updated to the already accepted G04 title `PRESS TO LEARN A KEY`
  and 1-based column 16; no accepted screen geometry was changed.
- `g07_classic_adapter_async` compiles the actual production adapter against a
  deterministic BTstack test double (not a second state-machine implementation).
  It injects early WORKING, asynchronous rejection, missing ACK/completion, radio
  timeout, name timeout/late response, connect timeout, late descriptor, foreign
  CID, cancellation, repeated rejection and successful descriptor completion.
- Runtime progress decoding and LCD phases/error retention/hint geometry are tested.
  The double does not emulate controller firmware, real SDP or RF behavior.

### Focused physical validation before the full G07 suite

Use the exact UF2 from the successful host + RP2350 workflow at the same commit.
Record the last phase, error byte and search/found counts if a test fails.

| Test | Procedure | Expected |
|---|---|---|
| T1 | Full power-cycle, Mouse off, HOME → OTHER OPTIONS → PAIR KEYBOARD, BKB-3G pairing | Advance through search/name/connect/PIN as applicable to KEYBOARD SAVED; no indefinite false SEARCHING. |
| T2 | Connect Mouse and confirm movement/click; then pair Keyboard | Keyboard connects, Mouse continues working. |
| T3 | Pair Keyboard first, then power Mouse on | Mouse connects without dropping Keyboard. Also repeat with Mouse switched on while Keyboard is still pairing. |
| T4 | Both connected: type, modifiers, movement, click, scroll | Both remain functional. |
| T5 | Both connected, turn only Keyboard off | Mouse remains usable; held physical Keyboard keys release. |
| T6 | Both connected, turn only Mouse off | Keyboard remains usable. |
| T7 | Start Keyboard pairing without a target; cancel with Key B | HAT stays responsive, returns to OTHER OPTIONS, Mouse discovery/reconnect resumes. |

Only after T1–T7 pass should the full G07-01 through G07-10 suite above run.
G07 remains unaccepted, PR #9 remains draft, and no merge or next gate is authorized.
