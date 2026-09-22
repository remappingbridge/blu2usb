# MUX-02 validation — host-pure connection coordinator

Status: **ACCEPTED**

Branch: `gate/mux-02-connection-coordinator`

Accepted dependency:

- MUX-01 accepted head: `f00b7c229f76f7a3111e22a8b128aba66bd9b890`

## Scope implemented

MUX-02 materializes the `connection_coordinator` as a deterministic host-pure state machine. It owns product connection/search policy and operation correlation, but does not call Bluetooth, storage, USB, renderer, HAT or UI code.

The G01 architecture graph has been deliberately refined for the new Mouse-only architecture:

```text
connection_coordinator
    -> domain
    -> device_registry
```

The old placeholder dependencies on `ble_hogp` and `keyboard_transport` were removed. Future transport gates will adapt BLE facts into this coordinator rather than making policy depend on the transport implementation.

## HOME resolver

The coordinator resolves HOME only from saved/live product truth:

```text
saved_count == 0
    -> FIRST_SEARCH

saved_count > 0 && authoritative Mouse exists
    -> CONNECTED

saved_count > 0 && no authoritative Mouse
    -> SAVED_SEARCH
```

Timeout/retry presentation remains a UI concern for MUX-03. The resolver itself does not infer connection from the current screen.

## Search purposes and deadlines

Exactly three purposes exist:

- FIRST: 8000 ms;
- SAVED: 8000 ms;
- NEW: 15000 ms.

### FIRST

FIRST is legal only with zero saved Mice.

On an 8-second deadline it automatically starts a new cycle with a new token. A result from the previous cycle is stale and cannot win.

FIRST cannot be cancelled through the normal search-cancel API because UI Layout 1.0 makes `searching-first` continuous/didactic.

A valid first unsaved Mouse is added with confirmed profile Passthrough and becomes the sole authoritative Mouse.

### SAVED

SAVED is legal only when at least one Mouse is saved and none is authoritative.

An unsaved candidate is ignored. The first ready candidate whose identity already exists in the registry becomes authoritative.

Timeout closes the search as TIMED_OUT. Cancellation closes it as CANCELLED. Later results carrying the old token are stale.

### NEW

NEW is legal only when at least one saved Mouse exists.

A candidate whose identity is already saved is rejected for NEW acceptance and search continues.

An unsaved candidate may enter handoff only when registry capacity permits another record. With a full 16-record registry, the candidate is rejected before any old-session retirement phase, preserving the current Mouse.

The current Mouse remains authoritative through candidate qualification and the first handoff phases.

## Operation token/generation

Each started search/remove operation receives a non-zero monotonically issued token.

Only the currently owned token may complete that operation. A result after:

- timeout;
- cancellation;
- FIRST cycle restart;
- successful first winner;
- successful saved winner;
- completed/cancelled remove;

cannot mutate product state.

Such results increment `stale_result_count` and are rejected.

Only one coordinator operation owner may exist at a time. For example, Remove cannot begin while a search is active, and a search cannot begin while Remove is pending.

## NEW replacement handoff

Qualifying a NEW candidate does **not** make it authoritative.

If an old authoritative Mouse is still live, the required phases are:

```text
FREEZE_OLD_INPUT
    ->
RELEASE_OLD_OWNERSHIP
    ->
RETIRE_OLD_TRANSPORT
    ->
PERSIST_CANDIDATE
    ->
PROMOTE_CANDIDATE
```

Each acknowledgement must match both:

- the active operation token;
- the exact expected phase.

Out-of-order phase acknowledgement is rejected.

Product-state effects are intentionally ordered:

- through FREEZE and RELEASE, the old Mouse remains authoritative;
- RETIRE clears live authority but preserves the old saved record;
- PERSIST adds the candidate as Passthrough but it is still not authoritative;
- PROMOTE makes the candidate authoritative and completes NEW.

If the old Mouse disconnects naturally during NEW before candidate qualification, the coordinator clears only the authoritative/live reference. It does not fabricate a connection. A later valid NEW candidate starts at PERSIST_CANDIDATE because there is no old live transport to freeze/retire.

Cancellation/timeout before handoff preserves the old authoritative Mouse. Once a fully qualified candidate has entered ordered handoff, ordinary search cancellation is rejected so an irreversible partial handoff cannot be represented as a simple rollback.

Physical feasibility of maintaining the old HOGP session while qualifying a provisional candidate remains deliberately unclaimed until mandatory physical gate MUX-05.

## Remove transaction

Remove is correlated by Mouse identity and operation token.

- Begin Remove captures the target ID.
- Cancel leaves registry/live truth unchanged.
- Failed completion leaves the record unchanged.
- Successful completion removes exactly the captured ID.
- Removing an inactive saved Mouse leaves the authoritative Mouse unchanged.
- Removing the authoritative Mouse clears authority.
- Removing the final saved Mouse makes HOME resolve to FIRST_SEARCH.
- Late completion after cancellation is stale and cannot remove the Mouse.

Transport-side release/disconnect ordering for a live removal is not claimed in this host-only gate; it will be bound when transport integration is introduced.

## Automated verification

Implementation-head GitHub Actions run **35706573709** / canonical CI run **#373** executed the host suite.

Host/architecture result:

```text
16/16 tests passed
0 failed
```

Inherited passing tests:

1. `bootstrap_contract`
2. `architecture_contract`
3. `g02_interaction_ux`
4. `g03_renderer_hat`
5. `g04_fixed_usb_hid`
6. `g05_canonical_hid_ownership`
7. `g05_ble_hogp_mouse_passthrough`
8. `g06_profiles_remap_hidpp`
9. `g06_product_persistence`
10. `g06_profile_ui_regressions`
11. `g06_architecture`
12. `screen_contract`
13. `mux01_device_registry`
14. `mux01_architecture`

New MUX-02 tests passing:

15. `mux02_connection_coordinator`
16. `mux02_architecture`

The MUX-02 behavioral test covers:

- HOME resolver;
- FIRST restart token;
- FIRST stale prior-cycle result;
- first-winner-only;
- SAVED eligibility;
- SAVED timeout/cancel and late result;
- NEW rejection of saved candidate;
- NEW preservation of current Mouse before handoff;
- exact handoff ordering;
- candidate not authoritative before promotion;
- old saved record preservation after replacement;
- cancel NEW preserving current;
- timeout NEW preserving current;
- disconnect during NEW without fabricated live state;
- NEW from an offline saved state;
- full-registry rejection before retirement;
- remove cancel/failure/success;
- remove inactive vs authoritative;
- removing last saved Mouse;
- single operation ownership.

## Architecture verification

The coordinator sources contain no Pico SDK, GPIO/SPI, BTstack, CYW43, BLE HOGP, TinyUSB, storage, UX or renderer dependency.

No files under these implementation areas were modified by MUX-02:

- `src/ble_hogp/`;
- `src/bt_runtime/`;
- `src/storage/`;
- `src/app/`;
- `src/usb_hid/`;
- `src/renderer/`;
- `src/hat/`.

## Physical acceptance

None required by MUX-02.

This gate proves product policy as a host state machine only. It makes no claim that RP2350/BTstack can physically keep the current Mouse live while qualifying another candidate.

That physical risk is intentionally isolated in **MUX-05**.

## Gate close

The final branch head must pass canonical CI after this evidence/checklist update.

Human acceptance is required before MUX-03 branches from the exact accepted MUX-02 SHA.


Human acceptance recorded on 2026-09-22. MUX-03 must branch from this accepted head.
