# Mouse UI Layout 1.0 native BLU2USB gates

Status: **PROPOSED IMPLEMENTATION SEQUENCE**

Base: accepted BLU2USB G06 commit `7eee024ad4ee726c5a85ffa2f32b9f47187878af`.

Gate prefix: **MUX** (Mouse UX native integration). These gates do not continue the old G07+ numbering because the product UX scope has changed.

---

## MUX-00 — Migration contract freeze

**Type:** documentation / architecture decision  
**Physical test:** none

### Scope

Freeze exactly which G00-G06 behaviors remain inherited and which Mouse-facing UX/product rules are superseded by Mouse UI Layout 1.0.

### Deliverables

- immutable reference to the accepted G06 base;
- immutable reference to the Mouse UI Layout 1.0 source commit;
- screen inventory mapping old BLU2USB screen families to the 30 new screen IDs;
- visible terminology mapping, including Default -> Standard;
- explicit statement that mouse-ui architecture/contracts are not dependencies;
- architecture tests updated only if necessary to declare `device_registry` and `connection_coordinator` as real future libraries.

### Acceptance

No unresolved contradiction remains about:
- FIRST/SAVED/NEW searches;
- one authoritative Mouse;
- Pair New handoff;
- saved registry;
- per-Mouse profile kind;
- global Custom template;
- Lock/Help/Back exceptions;
- synthetic Escape;
- Lift HID++;
- no-debug policy.

---

## MUX-01 — Host-pure Mouse registry and product snapshot

**Type:** implementation  
**Depends on:** MUX-00  
**Physical test:** none

### Scope

Implement `device_registry` and a host-pure product snapshot without changing BLE transport behavior or the physical firmware flow.

### Required model

- maximum 16 saved Mouse records;
- stable Mouse identity independent of list position;
- normalized display name;
- saved/disconnected vs connected truth;
- zero/one authoritative live Mouse reference;
- confirmed per-Mouse profile kind;
- connected-first presentation projection without changing identity;
- global Custom template reference remains outside per-Mouse private data.

### Automated acceptance

- add/update/remove by identity;
- no duplicate logical record for same identity;
- connected-first ordering;
- reorder cannot change a captured removal target;
- profile belongs to Mouse identity, not page index;
- max-capacity behavior deterministic;
- no Pico SDK/BTstack/TinyUSB includes in registry;
- inherited G02-G06 tests remain green.

---

## MUX-02 — Host-pure coordinator, operations and HOME resolver

**Type:** implementation  
**Depends on:** MUX-01  
**Physical test:** none

### Scope

Implement `connection_coordinator` as a deterministic state machine before connecting it to BTstack.

### Operation purposes

- `FIRST`: restartable 8-second cycles, unsaved candidate accepted as first Mouse;
- `SAVED`: bounded 8-second search limited to saved identities;
- `NEW`: bounded 15-second search limited to unsaved identities.

### Required semantics

- operation token/generation;
- one operation owner at a time;
- cancellation on Back/Help/Lock/leave where specified;
- stale/late result rejection;
- current live Mouse preserved during NEW qualification;
- candidate never authoritative before commit;
- HOME resolver derived only from registry/live truth;
- remove transaction by identity;
- explicit handoff phases: freeze old input -> release ownership -> retire old live transport -> persist/confirm candidate -> promote candidate.

### Automated acceptance

Cover all Mouse UI lab semantics at domain level:
- timeout;
- cancel;
- stale result;
- late result;
- saved candidate ignored during NEW;
- unsaved candidate ignored during SAVED;
- first winner only;
- no fabricated connection after current disconnect;
- cancel NEW preserves old live Mouse;
- remove-current vs remove-inactive;
- removing last saved Mouse resolves to FIRST.

No BLE code changes in this gate.

---

## MUX-03 — UI Layout 1.0 host model and renderer projection

**Type:** implementation  
**Depends on:** MUX-02  
**Physical test:** none

### Scope

Replace the old Mouse-facing UX model with the 30-screen Mouse UI Layout 1.0 behavior while retaining the BLU2USB interaction/renderer architecture.

### Requirements

- release-triggered actions;
- exact Back exceptions: `searching-first`, `first-mouse-connected`, `learn-the-keys`;
- Help consumes any key and cancels screen-owned operation;
- global Lock rules based on at least one saved Mouse;
- unlock consumes first interaction and invokes HOME resolver;
- exact FIRST/SAVED/NEW timeout presentation;
- connected HOME menu ordering;
- Standard visible terminology;
- profile active/not-active families;
- disconnect from active profile screen immediately projects not-active variant;
- Custom draft vs confirmed active state;
- one-record-per-page Saved Devices;
- connected record first/cyan;
- stable Remove target by identity;
- title/name rule: first 15 supported characters plus optional ` MOUSE`;
- no Bluetooth Keyboard/Composite screens.

### Automated acceptance

- all 30 canonical screen literals/IDs;
- semantic 21-column bounds;
- frozen instructional geometry;
- navigation matrix;
- color semantics;
- Help/Lock ownership;
- profile disconnect transitions;
- renderer hashes or deterministic cell projections as appropriate.

The UI consumes a host product snapshot and emits semantic coordinator commands. It must compile and test without BTstack.

---

## MUX-04 — Durable registry/product storage expansion

**Type:** implementation  
**Depends on:** MUX-03  
**Physical test:** deferred to MUX-06/MUX-08

### Scope

Expand G06 storage from profile-only product state into a versioned registry record.

### Persistent product data

- up to 16 saved Mouse identities;
- normalized/full name within schema limits;
- per-Mouse confirmed profile kind;
- authoritative/preferred reference as required by reconnect policy;
- global Custom template;
- Custom draft/dirty state if the frozen UX requires reboot persistence;
- schema version/generation/integrity.

### Requirements

- preserve separation from BTstack credential DB;
- power-loss-conscious alternating slots;
- deterministic migration from the accepted G06 profile-only record or explicit safe reset policy documented before implementation;
- no BLE calls inside storage.

### Automated acceptance

- encode/decode round trips;
- corrupt newest -> previous valid fallback;
- capacity boundary;
- record identity/order stability;
- migration/default behavior;
- profile and Custom persistence semantics.

---

## MUX-05 — Pair New transport feasibility risk gate

**Type:** implementation experiment + **mandatory physical validation**  
**Depends on:** MUX-02, accepted G06 transport baseline  
**Do not integrate the full new UX yet.**

### Purpose

Prove the hardest physical invariant before coupling the new UI/persistence to it:

> the current Mouse stays usable while an unsaved replacement candidate is discovered and qualified.

### Scope

Extend BLE HOGP only as much as required to expose provisional candidate facts to the coordinator. Keep old G05/G06 canonical event/remap/HID++ path intact for the authoritative Mouse.

### Automated acceptance

- separate authoritative vs provisional session state;
- no candidate Mouse events reach USB before promotion;
- cancellation disposes candidate only;
- stale candidate completion cannot replace current;
- handoff releases old held Mouse/Escape state before candidate input becomes authoritative;
- old-session late disconnect cannot clear the promoted session;
- no saved candidate accepted for NEW;
- host transport lifecycle harness exercises connect/security/HIDS success/failure/timeout.

### Mandatory physical scenarios

1. Flash exact MUX-05 artifact and reconnect the same G06-accepted Mouse.
2. Verify ordinary movement, buttons, wheel and HAT remain G06-correct.
3. With current Mouse live, start NEW discovery and continue moving/clicking the current Mouse throughout discovery.
4. Put an unsaved second BLE HOGP Mouse into pairing mode; qualify it while the first remains usable.
5. Cancel before commit: first Mouse remains live; second is not saved/promoted.
6. Repeat and commit handoff: all held state from old Mouse releases, old live session retires, new Mouse becomes the only authoritative source.
7. Power/cancel candidate during qualification: first Mouse remains usable.
8. Present an already-saved Mouse during NEW: it is not accepted as NEW.
9. Verify fixed USB Mouse+Keyboard identity never re-enumerates.

### Stop condition

If this gate cannot satisfy scenario 3/4 on the target hardware/BTstack composition, **stop the project here** and revise the Pair New product contract. Do not compensate in UI code.

---

## MUX-06 — FIRST/SAVED/NEW BLE integration and bonded registry mapping

**Type:** implementation + mandatory physical validation  
**Depends on:** accepted MUX-05

### Scope

Connect coordinator operations to the BLE adapter and registry.

### Requirements

- FIRST cycles until first valid unsaved Mouse is committed;
- SAVED uses saved/bonded identity eligibility only;
- NEW uses unsaved eligibility only;
- transport reports identity/name/capability facts; coordinator commits registry state;
- bounded timers are coordinator-owned semantics;
- bonded reconnect remains compatible with Logitech Lift behavior from G06;
- disconnect releases ownership and updates live truth;
- no screen-specific logic inside BLE.

### Physical scenarios

1. Factory/no-saved boot -> continuous FIRST cycles -> first Mouse saved/ready.
2. Reboot with saved Logitech Lift, no new-pair mode -> SAVED reconnect succeeds.
3. Saved Mouse absent -> SAVED expires cleanly.
4. Retry saved search succeeds when Mouse returns.
5. At least two saved mice: first eligible ready winner becomes sole live Mouse.
6. Disconnect current while HOME-visible equivalent state is active -> saved-search semantics become true.
7. Generic G05 Mouse regression.
8. Lift bonded reconnect regression.
9. HAT remains responsive throughout.

---

## MUX-07 — Per-Mouse profiles, Custom and HID++ binding

**Type:** implementation + mandatory physical validation  
**Depends on:** MUX-06, MUX-04

### Scope

Move G06 confirmed profile selection from a single global active-kind assumption to the active Mouse's saved profile kind while preserving the global Custom template.

### Requirements

- newly committed Mouse starts Passthrough;
- STANDARD uses exact G06 Default mapping;
- Escape uses exact G06 Escape mapping;
- Custom is global template + per-Mouse kind;
- current Mouse change loads that Mouse's confirmed profile before its input becomes authoritative;
- profile apply updates runtime then persistent Mouse record before UI success;
- stale apply result cannot change profile after screen/operation cancellation;
- HID++ Forward correction follows the newly active Mouse/profile automatically.

### Physical scenarios

1. Save two Mice with different confirmed profile kinds; reconnect each and verify its own profile.
2. Standard exact mapping.
3. Escape exact mapping and held/released synthetic Escape.
4. Global Custom mapping shared by Mice configured as Custom.
5. Profile switch while held -> no stuck output.
6. Disconnect on active profile screen -> backend truth supports immediate not-active projection.
7. Logitech Lift Forward hold/drag/release under Standard.
8. Return Lift to Passthrough -> native Forward restored.
9. Generic/non-Logitech remains safe.

---

## MUX-08 — Saved Devices, Remove and full embedded UI integration

**Type:** implementation + mandatory physical UX validation  
**Depends on:** MUX-07, MUX-03

### Scope

Bind the already host-accepted 30-screen UX model to real coordinator/product snapshots and the retained Waveshare renderer/HAT.

### Requirements

- `searching-first` and `first-mouse-connected` instructional behavior;
- connected/searching/retry HOME family;
- Pair New/help/retry;
- Remapper families;
- Custom editors;
- Learn the Keys;
- Saved Devices one record per page;
- Remove This;
- Lock/unlock;
- connected/disconnected background projection;
- operation cancellation on Help/Lock/leave;
- exact hint wording and colors from UI Layout 1.0.

### Physical acceptance

Run an enumerated 30-screen matrix on Pico 2 W + Waveshare HAT, including:
- literal text/layout;
- selection/pressed colors;
- Back exceptions;
- Help consumption;
- Lock/backlight/unlock HOME resolver;
- Saved pagination/reorder;
- Remove current/inactive/last record;
- Pair New from connected and disconnected HOME;
- timeout/retry flows;
- profile active -> not-active on disconnect.

No screen is accepted solely because its desktop golden passed.

---

## MUX-09 — Recovery, persistence and adversarial transaction qualification

**Type:** validation hardening  
**Depends on:** MUX-08

### Automated scenarios

- stale/late search completion;
- stale/late profile completion;
- stale/late remove completion;
- reboot during pending semantic state;
- corrupt newest product record;
- full 16-record registry;
- list reorder while Remove is open;
- candidate disconnect during handoff;
- old live disconnect after candidate promotion;
- source ownership release after every abort path.

### Physical scenarios

1. Power-cycle with multiple saved Mice and profiles.
2. Remove power after confirmed profile/storage changes and verify restoration.
3. Disconnect current during Pair New.
4. Cancel/Lock/Help during Pair New and verify old current/saved records.
5. Remove connected Mouse and confirm host receives releases before record/credential removal.
6. Remove final Mouse -> FIRST state.
7. Repeated reconnect/pair/remove cycles without USB re-enumeration or stuck buttons/Escape.
8. Long-run HAT responsiveness and Mouse forwarding while display locked.

---

## MUX-10 — Release qualification and freeze

**Type:** final validation  
**Depends on:** MUX-09

### Acceptance

- all inherited G01-G06 automated regressions that still apply are green;
- all MUX host tests are green;
- MUX-05/06/07/08/09 physical evidence is accepted;
- exact Mouse UI Layout 1.0 behavioral matrix is accepted;
- G05/G06 physical Mouse/Lift behaviors remain accepted;
- fixed USB Mouse+Keyboard identity is unchanged;
- production UF2 has no CDC/UART/debug descriptor/debug screen/debug target;
- CI builds Pico 2 W release artifact;
- exact release SHA and UF2 digest are recorded.

Do not merge or promote a release merely because CI is green.
