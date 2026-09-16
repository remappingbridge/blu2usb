# Canonical screen layouts

This file is the normative screen-layout contract. Unless a section explicitly says **example/dynamic body**, the text block is literal and its line breaks, indentation and wording are frozen.

All screens use the 9x21 grid and color/interaction rules from `00-interaction-visual-contract.md`.

## HOME

Literal:

```text
HOME
 STATUS
 MOUSE OPTIONS
 OTHER OPTIONS
 LEARN THE KEYS

JOY UP / DOWN: SELECT
JOY PRESS: ACCESS
KEY Y: LOCK / UNLOCK
```

## STATUS

`STATUS` always has exactly two pages and wraps with Joy Left/Right.

### MOUSE STATUS

Body is dynamic/example. When no mouse is connected, the first body line is `MOUSE NOT CONNECTED` in off-white yellow. When connected, it is `MOUSE CONNECTED` in cyan. Remaining body content is context-derived and unindented/off-white yellow unless it represents current/success state explicitly.

Approved example:

```text
MOUSE STATUS
MOUSE NOT CONNECTED
PROFILE: PASSTHROUGH
FWD: AUTO HIDPP
BACK: AUTO STD

JOY RIGHT\LEFT: PAGE
KEY B: BACK
KEY X: MOUSE HELP
```

### OTHER DEVICES STATUS

Body is dynamic/example. `KEYBOARD CONNECTED` and `COMPOSITE CONNECTED` are cyan when active; `... NOT CONNECTED` is off-white yellow.

Approved example:

```text
OTHER DEVICES STATUS
KEYBOARD
CONNECTED
COMPOSITE
NOT CONNECTED

JOY RIGHT\LEFT: PAGE
KEY B: BACK
KEY X: DEVICES HELP
```

### MOUSE HELP

Body is example/dynamic and may use up to six lines to explain abbreviations used by Mouse Status (`FWD`, `HIDPP`, `BACK`, `STD`, etc.). Body is unindented/off-white yellow.

```text
MOUSE HELP







ANY KEY: BACK
```

### DEVICES HELP

The approved wording may be retained. Body is unindented/off-white yellow.

```text
DEVICES HELP
KEYBOARD IS DIFFERENT
FROM COMPOSITE.
COMPOSITE IS TOUCHPAD
AND KEYBOARD EMBEDDED
TOGETHER AND IT PAIRS
ITS OWN BLUETOOTH.

ANY KEY: BACK
```

## MOUSE OPTIONS

Current active mouse profile is cyan. `PAIR MOUSE` is cyan when a Mouse is active; otherwise it follows the ordinary actionable/option color.

```text
MOUSE OPTIONS
 PAIR MOUSE
 PASSTHROUGH
 DEFAULT REMAP
 ESCAPE REMAP
 CUSTOM REMAP

JOY PRESS: ACCESS
JOY LEFT: BACK
```

## PAIR MOUSE

Entering this screen automatically starts BLE HID Mouse discovery for a bounded period. Body is example/dynamic, unindented/off-white yellow. A successful new Mouse replaces the active Mouse only after transactional pairing commit.

Approved example:

```text
PAIR MOUSE
SEARCHING BLE HID
TARGET MOUSE
AUTO SEARCH ACTIVE
FOUND 0 HID

KEY A: RETRY ON ERROR
KEY B: CANCEL
KEY X: HELP
```

### PAIR MOUSE HELP

Body is example/dynamic; explain BLE/HID succinctly in up to six lines.

```text
PAIR MOUSE HELP







ANY KEY: BACK
```

### MOUSE SAVED

Success body is example/dynamic, unindented and cyan; up to five body lines may be used.

Approved example:

```text
MOUSE SAVED
TYPE MOUSE
SAVED DEVICES UPDATED




KEY B: BACK
JOY LEFT: GO TO HOME
```

## PASSTHROUGH

Before apply, body is off-white yellow. If already active or after successful apply, body is cyan.

Literal pre-apply:

```text
APPLY PASSTHROUGH
ORIGINAL MOUSE
BUTTONS POSITION
ARE NOT ACTIVE


KEY A: APPLY
KEY B: BACK
JOY LEFT: GO TO HOME
```

Literal applied:

```text
PASSTHROUGH APPLIED
ORIGINAL MOUSE
BUTTONS POSITION
ARE ACTIVE NOW


KEY B: BACK
JOY LEFT: GO TO HOME
KEY Y: LOCK
```

## DEFAULT REMAP

This screen is also the normative Default mapping specification.

Literal pre-apply:

```text
APPLY DEFAULT REMAP
FORWARD IS LEFT
LEFT IS FORWARD
BACKWARD IS RIGHT
RIGHT IS BACKWARD

KEY A: APPLY
KEY B: CANCEL
JOY LEFT: GO TO HOME
```

Literal applied:

```text
DEFAULT REMAP APPLIED
FORWARD IS LEFT
LEFT IS FORWARD
BACKWARD IS RIGHT
RIGHT IS BACKWARD

KEY B: BACK
JOY LEFT: GO TO HOME
KEY Y: LOCK
```

Middle remains Middle even though it is not printed because the screen has only four mapping body lines.

## ESCAPE REMAP

This screen is also the normative Escape mapping specification.

Literal pre-apply:

```text
APPLY ESCAPE
FORWARD IS LEFT
BACKWARD IS RIGHT
LEFT IS ESCAPE
RIGHT IS BACKWARD
MIDDLE IS FORWARD

KEY A: APPLY
KEY B: CANCEL
```

Literal applied:

```text
ESCAPE APPLIED
FORWARD IS LEFT
BACKWARD IS RIGHT
LEFT IS ESCAPE
RIGHT IS BACKWARD
MIDDLE IS FORWARD

KEY B: BACK
JOY LEFT: GO TO HOME
```

## EDIT CUSTOM REMAP

This page edits the global CustomTemplate and is accessible with no Mouse connected or saved.

The text through `IS` is fixed. The target after `IS` is dynamic and reflects the current draft. When Custom is not the active profile for the active Mouse, mapping rows rest in light gray; when Custom is active, or immediately after a successful `APPLY CUSTOM`, the mapping rows are cyan according to current/applied-state rules.

```text
EDIT CUSTOM REMAP
 LEFT IS LEFT
 RIGHT IS RIGHT
 MIDDLE IS MIDDLE
 FORWARD IS FORWARD
 BACKWARD IS BACKWARD

JOY PRESS: ACCESS
KEY A: APPLY CUSTOM
```

### LEFT WILL BECOME

Current/draft target is cyan; selected is white.

```text
LEFT WILL BECOME
 LEFT
 RIGHT
 MIDDLE
 BACKWARD
 FORWARD
 ESCAPE

KEY A: APPLY AND BACK
```

### RIGHT WILL BECOME

```text
RIGHT WILL BECOME
 LEFT
 RIGHT
 MIDDLE
 BACKWARD
 FORWARD
 ESCAPE

KEY A: APPLY AND BACK
```

### MIDDLE WILL BECOME

```text
MIDDLE WILL BECOME
 LEFT
 RIGHT
 MIDDLE
 BACKWARD
 FORWARD
 ESCAPE

KEY A: APPLY AND BACK
```

### FORWARD WILL BECOME

```text
FORWARD WILL BECOME
 LEFT
 RIGHT
 MIDDLE
 BACKWARD
 FORWARD
 ESCAPE

KEY A: APPLY AND BACK
```

### BACKWARD WILL BECOME

```text
BACKWARD WILL BECOME
 LEFT
 RIGHT
 MIDDLE
 BACKWARD
 FORWARD
 ESCAPE

KEY A: APPLY AND BACK
```

## OTHER OPTIONS

`PAIR KEYBOARD` is cyan when a Keyboard is active; `PAIR COMPOSITE` is cyan when a Composite is active. Otherwise they use ordinary option colors.

```text
OTHER OPTIONS
 PAIR KEYBOARD
 PAIR COMPOSITE
 SAVED DEVICES


JOY PRESS: ACCESS
JOY LEFT: BACK
KEY Y: LOCK / UNLOCK
```

## OTHER OPTIONS HELP

Approved body may be retained:

```text
OTHER OPTIONS HELP
KEYBOARD IS DIFFERENT
FROM COMPOSITE.
COMPOSITE IS TOUCHPAD
AND KEYBOARD EMBEDDED
TOGETHER AND IT PAIRS
ITS OWN BLUETOOTH.

ANY KEY: BACK
```

## PAIR KEYBOARD

This screen is intentionally **transport-neutral**. Entering it automatically searches the keyboard transports enabled by the firmware (for example Classic HID and/or BLE HOGP Keyboard). The user does not choose the transport.

Body is example/dynamic. The old transport-specific line `SEARCHING BLE HID` is replaced by the approved transport-neutral wording `SEARCHING KEYBOARD`.

```text
PAIR KEYBOARD
SEARCHING KEYBOARD
TARGET KEYBOARD
AUTO SEARCH ACTIVE
FOUND 0 HID

KEY A: RETRY ON ERROR
KEY B: CANCEL
KEY X: HELP
```

### PAIR KEYBOARD HELP

Body is example/dynamic. Explain HID and that keyboard transport is selected automatically; do not promise BLE-only behavior.

```text
PAIR KEYBOARD HELP







ANY KEY: BACK
```

### KEYBOARD SAVED

Success body is example/dynamic, unindented and cyan.

```text
KEYBOARD SAVED
TYPE KEYBOARD
SAVED DEVICES UPDATED




KEY B: BACK
JOY LEFT: GO TO HOME
```

## PAIR COMPOSITE

Entering automatically starts supported Composite discovery; v1 target is BLE HID composite. Body is example/dynamic.

```text
PAIR COMPOSITE
SEARCHING BLE HID
TARGET COMPOSITE
AUTO SEARCH ACTIVE
FOUND 0 HID

KEY A: RETRY ON ERROR
KEY B: CANCEL
KEY X: HELP
```

### PAIR COMPOSITE HELP

Body is example/dynamic.

```text
PAIR COMPOSITE HELP







ANY KEY: BACK
```

### COMPOSITE SAVED

Success body is example/dynamic, unindented and cyan.

```text
COMPOSITE SAVED
TYPE COMPOSITE
SAVED DEVICES UPDATED




KEY B: BACK
JOY LEFT: GO TO HOME
```

## SAVED DEVICES

At most four saved devices per page. The title is dynamic pagination text. Active device option rows are cyan. Inactive saved rows use ordinary option color. Joy Up/Down scrolls within the current page; Joy Left/Right pages with wraparound.

Approved first-page example:

```text
1-4 OF 6 SAVED
 BKB-3G
 OFFICE MOUSE
 TRAVEL KEYBOARD
 GENERIC MOUSE

JOY UP\DOWN: SCROLL
JOY PRESS: ACCESS
JOY RIGHT\LEFT: PAGE
```

Approved second-page example:

```text
5-6 OF 6 SAVED
 MX MASTER 3
 DESK KEYBOARD



JOY UP\DOWN: SCROLL
JOY PRESS: ACCESS
JOY RIGHT\LEFT: PAGE
```

## DEVICE DETAILS — Mouse

The device name and dynamic values after `TYPE:`, `STATUS:` and `PROFILE:` are cyan when this saved device is the active Mouse. For an inactive saved Mouse those values are off-white yellow. `REMOVE DEVICE` is the only selectable option and stays immediately below the informational lines.

```text
DEVICE DETAILS
LOGITECH LIFT
TYPE: MOUSE
STATUS: CONNECTED
PROFILE: DEFAULT
 REMOVE DEVICE

JOY PRESS: ACCESS
KEY B: BACK
```

## DEVICE DETAILS — Keyboard

Active dynamic values are cyan; inactive saved values are off-white yellow.

```text
DEVICE DETAILS
BKB-3G
TYPE: KEYBOARD
STATUS: SAVED
 REMOVE DEVICE

JOY PRESS: ACCESS
KEY B: BACK
JOY LEFT: GO TO HOME
```

## DEVICE DETAILS — Composite

Active dynamic values are cyan; inactive saved values are off-white yellow.

```text
DEVICE DETAILS
DESK COMPOSITE
TYPE: COMPOSITE
STATUS: SAVED
 REMOVE DEVICE

JOY PRESS: ACCESS
KEY B: BACK
JOY LEFT: GO TO HOME
```

## REMOVE DEVICE

After successful remove, return to `SAVED DEVICES`. Body is unindented/off-white yellow.

```text
REMOVE DEVICE
BKB-3G
PAIRING AND MAPPINGS
WILL BE DELETED



KEY A: REMOVE
KEY B: CANCEL
```

## LEARN THE KEYS

This page is literal down to character placement. It uses the full dark-magenta background. Resting control labels are light gray and only the relevant words become white while their control is held.

```text
LEARN THE KEYS
      JOY UP
JOY    JOY    JOY
LEFT  PRESS  RIGHT
     JOY DOWN
              KEY A
LOCK SCREEN   KEY B
 AND UNLOCK   KEY X
  OPEN HOME -> KEY Y
```

Character-position rules, counted from column 1:

- `JOY UP`: `J` at column 7;
- row with three `JOY`: starts at columns 1, 8 and 15;
- `LEFT`, `PRESS`, `RIGHT`: starts at columns 1, 7 and 14;
- `JOY DOWN`: `J` at column 6;
- `KEY A`, `KEY B`, `KEY X`: `K` at column 15;
- `LOCK SCREEN`: `L` at column 1;
- `AND UNLOCK`: `A` at column 2;
- `OPEN HOME -> KEY Y`: row starts at column 3.

Press feedback:

- Joy Up -> `JOY UP` white;
- Joy Left -> first `JOY` and `LEFT` white;
- Joy Press -> second `JOY` and `PRESS` white;
- Joy Right -> third `JOY` and `RIGHT` white;
- Joy Down -> `JOY DOWN` white;
- Key A/B/X -> its own label white;
- Key Y -> `LOCK SCREEN`, `AND UNLOCK` and `OPEN HOME -> KEY Y` white.

On release all demonstration text returns to light gray. Key Y release additionally locks. Other controls have no normal navigation action while Learn The Keys owns the screen.
