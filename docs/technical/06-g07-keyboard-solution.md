# G07 — GT T1 Classic keyboard: authoritative solution and migration

## Status and source of truth

This is the new G07 implementation from the **physically accepted G06** commit
`7eee024ad4ee726c5a85ffa2f32b9f47187878af` ([acceptance PR #8](https://github.com/tiagooliveirajs/blu2usb/pull/8)).
Branch: `gate/g07-validated-classic-from-g06`. Earlier rejected G07 branches are
historical evidence only; none is a code base or proof of physical success.

**POC accepted; integrated G07 physical acceptance pending.** The operator
confirmed pairing and typing **a, s, d** through Pico 2 W USB on 2026-09-18.

- [Accepted POC implementation b04aaf1](https://github.com/tiagooliveirajs/picow_ble_usb_hid_bridge_gui/commit/b04aaf146844cead484e2a4191a4b07806bdf6f6)
- [Immutable POC acceptance record and log excerpts](https://github.com/tiagooliveirajs/picow_ble_usb_hid_bridge_gui/blob/857fd66e64d7ca4c24586d962063c5d43e925eee/docs/BKB3G_ACCEPTED_SOLUTION.md)
- [Machine-readable provenance and source hashes](../reference/g07-keyboard-provenance.json)
- [Scope/runtime decision and old gate numbering reconciliation](../decisions/0002-g07-validated-classic-rebuild.md)

The tested device is **Goldentec/TEC/SFIO GT T1, code 40062**, name
`Bluetooth keyboard 3.0` / `BKB-3G`, observed address `20:20:01:60:0B:94`.
The adapter matches the names, not one hard-coded hardware address.

## The fix that must not be lost

The working sequence is:

1. Classic inquiry, then remote-name resolution if the name is absent from EIR.
2. `gap_dedicated_bonding(target, 0)` — Security Level 2, **no MITM requirement**.
3. SSP `NoInputNoOutput`, MITM-not-required bonding and Just Works acceptance.
4. On successful `GAP_EVENT_DEDICATED_BONDING_COMPLETED`, schedule
   `start_hid_after_bonding` with `btstack_run_loop_execute_on_main_thread`.
5. **Return from the bonding event.** Only the queued callback may launch
   `hid_host_connect(target, HID_PROTOCOL_MODE_REPORT, ...)` and its SDP query.
6. Wait for HID opened **and** descriptor available before advertising ready.
7. Parse remote framing/report IDs in the adapter, then publish logical keyboard
   snapshots and apply them through source-scoped canonical ownership.

Do not restore synchronous `hid_host_connect` in the bonding event. Do not
replace the scheduling boundary with a guessed delay. Do not demand MITM to
"fix" this keyboard, or switch it to BLE HOGP.

In the pinned [BTstack HCI implementation](https://github.com/bluekitchen/btstack/blob/501e6d2b86e6c92bfb9c390bcf55709938e25ac1/src/hci.c),
the dedicated-bonding result is emitted from inside the disconnection handler,
**before** the old ACL entry is reset/removed. The
[HID host](https://github.com/bluekitchen/btstack/blob/501e6d2b86e6c92bfb9c390bcf55709938e25ac1/src/classic/hid_host.c)
starts [SDP synchronously](https://github.com/bluekitchen/btstack/blob/501e6d2b86e6c92bfb9c390bcf55709938e25ac1/src/classic/sdp_client.c)
when the SDP client is available. L2CAP can then wait for a new connection whose
Create Connection command HCI suppresses because it still sees the old ACL.
The [SDK queued callback](https://github.com/raspberrypi/pico-sdk/blob/2.2.0/src/rp2_common/pico_btstack/btstack_run_loop_async_context.c)
executes after event dispatch, avoiding that reentrancy defect.

Historical statuses: `0x66` was the HID L2CAP security refusal; `0x2f` was
insufficient security under required MITM. The accepted run reports pairing,
authentication, encryption and bonding success (`0x00`). Its deliberate bonding
disconnect reason `0x16` is local-host termination. Sniff-subrating is informational.

## Exact physical observations

The operator log shows this order:

```text
[0000053937] BOND: dedicated bonding complete status=0x00
[0000053938] BOND: HID start queued until HCI cleanup finishes
[0000053938] ACL: disconnection status=0x00 handle=0x000b reason=0x16
[0000053939] CONNECT: deferred HID start after bonding event cleanup
[0000054429] ACL: connection complete addr=20:20:01:60:0B:94 status=0x00 handle=0x000b
[0000054857] HID: CONNECTION OPEN cid=0x0001
[0000054857] HID report descriptor (263 bytes)
[0000054864] POC READY: Classic HID connected; USB keyboard path active
```

Keyboard reports, with each press followed by the all-zero release:

| Key | Remote input | Canonical usage |
| --- | --- | --- |
| a | `a1 01 00 00 04 00 00 00 00 00` | `04` |
| s | `a1 01 00 00 16 00 00 00 00 00` | `16` |
| d | `a1 01 00 00 07 00 00 00 00 00` | `07` |

The 263-byte descriptor is retained in `tests/fixtures/bkb3g_descriptor.h`.
Its byte SHA-256 is
`7626b8c021d9ee5f2f1ae472f58c53037f2772820602cfb9d913319d37823d79`.
Report ID `03` in the log has no Keyboard usage page; ignoring it is correct.
The operator did not supply a hash for the locally compiled POC UF2. This record
does not invent one or claim those observations were reproduced on remote hardware.

## Integration map and deliberate adaptations

| Obligation | Production owner | Adaptation from isolated POC |
| --- | --- | --- |
| Inquiry/name → Level 2 bonding → deferred HID | `src/classic_hid/classic_hid_pico.c` | Command-driven Pair Keyboard, address/CID guards and bounded attempts |
| Descriptor/report-ID parsing | `src/classic_hid/classic_hid_parser.c` | Actual BTstack parser retained; reject short frames; logical snapshot output |
| Logical Keyboard facade | `src/keyboard_transport/` | UI commands stay independent of Bluetooth transport |
| Radio lifecycle | `src/bt_runtime/bt_runtime_pico.c` | One BLE+Classic runtime on Core1, explicit 8 KiB stack |
| Flash safety | `bt_runtime` plus existing product storage | Both cores register; application serializes product writes with radio callbacks |
| Cross-core data | `bt_runtime` queue and adapter atomic mailboxes | Full snapshots retain release state; status is not lost with a full input queue |
| Canonical ownership | `src/hid_aggregator/keyboard_snapshot.c` | Replace only physical Keyboard ownership; synthetic Escape/Mouse survive |
| BLE coexistence | `src/ble_hogp/ble_hogp_pico.c` | Ignore foreign ACL disconnects; preserve G06 mouse reconnect/vendor logic |
| USB/HAT/LCD | Existing Core0 modules | Fixed production Mouse+Keyboard descriptors remain unchanged; no CDC copied |
| UI | `ux_model/keyboard_state.c`, renderer projection | Dynamic Pair progress, real connection status and success coloring |

Every Bluetooth translation unit uses both `ENABLE_BLE=1` and
`ENABLE_CLASSIC=1`, avoiding incompatible HCI struct layouts. Capacity is three
HCI connections (Mouse, Keyboard, one setup margin), one HIDS client, one HID
host, eight dynamic L2CAP channels and four Classic link keys. This sizing does
not claim acceptance of future three-device Composite topology.

Search is bounded to 90 seconds, bonding/HID setup to 30 seconds per phase.
Cancel/retry waits for in-flight operation cleanup. A disconnected ready keyboard
gets at most three reconnect attempts with its current RAM target and existing
link key; retries do not delete the key. Explicit Pair starts a fresh discovery/
bonding transaction. Full preferred-peer persistence and keyboard cold-boot
reconnect belong to the later registry gate.

Physical keyboard snapshots are submitted in order: the application waits for
USB acceptance before consuming the next snapshot, preserving short taps across
LCD redraws. Queue overflow releases held ownership; failed keyboard publication
stops that session. Profile changes continue to release only Mouse/synthetic
ownership. The combined fixed six-key USB report emits rollover rather than
silently dropping a seventh key.

## Automated evidence and how to reproduce

Host tests compile the **production** adapter and parser as separate sources,
with the exact SDK BTstack headers plus actual descriptor parser/utility code.
Controller operations and run-loop dispatch are test doubles; this is not radio
simulation or physical acceptance. Tests cover the accepted a/s/d reports,
release, modifiers, ignored non-keyboard report, truncated input, success ordering,
foreign events, cancel during deferred startup, bonding failure, bounded search,
overflow, source coexistence and live UI behavior.

`check_g07_negative_control.py` restores synchronous HID startup in a temporary
copy and requires failure at the stale-ACL invariant. It must never be disabled
to make CI green. All host C asserts remain enabled in Release with `-UNDEBUG`.
Two pre-existing G02 expectations were corrected to the already accepted Learn
title and column 16; product layout was not changed to satisfy those tests.

```sh
cmake -S . -B build-host -DBLU2USB_BUILD_TESTS=ON \
  -DBLU2USB_BTSTACK_ROOT="$HOME/pico/pico-sdk/lib/btstack" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-host --parallel
ctest --test-dir build-host --output-on-failure

cmake -S . -B build-pico -DBLU2USB_BUILD_PICO=ON \
  -DBLU2USB_BUILD_TESTS=OFF -DPICO_BOARD=pico2_w \
  -DPICO_SDK_PATH="$HOME/pico/pico-sdk" -DCMAKE_BUILD_TYPE=Release
cmake --build build-pico --parallel
```

Output: `build-pico/blu2usb_picow.uf2`. SDK 2.2.0, GNU Arm 13.2.Rel1 (Ubuntu
package `15:13.2.rel1-2`), target Pico 2 W/RP2350. The UF2 includes the source
revision in program metadata, readable with `picotool info -a`. Deliver only a
clean committed revision; a `-dirty` metadata marker is not a release candidate.

## Integrated physical acceptance (still pending)

1. Flash the supplied UF2 using BOOTSEL on **Pico 2 W**; boot into the accepted HAT UI.
2. With Mouse off, open **OTHER OPTIONS → PAIR KEYBOARD**. Put GT T1 slot FN+1 in pairing.
3. Confirm **KEYBOARD SAVED / KEYBOARD CONNECTED / READY TO USE** and type a/s/d.
4. Test Shift+A, Ctrl combinations, Space, Enter, Backspace, arrows and releases.
5. Pair/use the BLE Mouse; verify motion, buttons, wheel and the accepted profiles.
6. Repeat keyboard pairing with Mouse connected; Mouse must remain connected
   across the keyboard's deliberate bonding disconnect.
7. Hold physical Escape while generating mapped Escape; release each source in
   both orders. Escape must stay held until the last owner releases.
8. Disconnect Keyboard during a held key; no stuck key, Mouse remains usable.
   Restore Keyboard promptly and test reconnect; after bounded failure, use Pair again.
9. Cancel/retry Pair Keyboard, use Help and lock/unlock during search; HAT stays
   usable. Lock must not stop normal forwarding.
10. Reconfirm G06 profiles/Custom and Mouse bonded reconnect across power cycles;
    USB identity must remain fixed. Keyboard preferred cold-boot reconnect is
    not claimed in this gate.

Use LCD/HAT and an editor for acceptance. This firmware intentionally has no
serial console. Report the displayed phase/error and the failed numbered scenario.
