#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UI/CharacterCreatorSession.h"
#include "CharacterCreatorSaveGame.generated.h"

UCLASS()
class THREEDCHARACTER_API UCharacterCreatorSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentSaveVersion = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Save")
    int32 SaveVersion = CurrentSaveVersion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Save")
    FString SlotName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Save")
    FDateTime LastSavedUtc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Save")
    FCharacterAppearanceState Appearance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Save")
    FCharacterPreset ActivePreset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Save")
    TArray<FCharacterPreset> Presets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Save")
    FCharacterCreatorOnboardingState Onboarding;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Save")
    FCharacterCreatorSettings Settings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Save")
    FCharacterCreatorProjectBrowserState ProjectBrowser;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Save")
    TArray<FCharacterCreatorExportHistoryEntry> ExportHistory;

    bool IsCompatible() const { return SaveVersion > 0 && SaveVersion <= CurrentSaveVersion; }
};
