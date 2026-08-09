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

## Phase status

Phase 0 (UI and data foundation) is implemented as the current base layer. The runtime module remains intentionally unified until later phases establish enough preview, editor tooling, and persistence boundaries to justify a split.

The current session owns mutations; widgets only display state and forward user intent through session methods. Phase 1 will connect `FCharacterAssetReferences` to a real preview actor and asynchronous asset loading.

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

Generated Unreal folders such as `Binaries`, `DerivedDataCache`, `Intermediate`, and `Saved` are intentionally ignored.
