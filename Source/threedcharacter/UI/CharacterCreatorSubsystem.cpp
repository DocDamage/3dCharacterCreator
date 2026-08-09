#include "UI/CharacterCreatorSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/World.h"
#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UI/CharacterCreatorExportService.h"
#if WITH_EDITOR
#include "UI/CharacterCreatorEditorExportService.h"
#endif
#include "UI/CharacterCreatorImportService.h"
#include "UI/CharacterCreatorSaveGame.h"
#include "UI/CharacterCreatorSession.h"

namespace
{
    bool IsSafeCharacterCreatorSlot(const FString& SlotName)
    {
        if (SlotName.IsEmpty() || SlotName.Len() > 128) return false;
        for (const TCHAR Character : SlotName)
        {
            if (!(FChar::IsAlnum(Character) || Character == TEXT('_') || Character == TEXT('-'))) return false;
        }
        return true;
    }

    bool IsCataloguedProject(const FCharacterCreatorProjectBrowserState& Browser, const FString& SlotName)
    {
        return Browser.Projects.ContainsByPredicate([&SlotName](const FCharacterCreatorProjectRecord& Project) { return Project.SlotName == SlotName; });
    }
}

void UCharacterCreatorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Session = NewObject<UCharacterCreatorSession>(this, TEXT("CharacterCreatorSession"));
    if (Session)
    {
        Session->InitializeDefaults();
        Session->OnAppearanceChanged.AddUObject(this, &UCharacterCreatorSubsystem::HandleAppearanceChanged);
        Session->OnSettingsChanged.AddUObject(this, &UCharacterCreatorSubsystem::HandleSettingsChanged);
        LoadPreferences();
        LoadProjectCatalog();
        FCharacterCreatorSettings Settings = Session->GetSettings();
        if (Settings.ImportSourceDirectory.IsEmpty())
        {
            Settings.ImportSourceDirectory = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("FreeAnimationsPack"));
        }
        if (Settings.ImportDestinationDirectory.IsEmpty())
        {
            Settings.ImportDestinationDirectory = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("CharacterCreator"), TEXT("Imported"));
        }
        if (Settings.ExportDestinationDirectory.IsEmpty())
        {
            Settings.ExportDestinationDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CharacterCreator"), TEXT("Exports"));
        }
        Session->SetSettings(Settings);
        ActiveProjectSlotName = Session->GetProjectBrowserState().ActiveSlotName;
        if (ActiveProjectSlotName.IsEmpty())
        {
            ActiveProjectSlotName = Session->GetProjectBrowserState().SelectedSlotName;
        }
        if (ActiveProjectSlotName == LegacyAutosaveSlotName)
        {
            ActiveProjectSlotName.Reset();
        }
        RefreshProjectBrowser();
        UpdateRecoveryState();
        bPreferencesReady = true;
        HandleSettingsChanged(Session->GetSettings());
    }
}

void UCharacterCreatorSubsystem::Deinitialize()
{
    if (ActiveImportCancellation.IsValid())
    {
        ActiveImportCancellation->Store(true);
        ActiveImportCancellation.Reset();
    }
    if (bAutosavePending && Session)
    {
        SaveAutosave();
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutosaveTimerHandle);
        World->GetTimerManager().ClearTimer(ExportTimerHandle);
    }

    if (Session)
    {
        Session->OnAppearanceChanged.RemoveAll(this);
        Session->OnSettingsChanged.RemoveAll(this);
        Session->Shutdown();
        Session = nullptr;
    }

    Super::Deinitialize();
}

bool UCharacterCreatorSubsystem::SaveToSlot(const FString& SlotName)
{
    return SaveToSlotInternal(SlotName, Session ? Session->GetSettings().ProjectName : FString(), false, true);
}

bool UCharacterCreatorSubsystem::SaveCurrentProject()
{
    if (!Session)
    {
        return false;
    }
    if (ActiveProjectSlotName.IsEmpty())
    {
        return SaveCurrentProjectAs(Session->GetSettings().ProjectName);
    }
    return SaveToSlotInternal(ActiveProjectSlotName, Session->GetSettings().ProjectName, false, true);
}

bool UCharacterCreatorSubsystem::SaveCurrentProjectAs(const FString& DisplayName)
{
    if (!Session || DisplayName.TrimStartAndEnd().IsEmpty())
    {
        if (Session) Session->SetStatusMessage(FText::FromString(TEXT("Enter a project name before saving.")));
        return false;
    }
    const FString CleanName = DisplayName.TrimStartAndEnd();
    return SaveToSlotInternal(MakeUniqueProjectSlot(CleanName), CleanName, false, true);
}

bool UCharacterCreatorSubsystem::SaveToSlotInternal(const FString& SlotName, const FString& DisplayName, bool bAutosave, bool bSetActiveProject)
{
    if (!Session || !IsSafeCharacterCreatorSlot(SlotName))
    {
        return false;
    }

    if (!bAutosave && Session->GetSettings().bCreateBackups && UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        if (UCharacterCreatorSaveGame* ExistingSave = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
        {
            const FString BackupSlot = FString::Printf(TEXT("%s_Backup_%s_%03d"), *SlotName, *FDateTime::UtcNow().ToString(TEXT("%Y%m%d%H%M%S")), FMath::RandRange(0, 999));
            UGameplayStatics::SaveGameToSlot(ExistingSave, BackupSlot, 0);
            PruneBackups(SlotName);
        }
    }

    UCharacterCreatorSaveGame* SaveGame = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::CreateSaveGameObject(UCharacterCreatorSaveGame::StaticClass()));
    if (!SaveGame)
    {
        return false;
    }

    SaveGame->SaveVersion = UCharacterCreatorSaveGame::CurrentSaveVersion;
    SaveGame->SlotName = SlotName;
    SaveGame->LastSavedUtc = FDateTime::UtcNow();
    SaveGame->Appearance = Session->GetAppearanceStateNative();
    SaveGame->Appearance.bHasUnsavedChanges = false;
    SaveGame->ActivePreset = Session->GetActivePreset();
    SaveGame->Presets = Session->GetPresets();
    SaveGame->Onboarding = Session->GetOnboardingState();
    SaveGame->Settings = Session->GetSettings();
    SaveGame->ProjectBrowser = Session->GetProjectBrowserState();
    SaveGame->ExportHistory = Session->GetExportHistory();

    FCharacterCreatorProjectBrowserState Browser = SaveGame->ProjectBrowser;
    FCharacterCreatorProjectRecord* ExistingProject = Browser.Projects.FindByPredicate([&SlotName](const FCharacterCreatorProjectRecord& Project)
    {
        return Project.SlotName == SlotName;
    });
    if (!ExistingProject)
    {
        ExistingProject = &Browser.Projects.AddDefaulted_GetRef();
        ExistingProject->SlotName = SlotName;
    }
    ExistingProject->DisplayName = FText::FromString(DisplayName.IsEmpty() ? Session->GetSettings().ProjectName : DisplayName);
    ExistingProject->LastModifiedUtc = SaveGame->LastSavedUtc;
    ExistingProject->AssetCount = Session->GetAppearanceStateNative().AssetBrowser.Entries.Num();
    ExistingProject->bAutosave = bAutosave;
    ExistingProject->bHasUnsavedChanges = false;
    ExistingProject->BackupCount = PruneBackups(bAutosave ? ActiveProjectSlotName : SlotName);
    if (bSetActiveProject)
    {
        Browser.SelectedSlotName = SlotName;
        Browser.ActiveSlotName = SlotName;
    }
    SaveGame->ProjectBrowser = Browser;

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, 0);
    if (bSaved)
    {
        if (bSetActiveProject)
        {
            ActiveProjectSlotName = SlotName;
            FCharacterCreatorSettings Settings = Session->GetSettings();
            Settings.ProjectName = ExistingProject->DisplayName.ToString();
            Session->SetSettings(Settings);
            Session->ApplyAppearanceChanges();
        }
        const FString SavedDisplayName = ExistingProject->DisplayName.ToString();
        if (!bAutosave)
        {
            Browser.Projects.RemoveAll([](const FCharacterCreatorProjectRecord& Project) { return Project.bAutosave; });
            Session->SetProjectBrowserState(Browser);
            SaveProjectCatalog();
        }
        const FString SaveMessage = bAutosave
            ? FString::Printf(TEXT("Autosaved recovery snapshot for %s"), *SavedDisplayName)
            : FString::Printf(TEXT("Saved project %s"), *SavedDisplayName);
        Session->SetStatusMessage(FText::FromString(SaveMessage));
        bAutosavePending = false;
        if (!bAutosave)
        {
            UGameplayStatics::DeleteGameInSlot(GetAutosaveSlotName(), 0);
        }
        UpdateRecoveryState();
    }
    else
    {
        Session->SetStatusMessage(FText::FromString(TEXT("Unable to write character save")));
    }

    return bSaved;
}

bool UCharacterCreatorSubsystem::LoadFromSlot(const FString& SlotName)
{
    return LoadFromSlotInternal(SlotName, false);
}

bool UCharacterCreatorSubsystem::LoadFromSlotInternal(const FString& SlotName, bool bRecoveredAutosave)
{
    if (!Session || !IsSafeCharacterCreatorSlot(SlotName) || !UGameplayStatics::DoesSaveGameExist(SlotName, 0)) return false;

    UCharacterCreatorSaveGame* SaveGame = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    if (!SaveGame || !SaveGame->IsCompatible())
    {
        Session->SetStatusMessage(FText::FromString(TEXT("Save is missing or uses an unsupported version")));
        return false;
    }

    Session->SetPresetLibrary(SaveGame->Presets, SaveGame->ActivePreset);
    Session->SetAppearanceState(SaveGame->Appearance, false);
    Session->SetOnboardingState(SaveGame->Onboarding);
    Session->SetSettings(SaveGame->Settings);
    FCharacterCreatorProjectBrowserState Browser = Session->GetProjectBrowserState();
    if (!bRecoveredAutosave && SaveGame->ProjectBrowser.Projects.Num() > 0)
    {
        Browser = SaveGame->ProjectBrowser;
    }
    ActiveProjectSlotName = bRecoveredAutosave ? ActiveProjectSlotName : SlotName;
    Browser.ActiveSlotName = ActiveProjectSlotName;
    Browser.SelectedSlotName = ActiveProjectSlotName;
    Browser.bUnsavedConfirmationRequired = false;
    Browser.PendingSlotName.Reset();
    Session->SetProjectBrowserState(Browser);
    Session->SetExportHistory(SaveGame->ExportHistory);
    if (bRecoveredAutosave)
    {
        FCharacterAppearanceState RecoveredAppearance = Session->GetAppearanceStateNative();
        RecoveredAppearance.bHasUnsavedChanges = true;
        Session->SetAppearanceState(RecoveredAppearance, true);
    }
    const FString LoadMessage = bRecoveredAutosave
        ? FString::Printf(TEXT("Recovered autosave for %s; save to keep it"), *Session->GetSettings().ProjectName)
        : FString::Printf(TEXT("Loaded project %s"), *Session->GetSettings().ProjectName);
    Session->SetStatusMessage(FText::FromString(LoadMessage));
    bAutosavePending = false;
    return true;
}

bool UCharacterCreatorSubsystem::DoesSaveExist(const FString& SlotName) const
{
    return !SlotName.IsEmpty() && UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

bool UCharacterCreatorSubsystem::SaveAutosave()
{
    return Session && Session->HasUnsavedChanges()
        ? SaveToSlotInternal(GetAutosaveSlotName(), Session->GetSettings().ProjectName, true, false)
        : false;
}

bool UCharacterCreatorSubsystem::LoadAutosave()
{
    return RecoverAutosave();
}

bool UCharacterCreatorSubsystem::CreateProject(const FString& DisplayName)
{
    const FString CleanName = DisplayName.TrimStartAndEnd();
    if (!Session || CleanName.IsEmpty())
    {
        if (Session) Session->SetStatusMessage(FText::FromString(TEXT("Enter a name for the new project.")));
        return false;
    }
    if (Session->HasUnsavedChanges())
    {
        return StartPendingProjectChange(FString(), CleanName);
    }

    Session->ResetAppearance();
    FCharacterCreatorSettings Settings = Session->GetSettings();
    Settings.ProjectName = CleanName;
    Session->SetSettings(Settings);
    ActiveProjectSlotName.Reset();
    return SaveCurrentProjectAs(CleanName);
}

bool UCharacterCreatorSubsystem::RenameProject(const FString& SlotName, const FString& NewDisplayName)
{
    const FString CleanName = NewDisplayName.TrimStartAndEnd();
    if (!Session || !IsSafeCharacterCreatorSlot(SlotName) || !IsCataloguedProject(Session->GetProjectBrowserState(), SlotName) || CleanName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        if (Session) Session->SetStatusMessage(FText::FromString(TEXT("Select a saved project and enter a new name.")));
        return false;
    }

    UCharacterCreatorSaveGame* SaveGame = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    if (!SaveGame || !SaveGame->IsCompatible()) return false;
    SaveGame->Settings.ProjectName = CleanName;
    SaveGame->LastSavedUtc = FDateTime::UtcNow();
    FCharacterCreatorProjectBrowserState Browser = Session->GetProjectBrowserState();
    if (FCharacterCreatorProjectRecord* Project = Browser.Projects.FindByPredicate([&SlotName](const FCharacterCreatorProjectRecord& Item) { return Item.SlotName == SlotName; }))
    {
        Project->DisplayName = FText::FromString(CleanName);
        Project->LastModifiedUtc = SaveGame->LastSavedUtc;
    }
    SaveGame->ProjectBrowser = Browser;
    if (!UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, 0)) return false;
    if (SlotName == ActiveProjectSlotName)
    {
        FCharacterCreatorSettings Settings = Session->GetSettings();
        Settings.ProjectName = CleanName;
        Session->SetSettings(Settings);
    }
    Session->SetProjectBrowserState(Browser);
    SaveProjectCatalog();
    Session->SetStatusMessage(FText::FromString(FString::Printf(TEXT("Renamed project to %s"), *CleanName)));
    return true;
}

bool UCharacterCreatorSubsystem::DuplicateProject(const FString& SlotName, const FString& NewDisplayName)
{
    const FString CleanName = NewDisplayName.TrimStartAndEnd();
    if (!Session || !IsSafeCharacterCreatorSlot(SlotName) || !IsCataloguedProject(Session->GetProjectBrowserState(), SlotName) || CleanName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SlotName, 0)) return false;
    UCharacterCreatorSaveGame* SourceSave = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    if (!SourceSave || !SourceSave->IsCompatible()) return false;

    const FString NewSlot = MakeUniqueProjectSlot(CleanName);
    SourceSave->SlotName = NewSlot;
    SourceSave->LastSavedUtc = FDateTime::UtcNow();
    SourceSave->Settings.ProjectName = CleanName;
    FCharacterCreatorProjectBrowserState Browser = Session->GetProjectBrowserState();
    FCharacterCreatorProjectRecord& Duplicate = Browser.Projects.AddDefaulted_GetRef();
    Duplicate.SlotName = NewSlot;
    Duplicate.DisplayName = FText::FromString(CleanName);
    Duplicate.LastModifiedUtc = SourceSave->LastSavedUtc;
    Duplicate.AssetCount = SourceSave->Appearance.AssetBrowser.Entries.Num();
    SourceSave->ProjectBrowser = Browser;
    if (!UGameplayStatics::SaveGameToSlot(SourceSave, NewSlot, 0))
    {
        Browser.Projects.RemoveAll([&NewSlot](const FCharacterCreatorProjectRecord& Item) { return Item.SlotName == NewSlot; });
        return false;
    }
    Session->SetProjectBrowserState(Browser);
    SaveProjectCatalog();
    Session->SetStatusMessage(FText::FromString(FString::Printf(TEXT("Duplicated project as %s"), *CleanName)));
    return true;
}

bool UCharacterCreatorSubsystem::DeleteProject(const FString& SlotName)
{
    if (!Session || !IsSafeCharacterCreatorSlot(SlotName) || !IsCataloguedProject(Session->GetProjectBrowserState(), SlotName)) return false;
    const bool bDeleted = !UGameplayStatics::DoesSaveGameExist(SlotName, 0) || UGameplayStatics::DeleteGameInSlot(SlotName, 0);
    if (!bDeleted) return false;

    UGameplayStatics::DeleteGameInSlot(SlotName + TEXT("_Autosave"), 0);
    const FString SaveDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"));
    TArray<FString> BackupFiles;
    IFileManager::Get().FindFiles(BackupFiles, *FPaths::Combine(SaveDirectory, SlotName + TEXT("_Backup_*.sav")), true, false);
    for (const FString& BackupFile : BackupFiles)
    {
        IFileManager::Get().Delete(*FPaths::Combine(SaveDirectory, BackupFile), false, true);
    }

    FCharacterCreatorProjectBrowserState Browser = Session->GetProjectBrowserState();
    Browser.Projects.RemoveAll([&SlotName](const FCharacterCreatorProjectRecord& Item) { return Item.SlotName == SlotName; });
    if (ActiveProjectSlotName == SlotName)
    {
        ActiveProjectSlotName.Reset();
        Browser.ActiveSlotName.Reset();
        Browser.SelectedSlotName = Browser.Projects.Num() > 0 ? Browser.Projects[0].SlotName : FString();
        Session->ResetAppearance();
    }
    else if (Browser.SelectedSlotName == SlotName)
    {
        Browser.SelectedSlotName = Browser.ActiveSlotName;
    }
    Session->SetProjectBrowserState(Browser);
    SaveProjectCatalog();
    Session->SetStatusMessage(FText::FromString(TEXT("Project deleted")));
    return true;
}

bool UCharacterCreatorSubsystem::StartPendingProjectChange(const FString& TargetSlotName, const FString& NewProjectName)
{
    if (!Session) return false;
    PendingProjectSlotName = TargetSlotName;
    PendingNewProjectName = NewProjectName;
    bPendingProjectChange = true;
    FCharacterCreatorProjectBrowserState Browser = Session->GetProjectBrowserState();
    Browser.PendingSlotName = TargetSlotName;
    Browser.bUnsavedConfirmationRequired = true;
    Session->SetProjectBrowserState(Browser);
    Session->SetStatusMessage(FText::FromString(TEXT("Unsaved changes: choose Save, Discard, or Cancel before switching projects.")));
    return false;
}

bool UCharacterCreatorSubsystem::ResolvePendingProjectChange(ECharacterCreatorUnsavedDecision Decision)
{
    if (!Session || !bPendingProjectChange) return false;
    if (Decision == ECharacterCreatorUnsavedDecision::Cancel)
    {
        PendingProjectSlotName.Reset();
        PendingNewProjectName.Reset();
        bPendingProjectChange = false;
        FCharacterCreatorProjectBrowserState Browser = Session->GetProjectBrowserState();
        Browser.PendingSlotName.Reset();
        Browser.bUnsavedConfirmationRequired = false;
        Session->SetProjectBrowserState(Browser);
        Session->SetStatusMessage(FText::FromString(TEXT("Project switch cancelled")));
        return true;
    }
    if (Decision == ECharacterCreatorUnsavedDecision::Save && !SaveCurrentProject()) return false;
    return CompletePendingProjectChange();
}

bool UCharacterCreatorSubsystem::CompletePendingProjectChange()
{
    const FString TargetSlot = PendingProjectSlotName;
    const FString NewName = PendingNewProjectName;
    PendingProjectSlotName.Reset();
    PendingNewProjectName.Reset();
    bPendingProjectChange = false;
    FCharacterCreatorProjectBrowserState Browser = Session->GetProjectBrowserState();
    Browser.PendingSlotName.Reset();
    Browser.bUnsavedConfirmationRequired = false;
    Session->SetProjectBrowserState(Browser);
    if (!NewName.IsEmpty())
    {
        Session->ResetAppearance();
        FCharacterCreatorSettings Settings = Session->GetSettings();
        Settings.ProjectName = NewName;
        Session->SetSettings(Settings);
        ActiveProjectSlotName.Reset();
        return SaveCurrentProjectAs(NewName);
    }
    return LoadFromSlotInternal(TargetSlot, false);
}

bool UCharacterCreatorSubsystem::RecoverAutosave()
{
    const FString AutosaveSlot = GetAutosaveSlotName();
    if (!Session || !UGameplayStatics::DoesSaveGameExist(AutosaveSlot, 0)) return false;
    bRecoveryDismissed = true;
    const bool bRecovered = LoadFromSlotInternal(AutosaveSlot, true);
    UpdateRecoveryState();
    return bRecovered;
}

void UCharacterCreatorSubsystem::DismissAutosaveRecovery()
{
    bRecoveryDismissed = true;
    UpdateRecoveryState();
    if (Session) Session->SetStatusMessage(FText::FromString(TEXT("Autosave recovery dismissed; the snapshot is retained until the next manual save.")));
}

bool UCharacterCreatorSubsystem::ValidateCurrentAppearance(TArray<FCharacterCreatorValidationIssue>& OutIssues) const
{
    if (!Session)
    {
        OutIssues.Reset();
        FCharacterCreatorValidationIssue& Issue = OutIssues.AddDefaulted_GetRef();
        Issue.Severity = ECharacterCreatorValidationSeverity::Error;
        Issue.Code = TEXT("MissingSession");
        Issue.Message = FText::FromString(TEXT("No character creator session is available."));
        Issue.Remediation = FText::FromString(TEXT("Open the character creator workspace before exporting."));
        return false;
    }

    FCharacterCreatorExportProfile DefaultProfile;
    FCharacterCreatorExportService::ValidateAppearance(Session->GetAppearanceStateNative(), DefaultProfile, OutIssues);
    return !FCharacterCreatorExportService::HasErrors(OutIssues);
}

bool UCharacterCreatorSubsystem::RunValidation()
{
    const bool bValid = ValidateCurrentAppearance(LastValidationIssues);
    if (Session)
    {
        int32 ErrorCount = 0;
        int32 WarningCount = 0;
        for (const FCharacterCreatorValidationIssue& Issue : LastValidationIssues)
        {
            ErrorCount += Issue.Severity == ECharacterCreatorValidationSeverity::Error ? 1 : 0;
            WarningCount += Issue.Severity == ECharacterCreatorValidationSeverity::Warning ? 1 : 0;
        }
        Session->SetStatusMessage(FText::FromString(bValid
            ? FString::Printf(TEXT("Validation ready • %d warnings"), WarningCount)
            : FString::Printf(TEXT("Validation blocked • %d errors, %d warnings"), ErrorCount, WarningCount)));
    }
    return bValid;
}

bool UCharacterCreatorSubsystem::ApplyValidationFix(FName IssueCode)
{
    if (!Session || IssueCode.IsNone())
    {
        return false;
    }

    FCharacterAppearanceState Appearance = Session->GetAppearanceStateNative();
    bool bChanged = false;
    if (IssueCode == FName(TEXT("MissingCharacterMesh")))
    {
        FCharacterAssetReferences Defaults;
        Appearance.Assets.SkeletalMesh = Defaults.SkeletalMesh;
        Appearance.Assets.Skeleton = Defaults.Skeleton;
        bChanged = true;
    }
    else if (IssueCode == FName(TEXT("MissingBaseMaterial")))
    {
        FCharacterAssetReferences Defaults;
        Appearance.Assets.BaseMaterial = Defaults.BaseMaterial;
        bChanged = true;
    }
    else if (IssueCode == FName(TEXT("InvalidOutfitAsset")))
    {
        Appearance.Loadout.OutfitMesh.Reset();
        bChanged = true;
    }
    else if (IssueCode == FName(TEXT("InvalidHairAsset")))
    {
        Appearance.Loadout.HairMesh.Reset();
        bChanged = true;
    }

    if (bChanged)
    {
        Session->SetAppearanceState(Appearance, true);
        Session->SetStatusMessage(FText::FromString(FString::Printf(TEXT("Applied fix: %s"), *IssueCode.ToString())));
        RunValidation();
    }
    return bChanged;
}

int32 UCharacterCreatorSubsystem::ApplyAllValidationFixes()
{
    RunValidation();
    const TArray<FCharacterCreatorValidationIssue> Issues = LastValidationIssues;
    int32 FixedCount = 0;
    for (const FCharacterCreatorValidationIssue& Issue : Issues)
    {
        if (Issue.Severity == ECharacterCreatorValidationSeverity::Error && ApplyValidationFix(Issue.Code))
        {
            ++FixedCount;
        }
    }
    RunValidation();
    return FixedCount;
}

bool UCharacterCreatorSubsystem::ExportCurrentManifest(const FCharacterCreatorExportProfile& Profile, const FString& DestinationPath)
{
    if (!Session)
    {
        return false;
    }

    TArray<FCharacterCreatorValidationIssue> Issues;
    FCharacterCreatorExportService::ValidateAppearance(Session->GetAppearanceStateNative(), Profile, Issues);
    if (FCharacterCreatorExportService::HasErrors(Issues))
    {
        const FCharacterCreatorValidationIssue* FirstError = Issues.FindByPredicate([](const FCharacterCreatorValidationIssue& Issue)
        {
            return Issue.Severity == ECharacterCreatorValidationSeverity::Error;
        });
        const FText FailureMessage = FirstError ? FirstError->Remediation : FText::FromString(TEXT("Export validation failed"));
        Session->SetStatusMessage(FailureMessage);
        AddExportHistory(false, DestinationPath, { ECharacterCreatorDeliverable::Manifest }, FailureMessage);
        return false;
    }

    FString ManifestJson;
    if (!FCharacterCreatorExportService::BuildManifestJson(Session->GetAppearanceStateNative(), Session->GetActivePreset(), Profile, ManifestJson))
    {
        const FText FailureMessage = FText::FromString(TEXT("Unable to build the character export manifest"));
        Session->SetStatusMessage(FailureMessage);
        AddExportHistory(false, DestinationPath, { ECharacterCreatorDeliverable::Manifest }, FailureMessage);
        return false;
    }

    FString OutputPath = DestinationPath;
    if (OutputPath.IsEmpty())
    {
        OutputPath = Session->GetSettings().ExportDestinationDirectory;
    }
    if (OutputPath.IsEmpty())
    {
        OutputPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CharacterCreator"), TEXT("Exports"));
    }
    if (FPaths::GetExtension(OutputPath).IsEmpty())
    {
        OutputPath = FPaths::Combine(OutputPath, TEXT("ActiveCharacter.json"));
    }
    const FString OutputDirectory = FPaths::GetPath(OutputPath);
    if (!OutputDirectory.IsEmpty())
    {
        IFileManager::Get().MakeDirectory(*OutputDirectory, true);
    }

    const bool bWritten = FFileHelper::SaveStringToFile(ManifestJson, *OutputPath);
    Session->SetStatusMessage(bWritten
        ? FText::FromString(FString::Printf(TEXT("Export manifest written to %s"), *OutputPath))
        : FText::FromString(TEXT("Unable to write the export manifest")));
    AddExportHistory(bWritten, FPaths::GetPath(OutputPath), { ECharacterCreatorDeliverable::Manifest }, bWritten ? FText::FromString(TEXT("Manifest export completed")) : FText::FromString(TEXT("Manifest export failed")));
    return bWritten;
}

bool UCharacterCreatorSubsystem::ExportCurrentDeliverables(const FCharacterCreatorExportProfile& Profile, const FString& DestinationDirectory)
{
    if (!Session)
    {
        return false;
    }

    TArray<ECharacterCreatorDeliverable> Deliverables;
    if (Profile.bIncludeMetadata) Deliverables.Add(ECharacterCreatorDeliverable::Manifest);
    if (Profile.bGenerateBlueprint) Deliverables.Add(ECharacterCreatorDeliverable::Blueprint);
    if (Profile.bGenerateDataAsset) Deliverables.Add(ECharacterCreatorDeliverable::DataAsset);
    if (Profile.bGeneratePackage) Deliverables.Add(ECharacterCreatorDeliverable::Package);
    if (Deliverables.Num() == 0)
    {
        Session->SetStatusMessage(FText::FromString(TEXT("Select at least one deliverable before exporting")));
        AddExportHistory(false, DestinationDirectory, Deliverables, FText::FromString(TEXT("No deliverables selected")));
        ExportProgress.State = ECharacterCreatorOperationState::Failed;
        ExportProgress.Progress = 1.0f;
        ExportProgress.Stage = FText::FromString(TEXT("No deliverables selected"));
        return false;
    }

    ExportProgress.State = ECharacterCreatorOperationState::Running;
    ExportProgress.Progress = 0.15f;
    ExportProgress.Stage = FText::FromString(TEXT("Validating character and dependencies"));
    ExportProgress.Requested = Deliverables;
    ExportProgress.Succeeded.Reset();
    ExportProgress.Failed.Reset();

    TArray<FCharacterCreatorValidationIssue> Issues;
    FCharacterCreatorExportService::ValidateAppearance(Session->GetAppearanceStateNative(), Profile, Issues);
    LastValidationIssues = Issues;
    if (FCharacterCreatorExportService::HasErrors(Issues))
    {
        const FCharacterCreatorValidationIssue* FirstError = Issues.FindByPredicate([](const FCharacterCreatorValidationIssue& Issue)
        {
            return Issue.Severity == ECharacterCreatorValidationSeverity::Error;
        });
        const FText Message = FirstError ? FirstError->Remediation : FText::FromString(TEXT("Export validation failed"));
        Session->SetStatusMessage(Message);
        AddExportHistory(false, DestinationDirectory, Deliverables, Message);
        ExportProgress.State = ECharacterCreatorOperationState::Failed;
        ExportProgress.Progress = 1.0f;
        ExportProgress.Stage = Message;
        ExportProgress.Failed = Deliverables;
        return false;
    }

    FString OutputDirectory = DestinationDirectory.IsEmpty() ? Session->GetSettings().ExportDestinationDirectory : DestinationDirectory;
    if (OutputDirectory.IsEmpty()) OutputDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CharacterCreator"), TEXT("Exports"));
    IFileManager::Get().MakeDirectory(*OutputDirectory, true);
    const FCharacterAppearanceState Appearance = Session->GetAppearanceStateNative();
    const FCharacterPreset Preset = Session->GetActivePreset();
    TArray<ECharacterCreatorDeliverable> SucceededDeliverables;
    TArray<ECharacterCreatorDeliverable> FailedDeliverables;

    if (Profile.bIncludeMetadata)
    {
        ExportProgress.Progress = 0.35f;
        ExportProgress.Stage = FText::FromString(TEXT("Writing metadata manifest"));
        FString ManifestJson;
        const bool bManifestWritten = FCharacterCreatorExportService::BuildManifestJson(Appearance, Preset, Profile, ManifestJson)
            && FFileHelper::SaveStringToFile(ManifestJson, *FPaths::Combine(OutputDirectory, TEXT("ActiveCharacter.json")));
        (bManifestWritten ? SucceededDeliverables : FailedDeliverables).Add(ECharacterCreatorDeliverable::Manifest);
    }

    TArray<ECharacterCreatorDeliverable> RequestedRealDeliverables;
    if (Profile.bGenerateBlueprint) RequestedRealDeliverables.Add(ECharacterCreatorDeliverable::Blueprint);
    if (Profile.bGenerateDataAsset) RequestedRealDeliverables.Add(ECharacterCreatorDeliverable::DataAsset);
    if (Profile.bGeneratePackage) RequestedRealDeliverables.Add(ECharacterCreatorDeliverable::Package);
    if (RequestedRealDeliverables.Num() > 0)
    {
        ExportProgress.Progress = 0.65f;
        ExportProgress.Stage = FText::FromString(TEXT("Generating Unreal assets and staged packages"));
        bool bRealWritten = false;
#if WITH_EDITOR
        FCharacterCreatorRealExportResult RealExport;
        bRealWritten = FCharacterCreatorEditorExportService::GenerateDeliverables(Appearance, Preset, Profile, OutputDirectory, RealExport);
        if (!bRealWritten && !RealExport.ErrorMessage.IsEmpty())
        {
            Session->SetStatusMessage(FText::FromString(RealExport.ErrorMessage));
        }
#else
        Session->SetStatusMessage(FText::FromString(TEXT("Real Unreal deliverables require an editor export run.")));
#endif
        (bRealWritten ? SucceededDeliverables : FailedDeliverables).Append(RequestedRealDeliverables);
    }

    const bool bWritten = FailedDeliverables.Num() == 0 && SucceededDeliverables.Num() == Deliverables.Num();
    const FString SummaryString = bWritten
        ? FString::Printf(TEXT("Export complete • %d deliverables written"), SucceededDeliverables.Num())
        : SucceededDeliverables.Num() > 0
            ? FString::Printf(TEXT("Export partially complete • %d written • %d failed"), SucceededDeliverables.Num(), FailedDeliverables.Num())
            : FString::Printf(TEXT("Export failed • %d written • %d failed"), SucceededDeliverables.Num(), FailedDeliverables.Num());
    const FText Summary = FText::FromString(SummaryString);
    Session->SetStatusMessage(Summary);
    AddExportHistoryDetailed(OutputDirectory, Deliverables, SucceededDeliverables, FailedDeliverables, Summary);
    ExportProgress.Progress = 1.0f;
    ExportProgress.Succeeded = SucceededDeliverables;
    ExportProgress.Failed = FailedDeliverables;
    ExportProgress.State = bWritten ? ECharacterCreatorOperationState::Completed : SucceededDeliverables.Num() > 0 ? ECharacterCreatorOperationState::CompletedWithErrors : ECharacterCreatorOperationState::Failed;
    ExportProgress.Stage = Summary;
    return bWritten;
}

bool UCharacterCreatorSubsystem::StartExportCurrentDeliverables(const FCharacterCreatorExportProfile& Profile, const FString& DestinationDirectory)
{
    if (!Session || ExportProgress.State == ECharacterCreatorOperationState::Queued || ExportProgress.State == ECharacterCreatorOperationState::Running)
    {
        if (Session) Session->SetStatusMessage(FText::FromString(TEXT("An export operation is already running.")));
        return false;
    }

    ExportProgress = FCharacterCreatorExportProgress();
    if (Profile.bIncludeMetadata) ExportProgress.Requested.Add(ECharacterCreatorDeliverable::Manifest);
    if (Profile.bGenerateBlueprint) ExportProgress.Requested.Add(ECharacterCreatorDeliverable::Blueprint);
    if (Profile.bGenerateDataAsset) ExportProgress.Requested.Add(ECharacterCreatorDeliverable::DataAsset);
    if (Profile.bGeneratePackage) ExportProgress.Requested.Add(ECharacterCreatorDeliverable::Package);
    if (ExportProgress.Requested.Num() == 0)
    {
        ExportProgress.State = ECharacterCreatorOperationState::Failed;
        ExportProgress.Progress = 1.0f;
        ExportProgress.Stage = FText::FromString(TEXT("No deliverables selected"));
        Session->SetStatusMessage(ExportProgress.Stage);
        return false;
    }

    TArray<FCharacterCreatorValidationIssue> Issues;
    FCharacterCreatorExportService::ValidateAppearance(Session->GetAppearanceStateNative(), Profile, Issues);
    if (FCharacterCreatorExportService::HasErrors(Issues))
    {
        ExportProgress.State = ECharacterCreatorOperationState::Failed;
        ExportProgress.Progress = 1.0f;
        ExportProgress.Failed = ExportProgress.Requested;
        ExportProgress.Stage = FText::FromString(TEXT("Export validation failed"));
        Session->SetStatusMessage(ExportProgress.Stage);
        return false;
    }

    QueuedExportProfile = Profile;
    QueuedExportDestination = DestinationDirectory;
    bExportCancellationRequested = false;
    ExportProgress.State = ECharacterCreatorOperationState::Queued;
    ExportProgress.Progress = 0.05f;
    ExportProgress.Stage = FText::FromString(TEXT("Export queued; cancellation is available before generation starts"));
    Session->SetStatusMessage(ExportProgress.Stage);
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(ExportTimerHandle, this, &UCharacterCreatorSubsystem::HandleQueuedExport, 0.5f, false);
        return true;
    }
    HandleQueuedExport();
    return ExportProgress.State == ECharacterCreatorOperationState::Completed;
}

bool UCharacterCreatorSubsystem::CancelExport()
{
    if (ExportProgress.State == ECharacterCreatorOperationState::Running)
    {
        if (Session) Session->SetStatusMessage(FText::FromString(TEXT("The atomic Unreal asset-save phase is already running and cannot be cancelled safely.")));
        return false;
    }
    if (ExportProgress.State != ECharacterCreatorOperationState::Queued) return false;
    bExportCancellationRequested = true;
    if (Session) Session->SetStatusMessage(FText::FromString(TEXT("Export cancellation requested before generation starts.")));
    return true;
}

void UCharacterCreatorSubsystem::HandleQueuedExport()
{
    if (!Session) return;
    if (bExportCancellationRequested)
    {
        ExportProgress.State = ECharacterCreatorOperationState::Cancelled;
        ExportProgress.Progress = 1.0f;
        ExportProgress.Failed = ExportProgress.Requested;
        ExportProgress.Stage = FText::FromString(TEXT("Export cancelled before generation"));
        Session->SetStatusMessage(ExportProgress.Stage);
        bExportCancellationRequested = false;
        return;
    }
    ExportCurrentDeliverables(QueuedExportProfile, QueuedExportDestination);
    bExportCancellationRequested = false;
}

TArray<FCharacterCreatorExportHistoryEntry> UCharacterCreatorSubsystem::GetExportHistory() const
{
    return Session ? Session->GetExportHistory() : TArray<FCharacterCreatorExportHistoryEntry>();
}

void UCharacterCreatorSubsystem::AddExportHistory(bool bSucceeded, const FString& DestinationDirectory, const TArray<ECharacterCreatorDeliverable>& Deliverables, const FText& Summary)
{
    if (!Session)
    {
        return;
    }
    FCharacterCreatorExportHistoryEntry Entry;
    Entry.TimestampUtc = FDateTime::UtcNow();
    Entry.DestinationDirectory = DestinationDirectory;
    Entry.Deliverables = Deliverables;
    Entry.bSucceeded = bSucceeded;
    Entry.SuccessfulDeliverables = bSucceeded ? Deliverables : TArray<ECharacterCreatorDeliverable>();
    Entry.FailedDeliverables = bSucceeded ? TArray<ECharacterCreatorDeliverable>() : Deliverables;
    Entry.Summary = Summary;
    Session->AddExportHistoryEntry(Entry);
}

void UCharacterCreatorSubsystem::AddExportHistoryDetailed(const FString& DestinationDirectory, const TArray<ECharacterCreatorDeliverable>& Requested, const TArray<ECharacterCreatorDeliverable>& Succeeded, const TArray<ECharacterCreatorDeliverable>& Failed, const FText& Summary)
{
    if (!Session) return;
    FCharacterCreatorExportHistoryEntry Entry;
    Entry.TimestampUtc = FDateTime::UtcNow();
    Entry.DestinationDirectory = DestinationDirectory;
    Entry.Deliverables = Requested;
    Entry.SuccessfulDeliverables = Succeeded;
    Entry.FailedDeliverables = Failed;
    Entry.bSucceeded = Failed.Num() == 0 && Succeeded.Num() == Requested.Num();
    Entry.bPartialSuccess = Succeeded.Num() > 0 && Failed.Num() > 0;
    Entry.Summary = Summary;
    Session->AddExportHistoryEntry(Entry);
}

bool UCharacterCreatorSubsystem::SavePreferences()
{
    if (!Session)
    {
        return false;
    }
    UCharacterCreatorSaveGame* SaveGame = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::CreateSaveGameObject(UCharacterCreatorSaveGame::StaticClass()));
    if (!SaveGame)
    {
        return false;
    }
    SaveGame->SaveVersion = UCharacterCreatorSaveGame::CurrentSaveVersion;
    SaveGame->SlotName = SettingsSlotName;
    SaveGame->LastSavedUtc = FDateTime::UtcNow();
    SaveGame->Settings = Session->GetSettings();
    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SettingsSlotName, 0);
    Session->SetStatusMessage(bSaved ? FText::FromString(TEXT("Preferences saved")) : FText::FromString(TEXT("Unable to save preferences")));
    return bSaved;
}

bool UCharacterCreatorSubsystem::LoadPreferences()
{
    if (!Session || !UGameplayStatics::DoesSaveGameExist(SettingsSlotName, 0))
    {
        return false;
    }
    UCharacterCreatorSaveGame* SaveGame = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::LoadGameFromSlot(SettingsSlotName, 0));
    if (!SaveGame || !SaveGame->IsCompatible())
    {
        return false;
    }
    Session->SetSettings(SaveGame->Settings);
    return true;
}

bool UCharacterCreatorSubsystem::SaveProjectCatalog()
{
    if (!Session)
    {
        return false;
    }
    UCharacterCreatorSaveGame* SaveGame = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::CreateSaveGameObject(UCharacterCreatorSaveGame::StaticClass()));
    if (!SaveGame)
    {
        return false;
    }
    SaveGame->SaveVersion = UCharacterCreatorSaveGame::CurrentSaveVersion;
    SaveGame->SlotName = ProjectCatalogSlotName;
    SaveGame->LastSavedUtc = FDateTime::UtcNow();
    SaveGame->ProjectBrowser = Session->GetProjectBrowserState();
    return UGameplayStatics::SaveGameToSlot(SaveGame, ProjectCatalogSlotName, 0);
}

bool UCharacterCreatorSubsystem::LoadProjectCatalog()
{
    if (!Session || !UGameplayStatics::DoesSaveGameExist(ProjectCatalogSlotName, 0))
    {
        return RefreshProjectBrowser();
    }
    UCharacterCreatorSaveGame* SaveGame = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::LoadGameFromSlot(ProjectCatalogSlotName, 0));
    if (!SaveGame || !SaveGame->IsCompatible() || SaveGame->ProjectBrowser.Projects.Num() == 0)
    {
        return RefreshProjectBrowser();
    }
    Session->SetProjectBrowserState(SaveGame->ProjectBrowser);
    return true;
}

bool UCharacterCreatorSubsystem::RefreshProjectBrowser()
{
    if (!Session)
    {
        return false;
    }
    FCharacterCreatorProjectBrowserState Browser = Session->GetProjectBrowserState();
    Browser.bLoading = false;
    Browser.Projects.RemoveAll([this](const FCharacterCreatorProjectRecord& Project)
    {
        return Project.bAutosave || Project.SlotName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(Project.SlotName, 0);
    });

    const FString SaveDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"));
    TArray<FString> SaveFiles;
    IFileManager::Get().FindFiles(SaveFiles, *FPaths::Combine(SaveDirectory, TEXT("CharacterCreator_Project_*.sav")), true, false);
    for (const FString& SaveFile : SaveFiles)
    {
        const FString SlotName = FPaths::GetBaseFilename(SaveFile);
        if (SlotName.Contains(TEXT("_Backup_")) || SlotName.EndsWith(TEXT("_Autosave"))) continue;
        if (Browser.Projects.ContainsByPredicate([&SlotName](const FCharacterCreatorProjectRecord& Project) { return Project.SlotName == SlotName; })) continue;
        if (UCharacterCreatorSaveGame* Save = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
        {
            if (!Save->IsCompatible()) continue;
            FCharacterCreatorProjectRecord& Project = Browser.Projects.AddDefaulted_GetRef();
            Project.SlotName = SlotName;
            Project.DisplayName = FText::FromString(Save->Settings.ProjectName.IsEmpty() ? SlotName : Save->Settings.ProjectName);
            Project.LastModifiedUtc = Save->LastSavedUtc;
            Project.AssetCount = Save->Appearance.AssetBrowser.Entries.Num();
            Project.BackupCount = PruneBackups(SlotName);
        }
    }
    Browser.Projects.Sort([](const FCharacterCreatorProjectRecord& A, const FCharacterCreatorProjectRecord& B)
    {
        return A.LastModifiedUtc > B.LastModifiedUtc;
    });
    if (Browser.SelectedSlotName.IsEmpty())
    {
        Browser.SelectedSlotName = !ActiveProjectSlotName.IsEmpty() ? ActiveProjectSlotName : Browser.Projects.Num() > 0 ? Browser.Projects[0].SlotName : FString();
    }
    Browser.ActiveSlotName = ActiveProjectSlotName;
    Session->SetProjectBrowserState(Browser);
    UpdateRecoveryState();
    return true;
}

bool UCharacterCreatorSubsystem::SelectProject(const FString& SlotName)
{
    if (!Session || !IsSafeCharacterCreatorSlot(SlotName) || !IsCataloguedProject(Session->GetProjectBrowserState(), SlotName))
    {
        return false;
    }
    if (SlotName == ActiveProjectSlotName) return true;
    if (Session->HasUnsavedChanges()) return StartPendingProjectChange(SlotName, FString());
    Session->SelectProject(SlotName);
    return LoadFromSlotInternal(SlotName, false);
}

FString UCharacterCreatorSubsystem::MakeUniqueProjectSlot(const FString& DisplayName) const
{
    FString Slug;
    bool bPreviousUnderscore = false;
    for (const TCHAR Character : DisplayName)
    {
        if (FChar::IsAlnum(Character))
        {
            Slug.AppendChar(Character);
            bPreviousUnderscore = false;
        }
        else if (!bPreviousUnderscore && !Slug.IsEmpty())
        {
            Slug.AppendChar(TEXT('_'));
            bPreviousUnderscore = true;
        }
    }
    Slug.RemoveFromEnd(TEXT("_"));
    if (Slug.IsEmpty()) Slug = TEXT("Project");
    FString Candidate = TEXT("CharacterCreator_Project_") + Slug.Left(64);
    int32 Suffix = 2;
    while (UGameplayStatics::DoesSaveGameExist(Candidate, 0))
    {
        Candidate = FString::Printf(TEXT("CharacterCreator_Project_%s_%d"), *Slug.Left(56), Suffix++);
    }
    return Candidate;
}

FString UCharacterCreatorSubsystem::GetAutosaveSlotName() const
{
    return ActiveProjectSlotName.IsEmpty() ? LegacyAutosaveSlotName : ActiveProjectSlotName + TEXT("_Autosave");
}

int32 UCharacterCreatorSubsystem::PruneBackups(const FString& SlotName)
{
    if (!Session || SlotName.IsEmpty()) return 0;
    const FString SaveDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"));
    TArray<FString> BackupFiles;
    IFileManager::Get().FindFiles(BackupFiles, *FPaths::Combine(SaveDirectory, SlotName + TEXT("_Backup_*.sav")), true, false);
    BackupFiles.Sort();
    const int32 RetainCount = Session->GetSettings().bCreateBackups ? Session->GetSettings().MaxBackupCount : 0;
    while (BackupFiles.Num() > RetainCount)
    {
        const FString Oldest = BackupFiles[0];
        IFileManager::Get().Delete(*FPaths::Combine(SaveDirectory, Oldest), false, true);
        BackupFiles.RemoveAt(0);
    }
    return BackupFiles.Num();
}

void UCharacterCreatorSubsystem::UpdateRecoveryState()
{
    if (!Session) return;
    FCharacterCreatorProjectBrowserState Browser = Session->GetProjectBrowserState();
    const FString AutosaveSlot = GetAutosaveSlotName();
    bool bAvailable = !bRecoveryDismissed && UGameplayStatics::DoesSaveGameExist(AutosaveSlot, 0);
    if (bAvailable && !ActiveProjectSlotName.IsEmpty() && UGameplayStatics::DoesSaveGameExist(ActiveProjectSlotName, 0))
    {
        const UCharacterCreatorSaveGame* Autosave = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::LoadGameFromSlot(AutosaveSlot, 0));
        const UCharacterCreatorSaveGame* Manual = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::LoadGameFromSlot(ActiveProjectSlotName, 0));
        bAvailable = Autosave && Manual && Autosave->LastSavedUtc > Manual->LastSavedUtc;
    }
    Browser.bAutosaveRecoveryAvailable = bAvailable;
    Browser.RecoverySlotName = bAvailable ? AutosaveSlot : FString();
    for (FCharacterCreatorProjectRecord& Project : Browser.Projects)
    {
        Project.bRecoveryAvailable = bAvailable && Project.SlotName == ActiveProjectSlotName;
        Project.BackupCount = PruneBackups(Project.SlotName);
    }
    Session->SetProjectBrowserState(Browser);
}

bool UCharacterCreatorSubsystem::ValidateImportDirectory(const FString& SourceDirectory, FCharacterCreatorImportProgress& OutProgress)
{
    const bool bValid = FCharacterCreatorImportService::ValidateDirectory(SourceDirectory, OutProgress);
    if (Session)
    {
        Session->SetImportProgress(OutProgress);
        Session->SetStatusMessage(OutProgress.Message);
    }
    return bValid;
}

bool UCharacterCreatorSubsystem::ScanAssetDirectory(const FString& SourceDirectory, const FString& SearchQuery, const FString& CategoryFilter, FCharacterCreatorImportProgress& OutProgress)
{
    if (!Session)
    {
        return false;
    }

    TArray<FCharacterCreatorAssetCatalogEntry> Entries;
    TSet<FString> FavoriteFiles;
    for (const FCharacterCreatorAssetCatalogEntry& ExistingEntry : Session->GetAssetBrowserState().Entries)
    {
        if (ExistingEntry.bFavorite) FavoriteFiles.Add(ExistingEntry.SourceFile);
    }
    const bool bMountedPath = SourceDirectory.StartsWith(TEXT("/Game"));
    const bool bValid = bMountedPath
        ? FCharacterCreatorImportService::ScanMountedPath(SourceDirectory, SearchQuery, CategoryFilter, Entries, OutProgress)
        : FCharacterCreatorImportService::ScanDirectory(SourceDirectory, SearchQuery, CategoryFilter, Entries, OutProgress);
    for (FCharacterCreatorAssetCatalogEntry& Entry : Entries)
    {
        Entry.bFavorite = FavoriteFiles.Contains(Entry.SourceFile);
    }
    FCharacterCreatorAssetBrowserState BrowserState = Session->GetAssetBrowserState();
    BrowserState.Entries = Entries;
    BrowserState.SearchQuery = SearchQuery;
    BrowserState.CategoryFilter = CategoryFilter;
    BrowserState.FilteredCount = Entries.Num();
    BrowserState.bCanImport = Entries.ContainsByPredicate([](const FCharacterCreatorAssetCatalogEntry& Entry)
    {
        return Entry.bSelected && Entry.Compatibility != ECharacterCreatorAssetCompatibility::Incompatible;
    });
    BrowserState.LastScanMessage = OutProgress.Message;
    Session->SetAssetBrowserState(BrowserState);
    Session->SetImportProgress(OutProgress);
    Session->SetStatusMessage(OutProgress.Message);
    return bValid;
}

bool UCharacterCreatorSubsystem::ImportSelectedAssets(const FCharacterCreatorImportOptions& Options, FCharacterCreatorImportProgress& OutProgress)
{
    if (!Session)
    {
        return false;
    }

    const FCharacterCreatorAssetBrowserState BrowserState = Session->GetAssetBrowserState();
    const bool bImported = FCharacterCreatorImportService::ImportAssets(BrowserState.Entries, Options, OutProgress);
    Session->SetImportProgress(OutProgress);
    Session->SetStatusMessage(OutProgress.Message);
    return bImported;
}

bool UCharacterCreatorSubsystem::StartImportSelectedAssets(const FCharacterCreatorImportOptions& Options)
{
    if (!Session || ActiveImportCancellation.IsValid())
    {
        if (Session) Session->SetStatusMessage(FText::FromString(TEXT("An import operation is already running.")));
        return false;
    }

    const TArray<FCharacterCreatorAssetCatalogEntry> Entries = Session->GetAssetBrowserState().Entries;
    if (!Entries.ContainsByPredicate([](const FCharacterCreatorAssetCatalogEntry& Entry) { return Entry.bSelected && Entry.Compatibility != ECharacterCreatorAssetCompatibility::Incompatible; }))
    {
        Session->SetStatusMessage(FText::FromString(TEXT("Select at least one compatible asset before importing.")));
        return false;
    }

    ActiveImportCancellation = MakeShared<TAtomic<bool>, ESPMode::ThreadSafe>(false);
    const TSharedPtr<TAtomic<bool>, ESPMode::ThreadSafe> Cancellation = ActiveImportCancellation;
    TWeakObjectPtr<UCharacterCreatorSubsystem> WeakThis(this);
    Async(EAsyncExecution::ThreadPool, [WeakThis, Cancellation, Entries, Options]()
    {
        FCharacterCreatorImportProgress FinalProgress;
        const auto ProgressCallback = [WeakThis](const FCharacterCreatorImportProgress& Progress)
        {
            AsyncTask(ENamedThreads::GameThread, [WeakThis, Progress]()
            {
                if (UCharacterCreatorSubsystem* Subsystem = WeakThis.Get())
                {
                    if (Subsystem->Session)
                    {
                        Subsystem->Session->SetImportProgress(Progress);
                        Subsystem->Session->SetStatusMessage(Progress.Message);
                    }
                }
            });
        };
        FCharacterCreatorImportService::ImportAssets(
            Entries,
            Options,
            FinalProgress,
            ProgressCallback,
            [Cancellation]() { return Cancellation->Load(); });
        AsyncTask(ENamedThreads::GameThread, [WeakThis, FinalProgress]()
        {
            if (UCharacterCreatorSubsystem* Subsystem = WeakThis.Get())
            {
                Subsystem->ActiveImportCancellation.Reset();
                if (Subsystem->Session)
                {
                    Subsystem->Session->SetImportProgress(FinalProgress);
                    Subsystem->Session->SetStatusMessage(FinalProgress.Message);
                }
            }
        });
    });
    return true;
}

bool UCharacterCreatorSubsystem::CancelImport()
{
    if (!ActiveImportCancellation.IsValid()) return false;
    ActiveImportCancellation->Store(true);
    if (Session) Session->SetStatusMessage(FText::FromString(TEXT("Cancelling import after the current file...")));
    return true;
}

bool UCharacterCreatorSubsystem::OpenExportOutputFolder() const
{
    if (!Session) return false;
    FString Directory = Session->GetSettings().ExportDestinationDirectory;
    if (Directory.IsEmpty()) Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CharacterCreator"), TEXT("Exports"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FPlatformProcess::ExploreFolder(*Directory);
    return IFileManager::Get().DirectoryExists(*Directory);
}

void UCharacterCreatorSubsystem::HandleAppearanceChanged(const FCharacterAppearanceState& NewAppearance)
{
    bAutosavePending = Session && Session->GetSettings().bAutosaveEnabled && NewAppearance.bHasUnsavedChanges;
    EnsureAutosaveTimer();
}

void UCharacterCreatorSubsystem::HandleSettingsChanged(const FCharacterCreatorSettings& NewSettings)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutosaveTimerHandle);
    }
    if (NewSettings.bAutosaveEnabled)
    {
        EnsureAutosaveTimer();
    }

    if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        GameUserSettings->SetFrameRateLimit(static_cast<float>(NewSettings.TargetFrameRate));
        GameUserSettings->ApplySettings(false);
    }

    if (bPreferencesReady)
    {
        SavePreferences();
    }
}

void UCharacterCreatorSubsystem::EnsureAutosaveTimer()
{
    if (!Session || !Session->GetSettings().bAutosaveEnabled)
    {
        return;
    }
    if (AutosaveTimerHandle.IsValid())
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(AutosaveTimerHandle, this, &UCharacterCreatorSubsystem::HandleAutosaveTick, static_cast<float>(Session->GetSettings().AutosaveIntervalSeconds), true);
    }
}

void UCharacterCreatorSubsystem::HandleAutosaveTick()
{
    if (bAutosavePending && Session && Session->GetSettings().bAutosaveEnabled)
    {
        SaveAutosave();
    }
}
