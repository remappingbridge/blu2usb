# MUX-01 validation — host-pure Mouse registry and product snapshot

Status: **IMPLEMENTED — HUMAN ACCEPTANCE PENDING**

Branch: `gate/mux-01-device-registry`

Accepted dependency:

- MUX-00 accepted head: `b3cdc0d9575bf0d615e6643ea5e0b4c38dbeb6a8`

## Scope implemented

MUX-01 materializes the `device_registry` module that was already present in the G01 architecture graph. No Bluetooth transport, storage persistence, renderer, HAT or application pairing flow is integrated in this gate.

### Stable Mouse identity

`include/blu2usb/domain/device.h` defines:

- `blu2usb_mouse_id_t` as a 64-bit logical identity;
- invalid ID zero;
- exact saved capacity **16**;
- bounded product-name capacity 64 bytes.

The logical ID is independent from array/page position. Future transport gates are responsible for deriving/mapping this logical identity from persistent peer identity.

### Device registry

`blu2usb_device_registry_t` owns only host-pure product facts:

- up to 16 saved Mouse records;
- stable ID;
- bounded source/product name;
- confirmed per-Mouse profile kind;
- zero or one authoritative Mouse ID.

Supported operations:

- add by identity;
- update name by identity;
- update confirmed profile by identity;
- find by identity;
- remove by identity;
- set/clear authoritative Mouse;
- validate registry invariants.

A duplicate ID is rejected rather than creating a second logical record.

Removing the authoritative Mouse clears authoritative state. Removing an inactive Mouse leaves the authoritative Mouse untouched.

### Product snapshot

`blu2usb_product_snapshot_build()` creates an immutable value snapshot for future UX/coordinator consumers.

Presentation order is deliberately different from storage order:

1. authoritative/connected Mouse first, when one exists;
2. remaining saved records in registry order.

Building a snapshot **does not mutate or reorder the registry**. Therefore a target captured by Mouse ID remains the same logical Mouse even when the connected Mouse changes and presentation order changes.

A snapshot copies the confirmed per-Mouse profile at build time. Later registry mutation does not silently mutate an already-published snapshot; rebuilding publishes the new state.

### Name scope

MUX-01 stores a bounded NUL-terminated product name and preserves its bytes/case when it fits. The UI Layout 1.0 uppercase/15-character/` MOUSE` presentation rule remains a MUX-03 projection concern.

This separation prevents registry identity/state from becoming dependent on renderer formatting.

## Architecture boundary

The real library is:

```text
blu2usb_device_registry
  -> domain headers only
```

Its sources contain no dependency on:

- Pico SDK;
- GPIO/SPI;
- BTstack;
- CYW43;
- BLE HOGP;
- TinyUSB;
- physical storage.

The pre-existing architecture test already classifies `device_registry` as host-pure. MUX-01 adds a dedicated architecture test to make the boundary explicit.

No `src/ble_hogp/*`, `src/bt_runtime/*`, `src/storage/*`, `src/app/*`, USB, renderer or HAT implementation file was changed by this gate.

## Automated acceptance

GitHub Actions canonical CI run **35705219212** validated implementation head
`9fc5f95d35b259c05547463a91a9858c04a4855d`.

Host/architecture result:

```text
14/14 tests passed
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

New MUX-01 tests passing:

13. `mux01_device_registry`
14. `mux01_architecture`

The registry test covers:

- empty/valid initialization;
- add/update/remove by stable identity;
- duplicate rejection;
- invalid identity/profile rejection;
- per-Mouse profile association;
- authoritative Mouse selection;
- connected-first snapshot projection;
- preservation of internal registry order;
- identity-stable removal target across presentation reorder;
- removal of inactive vs authoritative Mouse;
- exact capacity 16;
- rejection of record 17;
- transactional rejection of an overlong name update;
- invalid-registry rejection by snapshot builder.

Production result from the same run:

- pinned ARM toolchain: PASS;
- Pico SDK fetch/configure: PASS;
- Pico 2 W production configure: PASS;
- Pico 2 W firmware build: PASS;
- production UF2 verification: PASS;
- production UF2 artifact upload: PASS.

## Physical acceptance

None required by MUX-01.

This gate does not bind the registry to Bluetooth or persistent flash and therefore does not claim any new hardware-visible behavior.

## Gate close

MUX-01 is implementation-complete when its final-head CI is green.

Human acceptance is required before MUX-02 branches from the exact accepted MUX-01 SHA.

MUX-02 will implement the host-pure coordinator, FIRST/SAVED/NEW operation semantics, tokens, cancellation, HOME resolver and handoff/remove state machines. It must still make no BLE transport changes.
