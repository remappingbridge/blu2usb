# G07 rebuild from physically accepted G06

Authority: operator instruction on 2026-09-18 after successful GT T1 a/s/d test.

Base is G06 `7eee024ad4ee726c5a85ffa2f32b9f47187878af`, physically accepted in
[PR #8](https://github.com/tiagooliveirajs/blu2usb/pull/8). New branch:
`gate/g07-validated-classic-from-g06`. No earlier G07 branch is an implementation
base, and no gate branch is automatically merged to main.

The original plan's Logitech G07 was incorporated and accepted in G06; the
current execution name **G07** means the original plan's Keyboard facade/Classic
HID gate (original G08), as already recorded in PR #9. Its acceptance criteria
are retained. This decision explicitly reconciles the old plan numbering.

Validated input is POC `picow_ble_usb_hid_bridge_gui@b04aaf146844cead484e2a4191a4b07806bdf6f6`,
marker `deferred-hid-after-bond-v1`. Pairing, HID setup and a/s/d press/releases
were physically reported successful. Earlier rejected G07 candidates are not
proof of compatibility and their code is not transplanted.

The runtime amendment to G05's single-core restriction is deliberate:

- One shared BLE+Classic radio/runtime on Core1, explicit 8192-byte stack and
  continuous run loop, carrying the now-validated POC execution envelope.
- Core0 owns existing USB Mouse+Keyboard, HAT/LCD and product composition.
- Transport reports stay in adapters. Bounded runtime messages and atomic
  command/status mailboxes cross the cores; application code never calls radio
  APIs. Serialize product flash writes with radio callbacks via the runtime lock.
- Both cores register for flash-safe execution; existing product storage and
  credential regions stay separate, with existing profile schema unchanged.
- BLE connection-specific events must match its own handle; the deliberate
  Classic bonding disconnect must not reset Mouse state.

This replaces only the G05 architecture test's old blanket multicore prohibition,
with checks of the explicit ownership and flash-safety boundaries. Other G03–G06
UX, profile, remap, vendor, persistence and fixed USB requirements remain active.

No diagnostic CDC, UART, alternate USB descriptor or debug firmware is added.
The POC CDC was an observation tool, not the Bluetooth fix. Existing dynamic
Pair Keyboard body rows may show ordinary progress/error feedback. Frozen
literal navigation, layout, colors and actions remain normative.

Automated completion of G07 produces a production UF2 candidate. Its integrated
physical acceptance remains pending a new test, including simultaneous Mouse,
Keyboard and synthetic Escape; the isolated POC acceptance cannot substitute it.
