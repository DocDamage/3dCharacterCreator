# UE 5.7.1 Standalone Character Creator
## Complete Penpot + Codex UI/UX Production Plan

**Project type:** Standalone AAA-quality character creator and animation authoring application
**Target engine:** Unreal Engine 5.7.1
**Primary design tool:** Penpot
**Primary implementation/design agent:** Codex via official Penpot MCP
**Primary visual reference package:** `UE_5_7_1_Character_Creator_ALL_UI_UX_Screens\` (expanded reference directory currently present; the ZIP itself is not present)
**Reference count:** 39 individual UI/UX screens + 2 overview/contact sheets
**Primary interaction modes:** Mouse/keyboard + gamepad
**Core UX principle:** AAA character creator on the surface; Unreal automation underneath.

> **Current-state audit — 2026-08-09 (America/New_York)**
>
> This document is still the product/design target. The repository currently contains a working Unreal C++ vertical slice, not the completed Penpot production system described below.
>
> **Repository baseline:** branch `agent/character-creator-foundation`, commit `b8c47d8` (`Implement character creator workflow foundation`).
>
> **Implemented and verified:** UE 5.7 editor target build; game-instance-owned character-creator session; dashboard and body/face editing; live Sidekick render-target preview with async asset loading and fallback states; outfit/hair/material/weapon-IK workflow routing; animation/technical/production workspace routing; versioned save/autosave/preset APIs; import-directory validation; JSON manifest export; modal focus management; keyboard/gamepad shortcut handling; four `CharacterCreator.*` automation tests.
>
> **Present as foundation or scaffold:** most named workspaces are routed C++ screens with reusable panels, labels, command buttons, and status summaries. Their domain operations are not yet implemented: Blend Space/Animation Blueprint/montage generation, retarget execution, socket and rig editing, physics simulation, gameplay movement/combat, LOD metrics, asset browsing/import, randomization, settings persistence, portrait capture, and Unreal package/Blueprint/Data Asset export.
>
> **P00 evidence present:** Penpot MCP verification and read/write/delete acceptance evidence are recorded in `docs/penpot/P00_PENPOT_MCP_ACCEPTANCE.md`. The connected editable file is `New File 1`; the requested P01–P04 audit and design-system documents, visual comparison evidence, and a complete gamepad focus graph remain outstanding. The reference directory is present and contains the expected 39 individual screens plus 2 contact sheets; no ZIP was found.
>
> **Verification snapshot:** `Build.bat threedcharacterEditor Win64 Development` succeeded; headless `Automation RunTests CharacterCreator` found and passed 4 tests: `CharacterCreator.Export.Contract`, `CharacterCreator.Import.FreeAnimationsPack`, `CharacterCreator.Session.Foundation`, and `CharacterCreator.UIAndSave.Contract`. These tests verify contracts and content validation, not visual completeness or full Unreal authoring behavior.

## Current implementation position

The project is at the transition from foundation work to functional workspace implementation. P00 is complete with a recorded Penpot read/write/delete acceptance run; P01 remains incomplete until the full reference audit is documented. In parallel, the C++ runtime has already reached early P05–P17 coverage as a deliberately unified module. Treat every routed workspace as **implemented foundation** until its domain behavior, visual fidelity, input states, and verification evidence are completed.

### Current evidence map

| Area | Evidence in the repository | Current interpretation |
|---|---|---|
| Runtime/UI shell | `Source/threedcharacter/UI/CharacterCreatorRootWidget.cpp`, `CharacterCreatorUIFramework.*` | Native runtime UMG/C++ shell at a fixed 1440×810 design canvas, with shared styling helpers and routed screens |
| Character state | `CharacterCreatorSession.*`, `CharacterCreatorSubsystem.*`, `CharacterCreatorSaveGame.*` | Centralized appearance/loadout/IK/animation/preset/onboarding state with save/autosave APIs |
| Preview | `CharacterCreatorPreviewActor.*`, `Content/Synty/SidekickCharacters/` | Live scene capture, Sidekick mesh/material references, modular outfit/hair components, camera modes, async loading/fallback |
| Animation content | `Content/FreeAnimationsPack/` | 95 imported `.uasset` packages, including Manny/Quinn animations, ABPs, Blend Spaces, IK/Control Rig, and retargeting assets; source assets are selected but not retargeted/generated into Sidekick output |
| Import/export | `CharacterCreatorImportService.*`, `CharacterCreatorExportService.*` | File/package validation and JSON manifest writing; no asset copy/import pipeline or Unreal deliverable generation |
| Tests | `Source/threedcharacter/UI/CharacterCreatorAutomationTests.cpp` | Four passing contract tests covering session, export, import validation, UI helper/save compatibility |
| Design evidence | `UE_5_7_1_Character_Creator_ALL_UI_UX_Screens/` | 39 individual references + 2 contact sheets are present; no Penpot artifacts or required evidence docs exist |
| Module boundary | `Source/threedcharacter/threedcharacter.Build.cs`, `threedcharacter.uproject` | One unified runtime module with Core/Engine/UMG/Slate/Json dependencies; no separate editor, authoring, data, or UI module exists yet |

### Current architectural constraints

- The runtime is intentionally unified. Do not split modules until the preview, session/persistence, editor-tooling, and asset-generation ownership boundaries are exercised and documented.
- The current UI is native C++ UMG constructed at runtime; there are no tracked Widget Blueprint screens or Penpot-to-UMG handoff artifacts.
- `Content/` is ignored by the current Git surface, so the local Sidekick/FAB assets are available to the working project but are not represented in the tracked source diff.
- Existing animation assets are Manny/Quinn source content. The current code records source paths and retargeting state; it does not yet prove a compatible Sidekick target animation or generated animation system.

### Known verification caveats

- The latest headless editor log reports a non-blocking Python exposure warning because `ECharacterCreatorAnimationState` and the generated `CharacterCreatorAnimationState` name collide when exposed to Python. Resolve or explicitly document this before expanding scripting/editor automation.
- The animation workspace currently stores the Manny mesh path `/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin` in the field named `SourceSkeleton`. Validate the actual `USkeleton` asset path before implementing retarget execution.
- `Config/DefaultEngine.ini` still uses the engine OpenWorld template as `GameDefaultMap`; the character creator is launched through the global game mode/HUD rather than a dedicated creator map.

### Status vocabulary used below

- **Complete:** the plan item is implemented and verified at the level claimed by this plan.
- **Partial:** the repository has a real foundation or limited behavior, but the plan acceptance criteria are not met.
- **Scaffold:** a route, control, state, or status message exists, but the underlying feature is not implemented.
- **Not started:** no meaningful repository implementation or evidence exists.
- **Blocked:** the item cannot be accepted until an external prerequisite is supplied or verified; the current blocker is recorded.

---

# 1. Executive Summary

The goal is to design and eventually implement a complete standalone character creator for Unreal Engine 5.7.1.

The application is not intended to be a simple cosmetic character editor. It must cover the complete character pipeline:

- character creation
- body sculpting
- advanced face sculpting
- hair and grooming
- skin, scars, tattoos, and makeup
- layered clothing
- armor
- accessories
- weapons
- materials and dyeing
- animation assignment
- locomotion setup
- Blend Space creation
- Animation Blueprint setup
- montage and combo construction
- weapon-specific pose and IK setup
- retargeting
- skeleton and socket inspection
- physics
- gameplay testing
- preview/rendering
- LOD and performance validation
- asset importing
- preset management
- randomization
- validation
- export to Unreal-compatible deliverables

The product must hide unnecessary Unreal complexity from normal users.

For example, the user should see:

> Movement Set → Sword Combat → Auto Setup

instead of being forced to manually work through:

> AnimGraph → State Machine → Transition Rule → BlendSpace2D → Montage Slots → IK Rig → Retargeter

The underlying implementation may still generate and configure those Unreal systems.

---

# 2. Product Vision

The final product should feel like a hybrid of:

- a premium AAA RPG character creator
- a polished wrestling/fighting-game creation suite
- a professional animation authoring tool
- an artist-friendly game-development application

The interface should feel more like a finished game creation environment than raw Unreal Editor tooling.

## Visual direction

Primary visual characteristics:

- near-black canvas
- charcoal/graphite panels
- subtle cool-gray separators
- restrained warm-gold highlight color
- off-white primary text
- muted gray secondary text
- blue informational states
- green success states
- amber warnings
- red errors
- cinematic 3D preview areas
- dense but readable controls
- game-quality polish
- compact professional spacing
- minimal visual clutter

Avoid:

- generic SaaS dashboard styling
- excessive rounded mobile-style cards
- stock Material Design appearance
- cloning Unreal Editor visually
- excessive neon/glow effects
- unnecessary graph complexity in normal workflows

---

# 3. Core Product Principles

## 3.1 AAA creator on the surface

The primary experience must feel approachable and visual.

Users should manipulate:

- characters
- materials
- animations
- weapons
- poses
- equipment
- presets

rather than raw engine graphs whenever possible.

## 3.2 Unreal automation underneath

The product should eventually be capable of configuring or generating Unreal-facing systems such as:

- Animation Blueprints
- locomotion state machines
- Blend Spaces
- animation montages
- animation slots
- IK rigs
- IK retargeters
- sockets
- weapon profiles
- motion warping
- root motion configuration
- gameplay-ready character Blueprints
- Data Assets
- skeletal mesh setup

## 3.3 Non-destructive workflow

Changes should be reversible and previewable.

Support:

- undo
- redo
- compare
- reset region
- reset category
- reset all
- duplicate preset
- versioned character saves

## 3.4 Real-time feedback

Whenever possible, users should immediately see:

- body morph results
- material changes
- animation transitions
- weapon grip changes
- IK corrections
- clipping problems
- validation results
- performance impact

## 3.5 Mouse and gamepad are equal citizens

The creator must not depend on hover-only UX.

Every major workflow must be navigable using:

- mouse
- keyboard
- gamepad

---

# 4. Source Reference Package

Use the expanded directory currently in the repository:

`UE_5_7_1_Character_Creator_ALL_UI_UX_Screens\`

The expected ZIP is not present in the current repository. If a future handoff supplies a ZIP, compare its contents against this expanded directory before replacing or adding references.

The expanded reference directory contains two major groups.

## 4.1 Original high-detail concepts

1. Initial Character Creator + Animation Preview
2. Character Creator + Integrated Animation Workspace
3. UI/UX Sitemap & Navigation Architecture
4. Project / Character Browser
5. Body & Face Advanced Creator
6. Weapon Setup & IK
7. Validation & Export

## 4.2 Additional individual concepts

1. Main Dashboard
2. Body Sculpting
3. Face Sculpting
4. Clothing / Armor / Accessories
5. Hair / Grooming / Skin
6. Materials & Color Studio
7. Animation Workspace Overview
8. Locomotion Setup
9. Blend Space Assistant
10. Animation Blueprint Assistant
11. Montage / Combo Builder
12. Retargeting Assistant
13. Skeleton / Rig / Socket Inspector
14. Physics Setup
15. Gameplay Test Workspace
16. Preview Studio
17. Photo / Portrait Studio
18. LOD / Performance
19. Import Wizard
20. Asset Browser Expanded
21. Character Preset Manager
22. Randomization Rules
23. Settings
24. New Character Modal
25. Asset Picker Modal
26. Color Picker Modal
27. Save As Template Modal
28. Export Options Modal
29. Loading / Progress
30. Error Dialog
31. Gamepad Navigation Overlay
32. Onboarding / Tutorial

## 4.3 Contact sheets

The package also contains overview contact sheets.

These are supplementary references only.

They must not be used instead of individual screen reconstruction.

---

# 5. Source-of-Truth Priority

When references conflict, use this order:

1. Original high-detail concept screens
2. Individual additional screens
3. Contact sheets
4. Reasoned UX decisions

Any meaningful discrepancy must be recorded.

Do not silently mix conflicting design decisions.

---

# 6. Penpot + Codex Architecture

Recommended workflow:

```text
User
  |
  v
Codex
  |
  v
Official Penpot MCP
  |
  v
Penpot MCP Plugin
  |
  v
Editable Penpot Project
  |
  v
Design System + Screens + Prototype
  |
  v
Unreal Engine 5.7.1 Implementation
```

Penpot becomes the UI/UX source of truth.

Codex should manipulate Penpot through the official MCP server.

---

# 7. Penpot MCP Setup Requirements

The local MCP workflow is preferred for this project because local reference images and development assets may be involved.

Expected local workflow:

```text
Codex
  |
  v
http://localhost:4401/mcp
  |
  v
Penpot MCP
  |
  v
Penpot plugin
  |
  v
Open Penpot design
```

Expected plugin manifest:

```text
http://localhost:4400/manifest.json
```

Expected startup command:

```powershell
npx -y @penpot/mcp@stable
```

These values must be verified against current Penpot documentation before use.

## Setup acceptance checks

Do not declare MCP setup complete until:

- MCP process starts
- MCP endpoint responds
- plugin manifest responds
- Codex sees Penpot tools
- Penpot plugin connects
- Codex can read the open design
- Codex can create a temporary object
- Codex can read the object back
- Codex can delete the test object
- no credentials are committed to source control

---

# 8. Penpot Project Structure

Recommended Penpot organization:

## 00 — Product Map

Contains:

- project cover
- product vision
- sitemap
- screen inventory
- navigation architecture
- primary user flows

## 01 — Foundations

Contains:

- color tokens
- typography
- spacing
- radius
- borders
- shadows/elevation
- icon principles
- focus system
- gamepad focus
- layout grid

## 02 — Components

Contains all reusable UI components.

## 03 — Character Creation

Contains:

- Project / Character Browser
- Main Dashboard
- Create / Base
- Body Sculpting
- Face Sculpting
- Face Advanced
- Hair / Grooming / Skin
- Clothing / Armor / Accessories
- Character Preset Manager
- Randomization Rules

## 04 — Equipment & Materials

Contains:

- Weapons
- Weapon Setup & IK
- Materials & Color Studio
- Asset Browser

## 05 — Animation

Contains:

- Animation Workspace
- Locomotion Setup
- Blend Space Assistant
- Animation Blueprint Assistant
- Montage / Combo Builder
- Weapon Animation Profiles
- Animation Set Builder

## 06 — Technical

Contains:

- Retargeting Assistant
- Skeleton / Rig / Socket Inspector
- Physics Setup
- LOD / Performance

## 07 — Preview & Testing

Contains:

- Gameplay Test Workspace
- Preview Studio
- Photo / Portrait Studio

## 08 — Import, Validation & Export

Contains:

- Import Wizard
- Validation Dashboard
- Export Workspace
- Export Options

## 09 — Modals & States

Contains:

- New Character
- Asset Picker
- Color Picker
- Save As Template
- Export Options
- Loading
- Error
- Empty
- Success
- Warning
- Confirm Delete
- Unsaved Changes
- Missing Dependency

## 10 — Prototype & Flows

Contains:

- new character flow
- animation setup flow
- weapon setup flow
- retarget/validation flow
- export flow
- gamepad navigation flow

---

# 9. Design Foundations

## 9.1 Color tokens

Create reusable semantic tokens.

### Surfaces

- `Surface/Canvas`
- `Surface/Header`
- `Surface/Panel`
- `Surface/PanelRaised`
- `Surface/Hover`
- `Surface/Selected`

### Text

- `Text/Primary`
- `Text/Secondary`
- `Text/Muted`
- `Text/Disabled`

### Borders

- `Border/Subtle`
- `Border/Strong`

### Accent

- `Accent/Primary`
- `Accent/Hover`
- `Accent/Pressed`

### Status

- `Status/Info`
- `Status/Success`
- `Status/Warning`
- `Status/Error`

---

# 10. Typography

Preferred typography direction:

## Headings

Use a condensed display typeface similar to:

- Barlow Condensed

## Controls / body copy

Use a clean UI sans-serif similar to:

- Inter

Create styles for:

- Display
- Workspace Title
- Section Heading
- Panel Heading
- Body
- Small Body
- Control Label
- Button
- Caption
- Metadata

---

# 11. Spacing System

Base spacing scale:

- 4
- 8
- 12
- 16
- 20
- 24
- 32
- 40
- 48

Avoid arbitrary spacing values unless justified.

---

# 12. Component System

The following reusable components should exist before the full screen set is considered complete.

## Navigation

- GlobalTopNavigation
- WorkspaceNavigation
- SidebarNavigation
- Breadcrumb
- UtilityToolbar

## Containers

- Panel
- InspectorPanel
- InspectorSection
- Accordion
- SplitPane
- ViewportFrame
- BottomAssetTray

## Buttons

- Button
- IconButton
- SplitButton
- DropdownButton

States:

- default
- hover
- pressed
- selected
- focused
- disabled

## Inputs

- TextField
- NumberField
- SearchField
- Select
- Slider
- RangeSlider
- Checkbox
- Toggle
- Radio
- ColorField

## Selection controls

- Tabs
- Tab
- SegmentedControl
- CategoryChip
- StatusChip

## Asset components

- AssetCard
- AssetThumbnail
- CharacterCard
- PresetCard
- AnimationCard
- WeaponCard
- MaterialCard

## Data / technical components

- PropertyRow
- MetricRow
- ValidationRow
- MappingRow
- SkeletonTreeRow
- SocketRow

## Animation components

- Timeline
- TimelineClip
- ComboNode
- StateNode
- BlendSpacePoint
- AnimationCoverageRow

## System components

- Alert
- ProgressBar
- Tooltip
- ModalShell
- ConfirmDialog
- EmptyState
- LoadingState
- FocusRing
- GamepadHint
- ControllerFocusCard

---

# 13. Global Application Shell

Create one consistent application shell.

It should provide:

- project/character context
- global navigation
- workspace navigation
- Save
- Load
- Import
- Export
- Undo
- Redo
- Help
- Settings
- status indicators

Major navigation categories:

- Create
- Body
- Face
- Hair
- Clothing
- Armor
- Accessories
- Weapons
- Materials
- Animation
- Retargeting
- Preview
- Validation
- Import
- Export
- Settings

Avoid putting every category in one overloaded row if hierarchy becomes unclear.

---

# 14. Canonical Screen Inventory

The complete application should target 30+ major workspaces plus system dialogs and states.

The required canonical screen inventory follows.

---

# 15. Project / Character Browser

Purpose:

Manage existing characters and projects.

Required features:

- New Character
- Load
- Save
- Duplicate
- Import
- Export
- recent characters
- favorites
- templates
- tags
- categories
- search
- sort
- selected-character details
- version
- Unreal target
- skeleton compatibility
- export status
- quick actions

---

# 16. Main Dashboard

Purpose:

Act as the main hub.

Provide shortcuts to:

- Continue Editing
- Animation Workspace
- Preview Studio
- Validation
- Export
- Import Assets
- Recent Characters

---

# 17. Create / Base

Support:

- template selection
- body archetype
- sex/gender options where applicable
- species/race if supported
- age
- height
- scale
- proportions
- starting pose
- randomize
- confirm character

---

# 18. Body Sculpting

Support:

- head
- neck
- torso
- chest
- back
- shoulders
- waist
- arms
- forearms
- hands
- hips
- thighs
- calves
- feet
- muscularity
- body fat
- height
- posture
- age
- asymmetry

Provide:

- region selection
- quick presets
- reset region
- randomize region
- compare

---

# 19. Face Sculpting

Support:

- forehead
- brow
- eyes
- nose
- cheeks
- mouth
- jaw
- chin
- ears
- neck
- asymmetry

Prefer direct visual editing.

Support:

- click/drag sculpt handles
- sliders
- presets
- symmetry toggle

---

# 20. Face Advanced

Support:

- detailed morph sliders
- bone-based adjustments
- corrective morph concepts
- expressions
- regional controls
- face comparison
- advanced sculpt mode
- morph presets

---

# 21. Hair / Grooming / Skin

Support:

## Hair

- hairstyle
- hairline
- length/style variants
- color
- highlights

## Grooming

- facial hair
- eyebrows
- eyelashes

## Skin

- skin tone
- complexion
- pores
- roughness
- aging
- wrinkles
- freckles
- moles
- scars
- tattoos
- makeup
- body paint

## Physics preview

Allow hair physics preview.

---

# 22. Clothing / Armor / Accessories

Support:

## Clothing

- upper body
- lower body
- underlayers
- outer layers
- gloves
- boots
- cloaks

## Armor

- helmet
- shoulders
- chest
- arms
- hands
- waist
- legs
- feet

## Accessories

- belts
- jewelry
- pouches
- masks
- glasses
- back items
- decorative attachments

## Compatibility

Provide:

- clipping warnings
- incompatibility warnings
- body-region hiding
- layered slot conflicts
- outfit presets

---

# 23. Materials & Color Studio

Support:

- material zones
- base color
- dye regions
- roughness
- metallic
- normal intensity
- emissive
- patterns
- saved colors
- saved palettes
- per-item overrides
- multi-channel materials

---

# 24. Weapons

Support:

- primary weapon
- secondary weapon
- offhand
- dual wield
- back-mounted
- holstered
- sheathed
- custom weapon profiles

Weapon categories should include:

- sword
- greatsword
- axe
- spear
- bow
- staff
- rifle
- pistol
- dual wield
- custom

---

# 25. Weapon Setup & IK

This is a critical screen.

Support:

- weapon category
- equipped weapon
- grip type
- weapon scale
- weapon offset
- right-hand socket
- left-hand socket
- sheath socket
- holster socket
- right-hand target
- left-hand target
- foot IK
- stance
- aim offset
- root motion
- motion warping
- combat profile
- attack chain
- compatibility validation

## Automation goal

When a user adds a new weapon, the product should eventually be able to assist with:

- hand positioning
- IK
- socket mapping
- stance selection
- locomotion set
- idle pose
- attack animation set
- sheathing/holstering
- weapon-specific pose corrections

---

# 26. Animation Workspace Overview

Provide categories for:

- locomotion
- combat
- emotes
- equip/utility
- hit reactions
- death
- weapon profiles

Display:

- assigned animations
- missing slots
- animation coverage
- validation status
- preview

---

# 27. Locomotion Setup

Support:

- Idle
- Walk
- Run
- Sprint
- Crouch
- Strafe Left
- Strafe Right
- Jump
- Fall
- Land
- Turn in Place

Allow:

- directional assignment
- speed values
- preview
- looping
- automatic setup

---

# 28. Blend Space Assistant

Hide raw Unreal complexity by default.

Provide:

- visual 2D directional map
- speed axis
- direction axis
- sample points
- assigned animations
- preview marker
- interpolation
- auto-generate
- validation

---

# 29. Animation Blueprint Assistant

Provide a simplified high-level architecture.

Example:

```text
Entry
  |
  v
Locomotion
  |
  +--> Airborne
  |
  +--> Combat
  |
  +--> Weapon Equipped
  |
  +--> Hit Reaction
  |
  +--> Death
```

Allow:

- auto-build
- state validation
- transition validation
- layer overview
- variable overview

Keep raw AnimGraph complexity in advanced mode only.

---

# 30. Montage / Combo Builder

Provide a visual timeline.

Support:

- attack clips
- combo sections
- branches
- hit windows
- cancel windows
- finishers
- montage slots
- animation notifies
- weapon trails
- SFX
- VFX
- gameplay events

Allow visual combo construction.

---

# 31. Weapon Animation Profiles

Each weapon type may have its own:

- idle
- walk
- run
- sprint
- strafe
- block
- parry
- light attack
- heavy attack
- combo
- finisher
- hit reaction
- dodge

Profiles should be reusable.

---

# 32. Animation Set Builder

Provide semantic slots.

Example:

```text
Idle
Walk Forward
Walk Back
Run Forward
Run Back
Strafe Left
Strafe Right
Jump
Fall
Land
Attack Light 1
Attack Light 2
Attack Heavy
Block
Parry
Dodge
Death
```

Support:

- drag/drop
- auto-detect
- missing-slot warnings
- duplicate detection
- preview

---

# 33. Retargeting Assistant

Support:

- source skeleton
- target skeleton
- chain mapping
- automatic bone mapping
- pose correction
- mapping quality
- missing bones
- before/after preview
- validation

---

# 34. Skeleton / Rig / Socket Inspector

Advanced workspace.

Support:

- bone hierarchy
- selected bone
- sockets
- virtual bones
- IK chains
- IK goals
- attachment points
- position
- rotation
- scale
- viewport visualization

---

# 35. Physics Setup

Support:

- cloth
- hair
- capes
- accessories
- collision
- ragdoll

Parameters:

- quality
- stiffness
- damping
- wind
- gravity
- collision

Provide simulation preview.

---

# 36. Gameplay Test Workspace

This must test actual character behavior.

Support:

- WASD
- mouse
- gamepad
- movement
- sprint
- jump
- crouch
- attack
- dodge
- block
- parry
- weapon switching
- interaction
- animation transitions
- camera

Provide optional debug overlays.

---

# 37. Preview Studio

Support:

- environment presets
- HDRI
- lighting
- light rotation
- intensity
- exposure
- camera
- FOV
- floor
- background
- pose
- animation

---

# 38. Photo / Portrait Studio

Support:

- portrait
- bust
- full-body
- thumbnail
- transparent background
- backdrop
- focal length
- depth of field
- aperture
- camera presets
- output resolution
- render/export

---

# 39. LOD / Performance

Display:

- LOD level
- triangle count
- vertex count
- bone count
- materials
- draw calls
- texture memory
- estimated character memory

Support:

- Generate LODs
- Optimize
- target platform
- performance budget
- warnings

---

# 40. Import Wizard

Use a multi-step wizard.

Steps:

1. Type
2. Source
3. Options
4. Mapping
5. Validation
6. Finish

Support importing:

- character mesh
- clothing
- armor
- weapon
- animation
- textures
- materials
- skeleton
- morph targets

---

# 41. Asset Browser

Support:

- search
- type filter
- category filter
- tags
- favorites
- recent
- compatibility
- thumbnails
- metadata
- drag/drop
- details inspector

Asset types:

- characters
- clothing
- armor
- accessories
- weapons
- animations
- materials
- textures
- presets

---

# 42. Character Preset Manager

Support:

- full character preset
- body-only preset
- face-only preset
- outfit-only preset
- material preset
- animation preset where useful

Actions:

- save
- load
- duplicate
- rename
- compare
- merge
- delete
- version

---

# 43. Randomization Rules

Allow category locks for:

- body
- face
- hair
- outfit
- armor
- accessories
- weapons
- materials

Allow constrained ranges for:

- age
- height
- muscularity
- body fat
- proportions

Include:

- seed
- generate
- regenerate
- apply

---

# 44. Settings

Groups:

- General
- Project
- Unreal Integration
- Input
- Navigation
- Controller
- UI
- Accessibility
- Performance
- Import / Export
- Autosave
- Backup

Key settings:

- Unreal version
- project path
- default asset paths
- autosave
- backup interval
- UI scale
- language
- asset naming
- performance target

---

# 45. Validation Dashboard

Validation categories:

- Character Health
- Missing Assets
- Clipping
- Mesh Penetration
- Skeleton Compatibility
- Animation Coverage
- Retargeting
- IK
- Socket Mapping
- Materials
- Textures
- LODs
- Performance
- Export Blockers

Severity:

- Error
- Warning
- Info
- Passed

Each issue should show:

- what is wrong
- affected subsystem
- why it matters
- suggested action
- one-click fix if feasible

---

# 46. Export Workspace

Deliverables:

- Character Preset
- Spawnable Character Blueprint
- Skeletal Mesh Setup
- Animation Blueprint
- Gameplay Package
- JSON / Data Asset
- Portrait
- Thumbnail

Options:

- Unreal target
- destination path
- naming convention
- dependencies
- include source assets
- include thumbnails
- include icons
- generate documentation

Provide:

- validation gate
- progress
- success
- export history

---

# 47. Required Modals

Must include:

- New Character
- Asset Picker
- Color Picker
- Save As Template
- Export Options
- Confirm Delete
- Replace Asset
- Unsaved Changes
- Import Conflict
- Missing Dependency
- Unsupported Skeleton

---

# 48. Required System States

Must include:

- Loading
- Progress
- Error
- Warning
- Success
- Empty Search
- Empty Asset Category
- No Character Selected
- Export Success
- Import Success
- Validation Passed
- Missing Dependencies

---

# 49. Onboarding / Tutorial

Introduce:

- character creation
- navigation
- assets
- animation setup
- weapon setup
- preview
- validation
- export

Allow:

- Next
- Back
- Skip
- Do Not Show Again

---

# 50. Gamepad Navigation

Gamepad support must be designed explicitly.

Define:

- D-pad focus movement
- left-stick focus/navigation where appropriate
- bumper workspace switching
- trigger category switching where appropriate
- A / Cross = confirm
- B / Circle = cancel/back
- X / Square = context
- Y / Triangle = alternate action
- right stick = viewport camera
- controller focus ring
- contextual input hints

Do not rely on mouse hover.

---

# 51. Primary Prototype Flows

## Flow A — New Character

```text
Project Browser
→ New Character
→ Create/Base
→ Body
→ Face
→ Hair/Skin
→ Clothing
→ Materials
→ Save
```

## Flow B — Animation Setup

```text
Animation Workspace
→ Locomotion
→ Blend Space Assistant
→ Animation Blueprint Assistant
→ Weapon Profile
→ Gameplay Test
```

## Flow C — Weapon Setup

```text
Weapons
→ Weapon Setup & IK
→ Auto Socket Mapping
→ Grip / IK
→ Combat Profile
→ Attack Preview
→ Gameplay Test
```

## Flow D — Retarget & Validate

```text
Import
→ Retargeting
→ Animation Setup
→ Validation
→ Fix Issues
→ Pass
```

## Flow E — Final Delivery

```text
Preview Studio
→ Portrait Studio
→ Validation
→ Export Options
→ Export
→ Success
```

---

# 52. Responsive Desktop Targets

The product is desktop-first.

Test layouts at:

- compact desktop
- approximately 1672×941 reference size
- 1920×1080
- 2560×1440

Rules:

- viewport should grow first
- inspector widths should remain usable
- asset grids should reflow
- timelines must remain readable
- navigation must not overflow badly
- gamepad targets must remain accessible

---

# 53. Penpot Build Rules

Codex must not:

- paste screenshots and claim completion
- flatten native UI into images
- implement only representative screens
- omit animation tooling
- omit gamepad states
- omit system states
- omit modals
- use inconsistent one-off controls
- silently ignore reference conflicts
- mark screens complete based on MCP success alone

Codex must:

- use native Penpot structure
- build reusable components
- use design tokens
- use text as editable text
- use raster imagery only where appropriate
- verify every screen visually/structurally
- document deviations

---

# 54. Raster Image Policy

Raster imagery is acceptable for:

- character preview
- environment preview
- thumbnail imagery
- reference art

Raster imagery is not acceptable as a substitute for:

- buttons
- tabs
- panels
- fields
- sliders
- navigation
- labels
- dialogs
- validation tables

Name raster reference assets clearly, such as:

```text
REF_RASTER/Characters/...
REF_RASTER/Environments/...
REF_RASTER/Assets/...
```

---

# 55. Codex Documentation Deliverables

Create:

## `PENPOT_SCREEN_MATRIX.md`

For every reference:

- filename
- screen
- purpose
- major regions
- reusable components
- conflicts
- implementation status
- verification status

## `PENPOT_DESIGN_SYSTEM.md`

Document:

- tokens
- typography
- spacing
- component rules
- naming
- focus states
- controller behavior

## `PENPOT_COMPONENT_INVENTORY.md`

For every reusable component:

- name
- purpose
- variants
- states
- usage

## `PENPOT_USER_FLOWS.md`

Document all prototype flows.

## `PENPOT_UNREAL_HANDOFF.md`

Map UX features to eventual Unreal responsibilities.

Examples:

```text
Blend Space Assistant
→ Unreal Blend Space asset generation/configuration

Animation Blueprint Assistant
→ Animation Blueprint + state machine configuration

Weapon Setup & IK
→ sockets + IK goals + offsets + animation profile

Validation
→ readiness checks before export
```

## `PENPOT_COMPLETION_EVIDENCE.md`

For every screen:

- reference
- Penpot target
- implementation status
- verification method
- known deviations
- remaining work

---

# 56. Unreal-Facing Functional Architecture

The Penpot work defines UI/UX, not actual Unreal logic.

Later implementation should connect the UI to Unreal systems.

Major implementation domains include:

## Character data

- character preset data
- morph values
- equipment slots
- materials
- colors
- animation profiles
- weapon profiles

## Animation

- Animation Blueprint
- locomotion state machine
- Blend Spaces
- montages
- notifies
- root motion
- motion warping

## IK / rigging

- IK Rig
- IK Retargeter
- hand IK
- foot IK
- weapon IK
- sockets
- pose corrections

## Rendering

- preview scenes
- portrait capture
- thumbnail capture
- lighting presets

## Export

- Data Assets
- Blueprints
- skeletal setup
- animation assignments
- package metadata

---

# 57. Weapon Automation Requirements

The system must eventually support user-added weapon types.

When a new weapon is introduced, the creator should help configure:

- weapon socket
- left hand target
- right hand target
- grip
- stance
- sheath/holster
- locomotion profile
- idle
- attack animations
- block/parry
- weapon-specific pose
- offsets
- IK
- validation

This must be treated as a first-class system rather than hardcoded per weapon.

---

# 58. Animation Automation Requirements

The system should be capable of assisting with:

- semantic animation classification
- missing animation detection
- locomotion set generation
- Blend Space generation
- state machine generation
- montage generation
- montage slot configuration
- animation coverage
- transition validation
- weapon animation profiles
- retargeting

---

# 59. Validation Philosophy

Passing automated tests is not enough.

A feature is not complete merely because:

- a node exists
- an asset loads
- an automation returned success
- a test says PASS

The actual product behavior must be checked.

For UI/UX this means:

- layout exists
- controls are usable
- states are present
- text is readable
- no clipping exists
- references are represented
- gamepad focus works conceptually
- design-system components are actually reused

---

# 60. Phase-Based Execution Plan

## P00 — Environment & MCP Verification

**Current status: Complete.** The connected Penpot file, active page, page inventory, readback, temporary-object create/read/delete test, and local reference access are recorded in `docs/penpot/P00_PENPOT_MCP_ACCEPTANCE.md`.

Tasks:

- verify Penpot MCP
- verify Codex MCP configuration
- verify current Penpot file
- verify read
- verify write
- verify delete
- verify local asset access

Exit criteria:

- real Penpot document can be read
- temporary object can be created and deleted

---

## P01 — Reference Audit

**Current status: Partial.** The expanded reference directory contains the expected 39 individual screens and 2 contact sheets. The ZIP, screen matrix, duplicate/conflict report, missing-state report, and per-screen audit evidence are absent.

Tasks:

- locate the expanded reference directory
- compare against any supplied ZIP, if one becomes available
- inventory files
- inspect every reference
- create screen matrix
- map duplicates
- map conflicts
- identify missing states

Exit criteria:

- all 39 individual references are catalogued

---

## P02 — Product Architecture

**Current status: Partial.** The runtime has 21 declared/routed screens and shared navigation in `CharacterCreatorRootWidget.cpp`, but no Penpot project structure, sitemap artifact, or documented canonical screen/flow mapping exists.

Tasks:

- create Penpot project structure
- create sitemap
- create navigation architecture
- define canonical screen inventory
- define major flows

Exit criteria:

- no major screen is unaccounted for

---

## P03 — Foundations

**Current status: Partial.** `CharacterCreatorUIFramework.*` provides a reusable dark palette, panel/button styles, labels, sliders, focus helpers, and popup clamping. Penpot tokens, typography decisions, spacing/radius/elevation documentation, and visual verification are not present.

Tasks:

- colors
- typography
- spacing
- radius
- border
- elevation
- layout
- focus
- validation states

Exit criteria:

- screens can be built without arbitrary styling

---

## P04 — Components

**Current status: Partial.** Reusable C++ UMG primitives exist for panels, buttons, command buttons, sliders, tabs, modal shells, and modal focus management. The complete component inventory and required asset/data/animation/state components are not implemented or evidenced.

Tasks:

- navigation
- buttons
- fields
- tabs
- panels
- cards
- sliders
- modals
- status
- validation
- asset cards
- animation components
- controller focus

Exit criteria:

- repeated UI uses reusable native components

---

## P05 — Global Shell

**Current status: Partial.** The runtime shell has a shared 1440×810 canvas, top bar, rail, preview region, inspector region, save/revert actions, status messaging, and screen routing. Project/character browser behavior, complete global actions, responsive layout proof, and Penpot shell fidelity remain incomplete.

Tasks:

- application frame
- global nav
- workspace nav
- utility actions
- project context
- viewport shell
- side panels
- bottom browser tray

Exit criteria:

- shared shell supports all major workspaces

---

## P06 — Character Creation

**Current status: Partial.** Dashboard, body/face sliders, live preview, outfit/hair/material/weapons routing, and Sidekick loadout preview exist. Advanced face controls, full clothing/armor/accessories coverage, grooming/skin authoring, browser/picker behavior, and reference-faithful screen reconstruction are not complete.

Tasks:

- Project Browser
- Main Dashboard
- Create/Base
- Body
- Face
- Face Advanced
- Hair/Skin
- Clothing/Armor/Accessories

Exit criteria:

- full cosmetic character workflow exists

---

## P07 — Materials & Equipment

**Current status: Partial.** Outfit and hair asset selection, four-color palette presets, a weapon slot, and right-hand/feet IK state toggles are wired to the session. A real weapon library, socket mapping, grip/offset authoring, IK solving, dye/material parameter editing, and user-added weapon automation are not implemented.

Tasks:

- Materials
- Color Studio
- Weapons
- Weapon Setup
- IK
- Asset Browser

Exit criteria:

- character can be visually equipped and configured

---

## P08 — Animation

**Current status: Partial / early foundation.** Seven animation workspaces are routed, 95 FAB packages are present, Manny sources can be selected, and retargeter/state fields persist. Blend Space editing, Animation Blueprint generation, montage/combo authoring, animation-set/weapon-profile generation, retarget execution, and Sidekick target playback are not implemented.

Tasks:

- Animation Overview
- Locomotion
- Blend Space
- Animation Blueprint Assistant
- Montage/Combo
- Animation Set Builder
- Weapon Profiles

Exit criteria:

- integrated animation workflow exists

---

## P09 — Technical Tools

**Current status: Scaffold.** Skeleton/rig/socket, physics, and LOD/performance workspaces expose routes and status/action buttons. Bone/socket/IK inspection, physics simulation, collision editing, LOD metrics, memory analysis, and optimization are absent.

Tasks:

- Retargeting
- Skeleton/Rig/Socket
- Physics
- LOD/Performance

Exit criteria:

- advanced character technical setup is represented

---

## P10 — Preview & Testing

**Current status: Partial.** The preview actor provides a live render target, async loading/fallback states, Sidekick loadout components, and front/three-quarter/side/portrait camera modes. Gameplay test, portrait capture/render, lighting/environment controls, and actual movement/combat testing are not implemented.

Tasks:

- Gameplay Test
- Preview Studio
- Portrait Studio
- controller hints

Exit criteria:

- user can test presentation and gameplay behavior

---

## P11 — Import & Asset Management

**Current status: Partial.** The import service validates `.uasset`/`.umap` packages and the FAB directory passes validation; the asset browser/import wizard routes exist. There is no source selection/copy/import pipeline, asset metadata browser, filter/search/grid, compatibility analysis, or conflict resolution.

Tasks:

- Import Wizard
- Asset Browser
- dependency warnings
- compatibility states

Exit criteria:

- external assets have a clear onboarding workflow

---

## P12 — Presets & Randomization

**Current status: Partial.** Versioned preset state supports create, duplicate, rename, delete, restore-default, save/load, and onboarding persistence. Randomization seeds, category locks, constrained ranges, compare/merge, and the preset-manager UI are not implemented.

Tasks:

- Preset Manager
- Randomization Rules
- seeds
- category locking

Exit criteria:

- preset reuse and controlled random generation exist

---

## P13 — Settings

**Current status: Scaffold.** A settings workspace exposes UI-scale, gamepad, and onboarding commands that currently update status/reset onboarding. No settings model, persistence, Unreal integration path, accessibility, backup, or performance configuration exists.

Tasks:

- General
- Project
- Unreal
- Input
- Controller
- UI
- Accessibility
- Performance
- Import/Export
- Backup

Exit criteria:

- application preferences are complete

---

## P14 — Validation & Export

**Current status: Partial.** Validation covers core mesh/material/loadout references and animation-preview warnings; export writes a validated JSON manifest under `Saved/CharacterCreator/Exports`. There is no validation dashboard, issue list/one-click fix system, export history, Blueprint/Data Asset/package generation, or complete Unreal-ready delivery.

Tasks:

- health dashboard
- warnings
- errors
- one-click fixes
- deliverable selection
- export progress
- export history

Exit criteria:

- complete delivery workflow exists

---

## P15 — Modals & States

**Current status: Partial.** A modal manager with focus restoration and concrete new-character, import, export, onboarding, save-template, and load-error dialogs exists. The full required modal/state inventory—color picker, replace/conflict/dependency/skeleton dialogs, empty/success/warning/validation/export states—is not complete.

Tasks:

- all supplied modals
- unsaved changes
- confirm delete
- replace asset
- dependency errors
- success states
- empty states

Exit criteria:

- normal failure and edge cases are designed

---

## P16 — Gamepad UX

**Current status: Partial / early foundation.** UI-only input, keyboard focus, modal focus stack, Escape, shoulder-button workspace cycling, and apply/revert face-button shortcuts exist. A complete focus graph, D-pad/analog behavior, controller overlay/hints, viewport camera controls, and per-control verification are absent.

Tasks:

- focus rules
- focus visuals
- input hints
- bumper switching
- confirm/cancel
- viewport camera
- modal navigation

Exit criteria:

- every major workflow is gamepad navigable

---

## P17 — Prototype Flows

**Current status: Partial.** New-character, onboarding, import-validation, save-template, navigation, save/revert, and export-dialog routes exist. The complete new-character, animation, weapon, retarget/validate, final-delivery, and gamepad flows are not end-to-end functional.

Tasks:

- New Character
- Animation Setup
- Weapon Setup
- Retarget
- Validation
- Export
- Gamepad

Exit criteria:

- major workflows can be navigated in prototype form

---

## P18 — Visual QA

**Current status: Not started.** No Penpot comparison pass, per-screen visual/structural review, responsive-size review, or screenshot evidence is present. Passing automation tests does not satisfy this phase.

Tasks:

- compare every screen to source
- inspect layout
- inspect hierarchy
- inspect controls
- inspect text
- inspect component reuse
- inspect focus
- inspect clipping
- inspect raster misuse

Exit criteria:

- all discrepancies are resolved or documented

---

## P19 — Completion Evidence

**Current status: Not started.** Only this master plan exists under `docs/`; the requested screen matrix, design-system, component inventory, user-flow, Unreal handoff, and completion-evidence documents have not been created.

Tasks:

- update all documentation
- mark every screen
- record known deviations
- record unresolved issues
- confirm no fake completion

Exit criteria:

- completion can be independently verified

---

# 61. Thread / Handoff Discipline for Codex

For long Codex work:

- one coherent task per thread
- do not continue indefinitely in one thread
- update handoff before closing
- record next authorized task
- new thread reads current handoff first

Each handoff should include:

- completed task
- evidence
- Penpot pages changed
- files changed
- unresolved issues
- next authorized task

No new thread should infer progress from prose alone.

---

# 62. Screen Completion Checklist

A screen is complete only when all applicable items pass:

- native Penpot frame exists
- correct workspace name
- major layout matches reference
- native text
- native controls
- design tokens
- component reuse
- selected state
- hover state where relevant
- focus state
- disabled state where relevant
- warning state where relevant
- gamepad consideration
- no unexpected clipping
- no screenshot-backed full UI
- relevant prototype links
- visual/structural verification

---

# 63. Overall Acceptance Criteria

**Current status: Not achieved.** The repository has 39/39 individual reference images, 21/21 declared runtime screens routable, 4/4 current automation tests passing, and a verified UE 5.7 editor build. It has 0 verified Penpot MCP acceptance runs, 0 editable Penpot pages evidenced in this repository, 0 of the 6 requested Penpot handoff/evidence documents, and no complete Unreal authoring/export pipeline. The criteria below remain the definition of done, not a claim that the current slice satisfies them.

The project design phase is not complete until:

- all 39 references have been audited
- all canonical workspaces exist
- all original high-detail concepts are represented
- all additional references are represented or merged into documented canonical screens
- design tokens exist
- reusable components exist
- mouse/keyboard UX exists
- gamepad UX exists
- body workflow exists
- face workflow exists
- hair/skin workflow exists
- clothing/armor workflow exists
- materials workflow exists
- weapons workflow exists
- IK workflow exists
- animation workflow exists
- Blend Space assistant exists
- Animation Blueprint assistant exists
- montage/combo builder exists
- retargeting exists
- skeleton inspector exists
- physics exists
- gameplay test exists
- preview studio exists
- portrait studio exists
- LOD/performance exists
- import wizard exists
- asset browser exists
- presets exist
- randomization exists
- settings exist
- validation exists
- export exists
- required modals exist
- loading/error/success states exist
- onboarding exists
- prototype flows exist
- completion evidence exists

---

# 64. Current Next Codex Task

The original “start with P00” recommendation is still valid as a prerequisite, but it is no longer an accurate description of repository progress. The next work should close the design-source gap and then turn the routed engineering foundations into verified product behavior.

## Immediate sequence

1. **P01 — Reference Audit:** use the present directory `UE_5_7_1_Character_Creator_ALL_UI_UX_Screens\`, record all 39 individual references, map the 2 contact sheets as supplementary, and create the screen matrix, conflict report, and missing-state report. There is no ZIP path to record in the current repository.
2. **P02–P04 — Product Architecture, Foundations, Components:** reconcile the existing C++ runtime shell with the Penpot source of truth and create the missing design-system/component/flow documentation.
3. **P08 — Animation functional slice:** implement one complete source-to-Sidekick retarget path, then use that path to validate locomotion, Blend Space, Animation Blueprint, and preview behavior before expanding the authoring surface.
4. **P14 — Validation and export:** extend the current JSON manifest foundation into real Unreal deliverables only after the animation and asset contracts are real.

## Required next handoff

- Penpot MCP status and editable file
- 39-reference screen matrix
- canonical screen inventory mapped to current runtime routes
- duplicate/conflict report
- missing-state report
- current C++/asset/test evidence
- explicit list of routed-only screens and their next functional milestones

Do not claim the project is complete because all workspace routes exist or because the automation suite passes. A route is a foundation until its domain behavior, visual fidelity, input states, and evidence are verified.

---

# 65. Final Product Definition

The finished character creator should let a user go from:

```text
New Character
```

to:

```text
Fully customized character
+ clothing
+ armor
+ accessories
+ weapons
+ materials
+ animation setup
+ locomotion
+ combat
+ IK
+ retargeting
+ preview
+ validation
+ Unreal-ready export
```

without requiring the user to manually understand every low-level Unreal animation or rigging system.

That is the core standard by which the product should be judged.

---

# 66. Final Guiding Rule

**Do not optimize for producing screens quickly. Optimize for producing a complete, coherent, reusable, testable character-creation system that Codex can later implement accurately in Unreal Engine 5.7.1.**
