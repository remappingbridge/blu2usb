# Documentation governance

## Contract-first rule

No product behavior is implemented from memory, from an old repository or from an inferred convention. A behavior must be supported by the frozen documentation in this repository.

If two documents appear to conflict, apply the precedence defined in `README.md`. If the conflict is still material, stop before implementation and resolve the contract explicitly.

## Literal, dynamic and example text

Screen documentation distinguishes three classes:

- **literal/fixed**: wording, line placement and spacing are normative;
- **dynamic**: field position/role is fixed, but the runtime value changes (device name, page count, status, profile, mapping target);
- **example**: the block demonstrates geometry/context, while implementation may define the body text because the page rule explicitly grants that freedom.

Examples that are already clear and fit the context should be preserved instead of rewritten without reason.

## Layout change control

A frozen layout is not adjusted merely because an alternative seems cleaner. Changes require a punctual product decision. Typography fixes already approved in the source material are normalized in the canonical screen document:

- `KEY C` -> `KEY X`;
- `FORWARED`/`FOREWARED` -> `FORWARD`;
- duplicated navigation rule -> distinct LEFT and RIGHT behavior;
- `JOY DOWN` means next/below item;
- `KEY LEFT/RIGHT` references are normalized to `JOY LEFT/RIGHT`;
- Saved Devices second-page example starts at `5-6 OF 6`;
- swapped `MIDDLE/FORWARD/BACKWARD WILL BECOME` titles are corrected to their actual screen identity.

These corrections are typo/consistency fixes, not permission for arbitrary layout redesign.

## Evidence reuse

Prior repositories are evidence sources only. Reuse means reexpressing accepted behavior behind the new architecture and tests, not copying experimental coupling.

## Physical acceptance

A gate that changes hardware-visible behavior remains pending until its enumerated physical scenarios pass. Terminal/UART/CDC logs are not physical acceptance criteria.

## No debug firmware

Debug-only firmware features are out of scope for the product repository. Host-side test binaries and CI diagnostics are allowed because they do not alter the shipped Pico firmware or its USB identity.
