# MUX-00 — G00-G06 inheritance and Mouse UI 1.0 contract delta

Status: **FROZEN FOR IMPLEMENTATION — HUMAN ACCEPTANCE PENDING**

## Architecture rule

Mouse UI Layout 1.0 supersedes the **Mouse-facing product/UX contract** frozen in the original G00, but it does not discard the technical architecture and physically accepted foundations established by G01-G06.

## Inheritance matrix

| Area | G00-G06 decision | MUX-00 decision |
|---|---|---|
| Pico target | Pico 2 W / RP2350 | inherit |
| Waveshare ST7789/HAT path | accepted G03 | inherit |
| release-triggered physical actions | accepted | inherit, with new per-screen semantics |
| stable USB Mouse + Keyboard identity | accepted G04 | inherit unchanged |
| synthetic USB Escape | accepted G04/G06 | inherit unchanged |
| canonical source ownership | accepted G05 | inherit unchanged |
| BLE HOGP Report Map Mouse parser | accepted G05 | inherit/protect |
| non-blocking USB/HAT during BLE | accepted G05 | inherit/protect |
| bonded BLE reconnect | accepted G06 | inherit, later generalized to saved registry |
| profile mappings | Passthrough/Default/Escape/Custom | mapping semantics inherit |
| visible Default name | `DEFAULT REMAP` | superseded by `STANDARD REMAP` |
| Custom model | one global template/draft | inherit |
| Logitech Lift HID++ | automatic vendor backend | inherit unchanged |
| product storage | G06 two-slot profile state | foundation inherited; schema expanded later |
| boot UX | Learn The Keys | superseded by HOME resolver / FIRST search |
| Mouse connection model | effectively one current Mouse | formalized as one authoritative Mouse + registry |
| saved registry | later-gate concept | now required, capacity 16 |
| Pair Mouse | generic pair/reconnect presentation | superseded by FIRST/SAVED/NEW purposes |
| Keyboard/Composite UX | present in old screen contract | removed from this product cycle |
| no-debug production policy | frozen | inherit unchanged |

## Frozen product invariants

### Registry and authority

- Maximum saved Mouse records: **16**.
- Product-visible authoritative Mouse count: **0 or 1**.
- Record identity is stable and independent of presentation order.
- Connected Mouse is presented first in Saved Devices.
- Reordering cannot change an already-selected Remove target.
- Each saved Mouse has its own confirmed profile kind.
- Global Custom template is shared by every Mouse whose kind is Custom.

### Search purposes

#### FIRST

- Applies when no Mouse is saved.
- Automatic search.
- Restartable **8-second** cycles.
- Continues logically until one valid unsaved BLE HOGP Mouse is authenticated, classified, persisted and made ready.
- HAT controls on `searching-first` are didactic only.

#### SAVED

- Applies when at least one Mouse is saved and no Mouse is live.
- Automatic HOME search.
- Bounded **8 seconds**.
- Only saved/known eligible identities may win.
- Timeout/cancel resolves to `home-retry`.

#### NEW

- Explicit Pair New operation.
- Bounded **15 seconds**.
- Only an unsaved Mouse may win.
- A saved candidate is ignored for NEW acceptance.
- Existing live Mouse remains authoritative and usable while candidate is qualified.
- Cancel/timeout before handoff preserves the existing live Mouse and saved state.

### Replacement handoff

A NEW candidate becomes authoritative only after qualification. Commit order is frozen:

1. stop accepting new input from old live session;
2. release its held Mouse and synthetic Escape ownership;
3. retire/disconnect old live session while preserving its saved record/bond;
4. persist/confirm the candidate;
5. promote candidate as the only authoritative Mouse;
6. end NEW operation.

The transport feasibility of keeping the current Mouse usable during candidate qualification is deliberately deferred to mandatory physical risk gate MUX-05. UI code may not fake this invariant.

### Operation ownership

Pair New, profile apply, Custom apply and Remove are operation-owned state transitions.

- Operations carry a token/generation.
- Back/Help/Lock/leave cancels operations where UI Layout 1.0 declares cancellation.
- A late or stale completion cannot promote state after cancellation/ownership change.
- Screen navigation is never used as a substitute for product truth.

## Terminology and interaction delta

| Old G06 visible concept | Mouse UI 1.0 |
|---|---|
| `DEFAULT REMAP` | `STANDARD REMAP` |
| `DEFAULT ... APPLIED` | `STANDARD ... ACTIVE` |
| generic `PAIR MOUSE` | FIRST, SAVED or `PAIR NEW MOUSE` depending on purpose |
| `MOUSE SAVED` | `FIRST MOUSE CONNECTED` only for first-use success |
| `LEARN THE KEYS` displayed title `PRESS TO LEARN A KEY` | displayed title `PRESS TO LEARN KEYS` |
| Saved Devices up to four records/page | one Mouse/page |
| Device Details page | folded into Saved Devices page |
| Custom target order LEFT/RIGHT/MIDDLE/BACKWARD/FORWARD/ESCAPE | LEFT/RIGHT/MIDDLE/ESCAPE/FORWARD/BACKWARD |
| profile success page semantics | ACTIVE/not-active family |
| Bluetooth Keyboard/Composite product menus | absent |

Internal G06 enum/function names containing `DEFAULT` may remain temporarily during migration if behavior is unchanged. User-visible projection must use STANDARD. Renaming internals is optional and must not create avoidable transport/profile risk.

## Back rules

`KEY B` is primary Back/cancel when a parent/cancel action exists, with explicit non-navigation exceptions:

- `searching-first`;
- `first-mouse-connected`;
- `learn-the-keys`.

Important special cases:

- `home-searching`: cancel SAVED search -> `home-retry`;
- Pair New: cancel -> HOME resolver;
- `remapper-options`: -> HOME resolver;
- profile active/not-active: -> `remapper-options`;
- Custom source editor: -> `custom-edit` without applying current selection;
- Remove: cancel -> `saved-devices`.

No `JOY LEFT: GO TO HOME` behavior exists.

## Lock and Help

When at least one Mouse is saved, Key Y is global Lock on ordinary product screens unless Help owns interaction.

- Lock cancels the operation/search owned by the current screen.
- Lock does not delete a Mouse, change confirmed profile, or intentionally disconnect the live Mouse.
- First complete HAT interaction while locked is consumed, unlocks, and invokes HOME resolver.
- Help has priority: any HAT control exits Help and is consumed.
- Opening Help cancels screen-owned asynchronous work.
- `searching-first` cannot Lock because no Mouse is saved.
- `first-mouse-connected` and `learn-the-keys`: A/B/X/joystick are didactic; Key Y Locks.
- Key B is deliberately inert on those instructional exceptions.

## Profile delta

Mappings are unchanged from accepted G06:

### Passthrough

Left->Left, Right->Right, Middle->Middle, Forward->Forward, Backward->Backward.

### Standard

This is the accepted G06 Default mapping:

Left->Forward, Right->Backward, Middle->Middle, Forward->Left, Backward->Right.

### Escape

Left->Escape, Right->Backward, Middle->Forward, Forward->Left, Backward->Right.

### Custom

Sources: Left, Right, Middle, Forward, Backward.

Allowed targets remain the same set:

Left, Right, Middle, Escape, Forward, Backward.

Only the displayed selection order changes.

## Scope removals

The following original UX/product flows are not part of this migration:

- Pair Keyboard;
- Keyboard Saved;
- Pair Composite;
- Composite Saved;
- Other Devices Status;
- Other Options;
- Keyboard/Composite Device Details.

The BLU2USB technical architecture may retain future extension seams for keyboard/composite transports. No MUX gate should remove a clean extension point merely because the current UX does not expose it.

## No unresolved product ambiguity

For implementation of MUX-01 onward, this document plus the source freeze and screen map resolve the identified G06-vs-UI-1.0 deltas. Any newly discovered conflict must stop the affected gate and be added to the migration contract before code proceeds.
