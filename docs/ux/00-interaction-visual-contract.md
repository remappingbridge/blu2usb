# Interaction and visual contract

## Grid and retained pixel relocation

Every screen uses the fixed 9x21 semantic grid. The physically accepted pixel relocation from the earlier remapper gates is retained for every new screen, regardless of wording changes. The semantic row number does **not** imply a uniform physical 27-pixel Y advance on normal screens.

Base geometry remains:

- LCD: 240x240;
- glyph source: 5x7, scale 2;
- glyph box: 10x14 pixels;
- first title origin: x=7, y=8;
- horizontal character advance: 11 pixels;
- semantic line advance: 27 pixels, used only as the fallback/base grid.

Horizontal placement is always `x = 7 + column*11`.

Vertical placement preserves the accepted relocation used by the previous `picow-remapper` gates:

- title row: y=8;
- first standard body row: `8 + 14 + 17 = 39`;
- standard body advance: `14 + 12 = 26` pixels;
- standard hints are anchored from the bottom: final hint row y=`240 - 12 - 14 = 214`, with 26-pixel advance upward;
- the dark-magenta hint region starts 11 pixels above the first visible hint;
- `LEARN THE KEYS` keeps title y=8, first body row y=39, and uses `14 + 11 = 25` pixels between didactic rows.

The semantic separator row remains text-empty. Its physical Y is only a fallback location; the standard black/dark-magenta boundary is derived from the relocated first hint. Wording changes never change these relocation rules.

Every canonical screen has exactly 9 rows and at most 21 characters per row. Exact layouts are in `01-screen-layouts.md`.

## Visual regions and colors

`LEARN THE KEYS` / displayed title `PRESS TO LEARN A KEY` uses a full dark-magenta background. Every other screen uses black main content and dark-magenta hint content.

Semantic colors are frozen:

- title: magenta;
- static/example main-body text without indentation: light desaturated yellow / off-white yellow;
- resting actionable text and ordinary option text: light gray;
- selected option or pressed visible actionable text: white;
- current/applied/success/connected active state: cyan.

Cyan is the general positive-state signal. Whenever an operation has been successfully applied, the body text that describes the resulting configuration is cyan. The currently active profile in `MOUSE OPTIONS` is cyan while it is not selected.

**Selection has visual priority over current/applied state everywhere in the application.** If a row that is normally cyan becomes the currently navigated/selected option, that row is white for as long as it is selected. When selection moves away, the row returns to cyan if it is still the current/applied state. A pressed visible control hint also uses white while physically held.

Option-list items have exactly one leading space. No `>` selector is used.

For `DEVICE DETAILS`, active-device name and dynamic values after `TYPE:`, `STATUS:` and `PROFILE:` are cyan. Inactive saved-device values are off-white yellow. `REMOVE DEVICE` remains an indented selectable action.

## Actions fire on release

No navigation, apply, cancel, retry, remove, lock or access action executes on initial press. Press only changes visible feedback when that control has a visible label. Action executes on release.

Controls may have hidden behavior even when no hint is printed. Absence from the hint area does not disable the control.

## Global Back rule

`KEY B` means one-screen **Back** everywhere except HOME and `LEARN THE KEYS`.

The visible word may be `BACK`, `CANCEL`, or another context label; the runtime behavior is still one-screen back. `CANCEL` therefore means "leave this page without applying its pending action" and return to the previous logical page.

HOME is the first page, so hidden `KEY B: BACK` is a no-op there.

There is no `GO TO HOME` action. No screen may expose or implement `JOY LEFT: GO TO HOME`. If one-screen Back happens to arrive at HOME, that is only because HOME is the previous logical page.

Help is the exception in presentation only: `ANY KEY: BACK` consumes any HAT control and returns to its owning page; Key Y does not lock while Help owns interaction.

Profile success pages are not an extra navigation level. `KEY B: BACK` from `PASSTHROUGH APPLIED`, `DEFAULT REMAP APPLIED`, `ESCAPE APPLIED`, or `CUSTOM APPLIED` returns directly to `MOUSE OPTIONS` on the first complete press/release interaction. It must never return to the corresponding `APPLY ...` or edit page.

`MOUSE PAIRED` is also a connection feedback page rather than an extra navigation level. `KEY B: BACK` from it returns directly to `MOUSE OPTIONS`.

## Profile application and feedback contract

The profile engine is the source of truth. A profile is considered applied only after the profiles/remap runtime has accepted the complete configuration. The UI then reflects that confirmed active profile. Pressing an Apply control must not optimistically change the active profile or enter the success screen before runtime confirmation.

For every Mouse profile:

- before application, a non-current preset opens its `APPLY ...` page and displays the `KEY A: APPLY` hint;
- successful Apply opens the feedback/success state and the configuration description is cyan;
- returning to `MOUSE OPTIONS` shows exactly the active profile name in cyan when that row is not selected;
- selecting that active-profile row changes it to white, and moving selection away restores cyan;
- opening the already-active preset goes directly to its feedback/success state without displaying an Apply hint;
- switching profiles releases ownership from the old mapping before the new mapping becomes authoritative;
- the text on each `APPLY ...` screen is normative: the runtime mapping must exactly match the relationships printed on that screen.

The frozen preset mappings are therefore:

- `PASSTHROUGH`: Left→Left, Right→Right, Middle→Middle, Forward→Forward, Backward→Backward;
- `DEFAULT REMAP`: Forward→Left, Left→Forward, Backward→Right, Right→Backward, Middle→Middle;
- `ESCAPE REMAP`: Forward→Left, Backward→Right, Left→Escape, Right→Backward, Middle→Forward.

For `CUSTOM REMAP`, the complete draft becomes authoritative only after `KEY A: APPLY CUSTOM` and runtime confirmation. Successful application opens a dedicated `CUSTOM APPLIED` page. It shows the five resulting mappings in cyan and exactly the hints `KEY B: BACK` and `KEY Y: LOCK`. `KEY B` returns directly to `MOUSE OPTIONS`, where `CUSTOM REMAP` is cyan while unselected. Re-entering an already-active Custom profile opens `EDIT CUSTOM REMAP` with the current five mappings; `KEY A: APPLY CUSTOM` remains hidden until at least one target changes.

## Live Mouse connection and status contract

The BLE HOGP `CONNECTED`/`DISCONNECTED` runtime events are the source of truth for live Mouse connection UX in G06. The UI must update from those events; it must not infer connection from whether Mouse reports happened recently.

When a Mouse is connected:

- `MOUSE STATUS` row 1 is exactly `MOUSE CONNECTED` and is cyan;
- all other ordinary status body text remains the normal off-white yellow unless another explicit semantic rule applies;
- `PROFILE:` reflects the confirmed current profile and must never remain frozen on Passthrough after a profile change;
- the supported display values are `PROFILE: PASSTHROUGH`, `PROFILE: DEFAULT`, `PROFILE: ESCAPE`, and `PROFILE: CUSTOM`;
- `PAIR MOUSE` in `MOUSE OPTIONS` is cyan while unselected, and becomes white while selected;
- accessing `PAIR MOUSE` while already connected does not start or display a new search. It opens `MOUSE PAIRED`, whose positive body text is cyan;
- if the connection completes while `PAIR MOUSE` is currently searching, the LCD immediately transitions to `MOUSE PAIRED` without requiring another HAT input.

When the Mouse disconnects, `MOUSE STATUS` returns to `MOUSE NOT CONNECTED` in the normal yellow status color and `PAIR MOUSE` no longer carries the cyan connected-state marker.

The G06 live-connection contract does **not** require persistent saved-device names/models after power cycle. Device-name persistence, saved-device management and restoration remain later-gate concerns.

## Option lists and pagination

On entry, the first option is selected unless a screen restores a meaningful current value.

- Joy Up: previous option;
- Joy Down: next option;
- Joy Press: access selected option;
- option selection wraps.

Where listed as hidden controls, Joy Up/Down keep exactly the same behavior without a printed hint.

Paginated screens use Joy Left/Right with wrap. `STATUS` has exactly two pages. `SAVED DEVICES` has at most four devices per page.

## Lock/unlock

Key Y release locks on every normal non-Help screen where the screen map declares Lock, including Pair, Custom target and Remove screens. `LEARN THE KEYS` also locks on Key Y release.

While locked, the first complete physical HAT interaction from any control is consumed solely to unlock, turn the display back on and return to HOME. The same interaction must not navigate or activate another action. Key Y remains the principal advertised unlock control.

## Per-screen controls

The normative per-screen visible and hidden controls are defined beside each layout in `01-screen-layouts.md`. Those declarations are part of the product contract, not commentary.

Hardware controls are `JOY UP`, `JOY DOWN`, `JOY LEFT`, `JOY RIGHT`, `JOY PRESS`, `KEY A`, `KEY B`, `KEY X`, and `KEY Y`. Pair Mouse, Pair Keyboard and Pair Composite all display `KEY X: HELP` and are driven by the physical Key X control on the Waveshare HAT.

## Learn The Keys

The screen identity and HOME option remain `LEARN THE KEYS`; displayed title is `PRESS TO LEARN A KEY`.

It is didactic. Other than Key Y lock, controls only demonstrate press/release feedback. `KEY A`, `KEY B`, and `KEY X` begin at character column 16 (1-based). Any control used to unlock while locked returns to HOME and is consumed.

## Custom Remap

`EDIT CUSTOM REMAP` edits the Pico-global CustomTemplate without requiring a connected or saved Mouse. Target order is LEFT, RIGHT, MIDDLE, BACKWARD, FORWARD, ESCAPE.

`KEY A: APPLY AND BACK` changes the draft mapping and returns to `EDIT CUSTOM REMAP`. The returned `EDIT CUSTOM REMAP` row must immediately reflect the accepted draft value. Example: starting from `LEFT IS LEFT`, choosing `RIGHT` on `LEFT WILL BECOME` and releasing `KEY A: APPLY AND BACK` must return showing `LEFT IS RIGHT`.

`KEY A: APPLY CUSTOM` submits the complete draft to the profile engine. Only after the engine confirms it does the LCD enter `CUSTOM APPLIED`. Hidden Key B performs one-screen Back without committing the current target page, and hidden Key Y locks.

## Inherited accepted rules

The following previously accepted gate corrections remain normative and must be treated as regressions if broken by later gates:

- retained pixel relocation applies to all later screens even when wording changes;
- on `PRESS TO LEARN A KEY`, `KEY A`, `KEY B`, and `KEY X` start at 1-based column 16;
- `KEY B` always means one-screen Back outside HOME/LEARN, regardless of whether the visible label says Back, Cancel, or another context word;
- there is no `GO TO HOME` action;
- hidden controls remain functional where the screen contract declares them;
- lock occurs on Key Y release; the unlock interaction is consumed and returns to HOME;
- Pair help is `KEY X: HELP`; `KEY C: HELP` is invalid;
- the Learn title is exactly `PRESS TO LEARN A KEY`;
- fixed USB Mouse + Keyboard identity remains stable through Bluetooth connect/disconnect and profile changes;
- Mouse forwarding continues while the LCD is locked;
- BLE disconnect/overflow releases persistent HID ownership so no Mouse button or synthetic key can remain stuck.

## Dynamic/example body text

Dynamic/example body text remains unindented and off-white yellow unless explicitly representing current/success/connected state. Fixed blocks in `01-screen-layouts.md` are literal.