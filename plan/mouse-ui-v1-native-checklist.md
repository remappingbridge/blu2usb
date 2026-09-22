# Mouse UI Layout 1.0 native integration checklist

Base branch lineage: accepted G06 `7eee024ad4ee726c5a85ffa2f32b9f47187878af`.

The abandoned `experimental/g06-mouse-ux-v1` branch is out of scope.

```markdown
- [x] MUX-00 Freeze migration contract and exact mouse-ui v1.0 source commit
- [x] MUX-00 Map all 30 screen IDs and visible terminology
- [x] MUX-00 Confirm no mouse-ui/mouse-core/UI-Core implementation dependency
- [x] MUX-00 HUMAN ACCEPTANCE

- [x] MUX-01 Implement host-pure device_registry
- [x] MUX-01 Prove max 16 records, stable identity and connected-first presentation
- [x] MUX-01 Prove per-Mouse confirmed profile association
- [x] MUX-01 HUMAN ACCEPTANCE

- [x] MUX-02 Implement host-pure connection_coordinator
- [x] MUX-02 Implement FIRST/SAVED/NEW operation purposes and deadlines
- [x] MUX-02 Implement operation tokens, cancellation and stale/late rejection
- [x] MUX-02 Implement HOME resolver and handoff/remove state machines
- [x] MUX-02 HUMAN ACCEPTANCE

- [x] MUX-03 Implement 30-screen UI Layout 1.0 model in BLU2USB architecture
- [x] MUX-03 Validate Back/Help/Lock/release ownership semantics
- [x] MUX-03 Validate profile/Custom/Saved Devices screen projections
- [x] MUX-03 Validate exact text/layout/color rules on host
- [x] MUX-03 HUMAN ACCEPTANCE

- [x] MUX-04 Expand durable product storage for registry + per-Mouse profile + global Custom
- [x] MUX-04 Preserve BTstack credential-store separation
- [x] MUX-04 Validate corruption fallback and migration/default policy
- [x] MUX-04 HUMAN ACCEPTANCE

- [ ] MUX-05 Extend BLE only for authoritative + provisional candidate lifecycle
- [ ] MUX-05 Host-test cancel/late/timeout/handoff transport lifecycle
- [ ] MUX-05 PHYSICAL: current Mouse remains usable during NEW discovery/qualification
- [ ] MUX-05 PHYSICAL: cancel preserves current Mouse
- [ ] MUX-05 PHYSICAL: controlled handoff promotes only one authoritative Mouse
- [ ] MUX-05 PHYSICAL: fixed USB identity remains stable
- [ ] MUX-05 STOP if live+candidate invariant is not physically feasible

- [ ] MUX-06 Bind FIRST/SAVED/NEW coordinator operations to BLE
- [ ] MUX-06 Map bonds/identities without storing UX state in BLE
- [ ] MUX-06 PHYSICAL: first pair, saved reconnect, timeout/retry and multi-saved winner
- [ ] MUX-06 PHYSICAL: Logitech Lift bonded reconnect regression
- [ ] MUX-06 PHYSICAL: generic G05 Mouse regression

- [ ] MUX-07 Bind active Mouse to its own confirmed profile kind
- [ ] MUX-07 Keep global Custom template semantics
- [ ] MUX-07 Preserve Standard/Escape/Passthrough exact mappings
- [ ] MUX-07 PHYSICAL: Lift Forward HID++ hold/drag/release
- [ ] MUX-07 PHYSICAL: no stuck Mouse/Escape on profile/session changes

- [ ] MUX-08 Bind real snapshots/commands to the embedded 30-screen UX
- [ ] MUX-08 PHYSICAL: full screen/layout/color/control matrix
- [ ] MUX-08 PHYSICAL: Saved Devices order/pagination/remove
- [ ] MUX-08 PHYSICAL: Lock/Help/Back exceptions
- [ ] MUX-08 PHYSICAL: Pair New connected/disconnected flows

- [ ] MUX-09 Run stale/late/cancel/reboot/corruption/full-registry adversarial suite
- [ ] MUX-09 PHYSICAL: repeated pair/reconnect/remove/power-cycle
- [ ] MUX-09 PHYSICAL: long-run locked-display forwarding and HAT responsiveness

- [ ] MUX-10 Run complete inherited + MUX regression suite
- [ ] MUX-10 Confirm no diagnostic CDC/UART/debug artifact
- [ ] MUX-10 Record exact physically accepted SHA
- [ ] MUX-10 Record exact release UF2 digest
```
