# Frozen decisions — v1 contract

This record resolves the ambiguities identified before implementation of `blu2usb-picow`.

## D1 — Custom Remap is appliance-owned editing

The editor does not learn mappings from physical mouse clicks. It is fully operated by the Pico HAT and remains available with no mouse connected or saved.

One global persistent `CustomTemplate` exists in v1. Saved Mouse records store only which profile kind is selected. A Mouse using `CUSTOM REMAP` references the current global template.

`APPLY AND BACK` changes draft only. `APPLY CUSTOM` persists the whole template. With an active Mouse, that Mouse also becomes Custom immediately; without one, only the template is saved.

## D2 — Escape is a Custom target

Every source button (`LEFT`, `RIGHT`, `MIDDLE`, `FORWARD`, `BACKWARD`) can map to `ESCAPE` in addition to the five mouse-button targets.

## D3 — Presets are literal product rules

Default:

- Forward -> Left
- Left -> Forward
- Backward -> Right
- Right -> Backward
- Middle -> Middle

Escape:

- Forward -> Left
- Backward -> Right
- Left -> Escape
- Right -> Backward
- Middle -> Forward

These maps are not inferred from previous repositories.

## D4 — Keyboard UX is transport-neutral

Keyboard is a logical device type. The UI does not ask the user to choose Classic vs BLE. The initial architecture must preserve the proven Bluetooth Classic BKB-3G route while allowing BLE HOGP Keyboard or future adapters without changing product UX.

The canonical Pair Keyboard body uses `SEARCHING KEYBOARD`, not `SEARCHING BLE HID`.

## D5 — Unlock is any HAT control

Any first HAT interaction while locked unlocks, is consumed, and returns HOME. Key Y remains the principal discoverable lock/unlock control in hints and Learn The Keys.

## D6 — Device Details active state is cyan

For a saved device that is active in its logical slot, the device name and dynamic detail values are cyan. Inactive saved values use the off-white yellow. The selectable `REMOVE DEVICE` row follows option colors.

## D7 — No embedded debug tooling

The new repository does not reproduce UART/CDC debug tooling from the experimental project. No diagnostic USB interface, alternate debug descriptor, debug UF2 or serial-dependent acceptance path is allowed.

## D8 — Old repositories are evidence, not specification

Accepted hardware behavior and proven POC techniques may be reexpressed. Old layouts, profile tables, experimental coupling and unaccepted G07 behavior have no authority over this repository when they differ from the frozen contract.
