# MUX-05 validation — Pair New transport feasibility risk gate

Status: **IMPLEMENTED — MANDATORY PHYSICAL ACCEPTANCE PENDING**

Branch: `gate/mux-05-pair-new-transport-risk`

Accepted dependency:

- MUX-04 accepted head: `d536856b662b688cfcc24c3bb9ffdc03ba4f450a`

## Gate purpose

MUX-05 isolates the hardest physical invariant before the new UI/product model is bound to real Bluetooth:

> while an authoritative Mouse is still connected and forwarding to USB, an unsaved second BLE HOGP Mouse can be discovered, secured and HIDS-qualified as a provisional candidate.

The full UI Layout 1.0, registry persistence and coordinator binding remain intentionally out of the physical path in this gate.

## Two transport roles

The BLE HOGP layer now has an explicit two-slot role model:

- slot 0: authoritative/current Mouse;
- slot 1: provisional NEW candidate.

Roles are:

- EMPTY;
- AUTHORITATIVE;
- PROVISIONAL;
- RETIRING.

Only a ready AUTHORITATIVE slot may forward canonical Mouse events.

A ready provisional candidate remains transport-qualified but its HID reports are discarded until promotion.

## Generation ownership

Every NEW attempt receives a non-zero generation.

Cancel, timeout or completion invalidates that generation.

A stale generation cannot:

- qualify a candidate;
- commit;
- replace the current Mouse;
- forward input.

This is host-tested independently in `session_roles.c`.

## BTstack capacity for the risk gate

The production Pico 2 W build receives a gate-local BTstack configuration with:

- `MAX_NR_HCI_CONNECTIONS 2`;
- `MAX_NR_GATT_CLIENTS 2`;
- `MAX_NR_HIDS_CLIENTS 2`;
- LE Central enabled;
- LE privacy address resolution enabled;
- 16 LE device DB / whitelist capacity.

The HIDS descriptor workspace was expanded to 4096 bytes for two HIDS client sessions.

The build remains on the accepted CYW43 threadsafe-background runtime.

## Candidate lifecycle

NEW starts only when a ready authoritative Mouse exists.

The provisional path is:

```text
SCANNING
  -> CONNECTING
  -> SECURING
  -> CONNECTING_HIDS
  -> READY
```

Qualification requires:

- BLE HID Service advertisement;
- no explicit non-Mouse HID appearance;
- not already bonded under the MUX-05 gate-local eligibility check;
- successful LE connection;
- successful pairing/security;
- successful HIDS client setup;
- a report descriptor accepted by the existing BLU2USB Mouse parser.

Only then is `PROVISIONAL_READY` published.

The provisional candidate uses a separate:

- connection handle;
- HIDS CID;
- parser;
- address/type;
- timeout;
- GATT callback.

It does not replace the authoritative session variables.

## Current Mouse preservation

During provisional scanning, connecting, security and HIDS qualification:

- the existing authoritative HIDS session stays open;
- its canonical Mouse pipeline remains the inherited G05/G06 path;
- Logitech HID++ remains attached to the authoritative session;
- vendor output is sent only through the authoritative HIDS CID;
- provisional Mouse reports cannot enter the runtime queue/USB.

This invariant is the subject of the mandatory physical scenarios.

## Cancel / timeout / candidate failure

Cancel or 15-second timeout:

- invalidates only the provisional generation;
- disconnects/cancels only the provisional transport;
- removes a newly-created provisional bond when appropriate;
- preserves the authoritative role/session;
- publishes PROVISIONAL_CLEARED or PROVISIONAL_TIMEOUT.

A candidate disconnect/failure during qualification also preserves the current authoritative Mouse.

An already-bonded Mouse is not accepted as NEW. Re-encryption during candidate security is treated as evidence that the peer is already bonded and the candidate is discarded without deleting that existing bond.

Full registry/privacy identity eligibility is deliberately deferred to MUX-06.

## Controlled handoff

The physical harness exposes an explicit commit only after the candidate is READY.

Before calling the transport commit, the application releases:

- the canonical Mouse source;
- the synthetic-remap source;

from the HID aggregator, forcing held Mouse buttons/synthetic Escape ownership to clear before candidate input may become authoritative.

Transport commit then:

1. marks the old slot RETIRING and freezes forwarding;
2. notifies the vendor/HID++ backend that the old session is ending;
3. disconnects the old authoritative BLE link;
4. waits for its real disconnection completion;
5. promotes the already-qualified candidate;
6. reattaches the vendor/HID++ session to the promoted HIDS CID;
7. publishes PROMOTED;
8. only then allows candidate reports to enter the canonical pipeline.

A late/duplicate disconnection from the retired slot cannot clear the promoted slot.

If the old disconnect request cannot be issued, role state is rolled back to the old authoritative session and the candidate remains provisional.

## MUX-05 physical harness

The accepted G06 embedded UI is intentionally retained except for a temporary risk-gate overlay on the old Pair Mouse screen.

With a current Mouse connected:

1. HOME -> MOUSE OPTIONS -> PAIR MOUSE starts provisional NEW discovery;
2. the screen shows `PAIR NEW RISK`;
3. `CURRENT MOUSE ACTIVE` instructs the tester to keep moving/clicking the old Mouse;
4. after qualification it shows `NEW MOUSE QUALIFIED`;
5. KEY B cancels the candidate;
6. KEY A commits only when qualified;
7. timeout/failure shows `CANDIDATE STOPPED`, where KEY A retries.

This harness is temporary and is not the MUX-08 final UI.

## Fixed USB identity

MUX-05 does not call TinyUSB disconnect/reconnect APIs and does not alter USB descriptors.

The fixed Mouse + Keyboard USB identity remains inherited from G04/G05/G06.

## Automated verification

Canonical CI run **#468**, run id **35735344019**, on implementation head
`cdc02d41035a77326c6ebd11f3d2e15d4f8220da` passed completely.

Host result:

```text
24/24 tests passed
0 failed
```

New MUX-05 suites:

- `mux05_session_roles`;
- `mux05_architecture`;
- `mux05_runtime_events`.

They cover:

- authoritative vs provisional ownership;
- candidate never forwards before promotion;
- cancel invalidates candidate only;
- stale generation cannot qualify/commit;
- commit-start rollback restores the current Mouse;
- ordered handoff;
- late old-session disconnect cannot clear promoted session;
- candidate disconnect preserves current;
- runtime provisional-ready/cleared/timeout/promoted message decoding;
- inherited connected/disconnected runtime compatibility;
- BTstack two-client build configuration;
- no TinyUSB re-enumeration call in the harness;
- held-source release before transport commit.

All inherited G02-G06 and MUX-01 through MUX-04 tests remain green.

## Pico 2 W build

The same run passed:

- pinned ARM toolchain;
- Pico SDK 2.2.0 fetch;
- Pico 2 W production configure;
- ARM build;
- UF2 verification;
- artifact upload.

Implementation-head UF2 SHA-256:

`858437d9b8e3cc538b98dd478e9a74f04baaeece2682513ec987a02a4956b653`

## Mandatory physical acceptance

MUX-05 is **not accepted** from CI alone.

The following must be run on the Pico 2 W:

1. Reconnect the same G06-accepted Mouse and verify ordinary movement/buttons/wheel/HAT.
2. Start Pair New while current Mouse is live and continuously move/click the current Mouse during discovery.
3. Put an unsaved second BLE HOGP Mouse into pairing mode and reach `NEW MOUSE QUALIFIED` while the first Mouse remains usable.
4. Cancel with KEY B and verify the first Mouse remains live and the second does not become authoritative.
5. Repeat qualification, press KEY A to commit, and verify the first Mouse stops owning input and the second becomes the only forwarding Mouse with no stuck held state.
6. Power off/cancel the second Mouse during qualification and verify the first remains usable.
7. Present an already-bonded Mouse during NEW and verify it is not accepted as NEW.
8. Verify the USB Mouse + Keyboard device does not disappear/re-enumerate during search, cancel or commit.
9. Verify HAT input/display remain responsive throughout.

## Stop condition

If the current Mouse cannot remain usable while the provisional candidate is physically discovered and qualified, MUX-05 fails.

Do not continue to MUX-06 or compensate in UI code. The Pair New product contract must be revised first.

## Gate close

Software implementation and automated validation are complete.

The gate remains open until the mandatory physical scenarios are reported and accepted.
