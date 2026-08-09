#include "UI/CharacterCreatorSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UI/CharacterCreatorExportService.h"
#include "UI/CharacterCreatorImportService.h"
#include "UI/CharacterCreatorSaveGame.h"
#include "UI/CharacterCreatorSession.h"

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
    }
}

void UCharacterCreatorSubsystem::Deinitialize()
{
    if (bAutosavePending && Session)
    {
        SaveAutosave();
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutosaveTimerHandle);
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
    if (!Session || SlotName.IsEmpty())
    {
        return false;
    }

    if (Session->GetSettings().bCreateBackups && UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        if (UCharacterCreatorSaveGame* ExistingSave = Cast<UCharacterCreatorSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
        {
            const FString BackupSlot = FString::Printf(TEXT("%s_Backup_%s"), *SlotName, *FDateTime::UtcNow().ToString(TEXT("%Y%m%d%H%M%S")));
            UGameplayStatics::SaveGameToSlot(ExistingSave, BackupSlot, 0);
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
    ExistingProject->DisplayName = FText::FromString(Session->GetSettings().ProjectName);
    ExistingProject->LastModifiedUtc = SaveGame->LastSavedUtc;
    ExistingProject->AssetCount = Session->GetAppearanceStateNative().AssetBrowser.Entries.Num();
    ExistingProject->bAutosave = SlotName == AutosaveSlotName;
    ExistingProject->bHasUnsavedChanges = false;
    Browser.SelectedSlotName = SlotName;
    SaveGame->ProjectBrowser = Browser;
    Session->SetProjectBrowserState(Browser);

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, 0);
    if (bSaved)
    {
        SaveProjectCatalog();
        Session->ApplyAppearanceChanges();
        Session->SetStatusMessage(FText::FromString(FString::Printf(TEXT("Saved character to %s"), *SlotName)));
        bAutosavePending = false;
    }
    else
    {
        Session->SetStatusMessage(FText::FromString(TEXT("Unable to write character save")));
    }

    return bSaved;
}

bool UCharacterCreatorSubsystem::LoadFromSlot(const FString& SlotName)
{
    if (!Session || SlotName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        return false;
    }

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
    Session->SetProjectBrowserState(SaveGame->ProjectBrowser);
    Session->SetExportHistory(SaveGame->ExportHistory);
    Session->SetStatusMessage(FText::FromString(FString::Printf(TEXT("Loaded character from %s"), *SlotName)));
    bAutosavePending = false;
    return true;
}

bool UCharacterCreatorSubsystem::DoesSaveExist(const FString& SlotName) const
{
    return !SlotName.IsEmpty() && UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

bool UCharacterCreatorSubsystem::SaveAutosave()
{
    return SaveToSlot(AutosaveSlotName);
}

bool UCharacterCreatorSubsystem::LoadAutosave()
{
    return LoadFromSlot(AutosaveSlotName);
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

    const FString OutputPath = DestinationPath.IsEmpty()
        ? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CharacterCreator"), TEXT("Exports"), TEXT("ActiveCharacter.json"))
        : DestinationPath;
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
        return false;
    }

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
        return false;
    }

    const FString OutputDirectory = DestinationDirectory.IsEmpty()
        ? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CharacterCreator"), TEXT("Exports"))
        : DestinationDirectory;
    IFileManager::Get().MakeDirectory(*OutputDirectory, true);
    const FCharacterAppearanceState Appearance = Session->GetAppearanceStateNative();
    const FCharacterPreset Preset = Session->GetActivePreset();
    bool bWritten = true;

    auto WriteDescriptor = [&bWritten, &OutputDirectory](const FString& Filename, const FString& Content)
    {
        bWritten = bWritten && FFileHelper::SaveStringToFile(Content, *FPaths::Combine(OutputDirectory, Filename));
    };

    if (Profile.bIncludeMetadata)
    {
        FString ManifestJson;
        bWritten = FCharacterCreatorExportService::BuildManifestJson(Appearance, Preset, Profile, ManifestJson);
        if (bWritten) WriteDescriptor(TEXT("ActiveCharacter.json"), ManifestJson);
    }
    if (bWritten && Profile.bGenerateBlueprint)
    {
        FString BlueprintJson;
        bWritten = FCharacterCreatorExportService::BuildBlueprintDescriptor(Appearance, Preset, BlueprintJson);
        if (bWritten) WriteDescriptor(TEXT("ActiveCharacter.Blueprint.json"), BlueprintJson);
    }
    if (bWritten && Profile.bGenerateDataAsset)
    {
        FString DataAssetJson;
        bWritten = FCharacterCreatorExportService::BuildDataAssetDescriptor(Appearance, Preset, DataAssetJson);
        if (bWritten) WriteDescriptor(TEXT("ActiveCharacter.DataAsset.json"), DataAssetJson);
    }
    if (bWritten && Profile.bGeneratePackage)
    {
        FString PackageJson;
        bWritten = FCharacterCreatorExportService::BuildPackageDescriptor(Appearance, Preset, Profile, PackageJson);
        if (bWritten) WriteDescriptor(TEXT("ActiveCharacter.Package.json"), PackageJson);
    }

    const FText Summary = bWritten
        ? FText::FromString(FString::Printf(TEXT("Export complete • %d deliverables written"), Deliverables.Num()))
        : FText::FromString(TEXT("Export failed while writing a deliverable"));
    Session->SetStatusMessage(Summary);
    AddExportHistory(bWritten, OutputDirectory, Deliverables, Summary);
    return bWritten;
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
    if (Browser.Projects.Num() == 0)
    {
        FCharacterCreatorProjectRecord DefaultProject;
        DefaultProject.SlotName = AutosaveSlotName;
        DefaultProject.DisplayName = FText::FromString(Session->GetSettings().ProjectName);
        DefaultProject.LastModifiedUtc = FDateTime::UtcNow();
        DefaultProject.bAutosave = true;
        Browser.Projects.Add(DefaultProject);
    }
    if (Browser.SelectedSlotName.IsEmpty())
    {
        Browser.SelectedSlotName = Browser.Projects[0].SlotName;
    }
    Session->SetProjectBrowserState(Browser);
    return true;
}

bool UCharacterCreatorSubsystem::SelectProject(const FString& SlotName)
{
    if (!Session || SlotName.IsEmpty())
    {
        return false;
    }
    Session->SelectProject(SlotName);
    return LoadFromSlot(SlotName);
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
    const bool bValid = FCharacterCreatorImportService::ScanDirectory(SourceDirectory, SearchQuery, CategoryFilter, Entries, OutProgress);
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
