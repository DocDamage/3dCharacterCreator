# P02 — Product Architecture and Flow Map

**Status:** Complete
**Penpot file:** `New File 1`

## Penpot structure

The file now has six purposeful pages:

| Page | Responsibility |
|---|---|
| `00 Design System` | Foundations, tokens, typography, components, and component usage proof |
| `01 Main Dashboard` | Dashboard reference and entry composition |
| `02 Screen Map` | 39-screen inventory and source-tier map |
| `03 Reference Sheets` | Supplementary reference-sheet placeholders and notes |
| `04 Screen Boards` | 39 native editable screen boards |
| `05 User Flows` | Major journey map with prototype links |

## Canonical information architecture

| Group | Canonical screens | Runtime ownership |
|---|---|---|
| Create / Base | 04 Project Browser, 08 Dashboard, 31 New Character, 39 Onboarding | Dashboard/root widget, modal manager, session onboarding |
| Customization | 05 Body & Face, 09 Body, 10 Face, 11 Clothing, 12 Hair, 13 Materials, 28 Presets, 29 Randomization | Character session, workflow widgets, future browser/preset UI |
| Animation Workspace | 02 Animation Workspace, 14 Overview, 15 Locomotion, 16 Blend Space, 17 Animation Blueprint, 18 Montage / Combo | Animation workspace widgets and session animation state |
| Technical Setup | 06 Weapon / IK, 19 Retargeting, 20 Skeleton / Rig, 21 Physics, 25 LOD / Performance | Workflow/utility widgets; domain operations remain foundation-level |
| Preview & Testing | 22 Gameplay Test, 23 Preview Studio, 24 Portrait Studio | Utility workspace widgets and preview actor |
| Validation & Export | 07 Validation & Export, 35 Export Options, 36 Loading, 37 Error | Export/import services, preview state, modal manager |
| Import / Asset Management | 26 Import Wizard, 27 Asset Browser, 32 Asset Picker | Import service, asset browser foundation, modal manager |
| Settings | 30 Settings, 33 Color Picker, 34 Save Template, 38 Gamepad Navigation | Settings utility route, modal shell, focus helpers |

The matrix in `P01_REFERENCE_SCREEN_MATRIX.md` remains the authoritative per-screen mapping. The grouping above resolves the source conflict between granular top tabs, sitemap workspaces, and the Create → Design → Animate → Test → Export pipeline.

## Major prototype flows

These flows are represented as rows on `05 User Flows` and use screen IDs from the matrix:

| Flow | Sequence |
|---|---|
| A — New Character | 04 Project Browser → 31 New Character → 09 Body / Face → 11 Clothing → 23 Preview → 35 Save / Export |
| B — Animation Setup | 14 Animation Overview → 15 Locomotion → 16 Blend Space → 17 Animation Blueprint → 22 Gameplay Test |
| C — Retarget & Validate | 26 Import → 19 Retargeting → 20 Skeleton / Rig → 07 Validation → 35 Export Options |
| D — Final Delivery | 23 Preview Studio → 24 Portrait Studio → 07 Validation → 35 Export → 08 Project Home |
| E — Gamepad Navigation | 39 Onboarding → 38 Focus Overlay → 08 Dashboard → 09 Body Sculpting → 23 Preview |

## Prototype evidence

- `05 User Flows` contains one native `Flow Map / Major User Journeys` board at 1500×1080.
- Five flow rows are present with native boards, labels, connectors, and node labels.
- 26 flow nodes have click → navigate-to prototype links to the corresponding screen board.
- 17 source-board shapes have prototype links: one open-overlay link for New Character and 16 navigate-to links through the core journeys.
- Penpot structural scans found no screenshot-backed whole-screen content in the 39 screen boards.

## Runtime reconciliation

The Unreal route enum currently exposes 21 routes. The following are intentionally mapped as design surfaces without pretending that their full behaviors exist yet:

- Project / Character Browser, Validation & Export screen, Preset Manager, and Randomization Rules need dedicated runtime surfaces.
- Color Picker is a Penpot reference and component requirement but has no current modal handler.
- Animation, technical, preview, and import routes exist as reusable foundation widgets; domain operations remain tracked in the master plan.
