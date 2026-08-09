# 3D Character Creator

Unreal Engine 5.7 character-creator prototype with a dark editor-style UI system based on the Penpot concept screens in `UE_5_7_1_Character_Creator_ALL_UI_UX_Screens`.

## Current vertical slice

- Main Dashboard with live preview, recent projects, quick actions, and status messaging.
- Character Creator body/face workspace with preview, proportion controls, save state, and back navigation.
- Authoritative `UCharacterCreatorSession` state for appearance parameters, presets, soft asset references, status, and screen routing.
- Game-instance-owned `UCharacterCreatorSubsystem` so the session is not tied to a HUD/widget lifetime.
- Event-driven appearance refresh with real slider controls and explicit widget teardown unbinding.
- Explicit UI-only input mode, keyboard focus ownership, and teardown cleanup.
- Reusable C++ UMG styling helpers for panels, labels, buttons, tabs, sliders, modals, focus, and viewport-safe popup placement.
- Real Sidekick preview actor with a live render target, camera, key/fill/rim lighting, soft asset references, asynchronous loading, and fallback/error states.
- Body and face controls resolve configured or token-matched Sidekick morph targets at runtime, use centered morph weights, and retain a scale fallback when a mesh has no matching target.
- Sidekick default mesh, material, and skeleton are the default asset references under `/Game/Synty/SidekickCharacters`.
- FAB's Free Animations Pack is imported under `/Game/FreeAnimationsPack` using the tutorial's extracted-Content workflow. The imported set includes the standalone animation samples plus the Manny/Quinn animation, skeleton, and retargeting assets.
- Core creator workflow screens for outfit/armor, hair/grooming, materials/color, and weapons/IK are routed through reusable UMG screen widgets with live preview, selection states, and session-owned Apply/Revert.
- Animation overview, locomotion, blend-space, animation blueprint, montage/combo, retargeting, and skeleton/rig/socket workspaces are routed separately and keep Manny source assets distinct from Sidekick target assets. The editor path performs real Manny-to-Sidekick IK retargeting and saves a target animation asset.
- Physics, gameplay test, preview/portrait studio, LOD/performance, asset browser, import wizard, and settings workspaces share the live preview and session contract.
- Modal focus-stack management, concrete new-character/import/loading-error/export/save-template/onboarding dialogs, explicit screen focus targets, Escape/shoulder-button navigation, and gamepad apply/revert shortcuts are part of the shared UI layer. Modal actions trap focus, and settings apply high contrast, text/UI scale, reduced motion, preview LOD, and frame-rate limits.
- Versioned save-game persistence, preset duplicate/rename/delete/default restoration, autosave/load APIs, export profiles, validation issues, and metadata manifest export are owned by the character-creator subsystem. Editor export additionally generates a real character Blueprint, a Primary Data Asset, and staged `.uasset` package files.

## Phase status

Phases 0–5 established the UI, preview, persistence, import/export, and routed workspace foundations. Phases 6–12 are complete as workflow contracts; P17 real-asset retarget/export is verified, and P18/P19 evidence is recorded. Visual parity and several dedicated production screens remain partial; see `docs/P18_VISUAL_QA.md` and `docs/P19_VERIFICATION_RECORD.md`.

The current session owns mutations; widgets only display state and forward user intent through session methods. The imported Free Animations Pack targets the UE Manny/Quinn skeletons, while the Sidekick preview uses `SKEL_Default_Sidekick`; the verified editor retarget path produces a compatible Sidekick target animation before it is recorded in session state. The preview actor loads configured assets asynchronously, applies preview LOD/performance settings, and falls back explicitly when an asset is unavailable.

## Run

1. Open `threedcharacter.uproject` with Unreal Engine 5.7.x.
2. Build the `threedcharacterEditor` target if prompted.
3. Play in Editor. The custom HUD opens in UI-only mode.

For a command-line build with the editor closed:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' `
  threedcharacterEditor Win64 Development `
  -Project="$PWD\threedcharacter.uproject" -WaitMutex
```

The Shipping game target also compiles with:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' `
  threedcharacter Win64 Shipping `
  -Project="$PWD\threedcharacter.uproject" -WaitMutex
```

Automation coverage is defined under `Source/threedcharacter/UI/CharacterCreatorAutomationTests.cpp` for session mutations, screen routing, export validation, imported FAB content, and the real P17 asset-to-delivery flow.

Run the current UE5.7 automation suite headlessly with the editor target built:

```powershell
$editor = 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$project = (Resolve-Path 'threedcharacter.uproject').Path
$args = '"' + $project + '" -unattended -nop4 -nosplash -nullrhi "-ExecCmds=Automation RunTests CharacterCreator; Quit" "-log=CharacterCreatorAutomation.log"'
Start-Process -FilePath $editor -ArgumentList $args -WorkingDirectory $PWD -Wait
```

The imported vendor Content remains local project content and is intentionally not folded into the C++ phase commits; the current tree contains 95 `FreeAnimationsPack` `.uasset` files and no maps or external actor folders from that pack.

Generated Unreal folders such as `Binaries`, `DerivedDataCache`, `Intermediate`, and `Saved` are intentionally ignored.
