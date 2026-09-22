# Technical architecture contract

## Target

Canonical hardware target: Raspberry Pi Pico 2 W with Waveshare Pico-LCD-1.3 HAT and ST7789 240x240 display.

The implementation should prefer Pico SDK 2.2.x-compatible APIs until a deliberate SDK upgrade gate is approved.

## Architectural principles

1. Transport-specific report layouts never reach USB-facing application logic.
2. Remote input becomes firmware-owned canonical Mouse/Keyboard events first.
3. Canonical ownership/refcount aggregation is the single authority for held buttons/keys.
4. USB identity is firmware-owned and independent from Bluetooth state.
5. UI emits application commands; UI code does not call Bluetooth transport primitives directly.
6. Product persistence is independent from BTstack credential storage.
7. Vendor quirks such as Logitech HID++ live behind vendor/capability adapters and emit canonical events.
8. No textual `.c` includes, macro interception, duplicated TinyUSB ownership or cross-module HAL leakage.

## Proposed module boundaries

The exact names may be refined in G01, but ownership is frozen:

- `domain`: canonical HID types, DeviceId, profile types, application command/result types; no Pico/BTstack/TinyUSB headers.
- `hid_aggregator`: per-source held ownership and relative-event accumulation; depends only on domain.
- `profiles`: presets, CustomTemplate draft/commit and mapping validation; host-pure.
- `remap`: canonical source->target translation, including synthetic Escape events; host-pure.
- `device_registry`: saved/preferred/active model and profile-kind association; no transport calls.
- `connection_coordinator`: high-level reconnect/pair/replace/remove policy.
- `bt_runtime`: single owner of CYW43/BTstack lifecycle.
- `ble_hogp`: BLE HID discovery/client/report-map parsing and canonical emission.
- `classic_hid`: Bluetooth Classic HID adapter and canonical Keyboard emission.
- `keyboard_transport`: logical Keyboard discovery/classification facade over enabled adapters; keeps UX transport-neutral.
- `logitech_hidpp`: Logitech quirk backend; no USB/UI dependency.
- `usb_hid`: only module allowed to own TinyUSB device descriptors/tasks/report submission.
- `storage`: versioned/integrity-checked product persistence, separate from Bluetooth credentials.
- `interaction`: release-triggered input state machine and navigation semantics.
- `ux_model`: screen definitions/projection state.
- `renderer`: semantic-cell/text renderer.
- `hat`: GPIO/button adapter.
- `app`: composition only; no raw BTstack/TinyUSB/GPIO/SPI primitives.

## USB contract

From boot, host sees stable Mouse + Keyboard HID interfaces. Bluetooth connect/disconnect, pairing, profile changes, lock/unlock and reconnect must not call forced USB disconnect/reconnect or change the production descriptor.

USB Keyboard exists even without a physical keyboard so remap-generated Escape can be emitted.

## Canonical ownership

Every physical or synthetic source has a stable internal source identity. Releasing/disconnecting one source only removes that source's ownership. A target remains held while any source still owns it.

Profile changes and disconnects must release stale synthetic ownership before the new mapping becomes authoritative.

Relative X/Y/wheel/pan are transient and never treated as held ownership.

## Mouse profiles

Preset tables are compile-time/product-contract data. CustomTemplate is global persistent product state. Per saved Mouse records store the selected profile kind. `CUSTOM REMAP` references the global template in v1.

The Custom editor and draft engine must have no dependency on active Mouse state.

## Keyboard transport

`Keyboard` is a logical capability, not a transport synonym.

At minimum, the architecture must support the proven Classic HID BKB-3G path. BLE HOGP Keyboard or another future transport can be added behind the same facade without changing the `PAIR KEYBOARD`, Saved Devices, canonical HID or USB contracts.

A Pair Keyboard transaction may search enabled keyboard transports according to coordinator policy; classification/commit chooses one successful candidate.

## Composite

Composite means one physical peer exposing both Mouse and Keyboard capabilities. It owns one saved record/connection identity while canonical Mouse and Keyboard streams have source ownership attributable to that Composite peer. Composite mouse input remains passthrough in v1.

## Logitech Lift

HID++ `REPROG_CONTROLS_V4` handling is a vendor backend selected automatically by peer capability/identity. It may correct Forward held-state semantics when the active mapping needs it. It must fail safe to Standard HID behavior for unsupported peers and must not become a user-facing toggle.

## Persistence

Product persistence must use schema versioning plus integrity protection and a power-loss-conscious update strategy. Bluetooth bonds/link keys and product records are distinct stores coordinated transactionally.

Persistent product state includes at least:

- saved devices and logical type;
- preferred references;
- per-mouse selected profile kind;
- global CustomTemplate;
- capability/quirk metadata where needed.

## Runtime responsiveness

USB servicing, HAT scanning, rendering and Bluetooth callbacks must not create an unbounded blocking path in the main product loop. Previous evidence showed the importance of non-blocking USB service and avoiding fragile CYW43/Core1 ownership patterns; the new implementation should preserve the proven stable approach rather than recreate experimental multicore coupling.

## Debug prohibition

Production composition has one USB descriptor only: the product Mouse + Keyboard identity.

The following are prohibited in this repository's firmware targets:

- diagnostic USB CDC interfaces;
- debug PID/product descriptor variants;
- UART logging as a diagnostic dependency;
- debug-only LCD pages;
- parallel `debug` UF2 targets;
- acceptance procedures that require serial logs.

CI compiler output, host unit tests, static architecture checks and normal product error/status screens are allowed.
