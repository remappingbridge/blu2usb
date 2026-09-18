# Continuous implementation checklist

## Current G07 rebuild (2026-09-18)

- [x] G03–G06 physically accepted baseline: `7eee024`, PR #8 (includes Lift HID++).
- [x] POC GT T1 pairing and a/s/d acceptance recorded at `857fd66` for implementation `b04aaf1`.
- [x] Fresh G07 branch from accepted G06; earlier G07 attempts not reused as code base.
- [x] Logical Keyboard facade, Classic Level 2 bonding and deferred HID launch implemented.
- [x] Core1 radio/Core0 USB-HAT-LCD, flash safety, canonical ownership and BLE handle isolation.
- [x] Accepted descriptor/reports, transition tests and synchronous-start negative control.
- [ ] Integrated physical G07 scenarios in `docs/technical/06-g07-keyboard-solution.md` accepted.

The original checklist below is historical numbering/status, superseded for
G03–G07 by the accepted PRs and the current execution record above. Original G07
Lift was absorbed into G06; current G07 is the original Keyboard gate.

```markdown
- [x] G00 Contract Freeze: approve frozen product, UX, architecture and gate documentation
- [x] G01 Create clean CMake/CI/test skeleton and enforce module boundaries
- [x] G01 Confirm production firmware has no CDC/UART/debug artifact path
- [x] G02 Implement host interaction engine with action-on-release
- [x] G02 Implement option wrap, pagination wrap, Help and any-control unlock semantics
- [x] G02 Validate every literal 9x21 screen and Learn The Keys character positions
- [x] G02 Implement offline CustomTemplate editor model with ESCAPE target
- [x] G03 Implement Waveshare Pico-LCD-1.3/ST7789 renderer and HAT adapter
- [ ] G03 Physically validate colors, Learn The Keys, lock and any-control unlock
- [ ] G04 Implement fixed USB Mouse + Keyboard identity from boot
- [ ] G04 Implement canonical HID source ownership/refcount aggregator
- [ ] G04 Physically validate stable USB identity and synthetic Escape
- [ ] G05 Implement BLE HOGP Mouse adapter and descriptor-driven parser
- [ ] G05 Reuse accepted framing/non-blocking/runtime lessons without debug tooling
- [ ] G05 Physically validate BLE Mouse passthrough and HAT responsiveness
- [ ] G06 Implement frozen PASSTHROUGH/DEFAULT/ESCAPE profile tables
- [ ] G06 Implement global persistent-capable CustomTemplate draft/commit independent of Mouse connection
- [ ] G06 Allow LEFT/RIGHT/MIDDLE/BACKWARD/FORWARD/ESCAPE as every Custom target
- [ ] G06 Physically validate offline Custom editing plus all profile mappings
- [ ] G07 Implement automatic Logitech Lift HID++ Forward held-state backend
- [ ] G07 Physically validate Forward tap/hold/drag/release and Standard HID Backward
- [ ] G08 Implement transport-neutral Keyboard facade
- [ ] G08 Implement and physically validate Bluetooth Classic HID BKB-3G adapter
- [ ] G08 Confirm future Keyboard adapters require no UX/domain/USB redesign
- [ ] G09 Implement BLE Composite classification and passthrough canonical streams
- [ ] G09 Physically validate representative Composite hardware
- [ ] G10 Prove BLE Mouse + BLE Composite + Keyboard concurrency
- [ ] G10 Stop and revise product contract if maximum topology is infeasible
- [ ] G11 Implement durable saved registry, preferred references and active slots
- [ ] G11 Persist global CustomTemplate and per-Mouse selected profile kind
- [ ] G11 Implement autonomous boot reconnect while Learn The Keys remains visible
- [ ] G11 Physically validate reboot, replace, remove and failed-pair transaction safety
- [ ] G12 Bind Pair Mouse/Keyboard/Composite, Status and Saved Devices UX to application commands
- [ ] G12 Validate active cyan Device Details and Saved Devices pagination rules
- [ ] G13 Run full physical 9x21 UX/layout/color/interaction acceptance matrix
- [ ] G14 Run final integrated release qualification without terminal-based acceptance
- [ ] G14 Confirm release UF2 contains no diagnostic CDC/UART/debug descriptor/debug screen
- [ ] G14 Freeze the exact physically accepted release SHA and artifact
```
