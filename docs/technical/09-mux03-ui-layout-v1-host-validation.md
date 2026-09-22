# MUX-03 validation — Mouse UI Layout 1.0 host model and projection

Status: **IMPLEMENTED — HUMAN ACCEPTANCE PENDING**

Branch: `gate/mux-03-ui-layout-v1-host`

Accepted dependency:

- MUX-02 accepted head: `82945394fe7949cccb28c77adc2f8dc2d8d54b77`

## Scope implemented

MUX-03 implements the frozen Mouse UI Layout 1.0 as a new host-pure surface inside the existing BLU2USB `ux_model` and `renderer` modules.

The accepted G06 firmware binding is intentionally retained in parallel. MUX-03 does **not** switch the production application to the new UX and does not change BLE, BTstack, storage, pairing, USB identity, HAT input or runtime remap behavior.

This compatibility strategy allows the new 30-screen product contract to be validated in isolation before physical binding.

## Native BLU2USB implementation

New host APIs:

- `include/blu2usb/ux_model/ux_v1.h`
- `src/ux_model/ux_v1.c`
- `include/blu2usb/renderer/renderer_v1.h`
- `src/renderer/renderer_v1.c`

These are native BLU2USB implementations. No source/header/runtime dependency is taken from:

- `mouse-ui`;
- `mouse-core`;
- UI-Core;
- SDL;
- mock-world/lab infrastructure.

The accepted Mouse UI Layout 1.0 golden is pinned as test evidence at:

- `tests/goldens/mux03/screens.txt`

## 30 canonical screens

The new enum contains exactly 30 screens:

1. `searching-first`
2. `first-mouse-connected`
3. `home-searching`
4. `home-searching-help`
5. `home-retry`
6. `home-retry-help`
7. `pair-new`
8. `help-pair-new`
9. `retry-pair-new`
10. `help-retry-pair-new`
11. `home-connected`
12. `help-home-connected`
13. `remapper-options`
14. `help-remapper-options`
15. `passthrough-active`
16. `passthrough-not-active`
17. `standard-not-active`
18. `standard-active`
19. `escape-not-active`
20. `escape-active`
21. `custom-edit`
22. `left`
23. `right`
24. `middle`
25. `forward`
26. `backward`
27. `saved-devices`
28. `remove-this`
29. `help-remove-this`
30. `learn-the-keys`

No Keyboard, Composite or Other Devices screen exists in the new UI v1 surface.

## Navigation and interaction

Actions continue to use the inherited BLU2USB release-triggered interaction model.

### Back exceptions

`KEY B` deliberately performs no navigation on:

- `searching-first`;
- `first-mouse-connected`;
- `learn-the-keys`.

No `JOY LEFT: GO TO HOME` behavior exists.

### Help

Contextual Help exists only for the frozen Help-owning screens.

Opening Help:

- records the owner screen/selection/page;
- cancels asynchronous work owned by that screen through the returned semantic intent;
- enters the matching Help screen.

Any complete HAT interaction exits Help and is consumed. Key Y therefore exits Help rather than locking.

A Help page opened from an active search returns to the coherent retry destination rather than resuming cancelled work:

- HOME searching Help -> HOME retry;
- Pair New Help -> Retry Pair New.

### Lock

When at least one Mouse is saved, Key Y locks ordinary screens.

Lock:

- marks screen-owned work for cancellation;
- clears UI-side ownership flags;
- uses the inherited interaction lock state.

The first complete interaction after lock is consumed by the interaction layer and resolves HOME from the supplied product snapshot.

`searching-first` cannot lock because there is no saved Mouse.

`first-mouse-connected` and `learn-the-keys` treat A/B/X/joystick as didactic. Key Y locks.

## HOME projection

HOME is resolved from product snapshot truth:

- zero saved -> `searching-first`;
- saved + authoritative -> `home-connected`;
- saved + no authoritative -> `home-searching`.

The UI returns semantic search intents; it does not call BLE or the connection coordinator directly.

Connected HOME options are:

1. confirmed remap summary;
2. Saved Devices;
3. Pair New Mouse;
4. Learn the Keys.

All HOME menu options use ordinary actionable color, with only the current selection emphasized. Cyan is not used for a HOME option status.

## Profiles

Visible terminology is now:

- PASSTHROUGH;
- STANDARD;
- ESCAPE;
- CUSTOM.

The internal inherited G06 profile enum value `BLU2USB_MOUSE_PROFILE_DEFAULT_REMAP` remains valid, but its v1 projection is `STANDARD`.

Selecting Passthrough/Standard/Escape projects active vs not-active from the current Mouse's confirmed profile.

An Apply request only sets a pending semantic intent. The UI moves to an ACTIVE screen only after `blu2usb_ui_v1_profile_result()` receives success and the product snapshot confirms the requested profile.

If the live Mouse disappears while an ACTIVE screen is visible:

- Passthrough Active -> Passthrough Not Active;
- Standard Active -> Standard Not Active;
- Escape Active -> Escape Not Active.

The Escape Active projection contains only the frozen Back and Lock hints. No GO TO HOME hint is present.

## Custom

Custom remains one global draft/template.

The five source rows are:

- Left;
- Right;
- Middle;
- Forward;
- Backward.

Editor target presentation order is:

1. Left;
2. Right;
3. Middle;
4. Escape;
5. Forward;
6. Backward.

`KEY A: APPLY AND BACK` updates the UI draft and emits a semantic Custom-set-target intent.

`KEY A: APPLY CUSTOM` is projected only when Custom is not already confirmed or the draft is dirty.

A successful Custom apply clears dirty state only after the supplied product snapshot confirms Custom as the active profile.

## Saved Devices

Saved Devices uses exactly one Mouse per page.

Projection uses the MUX-01 product snapshot, whose connected-first ordering is already identity-safe.

Each page contains:

- `N OF M`;
- formatted Mouse name;
- exact connected/disconnected status;
- confirmed profile;
- Remove Device action.

Only the connected Mouse name is cyan.

Remove This captures and retains `remove_target_id`. It resolves the target by identity from the current snapshot instead of by presentation index. Reordering caused by another Mouse becoming authoritative therefore cannot silently change the removal target.

Successful removal:

- returns to a valid Saved Devices page when records remain;
- resolves to FIRST and emits Search First when the final Mouse was removed.

## Mouse-name projection

The v1 projector implements the frozen display rule locally without mutating registry storage.

It:

- uses at most the first 15 renderer-supported characters;
- uppercases supported characters;
- trims trailing spaces;
- appends ` MOUSE` unless the original bounded name contains standalone word `MOUSE`, case-insensitive;
- uses `UNKNOWN MOUSE` for empty/unusable names;
- never exceeds 21 renderer columns.

The same rule is used by connected HOME, Saved Devices and Remove This.

Examples covered by test:

- `LIFT` -> `LIFT MOUSE`;
- `MOUSE GENERIC` -> unchanged;
- `XPTO ULTRA 2714` -> `XPTO ULTRA 2714 MOUSE`;
- `ABCDEFGHIJKLMNOP` -> `ABCDEFGHIJKLMNO MOUSE`.

## Frozen literal projection

The 30-screen executable golden pinned from the accepted Mouse UI Layout 1.0 baseline is checked row-by-row against BLU2USB's projected 9x21 frames.

This includes the MUX-00 discrepancy resolution for `help-home-connected`:

```text
REMOVE CONNECTED HELP
TO DISCONNECT THE
CURRENTLY CONNECTED
MOUSE NAVIGATE TO:
STEP 1. SAVED DEVICES
STEP 2. REMOVE DEVICE
STEP 3. KEY A: REMOVE

ANY KEY: BACK
```

## Visual semantics

The v1 frame projector reuses the accepted G03 renderer primitives/colors.

Validated behavior includes:

- magenta title;
- ordinary body yellow/off-white;
- ordinary actionable rows light gray;
- current/confirmed state cyan;
- selection/pressed feedback white;
- connected Saved Devices name cyan;
- HOME menu no cyan status;
- `searching-first` all-black background;
- `first-mouse-connected` black main body with dark-magenta hint row;
- standard dark-magenta hint region on ordinary screens.

The physical ST7789 renderer itself is not reimplemented.

## Architecture boundary

MUX-03 refines the existing host-pure UX dependency:

```text
ux_model
    -> domain
    -> interaction
    -> device_registry
```

The UI consumes product snapshots and emits semantic intents.

Its sources do not include:

- BTstack;
- CYW43;
- BLE HOGP;
- Pico SDK/hardware;
- TinyUSB;
- physical storage;
- mouse-ui/mouse-core/UI-Core.

## Automated verification

Implementation-head canonical CI run **#395** / run id **35708758061** compiled the new host surface and ran:

```text
19/19 tests passed
0 failed
```

Inherited tests passing:

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
15. `mux02_connection_coordinator`
16. `mux02_architecture`

New MUX-03 tests passing:

17. `mux03_ui_v1_navigation`
18. `mux03_ui_v1_projection`
19. `mux03_architecture`

The MUX-03 tests cover:

- all 30 exact screen identities and literal rows;
- 9x21 frame projection;
- FIRST didactic behavior;
- FIRST success instructional screen;
- Back exceptions;
- Lock/unlock HOME resolution;
- Help priority and cancellation ownership;
- SAVED search/retry;
- Pair New/retry/help;
- profile active/not-active;
- disconnect projection from active profile;
- Custom editor order/draft/confirmed behavior;
- one-record-per-page Saved Devices;
- connected-first display;
- stable Remove target by identity;
- final-record removal -> FIRST;
- name formatting;
- colors/background regions;
- native dependency boundary.

## Production compatibility

MUX-03 deliberately does not bind the new UI to the production app. The accepted legacy G06 UX API remains compiled so inherited production behavior can continue to build unchanged.

The final gate head must still pass Pico 2 W production configure/build/UF2 verification to prove that adding the new host surface caused no compilation regression.

## Physical acceptance

None required by MUX-03.

No new hardware-visible UX is claimed by this gate because the production app still uses the inherited G06 binding.

The first mandatory physical gate remains **MUX-05**.

## Gate close

Human acceptance is required after canonical CI passes on the final documentation/checklist head.

MUX-04 will expand durable product storage for the registry, per-Mouse confirmed profile and global Custom template. It still defers the first mandatory physical test to MUX-05.
