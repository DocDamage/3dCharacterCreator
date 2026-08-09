# P01 — Missing-State Report

**Status:** Complete
**Purpose:** Identify states that are not represented as distinct references or are only implied by the supplied screens. This is a design coverage report, not a claim that every state must be implemented in P01.

The references strongly cover the happy-path desktop composition. They do not consistently show failure, empty, focus, accessibility, persistence, or responsive states. The following states must be designed as explicit component variants or flow nodes in P03–P04 and P17.

| ID | Area | Missing or under-specified state | Evidence / risk | Required follow-up |
|---:|---|---|---|---|
| M01 | Global shell | Hover, keyboard focus, gamepad focus, pressed, disabled, and loading variants for primary/secondary/ghost actions | Screens mostly show default or selected controls only | Add state matrix to component inventory and focus rules |
| M02 | Global shell | Unsaved changes banner and close/navigation confirmation | Original concepts show Save/Load but no interrupted-edit state | Define non-destructive save/revert/confirm flow |
| M03 | Navigation | Narrow viewport, text overflow, collapsed rail, and reflow behavior | All references are desktop compositions | Define 1440×810 baseline plus minimum supported layout rules |
| M04 | Preview | No character selected / empty preview | Preview always shows a character or mannequin | Add empty state with create/import action |
| M05 | Preview | Async loading progress, skeleton, timeout, fallback, and render failure | Loading screen exists, but progress and fallback states are not fully linked | Design progress, fallback, and failure variants tied to preview state |
| M06 | Preview | Missing animation blueprint, incompatible preview mesh, or no preview animation | Original 01 shows named asset selectors; no invalid/empty selector state | Add compatibility and missing-asset states |
| M07 | Creator controls | Disabled/read-only controls when no compatible mesh, morph target, or material is loaded | Slider screens show active controls | Define disabled reasons and recovery action |
| M08 | Creator controls | Out-of-range, reset-region, zero-all, symmetry conflict, and validation feedback for morph editing | Advanced creator shows reset/randomize but not failed or conflicting input | Add inline validation and reversible reset states |
| M09 | Assets | Empty asset library, search no-results, filter no-results, and missing thumbnail | Asset browser screens show populated grids | Add empty and no-results components |
| M10 | Import | Invalid file type, unreadable package, duplicate asset, dependency missing, incompatible skeleton, and partial import | Import screen shows a happy-path wizard; error dialog is generic | Add typed import failure and conflict branches |
| M11 | Outfit / materials | Clipping warning, missing material, unsupported parameter, and revert-after-preview | Additional contact sheet hints at clipping, but no full state flow | Define validation badges, issue row, and revert behavior |
| M12 | Animation | No animation set, missing sequence, empty Blend Space, invalid graph, and unmapped retarget chain | Workspaces show populated graphs/timelines | Add empty/incomplete/blocked assistant states |
| M13 | Weapon / IK | Missing socket, invalid hand/foot target, incompatible skeleton, and failed auto-setup | Original 06 shows all checks green | Add actionable failed-check variants and fix routes |
| M14 | Technical | No physics asset, no sockets, no LODs, unsupported rig, and read-only inspection | Technical screens show populated skeleton/LOD data | Define empty/unsupported/locked states |
| M15 | Gameplay test | Test unavailable, map missing, play-in-editor failure, input disconnected, and stop/timeout state | Gameplay reference is a successful test scene | Add preflight and runtime error states |
| M16 | Presets | Empty preset list, duplicate name, delete confirmation, corrupt preset, and restore-default confirmation | Preset manager shows populated library only | Add preset lifecycle states; tie to session persistence |
| M17 | Randomization | No rule selected, locked category, invalid range, seed collision, and preview/revert state | Randomization reference shows configured ranges only | Add rule validation and non-destructive preview states |
| M18 | Settings | Unsaved settings, invalid path, engine mismatch, reset defaults, and permission failure | Settings screen shows values but no failure/confirmation states | Add settings persistence and recovery variants |
| M19 | Modals | Cancel, close, backdrop click, Escape, focus restoration, and stacked modal behavior for every dialog | Only modal content is shown; interaction semantics are implicit | Define shared modal shell contract and focus stack |
| M20 | Color picker | Color picker exists as a reference but has no current modal handler | Matrix row 33 is a known implementation gap | Add color picker component and apply/cancel semantics |
| M21 | Export | No selections, validation blockers, destination missing, write failure, partial export, success summary, and history | Original 07 shows ready-to-export and history, but not failure branches | Add explicit export state machine |
| M22 | Gamepad | Focus order per workspace, focus trap in modals, unavailable action, analog camera control, and controller disconnect | Overlay gives hints but not per-screen focus graph | Create focus graph and verify every major flow |
| M23 | Onboarding | Step back, skipped step, resume later, completed state, and first-run failure | Onboarding reference shows first step only | Define resumable onboarding state machine |
| M24 | Accessibility | Text scaling, color-meaning alternatives, reduced motion, high contrast, and remappable input | No reference screen covers accessibility | Add settings requirements and semantic status labels |

## Priority

1. **P03/P04:** M01, M02, M05, M07, M08, M09, M19, M20, M22.
2. **P08/P09/P11/P14:** M06, M10–M15, M21.
3. **P12/P13/P16/P17:** M16–M18, M23–M24.
