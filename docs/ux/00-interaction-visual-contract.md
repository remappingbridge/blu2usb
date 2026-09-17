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

`LEARN THE KEYS` / displayed title `PRESS TO LEAR A KEY` uses a full dark-magenta background. Every other screen uses black main content and dark-magenta hint content.

Semantic colors are frozen:

- title: magenta;
- static/example main-body text without indentation: light desaturated yellow / off-white yellow;
- resting actionable text and ordinary option text: light gray;
- selected option or pressed visible actionable text: white;
- current/applied/success/connected active state: cyan.

Option-list items have exactly one leading space. No `>` selector is used. Visual precedence is pressed, selected, current, actionable, static.

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

Hardware controls are `JOY UP`, `JOY DOWN`, `JOY LEFT`, `JOY RIGHT`, `JOY PRESS`, `KEY A`, `KEY B`, `KEY X`, and `KEY Y`. The Pair Mouse screen keeps the requested UI label `KEY C: HELP`; on the Waveshare HAT that action is driven by the physical help face control wired as `KEY X`. Pair Keyboard and Pair Composite continue to display `KEY X: HELP`.

## Learn The Keys

The screen identity and HOME option remain `LEARN THE KEYS`; displayed title is `PRESS TO LEAR A KEY`.

It is didactic. Other than Key Y lock, controls only demonstrate press/release feedback. `KEY A`, `KEY B`, and `KEY X` begin at character column 16 (1-based). Any control used to unlock while locked returns to HOME and is consumed.

## Custom Remap

`EDIT CUSTOM REMAP` edits the Pico-global CustomTemplate without requiring a connected or saved Mouse. Target order is LEFT, RIGHT, MIDDLE, BACKWARD, FORWARD, ESCAPE.

`KEY A: APPLY AND BACK` changes the draft mapping and returns to `EDIT CUSTOM REMAP`. `KEY A: APPLY CUSTOM` commits the complete draft. Hidden Key B performs one-screen Back without committing the current target page, and hidden Key Y locks.

## Dynamic/example body text

Dynamic/example body text remains unindented and off-white yellow unless explicitly representing current/success state. Fixed blocks in `01-screen-layouts.md` are literal.
