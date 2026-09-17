# G07 Keyboard live UX contract

This file supplements `00-interaction-visual-contract.md` and `01-screen-layouts.md` for the current G07 Keyboard transport gate. Existing inherited layout, geometry, Back, lock, action-on-release and color rules remain unchanged.

## Transport-neutral presentation

`Keyboard` is a logical product capability. The product UI must not expose Bluetooth Classic, BR/EDR, BLE, HOGP or adapter selection on `PAIR KEYBOARD`, Status or success screens.

The canonical search screen remains:

```text
PAIR KEYBOARD
SEARCHING KEYBOARD
TARGET KEYBOARD
AUTO SEARCH ACTIVE
FOUND 0 HID

KEY A: RETRY ON ERROR
KEY B: CANCEL
KEY X: HELP
```

The body is dynamic, so discovery/progress information may change while preserving the fixed 9x21 geometry and hint controls.

## Pairing PIN presentation

When the Keyboard adapter requires the user to enter a PIN/passkey on the physical Keyboard, the Pair Keyboard dynamic body becomes:

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

The shown number is dynamic and may be a six-digit SSP passkey or the four-digit legacy fallback `0000`. The `PIN: ...` row is cyan. The instruction rows remain ordinary off-white-yellow status text. The hint region and accepted pixel relocation do not move because wording changes.

No serial terminal, USB CDC or UART is required to discover the PIN.

## Connected state and semantic color

When a Keyboard is connected:

- `OTHER DEVICES STATUS` displays `KEYBOARD` followed by `CONNECTED`;
- `CONNECTED` is cyan;
- `PAIR KEYBOARD` in `OTHER OPTIONS` is cyan while unselected;
- selection has global priority, so selected `PAIR KEYBOARD` is white;
- moving selection away restores cyan while the Keyboard remains connected;
- `KEYBOARD SAVED` is positive feedback and its visible body text is cyan.

When Keyboard disconnects, `OTHER DEVICES STATUS` displays `NOT CONNECTED` in ordinary status color and `PAIR KEYBOARD` loses the cyan connected-state marker.

## Navigation and cancellation

`KEY B: CANCEL` on Pair Keyboard remains the global one-logical-page Back behavior and returns to `OTHER OPTIONS`. It cancels the current Keyboard discovery/pair transaction but does not alter Mouse state or USB identity.

Pair Keyboard Help does not cancel the pairing transaction merely by opening Help. `ANY KEY: BACK` returns from Help to the owning Pair Keyboard screen.

## Lock and forwarding

Lock affects only LCD/HAT presentation. A connected Keyboard keeps forwarding canonical input while the screen is locked, alongside Mouse/remap output. The first HAT interaction while locked is consumed solely to unlock and return HOME.

## Ownership

Physical Keyboard input uses its own canonical source identity. It may coexist with remap-generated synthetic Escape. Releasing/disconnecting the physical Keyboard must never release a target still owned by the synthetic remap source, and the reverse is also true.
