# Implementation gates

The gates below are the canonical implementation sequence for `blu2usb-picow`. Every new gate starts from the exact accepted SHA of its dependencies. Hardware-visible gates remain pending until their physical scenarios pass.

No gate may add diagnostic CDC/UART/debug firmware unless this contract is explicitly revised first.

---

## BLU2USB-G00 — Contract Freeze

**Type:** documentation / architecture decision  
**Depends on:** none

### Scope

Freeze product behavior, screen layouts, preset mappings, CustomTemplate semantics, transport-neutral Keyboard model, unlock behavior, active Device Details color and no-debug policy before code exists.

### Acceptance

- `docs/product/00-product-contract.md` is internally consistent.
- `docs/ux/00-interaction-visual-contract.md` and `docs/ux/01-screen-layouts.md` define literal vs dynamic/example text.
- Default and Escape mappings are explicit, not references to another repository.
- CustomTemplate is editable/committable with zero mouse connected and includes Escape target.
- Pair Keyboard is transport-neutral while preserving Classic HID POC evidence.
- Any HAT control unlocks; Key Y remains primary advertised lock/unlock control.
- Active Device Details values are cyan.
- Debug-only firmware tooling is explicitly prohibited.
- No unresolved product ambiguity remains before G01.

**Evidence:** documentation review + accepted commit.

---

## BLU2USB-G01 — Clean repository bootstrap and architecture enforcement

**Type:** implementation  
**Depends on:** G00

### Scope

Create CMake/CI/test skeleton and module boundaries without product transport behavior.

### Acceptance

- Host tests and Pico 2 W production target configure/build in CI.
- Architecture tests enforce the module ownership in `docs/technical/00-architecture-contract.md`.
- No textual `.c` includes, macro interception or transport leakage into domain/app.
- Production target has no diagnostic CDC, debug PID, debug UF2 or UART diagnostic dependency.
- SDK/toolchain versions are pinned/documented.

**Physical test:** none.

---

## BLU2USB-G02 — Host interaction, UX model and frozen layout validation

**Type:** implementation  
**Depends on:** G01

### Scope

Implement all screen identities, navigation, selection, pagination, action-on-release, lock/help/Learn exceptions and semantic text cells on host only.

### Acceptance

- Every frozen literal row is validated against the 9x21 canonical layouts.
- Option selection wraps Up/Down.
- Pagination wraps Left/Right.
- Status has exactly two pages.
- Learn The Keys character positions and press spans are testable.
- Any HAT control unlock contract is represented; first unlock interaction is consumed.
- Pair Keyboard presentation has no transport-specific BLE/Classic requirement.
- Custom editor is navigable with no Mouse model/connection present.
- `WILL BECOME` options include Escape in the frozen order.

**Physical test:** none.

---

## BLU2USB-G03 — Waveshare renderer and HAT acceptance

**Type:** validation  
**Depends on:** G02

### Scope

Reexpress accepted ST7789/HAT knowledge under the frozen new renderer contract.

### Acceptance

- Pico-LCD-1.3 renders all semantic colors, including the new off-white yellow.
- Learn The Keys alignment matches exact character positions.
- Pressed spans become white and return on release.
- Key Y locks/backlight-off; any first HAT control unlocks and returns HOME without also executing its normal action.
- Option indentation and selected/current precedence are visually correct.
- No terminal/serial tool is required for acceptance.

### Physical scenarios

1. Boot shows Learn The Keys exactly aligned.
2. Exercise every HAT control press/release visual span.
3. Lock with Key Y.
4. Unlock separately with Joy Up, Joy Press and a face key; each only unlocks.
5. Verify HOME and at least one option-list page colors/indentation.

---

## BLU2USB-G04 — Fixed USB identity and canonical HID ownership

**Type:** validation  
**Depends on:** G01

### Scope

Combine the proven fixed USB identity and canonical ownership foundations before Bluetooth transports.

### Acceptance

- Stable USB Mouse + Keyboard enumerate from boot.
- Synthetic Keyboard Escape can be emitted without a physical keyboard.
- Canonical Mouse/Keyboard domain has no remote report IDs/layouts.
- Multiple sources can own the same target without premature release.
- Source-scoped disconnect/release cannot clear another owner.
- Relative movement/wheel/pan are transient and chunk safely to USB report ranges.
- No forced USB re-enumeration APIs.

### Physical scenarios

1. Fresh boot exposes Mouse and Keyboard with no Bluetooth peer.
2. Synthetic Escape test is visible in host GUI.
3. Repeated UI/HAT activity does not alter USB identity.

---

## BLU2USB-G05 — BLE HOGP Mouse passthrough

**Type:** validation  
**Depends on:** G04, G03

### Scope

Reexpress the physically accepted BLE Mouse path without debug instrumentation.

### Acceptance

- One BLE HOGP Mouse discovers, pairs/connects and emits canonical Mouse events.
- Report Map parser handles buttons, X/Y, wheel and horizontal pan where present.
- BTstack/HIDS framing normalization handles duplicated Report ID safely.
- Non-mouse HID candidates are rejected from Mouse pairing.
- USB/HAT servicing remains non-blocking during pairing/reconnect.
- Failure states return to a usable product state rather than freezing.

### Physical scenarios

1. Pair representative BLE mouse.
2. Motion all directions has no ghost movement.
3. Left/Right/Middle hold/release and drag.
4. Wheel/pan where supported.
5. Disconnect during held button releases host state.
6. HAT remains responsive during scan, pair, connect and disconnect.
7. USB Mouse+Keyboard identity never re-enumerates because of Bluetooth state.

---

## BLU2USB-G06 — Profiles, remap engine and offline CustomTemplate

**Type:** validation  
**Depends on:** G05

### Scope

Implement Passthrough, frozen Default/Escape presets, global CustomTemplate, transactional draft/commit and synthetic Escape independently from Logitech vendor behavior.

### Acceptance

- Newly paired Mouse profile kind is Passthrough.
- Default mapping exactly matches product contract.
- Escape mapping exactly matches product contract.
- Custom editor/draft works with zero mouse connected/saved.
- Each source can target Left/Right/Middle/Backward/Forward/Escape.
- `APPLY AND BACK` changes draft only.
- `APPLY CUSTOM` persists the complete global template even with no active Mouse.
- With an active Mouse, `APPLY CUSTOM` also assigns that Mouse to Custom and applies immediately.
- A newly paired Mouse after offline Custom editing still starts Passthrough.
- A saved Mouse using Custom references the current global template.
- Profile switch releases stale synthetic ownership.

### Physical scenarios

1. Boot with no mouse; edit every Custom source using HAT only.
2. Set at least one source to Escape; commit Custom with no mouse.
3. Power-cycle/re-enter editor as allowed by the current persistence milestone and verify the intended state scope.
4. Pair mouse; verify it starts Passthrough, not automatically Custom.
5. Apply Default and validate Forward→Left, Left→Forward, Backward→Right, Right→Backward, Middle→Middle.
6. Apply Escape and validate Forward→Left, Backward→Right, Left→Escape, Right→Backward, Middle→Forward.
7. Apply Custom to active mouse and validate a mapping to Escape plus a mouse-button mapping.
8. Switch profiles while a mapped target is held; no stuck target remains.

Note: durable reboot persistence may be finalized in G11; G06 must still prove the profile/domain behavior and whatever storage scope has been introduced by then.

---

## BLU2USB-G07 — Logitech Lift HID++ Forward quirk

**Type:** validation  
**Depends on:** G06

### Scope

Add the proven Lift Forward held-state correction as an automatic vendor backend without changing generic profile semantics.

### Acceptance

- HID++ `REPROG_CONTROLS_V4` behavior is isolated behind Logitech vendor adapter.
- Forward CID `0x0056` down/hold/up becomes canonical Forward source ownership when correction is required.
- Backward stays Standard HID.
- Unsupported/generic mice fail safe to Standard HID and do not receive endless vendor retries.
- No HID++ setting appears in Custom/profile UX.

### Physical scenarios

1. Lift Passthrough retains ordinary behavior.
2. Under Default, Forward produces Left tap.
3. Hold Forward + move performs stable Left drag until physical Forward release.
4. Disconnect during Forward drag releases Left.
5. Backward remains correct Standard HID remap.
6. HAT and USB identity remain stable.

---

## BLU2USB-G08 — Keyboard transport facade and Classic HID validation

**Type:** validation  
**Depends on:** G04, G03

### Scope

Implement logical Keyboard transport coordination and reexpress the proven BKB-3G Bluetooth Classic HID adapter. The product-facing UX remains transport-neutral and future adapters can be added without changing domain/USB/UI contracts.

### Acceptance

- `PAIR KEYBOARD` requests logical Keyboard, not a hard-coded UX transport.
- Classic HID adapter can discover/pair/connect BKB-3G and emit canonical Keyboard events.
- Physical Keyboard ownership coexists with synthetic Escape ownership.
- Transport facade permits an additional BLE HOGP Keyboard adapter later without changing application command shapes.
- Pairing/reconnect failures do not freeze HAT/UI.

### Physical scenarios

1. Pair BKB-3G through Pair Keyboard.
2. Type representative keys/modifiers in host GUI.
3. Simultaneously generate remap Escape and physical keyboard input without premature release.
4. Disconnect/reconnect keyboard; Mouse ownership remains intact.
5. Screen wording remains transport-neutral.

---

## BLU2USB-G09 — BLE Composite device

**Type:** validation  
**Depends on:** G05, G04

### Scope

Support one BLE HID peer exposing both Mouse and Keyboard capabilities as one Composite record/connection.

### Acceptance

- Classification requires both Mouse and Keyboard application capabilities.
- Canonical Mouse and Keyboard streams share Composite device identity but preserve source ownership.
- Composite Mouse remains passthrough in v1.
- Pair Composite cannot accidentally claim a mouse-only or keyboard-only peer.

### Physical scenarios

1. Pair representative Composite.
2. Validate touchpad/mouse movement/buttons.
3. Validate embedded keyboard keys/modifiers.
4. Confirm no Mouse-slot remap is applied to Composite mouse input.
5. HAT/UI and USB remain responsive/stable.

---

## BLU2USB-G10 — Maximum Bluetooth concurrency risk gate

**Type:** experiment / validation  
**Depends on:** G08, G09

### Scope

Prove or falsify target topology before persistence/UX integration grows around an unproven radio/runtime assumption.

Target topology:

- one BLE Mouse;
- one BLE Composite;
- one Keyboard transport session (initially Classic HID BKB-3G).

### Acceptance

- Required simultaneous sessions remain active and deliver canonical input.
- Cross-source ownership remains correct under concurrent button/key activity.
- HAT remains responsive and USB reports remain valid.
- Required BTstack connection/client/buffer sizing is documented.
- If infeasible, stop dependent gates and revise product contract instead of hiding the limitation.

---

## BLU2USB-G11 — Persistent registry, profiles and autonomous reconnect

**Type:** validation  
**Depends on:** G10, G06

### Scope

Implement durable saved registry, preferred references, active slots, product storage, Bluetooth credentials coordination and boot reconnect.

### Acceptance

- Multiple saved devices per type survive reboot.
- Global CustomTemplate survives reboot with integrity/schema validation.
- Per-Mouse selected profile kind survives reboot; Custom points to global template.
- Preferred peers reconnect automatically while Learn The Keys remains visible.
- Failed replacement pairing does not destroy current saved/active peer.
- Remove transaction clears appropriate credentials, product record, profile association and source ownership.
- Power-loss-conscious storage strategy is documented/tested.

### Physical scenarios

1. Save multiple peers and reboot Pico.
2. Verify preferred reconnect without navigation.
3. Verify active mouse profile restoration.
4. Verify global CustomTemplate restoration edited previously without mouse.
5. Remove inactive and active devices and verify list/ownership cleanup.
6. Attempt failed replacement pair and confirm existing peer remains usable.

---

## BLU2USB-G12 — Pairing, Saved Devices and Status UX integration

**Type:** validation  
**Depends on:** G11, G03

### Scope

Bind frozen Pair Mouse/Keyboard/Composite, Status, Saved Devices, Device Details and Remove Device screens to coordinator commands/projections.

### Acceptance

- Pairing screens automatically start the correct logical discovery transaction.
- Pair Keyboard remains transport-neutral.
- Pair success screens use cyan body feedback.
- Status has exactly Mouse Status + Other Devices Status with correct connected colors.
- Saved Devices paginates four per page, wraps and collapses empty trailing page.
- Active Saved Device row is cyan.
- Device Details active name/dynamic values are cyan; inactive values off-white yellow.
- Remove returns to Saved Devices and is one user action.
- No screen directly calls transport primitives.

### Physical scenarios

Enumerate navigation and state transitions for every screen in `docs/ux/01-screen-layouts.md`, including active/inactive color variants and page wrap.

---

## BLU2USB-G13 — Full frozen UX acceptance

**Type:** validation  
**Depends on:** G12, G07

### Scope

Validate the complete 9x21 layout/interaction contract against hardware.

### Acceptance

- Every literal screen matches canonical text, indentation and row placement.
- Example/dynamic pages stay within granted freedom and color rules.
- Off-white yellow, light gray, white, cyan and magenta semantics are consistent.
- Action occurs on release only.
- Option and pagination wrap rules pass.
- Help `ANY KEY: BACK` including Key Y passes.
- Learn The Keys exact character alignment and spans pass.
- Any-control unlock + Key Y discoverability pass.
- Custom editor works entirely without physical mouse interaction.

---

## BLU2USB-G14 — Release qualification

**Type:** validation  
**Depends on:** G13

### Scope

Produce the release candidate only from accepted gate SHAs and run integrated stability qualification.

### Acceptance

- Cold boot USB identity stable before Bluetooth.
- Automatic reconnect and persistent state work repeatedly.
- Mouse remap, Lift held-state, physical Keyboard and Composite concurrency behave without stuck ownership.
- Disconnect/reconnect during held actions releases correctly.
- Lock/unlock never stops forwarding/reconnect.
- Remove/replace transactions remain safe.
- Production artifact contains **no debug CDC, debug UART dependency, debug descriptor/PID, debug screen or alternate debug UF2**.
- CI is green and the release UF2 is the exact physically accepted SHA.

### Physical scenarios

A final numbered regression matrix must cover all previously accepted hardware behaviors without terminal-based acceptance.
