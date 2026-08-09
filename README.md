# 3D Character Creator

Unreal Engine 5.7 character-creator prototype with a dark editor-style UI system based on the Penpot concept screens in `UE_5_7_1_Character_Creator_ALL_UI_UX_Screens`.

## Current vertical slice

- Main Dashboard with live preview, recent projects, quick actions, and status messaging.
- Character Creator body/face workspace with preview, proportion controls, save state, and back navigation.
- Event-driven `UCharacterCreatorSession` for screen and status changes.
- Explicit UI-only input mode, keyboard focus ownership, and teardown cleanup.
- Reusable C++ UMG styling helpers for panels, labels, buttons, and progress bars.

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
