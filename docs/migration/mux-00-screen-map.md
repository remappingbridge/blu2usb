# MUX-00 — Screen inventory and G06 migration map

Status: **FROZEN FOR IMPLEMENTATION — HUMAN ACCEPTANCE PENDING**

Mouse UI Layout 1.0 contains exactly **30 canonical screens**.

## Canonical 30-screen inventory

| # | Mouse UI 1.0 screen ID | Family |
|---:|---|---|
| 1 | `searching-first` | didactic / FIRST |
| 2 | `first-mouse-connected` | didactic / first-success |
| 3 | `home-searching` | HOME / SAVED search |
| 4 | `home-searching-help` | Help |
| 5 | `home-retry` | HOME / saved-search retry |
| 6 | `home-retry-help` | Help |
| 7 | `pair-new` | NEW search |
| 8 | `help-pair-new` | Help |
| 9 | `retry-pair-new` | NEW retry |
| 10 | `help-retry-pair-new` | Help |
| 11 | `home-connected` | HOME |
| 12 | `help-home-connected` | Help |
| 13 | `remapper-options` | profiles |
| 14 | `help-remapper-options` | Help |
| 15 | `passthrough-active` | profile |
| 16 | `passthrough-not-active` | profile |
| 17 | `standard-not-active` | profile |
| 18 | `standard-active` | profile |
| 19 | `escape-not-active` | profile |
| 20 | `escape-active` | profile |
| 21 | `custom-edit` | Custom |
| 22 | `left` | Custom source |
| 23 | `right` | Custom source |
| 24 | `middle` | Custom source |
| 25 | `forward` | Custom source |
| 26 | `backward` | Custom source |
| 27 | `saved-devices` | registry |
| 28 | `remove-this` | registry/remove |
| 29 | `help-remove-this` | Help |
| 30 | `learn-the-keys` | didactic |

No Keyboard, Composite, Other Devices, Device Details or multi-Mouse focus screen belongs to UI Layout 1.0.

## G06 screen migration map

The table maps the G06 `blu2usb_screen_id_t` product concepts to the new UX. It is a behavior map, not a requirement to keep the old enum values.

| G06 screen | MUX destination | Decision |
|---|---|---|
| `HOME` | `searching-first` / `home-searching` / `home-retry` / `home-connected` | split by saved/live truth |
| `MOUSE_STATUS` | `home-connected` + `saved-devices` | separate status screen removed |
| `OTHER_DEVICES_STATUS` | none | removed from Mouse UI 1.0 |
| `MOUSE_HELP` | `help-home-connected` where applicable | old generic status Help removed |
| `DEVICES_HELP` | none | removed |
| `MOUSE_OPTIONS` | `remapper-options` | retained concept, new controls/text |
| `PAIR_MOUSE` | `pair-new` for explicit new-only pair; `searching-first` for first use | split by purpose |
| `PAIR_MOUSE_HELP` | `help-pair-new` / `help-retry-pair-new` | expanded |
| `MOUSE_SAVED` | `first-mouse-connected` only for first accepted Mouse; otherwise HOME resolution | generic success page removed |
| `APPLY_PASSTHROUGH` | `passthrough-not-active` | renamed state model |
| `PASSTHROUGH_APPLIED` | `passthrough-active` | success represented as ACTIVE |
| `APPLY_DEFAULT` | `standard-not-active` | visible Default -> Standard |
| `DEFAULT_APPLIED` | `standard-active` | visible Default -> Standard |
| `APPLY_ESCAPE` | `escape-not-active` | retained mapping |
| `ESCAPE_APPLIED` | `escape-active` | active state |
| `EDIT_CUSTOM` | `custom-edit` | retained concept |
| `CUSTOM_APPLIED` | `custom-edit` + confirmed profile state | dedicated Custom success screen removed |
| `LEFT_WILL_BECOME` | `left` | retained |
| `RIGHT_WILL_BECOME` | `right` | retained |
| `MIDDLE_WILL_BECOME` | `middle` | retained |
| `FORWARD_WILL_BECOME` | `forward` | retained |
| `BACKWARD_WILL_BECOME` | `backward` | retained |
| `OTHER_OPTIONS` | none | removed |
| `OTHER_OPTIONS_HELP` | none | removed |
| `PAIR_KEYBOARD` | none | removed |
| `PAIR_KEYBOARD_HELP` | none | removed |
| `KEYBOARD_SAVED` | none | removed |
| `PAIR_COMPOSITE` | none | removed |
| `PAIR_COMPOSITE_HELP` | none | removed |
| `COMPOSITE_SAVED` | none | removed |
| `SAVED_DEVICES` | `saved-devices` | changes from up-to-4 list to one Mouse/page |
| `DEVICE_DETAILS_MOUSE` | folded into `saved-devices` | page already shows status/profile/remove |
| `DEVICE_DETAILS_KEYBOARD` | none | removed |
| `DEVICE_DETAILS_COMPOSITE` | none | removed |
| `REMOVE_DEVICE` | `remove-this` + `help-remove-this` | retained, now identity-stable target |
| `LEARN_KEYS` | `learn-the-keys` | no longer boot root |

## New screens with no one-to-one G06 predecessor

The following are first-class new product states and must not be simulated as text aliases over an unrelated G06 state:

- `searching-first`;
- `first-mouse-connected`;
- `home-searching`;
- `home-searching-help`;
- `home-retry`;
- `home-retry-help`;
- `retry-pair-new`;
- `help-retry-pair-new`;
- `help-home-connected`;
- `help-remapper-options`;
- `help-remove-this`.

They require the registry/coordinator/product snapshot planned in MUX-01/MUX-02 before embedded transport binding.

## Dynamic values

The following are not frozen sample literals:

- connected Mouse display name;
- Saved Devices `N OF M`;
- Saved Devices Mouse name;
- Saved Devices connected/disconnected status;
- Saved Devices confirmed profile;
- Custom mapping rows;
- HOME confirmed remap summary.

Their formatting/allowed values are frozen, but their current text is projected from product state.
