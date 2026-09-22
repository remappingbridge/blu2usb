# MUX-04 validation — durable Mouse registry/product storage

Status: **IMPLEMENTED — HUMAN ACCEPTANCE PENDING**

Branch: `gate/mux-04-durable-registry-storage`

Accepted dependency:

- MUX-03 accepted head: `a9433748288257be9da7ca8883714447cd679ccf`

## Scope implemented

MUX-04 expands the accepted G06 dual-slot product persistence so it can hold the Mouse UI Layout 1.0 durable product state.

This gate changes persistence format and storage ownership only. It does not bind the new registry to BLE discovery, BTstack bonds, the production UX, or physical reconnect policy. Those integrations remain deferred to later MUX gates.

## What is durable

The native MUX product payload persists:

- up to 16 saved Mouse identities;
- saved-record order;
- normalized stored Mouse name within the existing 64-byte product bound;
- confirmed profile kind for each saved Mouse;
- one global Custom template;
- one global Custom draft;
- Custom draft-valid/dirty state;
- optional G06 legacy-profile migration hint.

## What is deliberately not durable

The live/authoritative Mouse is **not** restored from flash.

`has_authoritative` and `authoritative_id` remain runtime connection truth. On restore they are always cleared.

This prevents reboot from fabricating a connection merely because a Mouse was authoritative before power loss.

The MUX-02 reconnect policy does not require a persistent preferred/current Mouse: when saved identities exist and no Mouse is live, SAVED search accepts the first eligible saved Mouse that becomes ready.

## Product payload schema

Native payload:

- schema version: 1;
- serialized size: **1184 bytes**;
- maximum saved records: 16.

The payload is deterministic:

```text
16-byte product header
+ 16 x 73-byte Mouse entries
= 1184 bytes
```

Each Mouse entry contains:

- 64-bit logical Mouse ID;
- confirmed profile kind;
- fixed 64-byte NUL-terminated product-name field.

The decoder rejects:

- invalid/zero identity;
- duplicate identity;
- invalid profile kind;
- name without NUL terminator inside the fixed field;
- invalid Custom targets;
- count above 16;
- malformed schema/flags.

Registry validation is reused rather than duplicated.

## Custom persistence

The durable product state contains:

- confirmed global Custom template;
- current Custom draft;
- `draft_valid`.

If `draft_valid == false`, restore normalizes the draft to the confirmed Custom template.

The storage helper can copy this global Custom state into the inherited G06 `blu2usb_profiles_t` without changing that profile engine's current active-kind field.

This keeps Custom template ownership separate from the per-Mouse confirmed profile kind.

## Expanded flash envelope

The accepted G06 product envelope was:

- schema version 1;
- 64-byte maximum payload;
- 80-byte record;
- one programmed flash page.

MUX-04 current envelope is:

- envelope schema version 2;
- 1280-byte maximum payload;
- **1536-byte record**;
- **6 x 256-byte flash pages**;
- still entirely inside one 4096-byte flash sector.

CRC32 still covers the complete record prefix through the CRC field.

The low-level decoder accepts both:

- current envelope version 2;
- accepted G06 envelope version 1.

New writes always use version 2.

## Dual-slot/torn-write behavior

Two whole flash sectors remain alternated.

Write sequence remains:

1. inspect both slots;
2. select newest valid generation;
3. choose the opposite sector;
4. encode generation + 1;
5. erase target sector;
6. program the complete 1536-byte record;
7. read back;
8. decode and verify generation, size and payload bytes.

If the newest record is corrupt/torn, slot selection falls back to the older valid record.

Generation comparison remains wrap-aware through signed subtraction, matching inherited G06 behavior.

## Pico stack/RAM safety

The larger storage record is no longer placed repeatedly on the Pico stack.

`storage_pico.c` owns static synchronous workspaces for:

- the two records;
- the encoded record;
- the verification/decode payload.

Slot selection validates envelope CRC/generation without allocating a full payload scratch buffer.

Compile-time assertions enforce:

- record <= flash sector;
- record size is a whole number of flash pages;
- product-storage base is sector aligned.

## Bluetooth credential separation

The existing flash layout is preserved.

Product state still owns the two sectors immediately before the Pico SDK/BTstack reserved tail.

The record grew **inside each existing product sector**; it did not consume a Bluetooth credential sector.

No product storage source contains a BTstack dependency.

Bluetooth bonds/credentials remain BTstack-owned.

## G06 migration policy

The accepted G06 product payload stores one global profile engine state, but it contains no stable Mouse identity. Therefore MUX-04 cannot honestly manufacture a saved Mouse record during migration.

The migration policy is deterministic and non-destructive:

1. the legacy envelope version 1 is recognized;
2. the legacy 14-byte `blu2usb_profiles` payload is restored;
3. the new registry starts empty because no Mouse identity exists in that payload;
4. the global Custom template is preserved;
5. an unapplied Custom draft is preserved;
6. the old global active profile becomes:
   - `legacy_profile_pending = true`;
   - `legacy_profile_kind = <G06 active kind>`;
7. no authoritative/live Mouse is fabricated.

MUX-06 may consume that hint when it maps the existing persistent BT bond to a real stable Mouse identity.

This allows the old profile to follow the recovered G06 Mouse without guessing an identity in MUX-04.

After native product state is later committed, ordinary MUX records normally have `legacy_profile_pending = false`.

## Automated verification

Implementation-head canonical CI run **#415** / run id **35709994187** passed completely.

Host/architecture result:

```text
21/21 tests passed
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
17. `mux03_ui_v1_navigation`
18. `mux03_ui_v1_projection`
19. `mux03_architecture`

New MUX-04 tests passing:

20. `mux04_durable_product_storage`
21. `mux04_architecture`

The MUX-04 behavioral test covers:

- full 16-record native round-trip;
- stable order and IDs;
- all four per-Mouse profile kinds;
- full stored Mouse names;
- authoritative/live truth cleared on restore;
- global Custom template;
- global unapplied Custom draft;
- native corruption detection;
- record-17 capacity rejection;
- malformed identity rejection;
- malformed profile rejection;
- G06 envelope version-1 decode;
- G06 14-byte profile payload migration;
- preservation of G06 active profile as legacy hint;
- preservation of G06 Custom/draft;
- current-vs-legacy generation selection;
- corrupt newest current record falling back to valid G06 slot.

## Pico 2 W production verification

The same implementation-head run passed:

- pinned ARM toolchain;
- Pico SDK fetch;
- Pico 2 W production configure;
- Pico 2 W firmware build;
- production UF2 verification;
- production UF2 artifact upload.

This proves the larger multi-page storage backend compiles into the current production firmware without disturbing the inherited G06 build path.

It is **not** a physical persistence claim: the production app still uses the inherited G06 payload until later MUX integration.

## Files intentionally changed

Storage/product scope:

- `include/blu2usb/storage/storage.h`
- `include/blu2usb/storage/product_state.h`
- `src/storage/storage.c`
- `src/storage/product_state.c`
- `src/storage/storage_pico.c`

Build/architecture/test wiring:

- `CMakeLists.txt`
- `cmake/Blu2UsbModules.cmake`
- `tests/test_architecture.py`
- `tests/CMakeLists.txt`
- `tests/test_mux04_storage.c`
- `tests/test_mux04_architecture.py`

No BLE HOGP, BT runtime, app pairing, USB HID, renderer, HAT or Logitech HID++ implementation file is changed by MUX-04.

## Physical acceptance

None required by MUX-04.

The new schema is host-validated and production-compiled, but the new product model is not yet the runtime owner of physical pairing/persistence.

The first mandatory physical gate remains **MUX-05**.

## Gate close

MUX-04 requires human acceptance before MUX-05 branches from its exact accepted SHA.

MUX-05 is the isolated physical Pair New transport-feasibility gate. It is the first gate where the user must flash a MUX artifact and perform mandatory hardware scenarios.
