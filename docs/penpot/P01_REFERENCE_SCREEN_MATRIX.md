# P01 — 39-Screen Reference Matrix

**Status:** Complete
**Audit date:** 2026-08-09
**Source directory:** `UE_5_7_1_Character_Creator_ALL_UI_UX_Screens/`

## Audit scope and evidence

- 39 individual references were found: 7 original high-detail concepts and 32 additional screens.
- 2 contact sheets were found and treated as supplementary only: `00_master_32_screen_contact_sheet.png` and `00_alternate_20_screen_contact_sheet.png`.
- No source ZIP is present in the repository.
- Image metadata was read for all 41 PNG files. The 7 original concepts are 1536×1024 or 1672×941; the additional screens are approximately 16:9 at 921×510/513, with modal references at 657/660×513.
- The Penpot file contains 39 native boards on `04 Screen Boards`, one per matrix row. The boards are 640×360 and the structural scan found child shapes and native text on every board; no image fill was used inside those screen boards.
- Runtime mapping is based on `ECharacterCreatorScreen` in `Source/threedcharacter/UI/CharacterCreatorSession.h`, the root screen switcher, modal handlers, and the session/export/import services.

## Matrix

| ID | Source tier | Reference file | Canonical Penpot board | Runtime mapping | Audit disposition |
|---:|---|---|---|---|---|
| 01 | Original | `01_Original_Concept_Screens/01_initial_character_creator_preview.png` | `Screen Board / 01 / Initial Character Creator` | `Dashboard` + `CharacterCreator` preview shell | Canonical high-detail shell; split into linked dashboard and creator entry states |
| 02 | Original | `01_Original_Concept_Screens/02_character_creator_animation_workspace.png` | `Screen Board / 02 / Animation Workspace` | `AnimationOverview` and animation workspace routes | Canonical high-detail animation composition; later screens decompose its tools |
| 03 | Original | `01_Original_Concept_Screens/03_ui_ux_sitemap_navigation_architecture.png` | `Screen Board / 03 / Sitemap & Navigation` | Product-map artifact; no single runtime route | Canonical information architecture and flow reference |
| 04 | Original | `01_Original_Concept_Screens/04_project_character_browser.png` | `Screen Board / 04 / Project Character Browser` | No dedicated route; nearest current entry is `Dashboard` | Canonical browser requirement; implementation gap recorded |
| 05 | Original | `01_Original_Concept_Screens/05_body_face_advanced_creator.png` | `Screen Board / 05 / Body & Face Creator` | `CharacterCreator` | Canonical advanced sculpting reference; current route is foundation-level |
| 06 | Original | `01_Original_Concept_Screens/06_weapon_setup_and_ik.png` | `Screen Board / 06 / Weapon Setup & IK` | `WeaponsAndIK` | Canonical weapon/IK reference; current route is scaffold-level |
| 07 | Original | `01_Original_Concept_Screens/07_validation_and_export.png` | `Screen Board / 07 / Validation & Export` | `ValidateCurrentAppearance` + `ExportCurrentManifest`; no dedicated screen route | Canonical validation/export reference; UI gap recorded |
| 08 | Additional | `02_Additional_Screens/01_main_dashboard.png` | `Screen Board / 08 / Main Dashboard` | `Dashboard` | Canonical dashboard task screen; current route exists |
| 09 | Additional | `02_Additional_Screens/02_body_sculpting.png` | `Screen Board / 09 / Body Sculpting` | `CharacterCreator` | Task-specific body view mapped into the shared creator route |
| 10 | Additional | `02_Additional_Screens/03_face_sculpting.png` | `Screen Board / 10 / Face Sculpting` | `CharacterCreator` | Task-specific face view; advanced face behavior remains a gap |
| 11 | Additional | `02_Additional_Screens/04_clothing_armor_accessories.png` | `Screen Board / 11 / Clothing / Armor` | `OutfitAndArmor` | Canonical cosmetic equipment view; route exists |
| 12 | Additional | `02_Additional_Screens/05_hair_grooming_skin.png` | `Screen Board / 12 / Hair / Grooming` | `HairAndGrooming` | Canonical grooming view; route exists |
| 13 | Additional | `02_Additional_Screens/06_materials_color_studio.png` | `Screen Board / 13 / Materials & Color` | `MaterialsAndColor` | Canonical materials view; route exists |
| 14 | Additional | `02_Additional_Screens/07_animation_workspace_overview.png` | `Screen Board / 14 / Animation Overview` | `AnimationOverview` | Canonical animation entry view; route exists |
| 15 | Additional | `02_Additional_Screens/08_locomotion_setup.png` | `Screen Board / 15 / Locomotion Setup` | `LocomotionSetup` | Canonical locomotion view; routed foundation |
| 16 | Additional | `02_Additional_Screens/09_blend_space_assistant.png` | `Screen Board / 16 / Blend Space Assistant` | `BlendSpaceAssistant` | Canonical assistant view; routed foundation |
| 17 | Additional | `02_Additional_Screens/10_animation_blueprint_assistant.png` | `Screen Board / 17 / Animation Blueprint` | `AnimationBlueprintWorkspace` | Canonical assistant view; routed foundation |
| 18 | Additional | `02_Additional_Screens/11_montage_combo_builder.png` | `Screen Board / 18 / Montage / Combo Builder` | `MontageComboBuilder` | Canonical authoring view; routed foundation |
| 19 | Additional | `02_Additional_Screens/12_retargeting_assistant.png` | `Screen Board / 19 / Retargeting Assistant` | `RetargetingAssistant` | Canonical retargeting view; routed foundation |
| 20 | Additional | `02_Additional_Screens/13_skeleton_rig_socket_inspector.png` | `Screen Board / 20 / Skeleton / Rig Inspector` | `SkeletonRigSocketInspector` | Canonical technical view; routed foundation |
| 21 | Additional | `02_Additional_Screens/14_physics_setup.png` | `Screen Board / 21 / Physics Setup` | `PhysicsSetup` | Canonical physics view; routed foundation |
| 22 | Additional | `02_Additional_Screens/15_gameplay_test_workspace.png` | `Screen Board / 22 / Gameplay Test Workspace` | `GameplayTest` | Canonical test view; routed foundation |
| 23 | Additional | `02_Additional_Screens/16_preview_studio.png` | `Screen Board / 23 / Preview Studio` | `PreviewStudio` | Canonical presentation view; routed foundation |
| 24 | Additional | `02_Additional_Screens/17_photo_portrait_studio.png` | `Screen Board / 24 / Photo / Portrait Studio` | `PortraitStudio` | Canonical capture view; routed foundation |
| 25 | Additional | `02_Additional_Screens/18_lod_performance.png` | `Screen Board / 25 / LOD / Performance` | `LODPerformance` | Canonical optimization view; routed foundation |
| 26 | Additional | `02_Additional_Screens/19_import_wizard.png` | `Screen Board / 26 / Import Wizard` | `ImportWizard` + `ValidateImportDirectory` | Canonical import flow; validation service exists, import UI is foundation |
| 27 | Additional | `02_Additional_Screens/20_asset_browser_expanded.png` | `Screen Board / 27 / Asset Browser Expanded` | `AssetBrowser` | Canonical asset browser view; routed foundation |
| 28 | Additional | `02_Additional_Screens/21_character_preset_manager.png` | `Screen Board / 28 / Character Preset Manager` | Preset APIs on session/subsystem; no dedicated route | Canonical preset-management requirement; UI gap recorded |
| 29 | Additional | `02_Additional_Screens/22_randomization_rules.png` | `Screen Board / 29 / Randomization Rules` | No dedicated route; no complete randomization workflow | Canonical randomization requirement; implementation gap recorded |
| 30 | Additional | `02_Additional_Screens/23_settings.png` | `Screen Board / 30 / Settings` | `Settings` | Canonical settings view; routed foundation |
| 31 | Additional | `02_Additional_Screens/24_new_character_modal.png` | `Screen Board / 31 / New Character Modal` | `new_character` modal | Canonical modal; current modal shell is concrete but simplified |
| 32 | Additional | `02_Additional_Screens/25_asset_picker_modal.png` | `Screen Board / 32 / Asset Picker Modal` | `asset_picker` modal | Canonical modal; current modal shell is concrete but simplified |
| 33 | Additional | `02_Additional_Screens/26_color_picker_modal.png` | `Screen Board / 33 / Color Picker Modal` | `color_picker` modal with RGB sliders and target actions | Functional coverage is complete; exact Penpot composition remains simplified |
| 34 | Additional | `02_Additional_Screens/27_save_as_template_modal.png` | `Screen Board / 34 / Save as Template Modal` | `save_template` modal | Canonical modal; current handler is concrete but simplified |
| 35 | Additional | `02_Additional_Screens/28_export_options_modal.png` | `Screen Board / 35 / Export Options Modal` | `export_options` modal | Canonical modal; current manifest export is wired |
| 36 | Additional | `02_Additional_Screens/29_loading_progress.png` | `Screen Board / 36 / Loading / Progress` | Preview `Loading` state and async asset load | Canonical state; current loading message is not a full progress workspace |
| 37 | Additional | `02_Additional_Screens/30_error_dialog.png` | `Screen Board / 37 / Error Dialog` | `load_error` modal and preview failure state | Canonical error state; current dialog is simplified |
| 38 | Additional | `02_Additional_Screens/31_gamepad_navigation_overlay.png` | `Screen Board / 38 / Gamepad Navigation` | UI-only input, focus helpers, shoulder navigation, modal focus stack | Canonical input reference; complete focus graph remains open |
| 39 | Additional | `02_Additional_Screens/32_onboarding_tutorial.png` | `Screen Board / 39 / Onboarding Tutorial` | `onboarding` modal and session onboarding state | Canonical onboarding state; current flow is simplified |

## Coverage summary

| Measure | Count | Interpretation |
|---|---:|---|
| Individual source references | 39 | Complete inventory |
| Original high-detail references | 7 | Highest source-of-truth priority |
| Additional individual references | 32 | Task and state expansion |
| Supplementary contact sheets | 2 | Not counted as screens |
| Penpot screen boards | 39 | One native board per individual reference |
| Unreal enum routes | 21 | Full route list is in the handoff and does not imply full domain behavior |
| Dedicated route gaps | 4 | Project browser, validation/export screen, preset manager, randomization rules |
| Modal/state gaps | 1+ | Color picker is functionally covered; additional states are listed in the missing-state report |
