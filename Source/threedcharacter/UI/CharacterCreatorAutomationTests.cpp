#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#include "UI/CharacterCreatorExportService.h"
#if WITH_EDITOR
#include "UI/CharacterCreatorEditorExportService.h"
#include "CharacterCreatorGeneratedAssets.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#endif
#include "UI/CharacterCreatorImportService.h"
#include "UI/CharacterCreatorSession.h"
#include "UI/CharacterCreatorSaveGame.h"
#include "UI/CharacterCreatorUIFramework.h"
#include "UI/CharacterCreatorUtilityWorkspaceWidget.h"
#include "UI/CharacterCreatorSubsystem.h"
#include "Engine/GameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCharacterCreatorSessionFoundationTest,
    "CharacterCreator.Session.Foundation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterCreatorSessionFoundationTest::RunTest(const FString& Parameters)
{
    UCharacterCreatorSession* Session = NewObject<UCharacterCreatorSession>(GetTransientPackage());
    TestNotNull(TEXT("Session is constructible"), Session);
    if (!Session)
    {
        return false;
    }
    // Asset creation/compilation can trigger GC while this test is running. Keep
    // the transient session alive until all generated assets have been checked.
    Session->AddToRoot();

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

    const FLinearColor CustomPickerColor(0.21f, 0.47f, 0.83f, 1.0f);
    Session->SetColorTarget(ECharacterCreatorColorTarget::PrimaryOutfit, CustomPickerColor);
    TestTrue(TEXT("Color-picker RGB values persist on the selected target"), Session->GetColorTarget(ECharacterCreatorColorTarget::PrimaryOutfit).Equals(CustomPickerColor, 0.001f));

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
    TestFalse(TEXT("Manifest no longer embeds fake asset descriptors"), Manifest.Contains(TEXT("Descriptor")));
    return true;
}

#if WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCharacterCreatorP17RealAssetsE2ETest,
    "CharacterCreator.P17.RealAssetsE2E",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterCreatorP17RealAssetsE2ETest::RunTest(const FString& Parameters)
{
    UCharacterCreatorSession* Session = NewObject<UCharacterCreatorSession>();
    TestNotNull(TEXT("P17 session is constructible"), Session);
    if (!Session)
    {
        return false;
    }

    Session->InitializeDefaults();
    Session->SetAnimationSource(
        FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Animations/Manny/MM_Idle.MM_Idle")),
        FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny")));
    Session->SetAnimationRetargeter(FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Rigs/RTG_Mannequin.RTG_Mannequin")));
    const bool bRetargeted = Session->ExecuteAnimationRetarget();
    TestTrue(TEXT("P17 retarget step reaches target-ready state"), bRetargeted);
    if (!bRetargeted)
    {
        AddError(FString::Printf(TEXT("P17 real retarget failed: %s"), *Session->GetAppearanceState().Animation.LastRetargetMessage.ToString()));
    }
    TestEqual(TEXT("P17 target animation path is recorded"), Session->GetAppearanceState().Animation.State, ECharacterCreatorAnimationState::TargetReady);
    TestTrue(TEXT("P17 target animation is a real generated asset"), Session->GetAppearanceState().Animation.TargetAnimation.TryLoad() != nullptr);

    FCharacterCreatorWeaponSetup Weapon = Session->GetWeaponSetup(ECharacterCreatorWeaponSlot::MainHand);
    Weapon.WeaponId = FName(TEXT("TrainingBlade"));
    Weapon.bEnabled = true;
    Weapon.SocketName = FName(TEXT("hand_r"));
    Session->SetWeaponSetup(ECharacterCreatorWeaponSlot::MainHand, Weapon);
    TestTrue(TEXT("P17 weapon setup is enabled"), Session->GetWeaponSetup(ECharacterCreatorWeaponSlot::MainHand).bEnabled);
    TestTrue(TEXT("P17 skeleton inspection succeeds"), Session->InspectSkeletons());

    FCharacterCreatorExportProfile Profile;
    Profile.bGenerateBlueprint = true;
    Profile.bGenerateDataAsset = true;
    Profile.bGeneratePackage = true;
    const FString Destination = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CharacterCreator"), TEXT("P17"));
    IFileManager::Get().MakeDirectory(*Destination, true);
    const FGuid ExpectedPresetId = Session->GetActivePreset().PresetId;

    FCharacterCreatorRealExportResult Result;
    const bool bGeneratedDeliverables = FCharacterCreatorEditorExportService::GenerateDeliverables(
        Session->GetAppearanceStateNative(), Session->GetActivePreset(), Profile, Destination, Result);
    TestTrue(TEXT("P17 generates real Unreal deliverables"), bGeneratedDeliverables);
    if (!bGeneratedDeliverables)
    {
        AddError(FString::Printf(TEXT("P17 real export failed: %s"), *Result.ErrorMessage));
    }
    TestTrue(TEXT("P17 generated Blueprint path is valid"), Result.BlueprintAsset.IsValid());
    TestTrue(TEXT("P17 generated Data Asset path is valid"), Result.DataAsset.IsValid());
    TestTrue(TEXT("P17 generated content path is in project Content"), Result.GeneratedContentPath.StartsWith(TEXT("/Game/CharacterCreator/Generated/")));
    TestTrue(TEXT("P17 staged package contains files"), Result.StagedFiles.Num() >= 2);
    TestTrue(TEXT("P17 package manifest exists"), FPaths::FileExists(FPaths::Combine(Result.StagedPackageDirectory, TEXT("ActiveCharacter.package-manifest.json"))));

    UBlueprint* Blueprint = Cast<UBlueprint>(Result.BlueprintAsset.TryLoad());
    UCharacterCreatorAppearanceDataAsset* DataAsset = Cast<UCharacterCreatorAppearanceDataAsset>(Result.DataAsset.TryLoad());
    TestNotNull(TEXT("P17 Blueprint is a real UBlueprint asset"), Blueprint);
    TestNotNull(TEXT("P17 Data Asset is a real UPrimaryDataAsset"), DataAsset);
    if (IsValid(Blueprint))
    {
        Blueprint->AddToRoot();
        TestTrue(TEXT("P17 Blueprint derives from generated character parent"), Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf(ACharacterCreatorGeneratedCharacter::StaticClass()));
    }
    if (IsValid(DataAsset))
    {
        DataAsset->AddToRoot();
        TestEqual(TEXT("P17 Data Asset stores the active preset"), DataAsset->PresetId, ExpectedPresetId);
        TestFalse(TEXT("P17 Data Asset stores an empty skeletal mesh"), DataAsset->Appearance.Assets.SkeletalMesh.IsNull());
    }
    // Keep the generated objects rooted through commandlet shutdown. The editor
    // may replace package objects during SavePackage, so removing roots here can
    // dereference stale UObject handles after an asset reload.
    return true;
}

#endif

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCharacterCreatorProjectLifecycleE2ETest,
    "CharacterCreator.ProjectLifecycle.E2E",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterCreatorProjectLifecycleE2ETest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
    TestNotNull(TEXT("Lifecycle game instance is constructible"), GameInstance);
    if (!GameInstance) return false;
    GameInstance->AddToRoot();
    GameInstance->Init();
    UCharacterCreatorSubsystem* Subsystem = GameInstance->GetSubsystem<UCharacterCreatorSubsystem>();
    TestNotNull(TEXT("Character creator subsystem initializes through a real game instance"), Subsystem);
    if (!Subsystem)
    {
        GameInstance->Shutdown();
        GameInstance->RemoveFromRoot();
        return false;
    }

    UCharacterCreatorSession* Session = Subsystem->GetSession();
    TestNotNull(TEXT("Lifecycle subsystem owns a session"), Session);
    const FCharacterCreatorSettings OriginalSettings = Session->GetSettings();
    for (const FCharacterCreatorProjectRecord& ExistingProject : Session->GetProjectBrowserState().Projects)
    {
        if (ExistingProject.DisplayName.ToString().StartsWith(TEXT("Automation Lifecycle")))
        {
            Subsystem->DeleteProject(ExistingProject.SlotName);
        }
    }
    const FString UniqueName = FString::Printf(TEXT("Automation Lifecycle %s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
    FCharacterCreatorSettings Settings = Session->GetSettings();
    Settings.bCreateBackups = true;
    Settings.MaxBackupCount = 2;
    Session->SetSettings(Settings);
    Session->SetParameterValue(ECharacterCreatorParameter::Height, 0.61f);
    TestTrue(TEXT("Save As creates a named project"), Subsystem->SaveCurrentProjectAs(UniqueName));
    const FString OriginalSlot = Session->GetProjectBrowserState().ActiveSlotName;
    TestTrue(TEXT("Named project has a stable slot"), !OriginalSlot.IsEmpty() && Subsystem->DoesSaveExist(OriginalSlot));

    for (int32 Index = 0; Index < 4; ++Index)
    {
        Session->SetParameterValue(ECharacterCreatorParameter::Height, 0.62f + static_cast<float>(Index) * 0.01f);
        TestTrue(TEXT("Repeated manual save succeeds"), Subsystem->SaveCurrentProject());
    }
    const FCharacterCreatorProjectRecord* OriginalRecord = Session->GetProjectBrowserState().Projects.FindByPredicate([&OriginalSlot](const FCharacterCreatorProjectRecord& Item) { return Item.SlotName == OriginalSlot; });
    TestTrue(TEXT("Backup retention is bounded"), OriginalRecord && OriginalRecord->BackupCount <= 2);

    const FString CopyName = UniqueName + TEXT(" Copy");
    TestTrue(TEXT("Project duplicate succeeds"), Subsystem->DuplicateProject(OriginalSlot, CopyName));
    const FCharacterCreatorProjectBrowserState BrowserAfterDuplicate = Session->GetProjectBrowserState();
    const FCharacterCreatorProjectRecord* CopyRecord = BrowserAfterDuplicate.Projects.FindByPredicate([&CopyName](const FCharacterCreatorProjectRecord& Item) { return Item.DisplayName.ToString() == CopyName; });
    TestNotNull(TEXT("Duplicate is independently catalogued"), CopyRecord);
    const FString CopySlot = CopyRecord ? CopyRecord->SlotName : FString();
    TestTrue(TEXT("Project rename succeeds"), Subsystem->RenameProject(CopySlot, CopyName + TEXT(" Renamed")));

    Session->SetParameterValue(ECharacterCreatorParameter::Height, 0.77f);
    TestFalse(TEXT("Switching projects with dirty state requires confirmation"), Subsystem->SelectProject(CopySlot));
    TestTrue(TEXT("Unsaved confirmation flag is exposed to UMG"), Session->GetProjectBrowserState().bUnsavedConfirmationRequired);
    TestTrue(TEXT("Cancelling a pending project change succeeds"), Subsystem->ResolvePendingProjectChange(ECharacterCreatorUnsavedDecision::Cancel));
    TestTrue(TEXT("Dirty state remains after cancellation"), Session->HasUnsavedChanges());
    TestFalse(TEXT("Second dirty switch still requests confirmation"), Subsystem->SelectProject(CopySlot));
    TestTrue(TEXT("Discard decision completes the pending switch"), Subsystem->ResolvePendingProjectChange(ECharacterCreatorUnsavedDecision::Discard));
    TestEqual(TEXT("Duplicate becomes active after confirmed switch"), Session->GetProjectBrowserState().ActiveSlotName, CopySlot);

    Session->SetParameterValue(ECharacterCreatorParameter::Height, 0.79f);
    TestTrue(TEXT("Per-project autosave writes a recovery snapshot"), Subsystem->SaveAutosave());
    TestTrue(TEXT("Autosave recovery is advertised"), Session->GetProjectBrowserState().bAutosaveRecoveryAvailable);
    TestTrue(TEXT("Autosave recovery restores the snapshot"), Subsystem->RecoverAutosave());
    TestTrue(TEXT("Recovered data remains dirty until manually saved"), Session->HasUnsavedChanges());

    TestTrue(TEXT("Duplicate deletion succeeds"), Subsystem->DeleteProject(CopySlot));
    TestTrue(TEXT("Original project deletion succeeds"), Subsystem->DeleteProject(OriginalSlot));
    Session->SetSettings(OriginalSettings);
    GameInstance->Shutdown();
    GameInstance->RemoveFromRoot();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCharacterCreatorImportHardeningTest,
    "CharacterCreator.Import.PartialAndCancellation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterCreatorImportHardeningTest::RunTest(const FString& Parameters)
{
    const FString Root = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CharacterCreator"), TEXT("Automation"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
    const FString Source = FPaths::Combine(Root, TEXT("Source"));
    const FString Destination = FPaths::Combine(Root, TEXT("Destination"));
    IFileManager::Get().MakeDirectory(*Source, true);
    const FString SourceFile = FPaths::Combine(Source, TEXT("ValidAsset.uasset"));
    TestTrue(TEXT("Import fixture is written"), FFileHelper::SaveStringToFile(TEXT("automation-package"), *SourceFile));

    FCharacterCreatorAssetCatalogEntry ValidEntry;
    ValidEntry.SourceFile = SourceFile;
    ValidEntry.RelativePath = TEXT("Folder/ValidAsset.uasset");
    ValidEntry.AssetName = TEXT("ValidAsset");
    ValidEntry.FileSize = IFileManager::Get().FileSize(*SourceFile);
    ValidEntry.Compatibility = ECharacterCreatorAssetCompatibility::Compatible;
    ValidEntry.bSelected = true;
    FCharacterCreatorAssetCatalogEntry InvalidEntry = ValidEntry;
    InvalidEntry.AssetName = TEXT("InvalidAsset");
    InvalidEntry.RelativePath = TEXT("Folder/InvalidAsset.uasset");
    InvalidEntry.Compatibility = ECharacterCreatorAssetCompatibility::Incompatible;

    FCharacterCreatorImportOptions Options;
    Options.DestinationContentDirectory = Destination;
    FCharacterCreatorImportProgress PartialProgress;
    TestFalse(TEXT("Partial import reports an overall non-success"), FCharacterCreatorImportService::ImportAssets({ ValidEntry, InvalidEntry }, Options, PartialProgress));
    TestEqual(TEXT("Partial import state is explicit"), PartialProgress.State, ECharacterCreatorImportState::CompletedWithErrors);
    TestEqual(TEXT("One package is verified imported"), PartialProgress.ValidFiles, 1);
    TestEqual(TEXT("One package is reported failed"), PartialProgress.FailedFiles, 1);
    TestEqual(TEXT("Per-file results cover every selected package"), PartialProgress.FileResults.Num(), 2);

    FCharacterCreatorImportProgress CancelProgress;
    TestFalse(TEXT("Cancellation returns non-success"), FCharacterCreatorImportService::ImportAssets({ ValidEntry }, Options, CancelProgress, FCharacterCreatorImportService::FProgressCallback(), []() { return true; }));
    TestEqual(TEXT("Cancelled import has a distinct state"), CancelProgress.State, ECharacterCreatorImportState::Cancelled);
    TestEqual(TEXT("Cancelled file has a recorded outcome"), CancelProgress.FileResults[0].Outcome, ECharacterCreatorFileOperationOutcome::Cancelled);

    FCharacterCreatorAssetCatalogEntry EscapeEntry = ValidEntry;
    EscapeEntry.RelativePath = TEXT("../Escaped.uasset");
    FCharacterCreatorImportProgress EscapeProgress;
    TestFalse(TEXT("Import rejects relative paths that escape the selected destination"), FCharacterCreatorImportService::ImportAssets({ EscapeEntry }, Options, EscapeProgress));
    TestEqual(TEXT("Unsafe path is reported as a failed file"), EscapeProgress.FailedFiles, 1);
    TestFalse(TEXT("Unsafe path never writes outside the destination"), IFileManager::Get().FileExists(*FPaths::Combine(Root, TEXT("Escaped.uasset"))));
    IFileManager::Get().DeleteDirectory(*Root, false, true);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCharacterCreatorActualUMGFlowsTest,
    "CharacterCreator.UMG.ActualFlows",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterCreatorActualUMGFlowsTest::RunTest(const FString& Parameters)
{
    UCharacterCreatorSession* Session = NewObject<UCharacterCreatorSession>(GetTransientPackage());
    Session->InitializeDefaults();
    FCharacterCreatorAssetBrowserState Browser;
    FCharacterCreatorAssetCatalogEntry Entry;
    Entry.SourceFile = TEXT("Automation/Asset.uasset");
    Entry.AssetName = TEXT("Automation Asset");
    Entry.AssetClass = TEXT("SkeletalMesh");
    Entry.Compatibility = ECharacterCreatorAssetCompatibility::Compatible;
    Entry.bSelected = false;
    Browser.Entries.Add(Entry);
    Session->SetAssetBrowserState(Browser);

    UCharacterCreatorUtilityWorkspaceWidget* AssetWidget = NewObject<UCharacterCreatorUtilityWorkspaceWidget>(GetTransientPackage());
    TestTrue(TEXT("Asset UMG widget initializes"), AssetWidget->Initialize());
    AssetWidget->SetWorkspaceScreen(ECharacterCreatorScreen::AssetBrowser);
    AssetWidget->InitializeWithSession(Session);
    TestTrue(TEXT("Asset UMG widget builds its real widget tree"), AssetWidget->BuildForAutomation());
    TestTrue(TEXT("Data-driven asset selection button exists"), AssetWidget->HasCommandForAutomation(FName(TEXT("asset_select_0"))));
    TestTrue(TEXT("Data-driven favorite button exists"), AssetWidget->HasCommandForAutomation(FName(TEXT("asset_favorite_0"))));
    TestTrue(TEXT("Favorite command dispatches through the UMG button delegate"), AssetWidget->ExecuteCommandForAutomation(FName(TEXT("asset_favorite_0"))));
    TestTrue(TEXT("Favorite state reaches the authoritative session"), Session->GetAssetBrowserState().Entries[0].bFavorite);
    TestTrue(TEXT("Selection command dispatches through the UMG button delegate"), AssetWidget->ExecuteCommandForAutomation(FName(TEXT("asset_select_0"))));
    TestTrue(TEXT("Selection state reaches the authoritative session"), Session->GetAssetBrowserState().Entries[0].bSelected);

    UCharacterCreatorUtilityWorkspaceWidget* GameplayWidget = NewObject<UCharacterCreatorUtilityWorkspaceWidget>(GetTransientPackage());
    TestTrue(TEXT("Gameplay UMG widget initializes"), GameplayWidget->Initialize());
    GameplayWidget->SetWorkspaceScreen(ECharacterCreatorScreen::GameplayTest);
    GameplayWidget->InitializeWithSession(Session);
    TestTrue(TEXT("Gameplay UMG widget builds its real widget tree"), GameplayWidget->BuildForAutomation());
    TestTrue(TEXT("UMG start command executes"), GameplayWidget->ExecuteCommandForAutomation(FName(TEXT("gameplay_start"))));
    TestEqual(TEXT("UMG start command reaches running state"), Session->GetGameplayTestState().State, ECharacterCreatorGameplayTestState::Running);
    TestTrue(TEXT("UMG complete command executes"), GameplayWidget->ExecuteCommandForAutomation(FName(TEXT("gameplay_stop"))));
    TestEqual(TEXT("UMG complete command reaches passed state"), Session->GetGameplayTestState().State, ECharacterCreatorGameplayTestState::Passed);

    UCharacterCreatorUtilityWorkspaceWidget* ProjectWidget = NewObject<UCharacterCreatorUtilityWorkspaceWidget>(GetTransientPackage());
    TestTrue(TEXT("Project UMG widget initializes"), ProjectWidget->Initialize());
    ProjectWidget->SetWorkspaceScreen(ECharacterCreatorScreen::ProjectBrowser);
    ProjectWidget->InitializeWithSession(Session);
    TestTrue(TEXT("Project UMG widget builds its real widget tree"), ProjectWidget->BuildForAutomation());
    TestTrue(TEXT("Project Save As action is present in actual UMG"), ProjectWidget->HasCommandForAutomation(FName(TEXT("project_save_as"))));
    TestTrue(TEXT("Project rename action is present in actual UMG"), ProjectWidget->HasCommandForAutomation(FName(TEXT("project_rename"))));
    TestTrue(TEXT("Project duplicate action is present in actual UMG"), ProjectWidget->HasCommandForAutomation(FName(TEXT("project_duplicate"))));
    TestTrue(TEXT("Project delete action is present in actual UMG"), ProjectWidget->HasCommandForAutomation(FName(TEXT("project_delete"))));

    UCharacterCreatorUtilityWorkspaceWidget* ImportWidget = NewObject<UCharacterCreatorUtilityWorkspaceWidget>(GetTransientPackage());
    TestTrue(TEXT("Import UMG widget initializes"), ImportWidget->Initialize());
    ImportWidget->SetWorkspaceScreen(ECharacterCreatorScreen::ImportWizard);
    ImportWidget->InitializeWithSession(Session);
    TestTrue(TEXT("Import UMG widget builds its real widget tree"), ImportWidget->BuildForAutomation());
    TestTrue(TEXT("Import execution action is present in actual UMG"), ImportWidget->HasCommandForAutomation(FName(TEXT("import_execute"))));
    TestTrue(TEXT("Import cancellation action is present in actual UMG"), ImportWidget->HasCommandForAutomation(FName(TEXT("import_cancel"))));
    TestTrue(TEXT("Import source selection is present in actual UMG"), ImportWidget->HasCommandForAutomation(FName(TEXT("import_source"))));
    TestTrue(TEXT("Import destination selection is present in actual UMG"), ImportWidget->HasCommandForAutomation(FName(TEXT("import_destination"))));

    UCharacterCreatorUtilityWorkspaceWidget* ExportWidget = NewObject<UCharacterCreatorUtilityWorkspaceWidget>(GetTransientPackage());
    TestTrue(TEXT("Export UMG widget initializes"), ExportWidget->Initialize());
    ExportWidget->SetWorkspaceScreen(ECharacterCreatorScreen::ValidationExport);
    ExportWidget->InitializeWithSession(Session);
    TestTrue(TEXT("Export UMG widget builds its real widget tree"), ExportWidget->BuildForAutomation());
    TestTrue(TEXT("Export full-package action is present in actual UMG"), ExportWidget->HasCommandForAutomation(FName(TEXT("export_full_package"))));
    TestTrue(TEXT("Export cancellation action is present in actual UMG"), ExportWidget->HasCommandForAutomation(FName(TEXT("export_cancel"))));
    TestTrue(TEXT("Export destination selection is present in actual UMG"), ExportWidget->HasCommandForAutomation(FName(TEXT("export_destination"))));
    TestTrue(TEXT("Open output folder action is present in actual UMG"), ExportWidget->HasCommandForAutomation(FName(TEXT("export_open_folder"))));
    return true;
}

#endif
