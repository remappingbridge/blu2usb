# Reusable evidence from prior work

This document records what may be reused conceptually from earlier repositories. It is evidence, not normative product specification.

## Physically accepted behavior worth preserving

### Waveshare HAT / renderer

Prior accepted work proved the Waveshare Pico-LCD-1.3/ST7789 path, button scanning, release-triggered interaction feedback and lock/unlock behavior on the target hardware.

The new repository may reexpress the same pinout, debounce strategy, font/rendering knowledge and semantic-color renderer, subject to the new frozen visual contract.

### Fixed USB identity

Prior accepted work proved a firmware-owned USB Mouse + Keyboard identity that is present from boot and independent of Bluetooth peer state.

The new implementation should retain this principle while rebuilding the code cleanly.

### Canonical HID ownership

Prior host-tested work proved the need for per-source held ownership, idempotent repeated press, source-scoped release, coexistence of physical and synthetic owners and transient relative motion accumulation.

These semantics should be reimplemented and retested early.

### BLE HOGP Mouse

Prior physical acceptance proved BLE HOGP mouse passthrough on Pico 2 W after several important fixes:

- non-blocking USB service so HAT/UI remains responsive;
- avoid fragile dedicated-Core1 CYW43/BTstack ownership;
- reject non-mouse HID candidates in the Mouse path;
- normalize BTstack/HIDS report framing where Report ID may also appear in the payload;
- handle connect/security/HIDS failure states without trapping the runtime.

This knowledge should be retained without copying debug instrumentation.

## Logitech Lift POC evidence

Prior POC work identified Logitech HID++ `REPROG_CONTROLS_V4` and Forward CID `0x0056` as a viable way to recover correct Forward down/hold/up semantics for remapped drag behavior.

The implementation must remain an automatic vendor backend. Backward remains Standard HID under the frozen product contract.

## Keyboard POC evidence

Prior POC work established that the Goldentec BKB-3G target is Bluetooth Classic HID and provided useful inquiry/pairing/report-forwarding knowledge.

That evidence validates one Keyboard transport adapter; it does **not** make `Keyboard` synonymous with Classic HID. The new architecture must allow additional keyboard transports later.

## What must not be copied as product behavior

- old mouse-click learn mode for Custom Remap;
- old Default/Escape preset tables when they differ from the frozen new contract;
- G07 logic that required an active mouse before Custom could be edited/committed;
- Custom target validation that omitted Escape;
- debug CDC/UART firmware variants;
- experimental `.c` textual includes, macro interception or transport/UI coupling;
- any old screen text that differs from `docs/ux/01-screen-layouts.md`.

## Evidence acceptance rule

When an implementation gate says prior evidence may be reused, the new code still needs its own automated checks and, for hardware-visible behavior, its own enumerated physical acceptance unless the gate explicitly declares a documentation-only carry-forward.
