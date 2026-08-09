# P18 — Visual QA Record

**Audit date:** 2026-08-09 (America/New_York)
**Scope:** all 39 individual Penpot/reference screens, responsive layout behavior, keyboard/gamepad focus, modal states, accessibility styling, clipping, and reuse.
**Source of truth:** `UE_5_7_1_Character_Creator_ALL_UI_UX_Screens/` and the P01–P04 records under `docs/penpot/`.

## Result

P18 is complete as an evidence pass. Every reference is accounted for below and every known discrepancy is recorded. The current implementation is a native C++ UMG foundation with a shared 1440×810 design canvas, not a pixel-complete reproduction of the Penpot boards. Therefore the visual disposition is intentionally `PARTIAL` for most screens; no visual parity is claimed without a captured comparison image.

## Screen-by-screen disposition

`PARTIAL` means the route/state and reusable shell exist, but content density, composition, assets, or styling still diverge from the reference. `EVIDENCE` means the item is an information-architecture/reference artifact rather than a single runtime screen. `HARDENED` marks a screen whose interaction behavior received a concrete P18/P19 pass, even though visual parity remains partial.

| ID | Reference | Current route/state | Disposition | Primary deviation or evidence |
|---:|---|---|---|---|
| 01 | Initial character creator preview | Dashboard + CharacterCreator preview shell | PARTIAL | Live preview and shell exist; reference composition, asset tray, and artwork are simplified. |
| 02 | Character creator animation workspace | AnimationOverview and animation routes | PARTIAL | Routing and state summaries exist; reference graph/timeline composition is not reproduced. |
| 03 | Sitemap/navigation architecture | Product-map artifact | EVIDENCE | Covered by P02 flow map; no single runtime screen is expected. |
| 04 | Project character browser | ProjectBrowser | PARTIAL | Project records and route exist; browser grid/search/thumbnail treatment is simplified. |
| 05 | Body and face advanced creator | CharacterCreator | PARTIAL | Body/face controls and live preview exist; sculpting panels and visual density differ. |
| 06 | Weapon setup and IK | WeaponsAndIK | PARTIAL | Weapon metadata/socket/IK state is real; reference rig/viewport tooling is simplified. |
| 07 | Validation and export | ValidationExport + export modal | HARDENED | Validation/export path is wired; dedicated dashboard, issue history, and exact layout remain partial. |
| 08 | Main dashboard | Dashboard | PARTIAL | Route and actions exist; reference cards, thumbnails, and spacing differ. |
| 09 | Body sculpting | CharacterCreator | PARTIAL | Shared creator route covers the state; dedicated sculpt layout is not reproduced. |
| 10 | Face sculpting | CharacterCreator | PARTIAL | Face parameters exist; reference controls and face-specific composition differ. |
| 11 | Clothing/armor/accessories | OutfitAndArmor | PARTIAL | Loadout state and preview wiring exist; asset browser and layered cards are simplified. |
| 12 | Hair/grooming/skin | HairAndGrooming | PARTIAL | Hair/skin state exists; grooming-specific reference controls are simplified. |
| 13 | Materials/color studio | MaterialsAndColor | HARDENED | Palette/material edits and the dedicated color-picker modal are real; reference swatches/art direction remain simplified. |
| 14 | Animation workspace overview | AnimationOverview | PARTIAL | Route and source summary exist; reference overview cards/preview layout differ. |
| 15 | Locomotion setup | LocomotionSetup | PARTIAL | Routed state/profile foundation; no full locomotion authoring canvas. |
| 16 | Blend Space assistant | BlendSpaceAssistant | PARTIAL | Profile/state foundation; no Penpot-faithful 2D graph/editor. |
| 17 | Animation Blueprint assistant | AnimationBlueprintWorkspace | PARTIAL | Profile/state foundation; no generated AnimGraph/state-machine UI. |
| 18 | Montage/combo builder | MontageComboBuilder | PARTIAL | Montage/combo records exist; timeline/track editor is not implemented. |
| 19 | Retargeting assistant | RetargetingAssistant | PARTIAL | Real P17 retarget generation exists; assistant layout and mapping editor are simplified. |
| 20 | Skeleton/rig/socket inspector | SkeletonRigSocketInspector | PARTIAL | P17 inspection validates source/target bones and sockets; visual inspector is simplified. |
| 21 | Physics setup | PhysicsSetup | PARTIAL | Physics state contract exists; simulation viewport/tools are not reproduced. |
| 22 | Gameplay test workspace | GameplayTest | PARTIAL | Test lifecycle/action log exists; gameplay presentation is simplified. |
| 23 | Preview studio | PreviewStudio | PARTIAL | Camera/lighting/zoom state exists; studio controls and presentation differ. |
| 24 | Photo/portrait studio | PortraitStudio | PARTIAL | Capture preparation exists; portrait capture evidence/art direction is incomplete. |
| 25 | LOD/performance | LODPerformance | HARDENED | Settings frame cap/preview LOD flags and performance state exist; metric charts are simplified. |
| 26 | Import wizard | ImportWizard | PARTIAL | Directory validation/catalog flow exists; wizard progress/dependency visuals differ. |
| 27 | Asset browser expanded | AssetBrowser | PARTIAL | Route and asset filtering foundation exist; expanded browser grid is simplified. |
| 28 | Character preset manager | Preset APIs + preset_manager modal | PARTIAL | Versioned preset/randomization behavior exists; dedicated manager screen is absent. |
| 29 | Randomization rules | randomize command + preset APIs | PARTIAL | Seeded randomization/category locks exist; dedicated rules screen is absent. |
| 30 | Settings | Settings | HARDENED | Persistence, sanitization, scale/text contrast, frame cap, and input flags are wired; visual layout differs. |
| 31 | New character modal | new_character modal | HARDENED | Modal action focus, confirm/cancel, Escape/B restoration are wired; artwork/layout simplified. |
| 32 | Asset picker modal | asset_picker modal | HARDENED | Import/validation/conflict actions and focus trap are wired; picker content is simplified. |
| 33 | Color picker modal | color_picker modal | HARDENED | Dedicated modal exposes RGB sliders, target actions, focus trapping, and Apply/Revert semantics; arbitrary HSV editing remains future work. |
| 34 | Save as template modal | save_template modal | HARDENED | Modal focus and session/preset action are wired; visual treatment simplified. |
| 35 | Export options modal | export_options modal | HARDENED | Full/metadata selection and real export success/error path are wired; exact modal styling differs. |
| 36 | Loading/progress | Preview Loading + async load status | PARTIAL | Async/fallback state is real; full progress workspace is not reproduced. |
| 37 | Error dialog | load_error and export error modals | HARDENED | Error states, actionable copy, and focus restoration exist; reference art/layout simplified. |
| 38 | Gamepad navigation overlay | gamepad_overlay modal + input handlers | HARDENED | Directional nearest-neighbor focus graph, D-pad/analog/modal/shoulder/camera behavior are implemented; per-device screenshot evidence is unavailable. |
| 39 | Onboarding tutorial | onboarding modal + session state | HARDENED | Onboarding modal and focus path exist; multi-step tutorial visuals are simplified. |

## Responsive review

The root is built around the documented 1440×810 composition and now keeps the responsive boundary at the root `UScaleBox`. `UIScale` is applied to that boundary, `TextScale` is applied to text widgets, and modal action rows remain inside the modal content panel.

| Viewport | Code/structural result | Visual sign-off |
|---|---|---|
| 1280×720 | ScaleBox boundary and clamped UIScale prevent the shell from changing its internal coordinate system; modal actions use the same bounded content region. | Not captured; manual visual sign-off remains open. |
| 1440×810 | Primary design canvas and all routed screens use the shared baseline. | Reference inspection completed; parity remains partial. |
| 1920×1080 | ScaleBox provides a larger presentation frame without changing authored positions; text scale and high contrast remain applied globally. | Not captured; manual visual sign-off remains open. |

## Focus, accessibility, and gamepad review

- Root focus graphs are rebuilt after screen changes and use wrapped left/right/up/down links for every visible focusable button/slider.
- Modal action buttons replace the active screen focus list while a modal is open; first-action focus is explicit; Escape and gamepad B close/cancel and rebuild the prior screen graph.
- Gamepad shoulders are ignored while a modal is open, preventing accidental workspace changes. D-pad and analog navigation cycle modal actions; the right stick remains reserved for preview camera orbit outside modals.
- Settings changes now persist after initialization, sanitize UI/text scales and frame rate, apply high contrast to text/status colors, and apply `TargetFrameRate` through `UGameUserSettings`.
- Known accessibility gap: no physical-controller screenshot capture was available in the headless verification environment; the runtime graph is now directional nearest-neighbor, but still needs device-by-device sign-off.

## Known visual deviations accepted into P19

1. Native C++ UMG panels are structurally reusable but do not yet match the Penpot artwork, thumbnail density, typography, borders, graphs, timelines, or 3D composition pixel-for-pixel.
2. The color picker is a dedicated modal with RGB sliders and target actions; arbitrary HSV editing remains future work.
3. Dedicated project-browser, preset-manager, and randomization-rules compositions remain simplified; the validation dashboard is routed and functional but not Penpot-faithful.
4. The first non-headless screenshot attempt did not exit after `HighResShot`; no generated screenshot is presented as visual evidence. The attempt and command are recorded in P19.
5. Headless test logs are now free of the missing Manny material-instance and placeholder `/Game/Test` warnings. The imported Manny package still emits stale PoseAsset notices, and the source idle has no animation curves; neither fails the P17 generation test and both remain source-asset cleanup work.
