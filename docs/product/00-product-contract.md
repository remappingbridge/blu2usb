# Product contract

## Purpose

`blu2usb-picow` is a plug-and-play HID appliance for Raspberry Pi Pico 2 W. The host must see a stable firmware-owned USB Mouse + Keyboard identity from boot. Bluetooth connection changes must not require host software or USB re-enumeration.

The LCD/HAT configures and inspects the appliance; it is not required for normal forwarding or reconnect.

## Logical device types

The product recognizes:

- **Mouse** — mouse-capable peer. Mouse profiles apply only to the active Mouse slot.
- **Keyboard** — keyboard-only peer. The UX is transport-neutral. Supported implementations may include Bluetooth Classic HID, BLE HOGP Keyboard or future keyboard transports behind the same canonical adapter boundary.
- **Composite** — one Bluetooth HID peer exposing both Mouse and Keyboard capabilities. Composite mouse input remains passthrough in v1 unless explicitly changed later.

Runtime has at most one active device in each logical slot: Mouse, Keyboard and Composite. Multiple saved devices may exist for every type.

## Pairing and replacement

Pairing is transactional:

`discover -> authenticate -> classify -> persist -> commit active/preferred transition`.

A currently working peer is not destroyed merely because a pairing attempt starts. If the candidate fails before commit, the previous saved/active peer remains valid. After successful commit, the previous active peer of the same logical type may be disconnected while remaining saved unless explicitly removed.

The `PAIR KEYBOARD` UX does not expose BLE-vs-Classic as a product choice. It searches the keyboard transports enabled by the firmware and classifies a successful candidate as Keyboard.

## Saved, preferred and active

These are distinct concepts:

- **saved**: persistent known peer;
- **preferred**: reconnect priority for a logical type;
- **active**: current runtime owner of that slot.

Removing a device is one user action that coordinates removal of credentials, product record, preferred reference, active ownership and mouse-profile association where applicable.

## Mouse profile kinds

The profile kinds are exactly:

- `PASSTHROUGH`;
- `DEFAULT REMAP`;
- `ESCAPE REMAP`;
- `CUSTOM REMAP`.

A newly paired Mouse starts in `PASSTHROUGH`.

### PASSTHROUGH

- Left -> Left
- Right -> Right
- Middle -> Middle
- Forward -> Forward
- Backward -> Backward

### DEFAULT REMAP

This preset is frozen:

- Forward -> Left
- Left -> Forward
- Backward -> Right
- Right -> Backward
- Middle -> Middle

### ESCAPE REMAP

This preset is frozen:

- Forward -> Left
- Backward -> Right
- Left -> Escape
- Right -> Backward
- Middle -> Forward

`Escape` is emitted through the firmware-owned USB Keyboard identity. Mouse and synthetic keyboard ownership must remain release-safe.

## CustomTemplate

Custom editing is a property of the Pico appliance, not a learn-mode tied to clicking a physical mouse.

There is one persistent **global `CustomTemplate`** with exactly five source buttons:

- Left;
- Right;
- Middle;
- Forward;
- Backward.

Each source may target exactly one of:

- Left;
- Right;
- Middle;
- Backward;
- Forward;
- Escape.

The Custom editor is fully usable with **zero mouse connected and with no previously saved mouse**.

Entering a per-source `WILL BECOME` screen edits a draft. `KEY A: APPLY AND BACK` updates only that in-memory draft and returns to `EDIT CUSTOM REMAP`. It does not require mouse input and does not persist the whole template by itself.

`KEY A: APPLY CUSTOM` on `EDIT CUSTOM REMAP` commits the complete draft to the persistent global `CustomTemplate`.

If an active Mouse exists when `APPLY CUSTOM` is committed, that Mouse's profile kind becomes `CUSTOM REMAP` and the newly committed template is applied immediately. If no Mouse is active, the template is still persisted, but no mouse profile assignment is created. A future newly paired Mouse still starts in Passthrough.

A saved Mouse whose selected profile kind is `CUSTOM REMAP` references the current global `CustomTemplate`; the per-mouse record does not own a separate custom mapping table in v1.

Preset-profile `APPLY` actions require an active Mouse. With no active Mouse, no profile assignment or persistence occurs and the pre-apply screen remains unchanged.

## Logitech Lift behavior

Known Logitech quirks are automatic implementation backends, never user-facing profile options.

When the active profile remaps Lift Forward and held-state correction is required, the firmware may use Logitech HID++ `REPROG_CONTROLS_V4` to recover correct down/hold/up semantics. Backward remains Standard HID unless later product documentation explicitly changes it.

Generic mice must continue to use Standard HID remapping without requiring Logitech vendor behavior.

## Unlock policy

Lock affects only LCD/HAT presentation. Bluetooth, USB, reconnect and remapping continue.

While locked, the first physical HAT control interaction is consumed only to unlock and return to HOME. **Any HAT control can unlock.** Key Y remains the primary discoverable control and therefore the UI may say `KEY Y: LOCK / UNLOCK`.

## Debug policy

The product firmware must not ship or maintain diagnostic USB CDC, UART diagnostic output, debug-only USB identities, debug screens or parallel debug firmware variants.
