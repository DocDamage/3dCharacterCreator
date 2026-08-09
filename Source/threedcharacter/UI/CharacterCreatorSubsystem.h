#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/CharacterCreatorExportService.h"
#include "CharacterCreatorSubsystem.generated.h"

class UCharacterCreatorSession;
class UCharacterCreatorSaveGame;
struct FCharacterAppearanceState;

UCLASS()
class THREEDCHARACTER_API UCharacterCreatorSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category = "Character Creator")
    UCharacterCreatorSession* GetSession() const { return Session; }

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Persistence")
    bool SaveToSlot(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Persistence")
    bool SaveCurrentProject();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Persistence")
    bool SaveCurrentProjectAs(const FString& DisplayName);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Persistence")
    bool LoadFromSlot(const FString& SlotName);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Persistence")
    bool DoesSaveExist(const FString& SlotName) const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Persistence")
    bool SaveAutosave();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Persistence")
    bool LoadAutosave();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Project")
    bool CreateProject(const FString& DisplayName);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Project")
    bool RenameProject(const FString& SlotName, const FString& NewDisplayName);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Project")
    bool DuplicateProject(const FString& SlotName, const FString& NewDisplayName);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Project")
    bool DeleteProject(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Project")
    bool ResolvePendingProjectChange(ECharacterCreatorUnsavedDecision Decision);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Project")
    bool RecoverAutosave();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Project")
    void DismissAutosaveRecovery();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Export")
    bool ValidateCurrentAppearance(TArray<FCharacterCreatorValidationIssue>& OutIssues) const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Export")
    bool ExportCurrentManifest(const FCharacterCreatorExportProfile& Profile, const FString& DestinationPath);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Export")
    bool RunValidation();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Export")
    bool ApplyValidationFix(FName IssueCode);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Export")
    int32 ApplyAllValidationFixes();

    UFUNCTION(BlueprintPure, Category = "Character Creator|Export")
    TArray<FCharacterCreatorValidationIssue> GetLastValidationIssues() const { return LastValidationIssues; }

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Export")
    bool ExportCurrentDeliverables(const FCharacterCreatorExportProfile& Profile, const FString& DestinationDirectory);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Export")
    bool StartExportCurrentDeliverables(const FCharacterCreatorExportProfile& Profile, const FString& DestinationDirectory);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Export")
    bool CancelExport();

    UFUNCTION(BlueprintPure, Category = "Character Creator|Export")
    FCharacterCreatorExportProgress GetExportProgress() const { return ExportProgress; }

    UFUNCTION(BlueprintPure, Category = "Character Creator|Export")
    TArray<FCharacterCreatorExportHistoryEntry> GetExportHistory() const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Settings")
    bool SavePreferences();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Settings")
    bool LoadPreferences();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Project")
    bool RefreshProjectBrowser();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Project")
    bool SelectProject(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Import")
    bool ValidateImportDirectory(const FString& SourceDirectory, FCharacterCreatorImportProgress& OutProgress);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Import")
    bool ScanAssetDirectory(const FString& SourceDirectory, const FString& SearchQuery, const FString& CategoryFilter, FCharacterCreatorImportProgress& OutProgress);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Import")
    bool ImportSelectedAssets(const FCharacterCreatorImportOptions& Options, FCharacterCreatorImportProgress& OutProgress);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Import")
    bool StartImportSelectedAssets(const FCharacterCreatorImportOptions& Options);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Import")
    bool CancelImport();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Files")
    bool OpenExportOutputFolder() const;

private:
    void HandleAppearanceChanged(const FCharacterAppearanceState& NewAppearance);
    void HandleSettingsChanged(const FCharacterCreatorSettings& NewSettings);
    void HandleAutosaveTick();
    void EnsureAutosaveTimer();
    bool SaveProjectCatalog();
    bool LoadProjectCatalog();
    bool SaveToSlotInternal(const FString& SlotName, const FString& DisplayName, bool bAutosave, bool bSetActiveProject);
    bool LoadFromSlotInternal(const FString& SlotName, bool bRecoveredAutosave);
    bool StartPendingProjectChange(const FString& TargetSlotName, const FString& NewProjectName);
    bool CompletePendingProjectChange();
    FString MakeUniqueProjectSlot(const FString& DisplayName) const;
    FString GetAutosaveSlotName() const;
    int32 PruneBackups(const FString& SlotName);
    void UpdateRecoveryState();
    void HandleQueuedExport();
    void AddExportHistory(bool bSucceeded, const FString& DestinationDirectory, const TArray<ECharacterCreatorDeliverable>& Deliverables, const FText& Summary);
    void AddExportHistoryDetailed(const FString& DestinationDirectory, const TArray<ECharacterCreatorDeliverable>& Requested, const TArray<ECharacterCreatorDeliverable>& Succeeded, const TArray<ECharacterCreatorDeliverable>& Failed, const FText& Summary);

    UPROPERTY()
    TObjectPtr<UCharacterCreatorSession> Session;

    FTimerHandle AutosaveTimerHandle;
    FTimerHandle ExportTimerHandle;
    FString LegacyAutosaveSlotName = TEXT("CharacterCreator_Autosave");
    FString SettingsSlotName = TEXT("CharacterCreator_Settings");
    FString ProjectCatalogSlotName = TEXT("CharacterCreator_ProjectCatalog");
    bool bAutosavePending = false;
    bool bPreferencesReady = false;
    FString ActiveProjectSlotName;
    FString PendingProjectSlotName;
    FString PendingNewProjectName;
    bool bPendingProjectChange = false;
    bool bRecoveryDismissed = false;

    TArray<FCharacterCreatorValidationIssue> LastValidationIssues;
    TSharedPtr<TAtomic<bool>, ESPMode::ThreadSafe> ActiveImportCancellation;
    FCharacterCreatorExportProgress ExportProgress;
    FCharacterCreatorExportProfile QueuedExportProfile;
    FString QueuedExportDestination;
    bool bExportCancellationRequested = false;
};
