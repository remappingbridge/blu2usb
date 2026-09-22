# Mouse UI Layout 1.0 native rebuild plan

Status: **PLANNING BASELINE — NO IMPLEMENTATION CODE YET**

This plan starts from the exact accepted BLU2USB G06 head:

`7eee024ad4ee726c5a85ffa2f32b9f47187878af`

The abandoned branch `experimental/g06-mouse-ux-v1` is explicitly **not** an input to this work. No code, persistence schema, BLE session manager or UX model from that experiment may be cherry-picked implicitly. Any idea that is reused later must be justified independently against the accepted G00-G06 architecture and tests.

## 1. Why this rebuild exists

Mouse UI Layout 1.0 is a substantially different product flow from the UX frozen at BLU2USB-G00. It introduces a saved-Mouse registry, a single authoritative live Mouse, separate FIRST/SAVED/NEW searches, cancellable asynchronous operations, replacement handoff, per-Mouse profile state, one-record-per-page Saved Devices, and a 30-screen Mouse-only experience.

The failed experimental integration attempted to introduce several of those concerns directly inside the existing application/BLE path. This rebuild instead follows the original BLU2USB architecture rule: transport produces canonical facts; domain/coordinator owns product state and transactions; UX projects that state and emits semantic commands.

## 2. Historical BLU2USB line that remains authoritative

The new work preserves the engineering direction established from G00 through G06.

### G00 — contracts before implementation

Keep the principle that product behavior, interaction, visual rules and architecture are explicit before code changes. For this rebuild, Mouse UI Layout 1.0 replaces the old Mouse-facing UX/product behavior, but **does not replace the BLU2USB technical architecture principles**.

### G01 — clean module boundaries and CI

Keep:
- CMake/host-test/Pico production split;
- architecture enforcement;
- pinned toolchain/SDK;
- no diagnostic CDC/UART/debug firmware;
- `app` as composition, not a transport implementation.

### G02 — host-pure interaction and UX

Keep:
- release-triggered actions;
- interaction ownership/epoch semantics;
- host-testable navigation and projection;
- UI behavior testable without BTstack, TinyUSB, GPIO or SPI.

The screen inventory and navigation are replaced by Mouse UI Layout 1.0.

### G03 — renderer and Waveshare HAT

Reuse the accepted ST7789/HAT/font/color/debounce path. Do not reimplement the LCD/HAT stack merely to adopt the new UX.

### G04 — fixed USB identity and canonical ownership

Preserve unchanged:
- stable USB Mouse + Keyboard identity from boot;
- synthetic Escape through the fixed Keyboard interface;
- canonical per-source held ownership;
- release safety on disconnect/profile change;
- no forced USB re-enumeration.

Mouse UI 1.0 is Mouse-only for Bluetooth input, but the USB Keyboard interface remains because Escape is a synthetic output.

### G05 — BLE HOGP Mouse path

Treat the accepted BLE HOGP transport as a protected baseline:
- Report Map driven Mouse parsing;
- non-mouse rejection;
- framing normalization;
- non-blocking USB/HAT servicing;
- reconnect/failure recovery;
- canonical Mouse event emission.

Do not redesign this transport until a dedicated transport gate requires an extension and its inherited physical regressions are listed.

### G06 — profiles/remap/storage/HID++

Preserve:
- Passthrough semantics;
- Default mapping semantics, presented as **STANDARD** in UI Layout 1.0;
- Escape mapping semantics;
- global Custom template;
- synthetic Escape release safety;
- two-slot/integrity-aware product storage foundation;
- Logitech Lift HID++ Forward held-state correction;
- bonded reconnect behavior.

G06 currently stores one global active profile rather than a full per-Mouse registry. That limitation is expanded deliberately in later gates; it must not be bypassed with ad-hoc state in `app`.

## 3. Frozen UX source

The behavior source for the new frontend is Mouse UI Layout 1.0 from:

- repository: `remappingbridge/mouse-ui`
- frozen/current reference commit inspected for this plan: `5f269e9625ae0d02a85b5d39eb87026edc448068`
- status in that repository: **UI Layout 1.0 — FROZEN / ACCEPTED**

Normative behavior inputs for implementation are:
- `docs/product/ui-layout-v1.0.md`
- `docs/spec/01-screen-reference.md`
- `docs/spec/02-controls-lock-help.md`
- `docs/spec/03-first-start-and-pairing.md`
- `docs/spec/04-home-and-connection.md`
- `docs/spec/05-remapping.md`
- `docs/spec/06-saved-devices.md`

The SDL desktop architecture, mock implementation, UI-Core adapter, contracts repository, private C types and lab tooling are **not implementation dependencies** for BLU2USB.

## 4. Product invariants to implement natively

1. Up to 16 saved Mouse records.
2. Zero or one authoritative live Mouse.
3. Saved identity and connected identity are distinct facts.
4. The connected Mouse is presented first in Saved Devices without changing record identity.
5. FIRST search: no saved Mouse; restartable 8-second cycles until first valid Mouse is committed.
6. SAVED search: saved records only; bounded 8-second HOME search.
7. NEW search: unsaved Mouse only; bounded 15-second Pair New search.
8. Pair New does not destroy or disable the current live Mouse merely because search starts.
9. A saved Mouse is never accepted as the winner of Pair New.
10. A replacement candidate is not authoritative until qualification and handoff commit complete.
11. Cancel, Help, Lock or leaving an operation-owning screen invalidates that operation; late/stale completion cannot mutate product state.
12. Every accepted asynchronous command/result is correlated with an operation token/generation.
13. Passthrough is the initial profile for a newly saved Mouse.
14. Each saved Mouse stores its own confirmed profile kind.
15. STANDARD is the visible name for the existing G06 Default mapping.
16. The Custom template remains global; a Mouse set to Custom references that global template.
17. Disconnect/profile switch/remove/handoff release all held Mouse and synthetic Escape ownership belonging to the affected session before state promotion.
18. Logitech HID++ remains automatic backend behavior, never a UX mode.
19. HOME is resolved from saved/current product truth, never from the previous screen alone.
20. Bluetooth Keyboard/Composite pairing is not exposed by Mouse UI Layout 1.0. Existing architectural extension points may remain, but this project does not implement those UX paths.

## 5. Layering for the rebuild

The intended dependency flow is:

```text
BTstack/CYW43
    |
ble_hogp adapter
    |
transport facts / candidate facts
    |
connection_coordinator
    +---- device_registry
    +---- storage
    +---- profiles/remap
    |
product snapshot + correlated operation results
    |
ux_model / navigation
    |
projector / renderer
    |
ST7789

canonical Mouse events
    |
remap -> hid_aggregator -> usb_hid
```

### Domain / registry

Introduce the already-planned `device_registry` module instead of storing saved Mouse records in `app` or `ble_hogp`.

It owns logical Mouse identity, display name, saved state, per-Mouse profile kind and presentation order. It does not call BTstack or flash directly.

### Connection coordinator

Introduce the already-planned `connection_coordinator` as the owner of:
- HOME resolution inputs;
- FIRST/SAVED/NEW operation purpose;
- deadlines;
- operation tokens;
- cancellation;
- candidate vs authoritative state;
- replacement handoff policy;
- remove policy;
- stale/late result rejection.

It consumes transport facts; it does not parse BLE packets.

### BLE HOGP

Keep BLE-specific mechanics here:
- scanning/filtering;
- connection/security/HIDS;
- bond/peer identity mapping;
- Report Map parsing;
- device name acquisition where transport supplies it;
- transport-level candidate lifecycle.

It must not choose screens or directly mutate registry/profile UI state.

### UX model

Own:
- the 30 canonical screen identities;
- selection/page/help/lock state;
- action-on-release behavior;
- HOME presentation choice from product snapshot;
- semantic command emission.

It does not call BLE, storage, remap or TinyUSB.

### Storage

Expand the G06 product record only after the registry/domain structures are proven in host tests. Bluetooth credentials remain BTstack-owned and separate.

## 6. The highest-risk requirement

Pair New requires the current Mouse to remain authoritative and usable while an unsaved candidate is discovered and qualified.

This is a **transport feasibility requirement**, not a UI requirement. It must therefore be proved in an isolated physical risk gate before full UX integration.

The risk gate must establish one of two acceptable implementations:

A. the current HOGP session and one provisional candidate session coexist long enough to qualify the candidate, while only the old session is authoritative; or

B. another transport mechanism is demonstrated that satisfies the same visible product invariant without pretending success.

If neither is physically feasible on the target stack, the work stops and the product contract is revised. The UI must not be implemented around a fake assumption.

## 7. Explicit non-goals

- no cherry-pick from `experimental/g06-mouse-ux-v1`;
- no adoption of `mouse-ui` SDL architecture;
- no adoption of `mouse-core` or UI-Core contract code;
- no screen-to-BTstack calls;
- no persistence owned by the BLE adapter;
- no profile state owned by navigation;
- no second authoritative Mouse;
- no Bluetooth Keyboard/Composite UX in this cycle;
- no debug CDC/UART/debug UF2;
- no physical acceptance inferred from host tests.

## 8. Gate execution rule

Every implementation gate starts from the exact accepted SHA of the previous gate. Automated tests are necessary but never substitute for a physical gate whose acceptance says hardware is required.

A gate that changes BLE session ownership, reconnect, pairing, HID++, USB identity, HAT behavior or physical persistence cannot be accepted solely from desktop/host simulation.

See `mouse-ui-v1-native-gates.md` for the sequence and `mouse-ui-v1-native-checklist.md` for the execution checklist.
