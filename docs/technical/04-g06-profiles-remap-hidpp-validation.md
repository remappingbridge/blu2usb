# BLU2USB-G06 validation — Mouse profiles, remap and Logitech HID++

## Objective

Add runtime Mouse profiles/remapping on top of the physically accepted G05 BLE HOGP Mouse path. Mouse buttons may remain Mouse buttons or generate synthetic USB Keyboard Escape, while the fixed G04 Mouse + Keyboard USB identity remains unchanged.

This gate also adds the Logitech HID++ `REPROG_CONTROLS_V4` backend needed to preserve true Forward held/released semantics when Forward is remapped on supported Logitech devices.

Bluetooth Keyboard pairing/input is not part of G06. The USB Keyboard interface is exercised only by synthetic Escape generated from a Mouse mapping.

G06 owns live Mouse connection/profile presentation, persistence of Mouse profile/Custom configuration across Pico power cycles, and reconnection to an already-bonded BLE Mouse. Persistent user-facing saved-device names/models and multi-device management remain later-gate concerns.

## Frozen profile contract

The LCD `APPLY ...` screen is normative: the runtime mapping must exactly match the relationships printed on that screen.

- `PASSTHROUGH`: Left→Left, Right→Right, Middle→Middle, Forward→Forward, Backward→Backward.
- `DEFAULT REMAP`: Left→Forward, Right→Backward, Middle→Middle, Forward→Left, Backward→Right.
- `ESCAPE REMAP`: Left→Escape, Right→Backward, Middle→Forward, Forward→Left, Backward→Right.
- `CUSTOM REMAP`: each source Left/Right/Middle/Forward/Backward may target Left/Right/Middle/Backward/Forward/Escape.
- Relative X/Y, vertical wheel and horizontal pan always pass through unchanged by button profiles.
- Changing profile releases persistent Mouse and synthetic Keyboard ownership from the old profile before the new mapping becomes authoritative.

## Persistent product-state contract

Every accepted configuration change must survive loss of power and be reconstructed before the Mouse runtime becomes authoritative again.

- The confirmed active profile kind is persistent.
- The committed global Custom mapping is persistent.
- A Custom draft accepted with `KEY A: APPLY AND BACK` is persistent even before `KEY A: APPLY CUSTOM` is used. Reboot restores that draft and its dirty state while keeping the last actually-applied profile active.
- Applying another preset does not silently destroy an existing Custom draft.
- Boot restores the active profile into the profile engine, remap engine, UX status, current-profile coloring, and Logitech Forward-fix requirement before normal BLE input is processed.
- Product state uses a versioned, integrity-checked record and two alternating flash slots. A torn/corrupt newest write falls back to the previous valid slot.
- Product-state flash is physically separate from the Pico SDK/BTstack credential area. Product serialization must not overwrite BLE bonds/IRKs/LTKs.
- If no valid product record exists, safe defaults are Passthrough plus the identity Custom mapping.

## Bonded BLE reconnect contract

BLE bonding credentials remain owned by the Pico SDK/BTstack persistent LE device database. On startup, an existing bonded Mouse must be attempted before generic HID discovery.

- Once BTstack reaches `HCI_STATE_WORKING`, the firmware loads the LE resolving list from the persistent device DB, places bonded peers in the LE whitelist/filter accept list, and starts a whitelist connection attempt.
- This bonded reconnect path must work for peripherals such as Logitech Lift that wait for the previously bonded central and may not expose the same generic HID advertisement used during first pairing.
- A bonded reconnect reuses the existing security relationship through re-encryption; it must not require the user to put the Mouse into fresh pairing mode.
- The bonded-connect attempt is bounded. If no bonded peer becomes available, the firmware cancels it and falls back to the normal generic BLE HID scan so another Mouse can still be paired.
- After an unexpected disconnect from a ready Mouse, the firmware again prefers bonded reconnect before falling back to generic scanning.
- Generic Mouse behavior remains valid: devices that freely advertise for any central may reconnect either through the bonded path or the generic fallback, but must not regress.

## Profile feedback contract

- The profiles/remap engine plus persistent commit are the source of truth. A profile is only considered successfully applied after the engine has accepted the full mapping and the persistent record has been written and verified; Apply input alone must not optimistically change the active profile or enter success feedback.
- Cyan is the general success/current-state color for applied configuration values.
- After a successful preset profile apply, every visible body line describing the resulting configuration on the feedback screen is cyan instead of yellow.
- In `MOUSE OPTIONS`, the currently active profile row is cyan while not selected.
- Selection has global visual priority: when the cursor rests on a cyan/current option, that selected row is white; after moving selection away it returns to cyan if it is still current.
- Entering the already-active Passthrough, Default Remap or Escape Remap opens its feedback/applied screen directly, without a `KEY A: APPLY` hint.
- Entering an already-active Custom Remap shows the current Custom configuration without the `KEY A: APPLY CUSTOM` hint until a Custom target is changed. In that edit screen, the selected mapping row is white and the remaining current rows are cyan.
- Successful `KEY A: APPLY CUSTOM` opens the dedicated `CUSTOM APPLIED` screen only after engine and persistence confirmation. It displays the five confirmed mapping rows in cyan plus exactly `KEY B: BACK` and `KEY Y: LOCK`.
- `KEY B` from `PASSTHROUGH APPLIED`, `DEFAULT REMAP APPLIED`, `ESCAPE APPLIED`, or `CUSTOM APPLIED` returns directly to `MOUSE OPTIONS` on the first complete press/release.

## Live Mouse UX contract

BLE HOGP `CONNECTED` and `DISCONNECTED` events are the source of truth for live Mouse connection state.

- On connect, the UI state is updated even if no Mouse movement/button report has occurred yet.
- On disconnect, the live connected marker is removed while the existing ownership-release behavior remains intact.
- `MOUSE STATUS` shows exactly `MOUSE CONNECTED` in cyan while connected and `MOUSE NOT CONNECTED` in ordinary off-white yellow while disconnected.
- Other ordinary `MOUSE STATUS` body text remains off-white yellow.
- `PROFILE:` always reflects the confirmed active profile using `PASSTHROUGH`, `DEFAULT`, `ESCAPE`, or `CUSTOM`; it must not remain frozen on Passthrough and it must be correct immediately after reboot from persistent state.
- In `MOUSE OPTIONS`, `PAIR MOUSE` is cyan while a Mouse is connected and the row is not selected. Selection turns it white; moving selection away restores cyan.
- Accessing `PAIR MOUSE` while already connected opens `MOUSE PAIRED` rather than starting/showing a search.
- If the Mouse becomes connected while the `PAIR MOUSE` search screen is already visible, the LCD immediately transitions to `MOUSE PAIRED` without requiring another HAT event.
- `MOUSE PAIRED` shows `MOUSE CONNECTED` and `READY TO USE` in cyan, plus `KEY B: BACK` and `KEY Y: LOCK`; Back returns directly to `MOUSE OPTIONS`.

## Inherited regression contract

The physically accepted corrections from earlier gates remain mandatory in G06 and every later gate:

- retained pixel relocation applies to all screens regardless of wording changes;
- on `PRESS TO LEARN A KEY`, `KEY A`, `KEY B`, and `KEY X` start at 1-based column 16;
- `KEY B` is always one-screen Back outside HOME/LEARN, even when the visible word is `CANCEL` or another context label;
- there is no `GO TO HOME` action; reaching HOME through Back is only a consequence of one-screen navigation;
- hidden controls remain functional where the screen contract declares them;
- lock occurs on Key Y release; the unlock interaction is consumed and returns to HOME;
- Pair help is exactly `KEY X: HELP`; `KEY C: HELP` is invalid;
- the Learn title is exactly `PRESS TO LEARN A KEY`;
- the fixed USB Mouse + Keyboard identity remains stable and must not re-enumerate due to Bluetooth or profile changes;
- BLE Mouse input continues while the LCD is locked;
- disconnect/overflow/profile changes release persistent HID ownership so Mouse buttons or synthetic Keyboard keys cannot remain stuck.

## HID++ contract

- HID++ is an optional vendor backend inside BLE HOGP composition; vendor report IDs/layouts never reach application or USB layers.
- When Forward remains Forward, no Forward diversion is required.
- When Forward is remapped, a supported peer may be probed for `REPROG_CONTROLS_V4` (`0x1b04`) and Forward CID `0x0056` is diverted only after the peer acknowledges the configuration.
- A peer that does not support the HID++ exchange falls back to standard HID without breaking normal Mouse input.
- Disconnect/profile changes release stale held ownership so neither Mouse buttons nor synthetic Escape can remain stuck.

## Automated acceptance

CI must prove:

1. all G02–G05 regression tests remain green;
2. preset mappings match the frozen screen contract and Default is distinct from Escape Remap;
3. Custom draft/commit supports Escape and validates serialized profile data;
4. active profile, committed Custom mapping and an unapplied accepted Custom draft round-trip through the product-state record;
5. two-slot product storage detects corruption and can select the previous valid generation;
6. product flash storage remains separate from Bluetooth credential ownership;
7. Mouse→Mouse remapping produces canonical target button events;
8. Mouse→Escape produces synthetic canonical Keyboard Escape press/release events;
9. source-aware aggregation prevents stuck Mouse/Keyboard ownership after remap transitions;
10. successful profile feedback is cyan, but any selected cyan/current option is white until selection moves away;
11. an already-active profile opens feedback without an Apply hint and `KEY B` leaves feedback for `MOUSE OPTIONS` on the first press/release;
12. `KEY A: APPLY AND BACK` on every `<BUTTON> WILL BECOME` screen updates the Custom draft and the returned `EDIT CUSTOM REMAP` text immediately reflects that accepted target;
13. Custom apply remains on the edit state until runtime confirmation, then opens `CUSTOM APPLIED` with confirmed mappings plus `KEY B: BACK` and `KEY Y: LOCK`;
14. live Mouse connection state projects `MOUSE CONNECTED` cyan / `MOUSE NOT CONNECTED` yellow and marks `PAIR MOUSE` cyan while connected, with selected-white precedence;
15. `MOUSE STATUS` projects the confirmed/restored current profile rather than a static Passthrough string;
16. an already-connected Mouse bypasses the pair-search presentation and uses `MOUSE PAIRED` feedback;
17. BLE production composition contains persistent-DB enumeration, resolving-list restore, whitelist connection and bounded fallback to scanning;
18. the screen contract preserves the accepted Learn geometry, Key-X help label, one-screen Back rule, and absence of `GO TO HOME`;
19. HID++ feature discovery and Forward diversion request bytes match the frozen feature/CID contract;
20. HID++ held/released events become canonical Forward transitions only after diversion is acknowledged;
21. unsupported HID++ transport writes fail safe to standard HOGP behavior;
22. profiles/remap/HID++ state-machine cores stay free of Pico SDK, TinyUSB, BTstack and raw report-layout dependencies;
23. the application contains no raw TinyUSB/BTstack/GPIO/SPI primitives and there is no forced USB re-enumeration path;
24. Pico 2 W production cross-build produces a non-empty UF2.

## Physical scenarios

Use the same Pico 2 W + Waveshare HAT/LCD + BLE HOGP Mouse that passed G05. A Logitech Lift or another compatible Logitech HID++ Mouse is required for the scenarios explicitly marked Logitech-specific.

### G06-01 — G05 regression, live connection UX and fixed USB identity

Flash G06 and power-cycle. The first LCD page remains `PRESS TO LEARN A KEY`; the BLE Mouse can connect and move/click/scroll as in G05. The host continues exposing the same fixed BLU2USB Mouse + Keyboard USB identity.

After the Mouse connects:

- `STATUS → MOUSE STATUS` must show `MOUSE CONNECTED` in cyan, while `PROFILE: ...`, `FWD: ...` and `BACK: ...` use their normal status presentation;
- `MOUSE OPTIONS` must show `PAIR MOUSE` cyan when another row is selected and white while `PAIR MOUSE` itself is selected;
- accessing `PAIR MOUSE` while the Mouse is already connected must open `MOUSE PAIRED`, with `MOUSE CONNECTED` and `READY TO USE` cyan, rather than showing a frozen/searching Pair screen;
- `KEY B: BACK` on `MOUSE PAIRED` must return directly to `MOUSE OPTIONS`.

Also validate the transition case at least once if practical: begin on the searching `PAIR MOUSE` page with the Mouse disconnected, then connect it. The LCD must move to `MOUSE PAIRED` immediately without another HAT press.

### G06-02 — PASSTHROUGH and success feedback

Navigate to Mouse Options → Passthrough. If Passthrough is the active/restored profile, it must open the applied/feedback screen directly with no Apply hint. Its configuration text is cyan. Left, Right, Middle, Backward and Forward keep their native meanings. `KEY B` returns to Mouse Options on the first press/release. In Mouse Options, `PASSTHROUGH` is cyan when another option is selected and white while `PASSTHROUGH` itself is selected.

`MOUSE STATUS` must read `PROFILE: PASSTHROUGH`.

### G06-03 — DEFAULT REMAP exact mapping

Navigate to Mouse Options → Default Remap and apply it. Validate the exact mapping:

- Left generates Mouse Forward;
- Right generates Mouse Backward;
- Middle remains Mouse Middle;
- Forward generates Mouse Left;
- Backward generates Mouse Right.

After Apply, every configuration line on the feedback screen is cyan. `KEY B` returns directly to Mouse Options on the first press/release. `DEFAULT REMAP` is cyan while another row is selected and becomes white while its own row is selected. Re-entering Default Remap opens the feedback screen directly without an Apply hint.

`MOUSE STATUS` must now read `PROFILE: DEFAULT`; it must not remain `PROFILE: PASSTHROUGH`.

### G06-04 — ESCAPE REMAP exact mapping

Apply Escape Remap and validate:

- Left generates Keyboard Escape;
- Right generates Mouse Backward;
- Middle generates Mouse Forward;
- Forward generates Mouse Left;
- Backward generates Mouse Right.

After Apply, every configuration line on the feedback screen is cyan. `KEY B` returns directly to Mouse Options on the first press/release. `ESCAPE REMAP` is cyan while another row is selected and white while its own row is selected. Re-entering Escape Remap opens feedback directly without an Apply hint.

`MOUSE STATUS` must read `PROFILE: ESCAPE`.

### G06-05 — Synthetic Escape press/release

With Escape Remap or a Custom profile that maps a button to Escape, open a host menu/dialog where Escape is observable. Press and hold the physical mapped button: the remapped Keyboard Escape ownership is held. Release it: Escape is released. Repeated clicks must not leave the USB Keyboard in a held state.

### G06-06 — CUSTOM REMAP

Open Custom Remap. Before applying the complete Custom profile, validate draft reflection explicitly: select `LEFT IS LEFT`, enter `LEFT WILL BECOME`, choose `RIGHT`, then release `KEY A: APPLY AND BACK`. On return to `EDIT CUSTOM REMAP`, the first row must immediately read `LEFT IS RIGHT`. Repeat with at least one additional source, including one target mapped to `ESCAPE`.

Then release `KEY A: APPLY CUSTOM`. The UI must not claim success before the profile engine accepts and persists the full Custom configuration. On successful confirmation it must open a dedicated page titled exactly `CUSTOM APPLIED`. The five confirmed mappings are cyan and the visible hints are exactly `KEY B: BACK` and `KEY Y: LOCK`. Both mappings must take effect simultaneously while X/Y/wheel remain unchanged.

`KEY B` from `CUSTOM APPLIED` returns directly to `MOUSE OPTIONS`, where `CUSTOM REMAP` is cyan when not selected and white when selected. `MOUSE STATUS` must read `PROFILE: CUSTOM`. Re-entering Custom opens `EDIT CUSTOM REMAP` with the current mappings and no `KEY A: APPLY CUSTOM` hint until a target changes.

### G06-07 — Profile change while a mapped control was active

Exercise a mapped button, then change profile and continue testing. No Mouse button or Escape key from the previous profile may remain stuck after the profile transition. The new profile must become authoritative immediately after Apply.

### G06-08 — Generic/non-Logitech fail-safe

If the G05 test Mouse is not a supported Logitech HID++ device, apply Default Remap or a Custom profile that remaps Forward. Normal Mouse movement/buttons and the standard Forward path must remain usable; an unsupported HID++ probe must not freeze, disconnect-loop, or break the Mouse.

### G06-09 — Logitech Forward hold under remap (Logitech-specific)

With a supported Logitech Lift/HID++ Mouse, apply Default Remap, where Forward→Left. Hold physical Forward and move the Mouse. The host must see Mouse Left held for the entire physical Forward hold so a drag can be performed. Releasing physical Forward must release Mouse Left immediately.

### G06-10 — Logitech diversion removed by Passthrough (Logitech-specific)

After G06-09, switch back to Passthrough. Physical Forward must return to native Forward behavior and must no longer generate Mouse Left. Repeating profile changes must not leave either Forward or Left stuck.

### G06-11 — Lock/UI while remap is active

With any non-Passthrough profile active, lock the LCD using Key Y. Mouse movement and remapped button/Keyboard Escape output continue while locked. Unlock remains consumed exactly as in G03–G05 and does not alter the active profile.

### G06-12 — USB identity stability through profile changes

Switch repeatedly among Passthrough, Default Remap, Escape Remap and Custom while observing the host. The BLU2USB USB device must not disappear/re-enumerate and must retain both fixed Mouse + Keyboard HID interfaces throughout.

### G06-13 — Configuration persistence across power cycle

With a Mouse connected, apply a non-Passthrough profile such as `DEFAULT REMAP`. Verify its behavior and `MOUSE STATUS → PROFILE: DEFAULT`. Remove power from the Pico 2 W, then power it again without changing the Mouse configuration. After boot/reconnect, `MOUSE STATUS` must still show `PROFILE: DEFAULT`, `DEFAULT REMAP` must be the current cyan option when not selected, and the Default mappings must already be active without re-applying them.

Repeat with `ESCAPE REMAP` or `CUSTOM REMAP` at least once. For Custom, verify the exact mapping survives power loss.

Also validate the draft case: while some profile is active, change one Custom target with `<BUTTON> WILL BECOME → KEY A: APPLY AND BACK`, but do not press `APPLY CUSTOM`. Power-cycle. The previously active profile must remain active, while re-entering Custom must restore the draft change and still present it as unapplied/dirty.

### G06-14 — Logitech Lift bonded reconnect after Pico power cycle (Logitech-specific)

Pair a Logitech Lift normally and confirm it works. Leave the Lift on the same Easy-Switch channel and do not put it into new-pairing mode. Remove power from the Pico 2 W, then power the Pico again while the Lift is waiting for its previously bonded host.

Expected behavior: the Pico uses its persistent BLE bond to initiate/re-establish the connection; the Lift reconnects without a fresh pairing action, `MOUSE STATUS` becomes `MOUSE CONNECTED`, and normal movement/buttons work. If a non-Passthrough profile was active before power loss, that restored profile must also be active after this reconnect.

As a regression check, a generic previously bonded Mouse must continue reconnecting successfully after the same Pico power cycle. If the previously bonded Mouse is absent, the bounded reconnect attempt must eventually fall back to generic Pair Mouse discovery rather than blocking pairing indefinitely.

## Gate close

G06 is accepted when final-head CI is green and all applicable scenarios G06-01 through G06-14 pass. Do not merge automatically.
