# P03 — Penpot Foundations

**Status:** Complete
**Penpot page:** `00 Design System`

## Token library

The active `Character Creator Foundations` token set contains 25 tokens.

| Token family | Tokens |
|---|---|
| Surfaces | `color.surface.0 #070A0D`, `color.surface.1 #0D1217`, `color.surface.2 #141B22`, `color.surface.3 #1C252E` |
| Text | `color.text.primary #F4F0E6`, `color.text.muted #A4ACB4` |
| Accent | `color.accent.brass #C89B3C`, `color.accent.focus #4D9CFF` |
| Validation | `color.status.success #63C174`, `color.status.warning #E4B45C`, `color.status.error #D66060`, `color.status.info #5DA9E9` |
| Spacing | `spacing.04 4`, `spacing.08 8`, `spacing.12 12`, `spacing.16 16`, `spacing.24 24`, `spacing.32 32` |
| Radius | `radius.small 4`, `radius.large 8` |
| Borders | `border.default 1`, `border.focus 2` |
| Opacity | `opacity.disabled 0.45` |
| Elevation | `shadow.panel 0/4/12`, `shadow.modal 0/12/32` (x/y/blur, black drop shadows) |

The sample swatches, button primitives, radius samples, and usage panel have token bindings applied. Panel and modal shadow tokens are also applied to the usage proof and modal component.

## Typography

The library contains six typography styles using the available `M PLUS 2` family:

| Role | Size | Weight | Line height | Tracking |
|---|---:|---:|---:|---:|
| Display | 28 | 700 | 32 | 0 |
| Heading | 18 | 700 | 22 | 0 |
| Body | 14 | 400 | 20 | 0 |
| Label | 12 | 600 | 16 | 0.4 |
| Numeric | 12 | 500 | 16 | 0 |
| Caption | 10 | 400 | 14 | 0.2 |

Penpot exposes typography names by their terminal size name, so the semantic roles above are additionally recorded by font family, size, weight, and their applied sample text names (`Type / Display`, `Type / Heading`, and so on).

## Layout and interaction contract

- Baseline canvas: 1440×810 for runtime handoff and 16:9 Penpot screen boards at 640×360 for reference reconstruction.
- Spacing uses a 4px base with 8/12/16/24/32px production steps.
- Panels use surface.1–surface.3; the canvas uses surface.0; primary actions and focus-critical affordances use brass.
- Default borders are 1px; keyboard/gamepad focus is a visible 2px focus accent and cannot depend on hover.
- Validation is semantic: green success, amber warning, red error, blue information.
- Modal shells use the large radius and modal elevation; modal focus returns to the invoking control.
- The runtime’s fixed canvas, panel hierarchy, focus helpers, and modal manager are the initial Unreal handoff contract.

## Foundation verification

- Penpot library: 1 active token set, 25 tokens, 12 colors, 6 typographies.
- Design System board: native editable shapes and text; 1500×1320 after adding component state samples.
- Token bindings were applied to sample surfaces, accents, radius samples, typography samples, panel usage, and modal shell.
