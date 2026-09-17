# BLU2USB-G06 validation — Mouse profiles, remap and Logitech HID++

## Objective

Add runtime Mouse profiles/remapping on top of the physically accepted G05 BLE HOGP Mouse path. Mouse buttons may remain Mouse buttons or generate synthetic USB Keyboard Escape, while the fixed G04 Mouse + Keyboard USB identity remains unchanged.

This gate also adds the Logitech HID++ `REPROG_CONTROLS_V4` backend needed to preserve true Forward held/released semantics when Forward is remapped on supported Logitech devices.

Bluetooth Keyboard pairing/input is not part of G06. The USB Keyboard interface is exercised only by synthetic Escape generated from a Mouse mapping.

## Frozen profile contract

- `PASSTHROUGH`: Left→Left, Right→Right, Middle→Middle, Forward→Forward, Backward→Backward.
- `DEFAULT REMAP`: Left→Escape, Right→Backward, Middle→Forward, Forward→Left, Backward→Right.
- `ESCAPE REMAP`: Left→Escape; all other buttons remain identity.
- `CUSTOM REMAP`: each source Left/Right/Middle/Forward/Backward may target Left/Right/Middle/Backward/Forward/Escape.
- Relative X/Y, vertical wheel and horizontal pan always pass through unchanged by button profiles.
- Changing profile releases persistent Mouse and synthetic Keyboard ownership from the old profile before the new mapping becomes authoritative.
- Profile persistence across Pico power cycles is not required in G06; saved-device/product persistence belongs to a later gate.

## HID++ contract

- HID++ is an optional vendor backend inside BLE HOGP composition; vendor report IDs/layouts never reach application or USB layers.
- When Forward remains Forward, no Forward diversion is required.
- When Forward is remapped, a supported peer may be probed for `REPROG_CONTROLS_V4` (`0x1b04`) and Forward CID `0x0056` is diverted only after the peer acknowledges the configuration.
- A peer that does not support the HID++ exchange falls back to standard HID without breaking normal Mouse input.
- Disconnect/profile changes release stale held ownership so neither Mouse buttons nor synthetic Escape can remain stuck.

## Automated acceptance

CI must prove:

1. all G02–G05 regression tests remain green;
2. preset mappings match the frozen contract;
3. Custom draft/commit supports Escape and validates serialized profile data;
4. Mouse→Mouse remapping produces canonical target button events;
5. Mouse→Escape produces synthetic canonical Keyboard Escape press/release events;
6. source-aware aggregation prevents stuck Mouse/Keyboard ownership after remap transitions;
7. HID++ feature discovery and Forward diversion request bytes match the frozen feature/CID contract;
8. HID++ held/released events become canonical Forward transitions only after diversion is acknowledged;
9. unsupported HID++ transport writes fail safe to standard HOGP behavior;
10. profiles/remap/HID++ state-machine cores stay free of Pico SDK, TinyUSB, BTstack and raw report-layout dependencies;
11. the application contains no raw TinyUSB/BTstack/GPIO/SPI primitives and there is no forced USB re-enumeration path;
12. Pico 2 W production cross-build produces a non-empty UF2.

## Physical scenarios

Use the same Pico 2 W + Waveshare HAT/LCD + BLE HOGP Mouse that passed G05. A Logitech Lift or another compatible Logitech HID++ Mouse is only required for the scenarios explicitly marked Logitech-specific.

### G06-01 — G05 regression and fixed USB identity

Flash G06 and power-cycle. The first LCD page remains `PRESS TO LEARN A KEY`; the BLE Mouse can connect and move/click/scroll as in G05. The host continues exposing the same fixed BLU2USB Mouse + Keyboard USB identity.

### G06-02 — PASSTHROUGH

Navigate to Mouse Options → Passthrough and apply it. Left, Right, Middle, Backward and Forward must keep their native meanings. X/Y and wheel remain unchanged.

### G06-03 — DEFAULT REMAP exact mapping

Navigate to Mouse Options → Default Remap and apply it. Validate the exact mapping:

- Left generates Keyboard Escape and must not generate Mouse Left;
- Right generates Mouse Backward;
- Middle generates Mouse Forward;
- Forward generates Mouse Left;
- Backward generates Mouse Right.

Use a host UI where Escape and browser Back/Forward are observable. Hold/release behavior must follow the physical button without a stuck target.

### G06-04 — ESCAPE REMAP

Apply Escape Remap. Left must generate Keyboard Escape. Right, Middle, Backward and Forward remain their native Mouse buttons. Pointer motion and wheel remain passthrough.

### G06-05 — Synthetic Escape press/release

With a profile that maps Left to Escape, open a host menu/dialog where Escape is observable. Press and hold physical Left: the remapped Keyboard Escape ownership is held. Release Left: Escape is released. Repeated clicks must not leave the USB Keyboard in a held state.

### G06-06 — CUSTOM REMAP

Open Custom Remap. Change at least two source buttons, including one mapping to `ESCAPE` and one mapping to another Mouse button, then apply Custom. Both mappings must take effect simultaneously while X/Y/wheel remain unchanged.

### G06-07 — Profile change while a mapped control was active

Exercise a mapped button, then change profile and continue testing. No Mouse button or Escape key from the previous profile may remain stuck after the profile transition. The new profile must become authoritative immediately after Apply.

### G06-08 — Generic/non-Logitech fail-safe

If the G05 test Mouse is not a supported Logitech HID++ device, apply Default Remap or a Custom profile that remaps Forward. Normal Mouse movement/buttons and the standard Forward path must remain usable; an unsupported HID++ probe must not freeze, disconnect-loop, or break the Mouse.

This scenario is not required separately when the only available test Mouse is the supported Logitech device used in G06-09/G06-10.

### G06-09 — Logitech Forward hold under remap (Logitech-specific)

With a supported Logitech Lift/HID++ Mouse, apply Default Remap, where Forward→Left. Hold physical Forward and move the Mouse. The host must see Mouse Left held for the entire physical Forward hold so a drag can be performed. Releasing physical Forward must release Mouse Left immediately.

If no compatible Logitech HID++ Mouse is available, report this scenario as `N/A`, not failed.

### G06-10 — Logitech diversion removed by Passthrough (Logitech-specific)

After G06-09, switch back to Passthrough. Physical Forward must return to native Forward behavior and must no longer generate Mouse Left. Repeating profile changes must not leave either Forward or Left stuck.

If no compatible Logitech HID++ Mouse is available, report this scenario as `N/A`.

### G06-11 — Lock/UI while remap is active

With any non-Passthrough profile active, lock the LCD using Key Y. Mouse movement and remapped button/Keyboard Escape output continue while locked. Unlock remains consumed exactly as in G03–G05 and does not alter the active profile.

### G06-12 — USB identity stability through profile changes

Switch repeatedly among Passthrough, Default Remap, Escape Remap and Custom while observing the host. The BLU2USB USB device must not disappear/re-enumerate and must retain both fixed Mouse + Keyboard HID interfaces throughout.

## Gate close

G06 is accepted when final-head CI is green and all applicable scenarios G06-01 through G06-12 pass. Logitech-specific scenarios may be recorded as N/A only when no compatible HID++ test Mouse is available. Do not merge automatically.
