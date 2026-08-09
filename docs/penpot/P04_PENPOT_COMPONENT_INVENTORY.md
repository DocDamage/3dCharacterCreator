# P04 — Penpot Component Inventory

**Status:** Complete
**Penpot page:** `00 Design System`

## Component library

The local Penpot library contains 20 reusable components. The `CC` prefix identifies the Character Creator design system and keeps the library distinct from raw reference-shape names.

| Component | Semantic role | States / use |
|---|---|---|
| `CC Button Primary` | High-priority command | Default primary action |
| `CC Button Secondary` | Supporting command | Default secondary action |
| `CC Button Disabled` | Unavailable command | Disabled / explain reason in adjacent status |
| `CC Button Focus` | Input navigation proof | Keyboard/gamepad focus |
| `CC Input Default` | Text/value field | Default value or placeholder |
| `CC Input Error` | Invalid field | Required/error state |
| `CC Tab Active` | Selected workspace tab | Active navigation state |
| `CC Tab Default` | Unselected workspace tab | Default navigation state |
| `CC Checkbox Checked` | Boolean control | Selected state |
| `CC Checkbox Unchecked` | Boolean control | Unselected state |
| `CC Slider Default` | Numeric adjustment | Default track and thumb |
| `CC Slider Disabled` | Numeric adjustment | Disabled/read-only state |
| `CC Panel Surface` | Shared container | Surface and inspector panels |
| `CC Asset Card` | Asset/preset tile | Browser grids and picker results |
| `CC Modal Shell` | Dialog container | Scrim, surface, title, actions |
| `CC Focus Ring` | Focus affordance | Visible non-hover focus proof |
| `CC Status Success` | Validation status | Success/ready |
| `CC Status Warning` | Validation status | Warning/action needed |
| `CC Status Error` | Validation status | Blocking failure |
| `CC Status Info` | Validation/status message | Informational progress |

## Usage proof

The Design System page contains `Component Usage / Shared Controls` with five linked Penpot instances: `CC Button Primary`, `CC Button Secondary`, `CC Input Default`, `CC Status Success`, and `CC Focus Ring`. Each instance reports `isComponentInstance() === true` and resolves back to its local library component.

## Unreal handoff mapping

| Penpot primitive | Current Unreal primitive |
|---|---|
| Button / command | `UCharacterCreatorButtonWidget`, `UCharacterCreatorCommandButtonWidget`, `FCharacterCreatorUIFactory::MakeButton`, `MakeCommandButton` |
| Panel | `UCharacterCreatorPanelWidget`, `FCharacterCreatorUIFactory::MakePanel` |
| Slider | `UCharacterCreatorSliderWidget`, `FCharacterCreatorUIFactory::MakeSlider` |
| Tab | `UCharacterCreatorTabButtonWidget` |
| Modal shell | `UCharacterCreatorModalWidget`, `UCharacterCreatorModalManager` |
| Label / typography | `FCharacterCreatorUIFactory::MakeLabel`, `AddLabel` |
| Focus | `UCharacterCreatorUIHelpers::FocusWidget`, root focus targets, modal focus stack |
| Status / preview state | Session status messages and preview state colors in `CharacterCreatorRootWidget.cpp` |

## Reuse rules

1. Repeated controls use the library component or its runtime counterpart; do not redraw a new button for a new screen.
2. State changes are variants or documented component properties, not new one-off colors.
3. Every modal uses the shared shell and focus-stack contract.
4. Asset cards, animation tiles, validation rows, and controller hints inherit the same surface, spacing, border, and focus tokens.
5. The remaining P01 missing states are tracked as future component variants rather than silently omitted.
