# P20 — P0/P1 Production-Hardening Completion

**Record date:** 2026-08-09 (America/New_York)
**Engine:** Unreal Engine 5.7.4
**Target:** Windows 64-bit

## Disposition

The requested P0/P1 slice is implemented and verified. This record separates automated proof, packaged UI proof, and the one physical-device-assisted proof so that device coverage is not inferred from controller enumeration alone.

| Priority | Improvement | Delivered outcome | Acceptance evidence |
|---|---|---|---|
| P0 | Project/save lifecycle | Named projects; Save/Save As; rename, duplicate, and delete; Save/Discard/Cancel dirty-state confirmation; per-project autosave recovery; bounded retained backups | `CharacterCreator.ProjectLifecycle.E2E`; `CharacterCreator.UIAndSave.Contract` |
| P0 | Align UI promises with behavior | Portrait capture writes a real PNG; physics loads an asset; input testing is honestly labeled as a session check; LOD actions apply real preview settings; generation/export buttons either generate actual output or state their preparation role | `CharacterCreator.UIAndSave.Contract`; `CharacterCreator.Export.Contract`; packaged screenshots |
| P0 | Import/export hardening | Asset Registry dependency analysis, missing dependency reporting, per-file partial results, source/destination selection, conflict policy, traversal rejection, background progress/cancel, export history, atomic output generation, and open-output-folder action | `CharacterCreator.Import.PartialAndCancellation`; `CharacterCreator.Export.Contract` |
| P1 | Data-driven asset library | Mounted Asset Registry scan suitable for IoStore packages, 316 Sidekick entries, search/filter, class and compatibility badges, dependency counts, thumbnails where available, favorites, and independent selection | packaged screenshots `07_asset_browser.png` and `08_asset_favorite_rebuild.png` |
| P1 | Visual and device QA | Packaged Windows UI exercised at 1920x1200 with real mouse/keyboard input; physical USB DualSense detected by GameInput and used to change the live packaged UI | `Saved/PackagedQAArtifacts_Final24/report.json`; `Saved/PackagedQAArtifacts_PhysicalDualSense4/report.json` |
| P1 | End-to-end UI automation | Actual C++ UMG delegates exercise dashboard, project creation prompt, Save As, import result modal, and export validation flow in addition to session contracts | `CharacterCreator.UMG.ActualFlows` |

## Missing-asset incident and recovery

The missing Sidekick assets were a real packaging issue, not harmless log noise. The source tree contained 316 invalid dotted duplicate package names such as `Asset.Asset.uasset` beside 316 valid `Asset.uasset` counterparts. During cook, those invalid filenames caused an incorrect nested mount to be registered as `/Game`, so valid Sidekick dependencies resolved beneath the wrong package root and were omitted from the package.

Every dotted file was verified to have a valid counterpart. The 316 invalid duplicates (87,428,151 bytes) were moved, not deleted, to the recoverable location:

`Saved/AssetRecovery/SidekickDottedDuplicates_20260809_1605`

After recovery, a clean Development cook produced 939 packages, with zero Sidekick missing-package messages, zero duplicate mount warnings, and zero `LogCook` warnings/errors. The packaged preview now renders the default Sidekick and the mounted asset library enumerates 316 Sidekick assets.

## Automated test evidence

Headless automation log: `Saved/Logs/CharacterCreatorP0P1Final5.log`

Result: exit code 0, eight successes, zero failures:

- `CharacterCreator.Export.Contract`
- `CharacterCreator.Import.FreeAnimationsPack`
- `CharacterCreator.Import.PartialAndCancellation`
- `CharacterCreator.P17.RealAssetsE2E`
- `CharacterCreator.ProjectLifecycle.E2E`
- `CharacterCreator.Session.Foundation`
- `CharacterCreator.UIAndSave.Contract`
- `CharacterCreator.UMG.ActualFlows`

## Packaged UI evidence

Harness: `tests/packaged_ui_qa.py`
Artifacts: `Saved/PackagedQAArtifacts_Final24`
Result: `passed: true`

The harness launches the archived executable, resolves the real game process/window, uses Win32 mouse and keyboard input, captures the 1920x1200 client, and verifies image changes for:

1. Dashboard to named-project prompt.
2. Escape closing a focused text-entry modal.
3. Dashboard to project browser.
4. Gamepad-help overlay.
5. Keyboard return to dashboard.
6. Dashboard to the mounted asset browser.
7. Favorite toggle and live list rebuild.

The final runtime log contains no fatal error, unhandled exception, missing Sidekick package, invalid mount, or failed character-asset load. Unreal's normal probes for absent optional profiling DLLs are not runtime failures.

## Physical DualSense evidence

Harness: `tests/physical_gamepad_qa.py`
Artifacts: `Saved/PackagedQAArtifacts_PhysicalDualSense4`
Result: `physical_gamepad_ui_passed: true`

Windows enumerated `VID_054C&PID_0CE6` as a DualSense HID controller. GameInput assigned it to platform user 0/input device 1. A user-actuated R1 press was received as generic controller index 5, mapped by project configuration to `Gamepad_RightShoulder`, emitted press/release events, and changed the packaged UI by a measured 14.85% pixel delta.

The required mapping is stored in `Config/DefaultInput.ini`; `GameInputWindows` is enabled in `threedcharacter.uproject`. Face buttons, shoulder/trigger buttons, stick clicks, D-pad switch state, and analog stick/trigger axes are configured for the USB DualSense product ID.

## Build and package evidence

- Editor Development build: succeeded.
- Win64 Development BuildCookRun: succeeded, 939 packages, IoStore archive at `Saved/PackagedQA_Final/Windows`.
- Win64 Shipping compilation after all source and input changes: succeeded on 2026-08-09.

Development package command:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun `
  -project="$PWD\threedcharacter.uproject" -noP4 -platform=Win64 `
  -clientconfig=Development -build -cook -stage -pak -iostore -archive `
  -archivedirectory="$PWD\Saved\PackagedQA_Final" -utf8output
```

Shipping compile command:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' `
  threedcharacter Win64 Shipping -Project="$PWD\threedcharacter.uproject" `
  -WaitMutex -NoHotReloadFromIDE
```

## Scope note

This completion claim covers the requested behavior, resilience, packaged interaction, asset-library, and actual-UMG automation outcomes. It does not claim pixel-identical reproduction of every Penpot reference or exhaustive certification of every controller transport/product revision. The tested physical configuration is a wired Sony DualSense with product ID `0CE6`.
