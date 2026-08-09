#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#include "UI/CharacterCreatorExportService.h"
#include "UI/CharacterCreatorImportService.h"
#include "UI/CharacterCreatorSession.h"
#include "UI/CharacterCreatorSaveGame.h"
#include "UI/CharacterCreatorUIFramework.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCharacterCreatorSessionFoundationTest,
    "CharacterCreator.Session.Foundation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterCreatorSessionFoundationTest::RunTest(const FString& Parameters)
{
    UCharacterCreatorSession* Session = NewObject<UCharacterCreatorSession>();
    TestNotNull(TEXT("Session is constructible"), Session);
    if (!Session)
    {
        return false;
    }

    Session->InitializeDefaults();
    TestEqual(TEXT("Default screen is dashboard"), Session->GetScreen(), ECharacterCreatorScreen::Dashboard);
    TestFalse(TEXT("Defaults are clean"), Session->HasUnsavedChanges());
    TestFalse(TEXT("Default Sidekick mesh reference is assigned"), Session->GetAssetReferences().SkeletalMesh.IsNull());

    Session->SetParameterValue(ECharacterCreatorParameter::Height, 0.9f);
    TestTrue(TEXT("Editing marks the session dirty"), Session->HasUnsavedChanges());
    Session->ApplyAppearanceChanges();
    TestFalse(TEXT("Apply clears the dirty flag"), Session->HasUnsavedChanges());
    Session->SetParameterValue(ECharacterCreatorParameter::Height, 0.1f);
    Session->RevertAppearanceChanges();
    TestEqual(TEXT("Revert restores the applied value"), Session->GetParameterValue(ECharacterCreatorParameter::Height), 0.9f);

    Session->SetAdvancedFaceValue(ECharacterCreatorFaceAdvancedParameter::CheekWidth, 1.5f);
    TestEqual(TEXT("Advanced face values are constrained"), Session->GetAdvancedFaceValue(ECharacterCreatorFaceAdvancedParameter::CheekWidth), 1.0f);
    Session->SetGroomingValue(ECharacterCreatorGroomingParameter::HairLength, -1.0f);
    TestEqual(TEXT("Grooming values are constrained"), Session->GetGroomingValue(ECharacterCreatorGroomingParameter::HairLength), 0.0f);
    Session->SetHairStyle(FName(TEXT("ShortClean")));
    Session->SetClothingAsset(ECharacterCreatorClothingSlot::Legs, FSoftObjectPath(TEXT("/Game/Test/Legs.Legs")));
    TestEqual(TEXT("Clothing slot stores its selected asset"), Session->GetClothingAsset(ECharacterCreatorClothingSlot::Legs), FSoftObjectPath(TEXT("/Game/Test/Legs.Legs")));

    FCharacterCreatorMaterialSlotState MaterialState = Session->GetMaterialSlotState(FName(TEXT("PrimaryOutfit")));
    MaterialState.Metallic = 2.0f;
    MaterialState.Roughness = -1.0f;
    Session->SetMaterialSlotState(FName(TEXT("PrimaryOutfit")), MaterialState);
    const FCharacterCreatorMaterialSlotState SanitizedMaterial = Session->GetMaterialSlotState(FName(TEXT("PrimaryOutfit")));
    TestEqual(TEXT("Material metallic values are constrained"), SanitizedMaterial.Metallic, 1.0f);
    TestEqual(TEXT("Material roughness values are constrained"), SanitizedMaterial.Roughness, 0.0f);

    FCharacterCreatorWeaponSetup WeaponSetup;
    WeaponSetup.WeaponId = FName(TEXT("TestBlade"));
    WeaponSetup.DisplayName = FText::FromString(TEXT("Test Blade"));
    WeaponSetup.AssetPath = FSoftObjectPath(TEXT("/Game/Test/TestBlade.TestBlade"));
    WeaponSetup.GripScale = FVector::ZeroVector;
    Session->SetWeaponSetup(ECharacterCreatorWeaponSlot::MainHand, WeaponSetup);
    const FCharacterCreatorWeaponSetup StoredWeapon = Session->GetWeaponSetup(ECharacterCreatorWeaponSlot::MainHand);
    TestEqual(TEXT("Weapon socket defaults to a hand socket"), StoredWeapon.SocketName, FName(TEXT("hand_r")));
    TestTrue(TEXT("Weapon grip scale is sanitized"), StoredWeapon.GripScale.X > 0.0f && StoredWeapon.GripScale.Y > 0.0f && StoredWeapon.GripScale.Z > 0.0f);
    TestTrue(TEXT("Weapon library contains default entries"), Session->GetWeaponLibrary().Num() >= 2);

    FCharacterCreatorBlendSpaceState BlendSpace;
    BlendSpace.AssetPath = FSoftObjectPath(TEXT("/Game/Test/WalkRun.WalkRun"));
    BlendSpace.SpeedMax = -20.0f;
    Session->SetBlendSpaceState(BlendSpace);
    TestTrue(TEXT("Blend space state is configured"), Session->GetBlendSpaceState().bConfigured);
    TestTrue(TEXT("Blend space max speed is constrained"), Session->GetBlendSpaceState().SpeedMax >= Session->GetBlendSpaceState().SpeedMin);

    FCharacterCreatorAnimationBlueprintState BlueprintState;
    BlueprintState.SourceBlueprint = FSoftObjectPath(TEXT("/Game/Test/ABP_Source.ABP_Source"));
    BlueprintState.EnabledLayers = { FName(TEXT("Locomotion")), FName(TEXT("WeaponOverlay")) };
    Session->SetAnimationBlueprintState(BlueprintState);
    TestEqual(TEXT("Animation blueprint layers persist"), Session->GetAnimationBlueprintState().EnabledLayers.Num(), 2);

    FCharacterCreatorMontageComboState MontageState;
    MontageState.Sections = { FName(TEXT("Attack")), FName(TEXT("Recover")) };
    MontageState.ComboWindowSeconds = 8.0f;
    Session->SetMontageComboState(MontageState);
    TestTrue(TEXT("Montage combo state is authoring ready"), Session->GetMontageComboState().bAuthoringReady);
    TestTrue(TEXT("Montage combo window is constrained"), Session->GetMontageComboState().ComboWindowSeconds <= 2.0f);

    Session->SetAnimationSource(FSoftObjectPath(TEXT("/Game/Test/Idle.Idle")), FSoftObjectPath(TEXT("/Game/Test/SourceSkeleton.SourceSkeleton")));
    Session->SetAnimationRetargeter(FSoftObjectPath(TEXT("/Game/Test/Retargeter.Retargeter")));
    TestTrue(TEXT("Animation retarget execution succeeds with required inputs"), Session->ExecuteAnimationRetarget());
    TestEqual(TEXT("Retarget execution reaches target ready"), Session->GetAppearanceState().Animation.State, ECharacterCreatorAnimationState::TargetReady);
    TestTrue(TEXT("Retarget execution records mapped bones"), Session->GetAppearanceState().Animation.MappedBoneCount > 0);

    TestTrue(TEXT("Skeleton inspection succeeds with source and target references"), Session->InspectSkeletons());
    TestTrue(TEXT("Skeleton inspection records source bones"), Session->GetSkeletonInspection().SourceBoneCount > 0);
    TestTrue(TEXT("Skeleton inspection records required sockets"), Session->GetSkeletonInspection().RequiredSockets.Num() >= 4);

    FCharacterCreatorPhysicsSetupState PhysicsState;
    PhysicsState.PhysicsAsset = FSoftObjectPath(TEXT("/Game/Test/Physics.Physics"));
    PhysicsState.GravityScale = 99.0f;
    Session->SetPhysicsSetup(PhysicsState);
    TestTrue(TEXT("Physics setup validates an assigned physics asset"), Session->GetPhysicsSetup().bValidated);
    TestTrue(TEXT("Physics gravity scale is constrained"), Session->GetPhysicsSetup().GravityScale <= 5.0f);

    TestTrue(TEXT("LOD performance profile stays within its default budget"), Session->RunLODPerformanceProfile());
    TestTrue(TEXT("LOD performance profile estimates memory"), Session->GetLODPerformance().EstimatedMemoryKB > 0);

    Session->StartGameplayTest();
    Session->RecordGameplayAction(FName(TEXT("MoveForward")));
    Session->RecordGameplayAction(FName(TEXT("Attack")));
    Session->StopGameplayTest(true);
    TestEqual(TEXT("Gameplay test reaches passed state"), Session->GetGameplayTestState().State, ECharacterCreatorGameplayTestState::Passed);
    TestEqual(TEXT("Gameplay test records movement and combat actions"), Session->GetGameplayTestState().Actions.Num(), 2);

    FCharacterCreatorPreviewStudioState StudioState;
    StudioState.CameraMode = FName(TEXT("Portrait"));
    StudioState.LightingProfile = FName(TEXT("Dramatic"));
    StudioState.Zoom = 9.0f;
    Session->SetPreviewStudioState(StudioState);
    TestEqual(TEXT("Preview studio camera mode persists"), Session->GetPreviewStudioState().CameraMode, FName(TEXT("Portrait")));
    TestTrue(TEXT("Preview studio zoom is constrained"), Session->GetPreviewStudioState().Zoom <= 2.0f);

    TestTrue(TEXT("Portrait capture preparation succeeds"), Session->PreparePortraitCapture(TEXT("Saved/Portrait.png"), 32, 8192, FName(TEXT("PNG"))));
    TestTrue(TEXT("Portrait capture clamps to a usable size"), Session->GetPortraitCaptureState().Width >= 128 && Session->GetPortraitCaptureState().Height <= 4096);
    Session->SetControllerHint(FName(TEXT("TestAction")), FText::FromString(TEXT("Test hint")));
    TestTrue(TEXT("Controller hints are persisted"), Session->GetControllerHintState().Hints.Contains(FName(TEXT("TestAction"))));

    FCharacterCreatorSettings Settings = Session->GetSettings();
    Settings.UIScale = 9.0f;
    Settings.AutosaveIntervalSeconds = 1;
    Settings.bHighContrast = true;
    Session->SetSettings(Settings);
    TestTrue(TEXT("Settings sanitize UI scale"), Session->GetSettings().UIScale <= 1.5f);
    TestTrue(TEXT("Settings sanitize autosave interval"), Session->GetSettings().AutosaveIntervalSeconds >= 5);
    TestTrue(TEXT("Accessibility preference persists in session"), Session->GetSettings().bHighContrast);
    TestTrue(TEXT("Project browser has a default project"), Session->GetProjectBrowserState().Projects.Num() > 0);
    Session->SelectProject(Session->GetProjectBrowserState().Projects[0].SlotName);
    TestEqual(TEXT("Project browser selection persists"), Session->GetProjectBrowserState().SelectedSlotName, Session->GetProjectBrowserState().Projects[0].SlotName);

    UCharacterCreatorSession* RandomSessionA = NewObject<UCharacterCreatorSession>();
    UCharacterCreatorSession* RandomSessionB = NewObject<UCharacterCreatorSession>();
    RandomSessionA->InitializeDefaults();
    RandomSessionB->InitializeDefaults();
    RandomSessionA->SetRandomizationSeed(42);
    RandomSessionB->SetRandomizationSeed(42);
    RandomSessionA->SetRandomizationCategoryLocked(ECharacterCreatorRandomizationCategory::Body, true);
    RandomSessionB->SetRandomizationCategoryLocked(ECharacterCreatorRandomizationCategory::Body, true);
    RandomSessionA->SetRandomizationParameterRange(ECharacterCreatorParameter::EyeSize, 0.2f, 0.3f);
    RandomSessionB->SetRandomizationParameterRange(ECharacterCreatorParameter::EyeSize, 0.2f, 0.3f);
    RandomSessionA->RandomizeAppearance();
    RandomSessionB->RandomizeAppearance();
    TestEqual(TEXT("Locked body category remains unchanged"), RandomSessionA->GetParameterValue(ECharacterCreatorParameter::Height), 0.56f);
    TestEqual(TEXT("Seeded randomization is deterministic"), RandomSessionA->GetParameterValue(ECharacterCreatorParameter::EyeSize), RandomSessionB->GetParameterValue(ECharacterCreatorParameter::EyeSize));
    TestTrue(TEXT("Constrained randomization range is respected"), RandomSessionA->GetParameterValue(ECharacterCreatorParameter::EyeSize) >= 0.2f && RandomSessionA->GetParameterValue(ECharacterCreatorParameter::EyeSize) <= 0.3f);

    const FCharacterPreset PresetA = Session->CreatePresetFromCurrent(FText::FromString(TEXT("Preset A")), FText::FromString(TEXT("Base")));
    Session->SetParameterValue(ECharacterCreatorParameter::Height, 0.15f);
    const FCharacterPreset PresetB = Session->CreatePresetFromCurrent(FText::FromString(TEXT("Preset B")), FText::FromString(TEXT("Variant")));
    const FCharacterCreatorPresetComparison Comparison = Session->ComparePresets(PresetA.PresetId, PresetB.PresetId);
    TestFalse(TEXT("Preset comparison finds parameter differences"), Comparison.bEquivalent);
    TestTrue(TEXT("Preset comparison reports parameter category"), Comparison.Differences.Contains(FName(TEXT("Parameters"))));
    FCharacterCreatorPresetMergeOptions MergeOptions;
    MergeOptions.bUseSourceBody = true;
    const FCharacterPreset MergedPreset = Session->CreateMergedPreset(PresetA.PresetId, PresetB.PresetId, MergeOptions);
    TestTrue(TEXT("Preset merge creates a new preset"), MergedPreset.PresetId.IsValid() && MergedPreset.PresetId != PresetA.PresetId && MergedPreset.PresetId != PresetB.PresetId);
    Session->SetPresetSearchQuery(TEXT("Preset"));
    Session->SetPresetSelection({ PresetA.PresetId, PresetB.PresetId }, true);
    TestTrue(TEXT("Preset manager stores comparison selection"), Session->GetPresetManagerState().bCompareMode && Session->GetPresetManagerState().SelectedPresetIds.Num() == 2);

    const FCharacterPreset Duplicate = Session->CreatePresetFromCurrent(FText::FromString(TEXT("Test Preset")), FText::FromString(TEXT("Automation preset")));
    TestTrue(TEXT("Preset has a valid id"), Duplicate.PresetId.IsValid());
    TestTrue(TEXT("Preset duplicate succeeds"), Session->DuplicatePreset(Duplicate.PresetId));
    TestTrue(TEXT("Preset rename succeeds"), Session->RenamePreset(Duplicate.PresetId, FText::FromString(TEXT("Renamed Preset"))));
    TestTrue(TEXT("Preset delete succeeds"), Session->DeletePreset(Duplicate.PresetId));

    const TArray<ECharacterCreatorScreen> Screens = {
        ECharacterCreatorScreen::Dashboard,
        ECharacterCreatorScreen::ProjectBrowser,
        ECharacterCreatorScreen::CharacterCreator,
        ECharacterCreatorScreen::OutfitAndArmor,
        ECharacterCreatorScreen::HairAndGrooming,
        ECharacterCreatorScreen::MaterialsAndColor,
        ECharacterCreatorScreen::WeaponsAndIK,
        ECharacterCreatorScreen::AnimationOverview,
        ECharacterCreatorScreen::LocomotionSetup,
        ECharacterCreatorScreen::BlendSpaceAssistant,
        ECharacterCreatorScreen::AnimationBlueprintWorkspace,
        ECharacterCreatorScreen::MontageComboBuilder,
        ECharacterCreatorScreen::RetargetingAssistant,
        ECharacterCreatorScreen::SkeletonRigSocketInspector,
        ECharacterCreatorScreen::PhysicsSetup,
        ECharacterCreatorScreen::GameplayTest,
        ECharacterCreatorScreen::PreviewStudio,
        ECharacterCreatorScreen::PortraitStudio,
        ECharacterCreatorScreen::LODPerformance,
        ECharacterCreatorScreen::AssetBrowser,
        ECharacterCreatorScreen::ImportWizard,
        ECharacterCreatorScreen::Settings,
        ECharacterCreatorScreen::ValidationExport
    };
    for (const ECharacterCreatorScreen Screen : Screens)
    {
        Session->SetScreen(Screen);
        TestEqual(TEXT("Every declared screen is routable"), Session->GetScreen(), Screen);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCharacterCreatorExportContractTest,
    "CharacterCreator.Export.Contract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterCreatorExportContractTest::RunTest(const FString& Parameters)
{
    const FCharacterAppearanceState Appearance;
    const FCharacterPreset Preset;
    const FCharacterCreatorExportProfile Profile;
    TArray<FCharacterCreatorValidationIssue> Issues;
    FCharacterCreatorExportService::ValidateAppearance(Appearance, Profile, Issues);
    TestFalse(TEXT("Default Sidekick state has no blocking export errors"), FCharacterCreatorExportService::HasErrors(Issues));

    FString Manifest;
    TestTrue(TEXT("Default state produces a manifest"), FCharacterCreatorExportService::BuildManifestJson(Appearance, Preset, Profile, Manifest));
    TestTrue(TEXT("Manifest contains the profile version"), Manifest.Contains(TEXT("profileVersion")));
    FString BlueprintDescriptor;
    FString DataAssetDescriptor;
    FString PackageDescriptor;
    TestTrue(TEXT("Blueprint descriptor is generated"), FCharacterCreatorExportService::BuildBlueprintDescriptor(Appearance, Preset, BlueprintDescriptor));
    TestTrue(TEXT("Data Asset descriptor is generated"), FCharacterCreatorExportService::BuildDataAssetDescriptor(Appearance, Preset, DataAssetDescriptor));
    TestTrue(TEXT("Package descriptor is generated"), FCharacterCreatorExportService::BuildPackageDescriptor(Appearance, Preset, Profile, PackageDescriptor));
    TestTrue(TEXT("Blueprint descriptor identifies its format"), BlueprintDescriptor.Contains(TEXT("BlueprintDescriptor")));
    TestTrue(TEXT("Data Asset descriptor identifies its format"), DataAssetDescriptor.Contains(TEXT("DataAssetDescriptor")));
    TestTrue(TEXT("Package descriptor identifies its format"), PackageDescriptor.Contains(TEXT("PackageDescriptor")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCharacterCreatorImportContractTest,
    "CharacterCreator.Import.FreeAnimationsPack",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterCreatorImportContractTest::RunTest(const FString& Parameters)
{
    FCharacterCreatorImportProgress Progress;
    const FString ContentPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("FreeAnimationsPack"));
    const bool bValid = FCharacterCreatorImportService::ValidateDirectory(ContentPath, Progress);
    TestTrue(TEXT("Imported FAB Content directory validates"), bValid);
    TestEqual(TEXT("Import validation reaches ready state"), Progress.State, ECharacterCreatorImportState::Ready);
    TestTrue(TEXT("Imported package count is non-zero"), Progress.ValidFiles > 0);
    TestEqual(TEXT("Import validation reaches 100 percent"), Progress.Progress, 1.0f);

    TArray<FCharacterCreatorAssetCatalogEntry> Entries;
    FCharacterCreatorImportProgress ScanProgress;
    TestTrue(TEXT("Asset catalog scan succeeds"), FCharacterCreatorImportService::ScanDirectory(ContentPath, TEXT("Manny"), FString(), Entries, ScanProgress));
    TestTrue(TEXT("Asset catalog scan returns filtered entries"), Entries.Num() > 0);
    TestTrue(TEXT("Asset catalog records compatibility state"), Entries[0].Compatibility != ECharacterCreatorAssetCompatibility::Unknown);
    FCharacterCreatorImportOptions InvalidOptions;
    TestFalse(TEXT("Import requires an explicit destination directory"), FCharacterCreatorImportService::ImportAssets(Entries, InvalidOptions, ScanProgress));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCharacterCreatorUIAndSaveContractTest,
    "CharacterCreator.UIAndSave.Contract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterCreatorUIAndSaveContractTest::RunTest(const FString& Parameters)
{
    const FVector2D Clamped = UCharacterCreatorUIHelpers::ClampPopupPosition(
        FVector2D(1400.0f, 800.0f),
        FVector2D(240.0f, 120.0f),
        FVector2D(1440.0f, 810.0f));
    TestTrue(TEXT("Popup X position is clamped to the viewport"), FMath::IsNearlyEqual(Clamped.X, 1200.0f));
    TestTrue(TEXT("Popup Y position is clamped to the viewport"), FMath::IsNearlyEqual(Clamped.Y, 690.0f));

    UCharacterCreatorSaveGame* SaveGame = NewObject<UCharacterCreatorSaveGame>();
    TestNotNull(TEXT("Save game object is constructible"), SaveGame);
    if (!SaveGame)
    {
        return false;
    }

    SaveGame->SaveVersion = UCharacterCreatorSaveGame::CurrentSaveVersion;
    TestTrue(TEXT("Current save version is compatible"), SaveGame->IsCompatible());
    SaveGame->SaveVersion = UCharacterCreatorSaveGame::CurrentSaveVersion + 1;
    TestFalse(TEXT("Future save versions are rejected"), SaveGame->IsCompatible());
    SaveGame->SaveVersion = 0;
    TestFalse(TEXT("Zero save versions are rejected"), SaveGame->IsCompatible());
    return true;
}

#endif
