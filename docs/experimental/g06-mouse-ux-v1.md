# Experimental mouse UX 1.0 on G06

This branch starts at the physically accepted G06 commit
`7eee024ad4ee726c5a85ffa2f32b9f47187878af`.
Its UX reference is `remappingbridge/mouse-ui` at
`5f269e9625ae0d02a85b5d39eb87026edc448068`, UI Layout 1.0.

This is an experiment, not a new accepted gate. Physical acceptance of G06 does
not automatically extend to the changed session manager or firmware here.

## Implementation

The existing blu2usb interaction, UX model, renderer, HAT, ST7789, profiles,
remapping, canonical HID aggregator, TinyUSB, storage and Bluetooth runtime remain
the implementation. There is no mouse-ui or mouse-core build dependency and no
UI/Core adapter, snapshot protocol, or imported process.

The 30 screen literals are translated into the existing UX screen table.
Navigation still emits native commands on release. Help consumes interaction;
Back is inert in first search, first connected and Learn. Y locks anywhere with
a saved mouse except Help. Any complete HAT interaction unlocks to resolved HOME.

The G06 HID report parser and HID++ backend remain in use. The parser also marks
keyboard/keypad collections so admission can reject composite input without
changing canonical mouse report decoding. Bluetooth keyboard pairing is absent;
USB keyboard remains exclusively for synthetic Escape.

BLE adds a provisional candidate slot, controlled search windows (8 seconds for
first/saved, 15 seconds for new), cancellation and explicit acceptance after
product persistence. Only the live slot emits canonical input and runs HID++.
The old live slot remains usable during candidate qualification. On acceptance,
queued old reports are discarded and USB releases precede new-session reports.
Handle/CID ownership prevents old disconnect callbacks from clearing the new mouse.

The 16 record slots correspond to Bluetooth bond indexes. Product records store
name and confirmed profile per mouse. Custom configuration/draft remain global.
Connected mouse sorts first without moving identities. Removal captures its
index, cancels live input, commits the association removal and deletes the bond.
Boot removes orphan credentials left by interrupted removal or canceled pairing.

Storage retains the two alternating CRC-protected flash sectors. Records expand
to accommodate the mouse registry, and the application stack is enlarged.
Version-1 G06 profile records are recognized and imported with existing bonds;
name is recovered through the Device Name characteristic on reconnect. A G06
firmware downgrade does not understand the new product-record schema.

## Verification

Host CTest: eight suites (including the production radio lifecycle with mocked
HCI/SM/GATT completions against the pinned BTstack headers) covering USB descriptors, canonical HID ownership, G06
report decoding, profile/remap/HID++, storage CRC/recovery and the replacement UX. The radio test verifies candidate qualification before
promotion, preservation of the live mouse on cancellation, rejection of canceled
completions, immunity to the old session disconnect, timeout, and bond removal.
The old G02/G03/G06 UI golden tests describe the superseded G06 screens and remain
in source for historical reference, but are replaced in this branch's test run.
Old textual architecture/screen-contract tests are not treated as acceptance of
this experiment. Pico firmware must also build in CI at the branch commit.

Physical validation is pending. No physical pairing result is claimed from host
tests or firmware compilation.

## Manual acceptance scenarios

1. With no saved mouse: boot into SEARCHING FIRST MOUSE. Every control is didactic;
   B and Y neither navigate nor stop search. Pair a generic BLE mouse.
2. FIRST MOUSE CONNECTED: B stays put. Y extinguishes backlight. Each HAT control
   must independently unlock to connected HOME and consume its first interaction.
3. Repeat first pairing with Logitech Lift. Exercise motion, wheel, five buttons,
   held Forward, and release in Passthrough, Standard and Escape. There must be
   no double click, stuck button or duplicate Forward from HID and HID++.
4. Reboot with the mouse bonded. Reconnect, retain its name and profile, and verify
   Custom draft/application survives reboot. Also test migration from G06 records.
5. Power off the live mouse on each ACTIVE preset page: switch immediately to its
   not-active variant. Back to HOME starts an 8-second saved-only search.
6. Let saved search expire: retry appears. A restarts; B cancels; X cancels and opens
   Help. Y in Help closes Help rather than locking or resuming canceled search.
7. Pair New with mouse A live. Continue moving/clicking A while B is discovered and
   authenticated. B must become sole live mouse only at acceptance; held old output
   must be released. Cancel, Help, Lock and timeout must retain A.
8. Offer an already-saved mouse during Pair New. It must not replace the current
   mouse. Offer a keyboard and keyboard/mouse composite; neither is admitted.
9. Repeatedly cancel or lock at connection/security/discovery boundaries; no late
   callback may promote the canceled candidate. Unplug A while B is qualifying.
10. Verify Saved Devices pagination wraps with connected mouse first, correct
    status/profile/name, and a stable removal target even if connection changes.
11. Remove disconnected, connected and last mouse. Verify held USB releases, bond
    deletion, persistence after power cycle and first search after last removal.
12. Custom editor targets are Left, Right, Middle, Escape, Forward, Backward.
    A saves draft and returns; Apply Custom activates after persistence succeeds.
13. Check all 30 screens on ST7789: no truncation/overlap; didactic feedback on exact
    labels; yellow waiting instruction; black First Connected/Learn bodies;
    dark-magenta hint fields; only confirmed active/connected state is cyan.
14. Test full capacity (16 mice), interrupted flash writes, removal interrupted by
    power loss, USB disconnected during handoff, and sustained Lift input while
    saving settings. Confirm recovery never associates one bond with another name.
