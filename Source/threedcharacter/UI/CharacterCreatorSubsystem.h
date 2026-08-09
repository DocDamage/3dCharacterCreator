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
    bool LoadFromSlot(const FString& SlotName);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Persistence")
    bool DoesSaveExist(const FString& SlotName) const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Persistence")
    bool SaveAutosave();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Persistence")
    bool LoadAutosave();

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

private:
    void HandleAppearanceChanged(const FCharacterAppearanceState& NewAppearance);
    void HandleSettingsChanged(const FCharacterCreatorSettings& NewSettings);
    void HandleAutosaveTick();
    void EnsureAutosaveTimer();
    bool SaveProjectCatalog();
    bool LoadProjectCatalog();
    void AddExportHistory(bool bSucceeded, const FString& DestinationDirectory, const TArray<ECharacterCreatorDeliverable>& Deliverables, const FText& Summary);

    UPROPERTY()
    TObjectPtr<UCharacterCreatorSession> Session;

    FTimerHandle AutosaveTimerHandle;
    FString AutosaveSlotName = TEXT("CharacterCreator_Autosave");
    FString SettingsSlotName = TEXT("CharacterCreator_Settings");
    FString ProjectCatalogSlotName = TEXT("CharacterCreator_ProjectCatalog");
    bool bAutosavePending = false;

    TArray<FCharacterCreatorValidationIssue> LastValidationIssues;
};
