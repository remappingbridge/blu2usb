# Canonical screen layouts

This file is the normative 9x21 screen and control-map contract. Unless a section says **example/dynamic body**, its text block is literal. Visible hints and hidden controls are both functional requirements.

All layouts use the retained pixel relocation in `00-interaction-visual-contract.md`.

## v0.6.1 — SEARCHING FIRST MOUSE

This is the physical firmware startup screen. It does not change the accepted G06 BLE search/reconnect implementation; it only replaces the pre-HOME presentation while that existing transport is looking for the first Mouse connection of the boot flow.

All HAT controls are didactic only. In particular, Key B does not navigate and Key Y does not lock while no Mouse has reached the first-connected feedback screen.

The entire screen background is black. `PRESS TO LEARN KEYS` and `WHILE WAIT CONNECTION` use the ordinary static yellow/off-white tone. A held HAT control turns only its own didactic label white.

```text
SEARCHING FIRST MOUSE
PRESS TO LEARN KEYS
WHILE WAIT CONNECTION
       JOY UP
  JOY    JOY    JOY
  LEFT  PRESS  RIGHT
      JOY DOWN
 KEY A         KEY X
 KEY B         KEY Y
```

If the Mouse connection disappears before first-start is completed, this screen is restored.

## v0.6.1 — FIRST MOUSE CONNECTED

The first accepted BLE Mouse connection of the startup flow opens this feedback screen.

Joystick, Key A, Key B and Key X remain didactic; Key B is explicitly inert. Key Y is the only functional control and locks the display.

The main body background is black. Only the final `KEY Y: LOCK` hint row uses the standard dark-magenta hint background.

```text
FIRST MOUSE CONNECTED
       JOY UP
  JOY    JOY    JOY
  LEFT  PRESS  RIGHT
      JOY DOWN
 KEY A         KEY X
 KEY B         KEY Y

 KEY Y: LOCK
```

After Key Y locks, the first complete HAT interaction unlocks the display, is consumed, and opens the existing v0.6 HOME with selection zero. From that point onward the original v0.6 HOME/navigation behavior owns the session.


## HOME

Hidden: Joy Up/Down Select; Key B Back (no-op because HOME is first); Key Y Lock.

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

## MOUSE STATUS

Body is dynamic. Hidden: Key Y Lock.

Disconnected example:

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

Connected example:

```text
MOUSE STATUS
MOUSE CONNECTED
PROFILE: DEFAULT
FWD: AUTO HIDPP
BACK: AUTO STD

JOY RIGHT\LEFT: PAGE
KEY B: BACK
KEY X: MOUSE HELP
```

`MOUSE CONNECTED` is cyan. The disconnected text and the remaining ordinary status body text are off-white yellow. `PROFILE:` always reflects the confirmed active profile and may display `PASSTHROUGH`, `DEFAULT`, `ESCAPE`, or `CUSTOM`.

## OTHER DEVICES STATUS

Body is example/dynamic. Hidden: Key Y Lock.

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

## MOUSE HELP

Body is example/dynamic.

```text
MOUSE HELP







ANY KEY: BACK
```

## DEVICES HELP

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

Hidden: Joy Up/Down Select; Key Y Lock.

```text
MOUSE OPTIONS
 PAIR MOUSE
 PASSTHROUGH
 DEFAULT REMAP
 ESCAPE REMAP
 CUSTOM REMAP

JOY PRESS: ACCESS
KEY B: BACK
```

When a Mouse is connected, `PAIR MOUSE` is cyan while unselected. If the selection cursor rests on it, it is white. The active profile follows the same cyan-unselected/white-selected rule.

## PAIR MOUSE

Body is example/dynamic. Hidden: Key B Back; Key Y Lock. Help is driven by the physical Key X control on the Waveshare HAT.

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

`KEY B: CANCEL` has the same navigation rule as Back: it returns exactly one logical page, to `MOUSE OPTIONS`, without applying a pending action.

If a Mouse is already connected, accessing `PAIR MOUSE` must not show this searching screen. If the Mouse becomes connected while this searching screen is visible, the LCD must immediately leave it. Both cases open the `MOUSE PAIRED` feedback below.

## PAIR MOUSE HELP

```text
PAIR MOUSE HELP







ANY KEY: BACK
```

## MOUSE PAIRED

Positive body text is cyan. This is live connection feedback, not the later saved-device persistence feature.

```text
MOUSE PAIRED
MOUSE CONNECTED
READY TO USE




KEY B: BACK
KEY Y: LOCK
```

`KEY B: BACK` returns directly to `MOUSE OPTIONS`.

## PASSTHROUGH — before apply

```text
APPLY PASSTHROUGH
ORIGINAL MOUSE
BUTTONS POSITION
ARE NOT ACTIVE


KEY A: APPLY
KEY B: CANCEL
KEY Y: LOCK
```

## PASSTHROUGH — applied

```text
PASSTHROUGH APPLIED
ORIGINAL MOUSE
BUTTONS POSITION
ARE ACTIVE NOW



KEY B: BACK
KEY Y: LOCK
```

## DEFAULT REMAP — before apply

```text
APPLY DEFAULT REMAP
FORWARD IS LEFT
LEFT IS FORWARD
BACKWARD IS RIGHT
RIGHT IS BACKWARD

KEY A: APPLY
KEY B: CANCEL
KEY Y: LOCK
```

## DEFAULT REMAP — applied

```text
DEFAULT REMAP APPLIED
FORWARD IS LEFT
LEFT IS FORWARD
BACKWARD IS RIGHT
RIGHT IS BACKWARD


KEY B: BACK
KEY Y: LOCK
```

Middle remains Middle.

## ESCAPE REMAP — before apply

Hidden: Key Y Lock.

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

## ESCAPE REMAP — applied

```text
ESCAPE APPLIED
FORWARD IS LEFT
BACKWARD IS RIGHT
LEFT IS ESCAPE
RIGHT IS BACKWARD
MIDDLE IS FORWARD

KEY B: BACK
KEY Y: LOCK
```

## EDIT CUSTOM REMAP

Hidden: Joy Up/Down Select; Key B Back; Key Y Lock. The five mapping rows are dynamic and must reflect the current Custom draft.

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

When the already-active Custom profile is opened without pending changes, `KEY A: APPLY CUSTOM` is hidden until a mapping is changed.

## CUSTOM APPLIED

The five mapping rows are dynamic and show the exact confirmed Custom configuration. All five are cyan.

```text
CUSTOM APPLIED
LEFT IS RIGHT
RIGHT IS RIGHT
MIDDLE IS MIDDLE
FORWARD IS FORWARD
BACKWARD IS BACKWARD

KEY B: BACK
KEY Y: LOCK
```

The shown `LEFT IS RIGHT` is only an example of a confirmed custom mapping. `KEY B: BACK` returns directly to `MOUSE OPTIONS`.

## LEFT WILL BECOME

Hidden: Joy Up/Down Select; Key B Back; Key Y Lock.

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

## RIGHT WILL BECOME

Hidden: Joy Up/Down Select; Key B Back; Key Y Lock.

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

## MIDDLE WILL BECOME

Hidden: Joy Up/Down Select; Key B Back; Key Y Lock.

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

## FORWARD WILL BECOME

Hidden: Joy Up/Down Select; Key B Back; Key Y Lock.

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

## BACKWARD WILL BECOME

Hidden: Joy Up/Down Select; Key B Back; Key Y Lock.

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

Hidden: Key Y Lock.

```text
OTHER OPTIONS
 PAIR KEYBOARD
 PAIR COMPOSITE
 SAVED DEVICES


JOY UP\DOWN: SELECT
JOY PRESS: ACCESS
KEY B: BACK
```

## OTHER OPTIONS HELP

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

Transport-neutral. Body is example/dynamic. Hidden: Key Y Lock.

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

## PAIR KEYBOARD HELP

```text
PAIR KEYBOARD HELP







ANY KEY: BACK
```

## KEYBOARD SAVED

```text
KEYBOARD SAVED
TYPE KEYBOARD
SAVED DEVICES UPDATED




KEY B: BACK
KEY Y: LOCK
```

## PAIR COMPOSITE

Body is example/dynamic. Hidden: Key Y Lock.

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

## PAIR COMPOSITE HELP

```text
PAIR COMPOSITE HELP







ANY KEY: BACK
```

## COMPOSITE SAVED

```text
COMPOSITE SAVED
TYPE COMPOSITE
SAVED DEVICES UPDATED




KEY B: BACK
KEY Y: LOCK
```

## SAVED DEVICES — page 1 example

Hidden: Key B Back; Key Y Lock.

```text
1-4 OF 6 SAVED
 BKB-3G
 OFFICE MOUSE
 TRAVEL KEYBOARD
 GENERIC MOUSE

JOY UP\DOWN: SELECT
JOY PRESS: ACCESS
JOY RIGHT\LEFT: PAGE
```

## SAVED DEVICES — page 2 example

Hidden: Key B Back; Key Y Lock.

```text
5-6 OF 6 SAVED
 MX MASTER 3
 DESK KEYBOARD



JOY UP\DOWN: SELECT
JOY PRESS: ACCESS
JOY RIGHT\LEFT: PAGE
```

## DEVICE DETAILS — Mouse

Hidden: Key Y Lock. Active-device dynamic values are cyan.

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

```text
DEVICE DETAILS
BKB-3G
TYPE: KEYBOARD
STATUS: SAVED
 REMOVE DEVICE

JOY PRESS: ACCESS
KEY B: BACK
KEY Y: LOCK
```

## DEVICE DETAILS — Composite

```text
DEVICE DETAILS
DESK COMPOSITE
TYPE: COMPOSITE
STATUS: SAVED
 REMOVE DEVICE

JOY PRESS: ACCESS
KEY B: BACK
KEY Y: LOCK
```

## REMOVE DEVICE

```text
REMOVE DEVICE
BKB-3G
PAIRING AND MAPPINGS
WILL BE DELETED


KEY A: REMOVE
KEY B: CANCEL
KEY Y: LOCK
```

`KEY B: CANCEL` returns to the device-details page that opened this confirmation.

## LEARN THE KEYS

Screen identity and HOME option remain `LEARN THE KEYS`; displayed title is `PRESS TO LEARN A KEY`. This page is literal down to character placement.

```text
PRESS TO LEARN A KEY
      JOY UP
JOY    JOY    JOY
LEFT  PRESS  RIGHT
     JOY DOWN
               KEY A
LOCK SCREEN    KEY B
 AND UNLOCK    KEY X
  OPEN HOME -> KEY Y
```

Character positions are 1-based:

- `JOY UP`: J at column 7;
- row with three `JOY`: columns 1, 8, 15;
- `LEFT`, `PRESS`, `RIGHT`: columns 1, 7, 14;
- `JOY DOWN`: J at column 6;
- `KEY A`, `KEY B`, `KEY X`: K at column 16;
- `LOCK SCREEN`: L at column 1;
- `AND UNLOCK`: A at column 2;
- `OPEN HOME -> KEY Y`: row begins at column 3.

Press feedback: each control makes only its own didactic label(s) white while held. Key Y release locks. Other controls have no normal navigation action while this screen owns interaction.
