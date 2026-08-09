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

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, 0);
    if (bSaved)
    {
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
        Session->SetStatusMessage(FirstError ? FirstError->Remediation : FText::FromString(TEXT("Export validation failed")));
        return false;
    }

    FString ManifestJson;
    if (!FCharacterCreatorExportService::BuildManifestJson(Session->GetAppearanceStateNative(), Session->GetActivePreset(), Profile, ManifestJson))
    {
        Session->SetStatusMessage(FText::FromString(TEXT("Unable to build the character export manifest")));
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
    return bWritten;
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

void UCharacterCreatorSubsystem::HandleAppearanceChanged(const FCharacterAppearanceState& NewAppearance)
{
    bAutosavePending = NewAppearance.bHasUnsavedChanges;
    EnsureAutosaveTimer();
}

void UCharacterCreatorSubsystem::EnsureAutosaveTimer()
{
    if (AutosaveTimerHandle.IsValid())
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(AutosaveTimerHandle, this, &UCharacterCreatorSubsystem::HandleAutosaveTick, 15.0f, true);
    }
}

void UCharacterCreatorSubsystem::HandleAutosaveTick()
{
    if (bAutosavePending)
    {
        SaveAutosave();
    }
}
