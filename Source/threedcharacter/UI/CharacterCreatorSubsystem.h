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

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Import")
    bool ValidateImportDirectory(const FString& SourceDirectory, FCharacterCreatorImportProgress& OutProgress);

private:
    void HandleAppearanceChanged(const FCharacterAppearanceState& NewAppearance);
    void HandleAutosaveTick();
    void EnsureAutosaveTimer();

    UPROPERTY()
    TObjectPtr<UCharacterCreatorSession> Session;

    FTimerHandle AutosaveTimerHandle;
    FString AutosaveSlotName = TEXT("CharacterCreator_Autosave");
    bool bAutosavePending = false;
};
