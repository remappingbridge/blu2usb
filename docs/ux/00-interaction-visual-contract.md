# Interaction and visual contract

## Grid

Every screen uses a fixed monospaced semantic grid:

- 9 rows;
- 21 characters maximum per row;
- title on row 1;
- main body below the title;
- hint region at the bottom;
- standard screens preserve an empty semantic separator between body and hints where shown by the canonical layout.

Exact layouts are defined in `01-screen-layouts.md`.

## Visual regions

`LEARN THE KEYS` is a special full-screen dark-magenta page.

Every other screen uses:

- main region: black;
- hint region: dark magenta.

## Colors

Semantic colors are frozen:

- title: magenta;
- static/example main-body text without indentation: **light desaturated yellow / off-white yellow**;
- resting actionable text and ordinary option text: light gray;
- selected option or pressed actionable text: white;
- current/applied/success/connected active state: cyan.

Cyan is a main-body state color, not a hint color.

Text rendered in the off-white yellow has no leading indentation.

Option-list items always use exactly one leading space. No `>` selection marker is used.

Visual precedence:

1. pressed -> white;
2. selected -> white;
3. current/applied/connected -> cyan;
4. actionable -> light gray;
5. static/example -> off-white yellow.

For `DEVICE DETAILS`, an active device keeps the device name and dynamic values after `TYPE:`, `STATUS:` and `PROFILE:` in cyan. Inactive saved-device values use the off-white yellow. The `REMOVE DEVICE` option remains an indented selectable action.

## Actions fire on release

No navigation, apply, cancel, retry, remove, lock or access action executes on the initial press. Press only changes visual state. The action executes on release.

## Option lists

On entry, the first option is selected unless a screen-specific rule restores a meaningful current value.

- `JOY UP`: previous item;
- `JOY DOWN`: next item;
- `JOY PRESS`: access selected item;
- selection wraps: Up from first -> last; Down from last -> first.

Where a `WILL BECOME` screen represents an already configured source, its current/draft target is the initial selected/current cyan option.

## Pagination

Paginated screens use `JOY LEFT` and `JOY RIGHT` for page navigation.

Pagination wraps:

- Left from first page -> last page;
- Right from last page -> first page.

`STATUS` always has exactly two pages: `MOUSE STATUS` and `OTHER DEVICES STATUS`.

`SAVED DEVICES` has as many pages as required, with at most four devices per page. Removing the last item on a trailing page removes that page from pagination.

## Global HAT policy

Terminology is fixed:

- directional stick and center: `JOY UP`, `JOY DOWN`, `JOY LEFT`, `JOY RIGHT`, `JOY PRESS`;
- face buttons: `KEY A`, `KEY B`, `KEY X`, `KEY Y`.

Usual functions:

- Key A: primary mutation (`APPLY`, `RETRY`, `REMOVE`, `APPLY AND BACK`, etc.);
- Key B: one-screen Back/Cancel where applicable;
- Key X: contextual Help where defined;
- Key Y: Lock and principal advertised Unlock control;
- Joy Left/Right: pagination when the current screen is paginated;
- Joy Up/Down: selection in option lists;
- Joy Press: access selected option.

Screen-specific canonical hints override the generic presentation but not the underlying frozen behavior unless explicitly declared as an exception.

## Lock/unlock

Key Y release locks from normal screens where lock is allowed and turns off the backlight.

While locked, the first physical HAT control press/release interaction from **any control** is consumed solely to unlock, turn the display back on and return to HOME. Its normal action does not also execute. Key Y is shown as the primary discoverable unlock mechanism in the UI.

## Help

A Help screen owns the interaction context. `ANY KEY: BACK` means any HAT control, including Key Y; Key Y does not lock while a Help screen is active.

## Learn The Keys

`LEARN THE KEYS` is didactic and follows its exact character alignment from the screen-layout contract.

On this screen controls primarily provide press/release visual demonstration. Key Y is the only control with a product action: its release locks the display. Other controls do not navigate or apply actions while this page owns the interaction context.

## Custom Remap

`EDIT CUSTOM REMAP` is an option-list editor for the Pico's global CustomTemplate and is available without a mouse.

Each `... WILL BECOME` page contains exactly these targets in this order:

1. LEFT
2. RIGHT
3. MIDDLE
4. BACKWARD
5. FORWARD
6. ESCAPE

`KEY A: APPLY AND BACK` changes the draft mapping for that source and returns to `EDIT CUSTOM REMAP`.

`KEY A: APPLY CUSTOM` on the editor commits the entire draft as defined by the product contract.

## Dynamic/example body text

When a page says its black-area body is an example, implementation may define that body text from runtime context. Such text remains unindented and off-white yellow unless the page defines a cyan success/current state.

Fixed blocks in `01-screen-layouts.md` are literal and must not be paraphrased by implementation.
