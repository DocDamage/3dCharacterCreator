#pragma once

#include "CoreMinimal.h"
#include "UI/CharacterCreatorExportService.h"

struct FCharacterCreatorRealExportResult
{
    bool bSucceeded = false;
    FString ErrorMessage;
    FSoftObjectPath BlueprintAsset;
    FSoftObjectPath DataAsset;
    FString GeneratedContentPath;
    FString StagedPackageDirectory;
    TArray<FString> StagedFiles;
};

/** Editor-only bridge that creates and saves real Unreal assets for the runtime export flow. */
struct THREEDCHARACTER_API FCharacterCreatorEditorExportService
{
    /** Build a real IK Retargeter pipeline and duplicate a source animation onto the target mesh. */
    static bool GenerateRetargetedAnimation(
        const FCharacterAppearanceState& Appearance,
        FSoftObjectPath& OutTargetAnimation,
        FString& OutErrorMessage);

    static bool GenerateDeliverables(
        const FCharacterAppearanceState& Appearance,
        const FCharacterPreset& Preset,
        const FCharacterCreatorExportProfile& Profile,
        const FString& DestinationDirectory,
        FCharacterCreatorRealExportResult& OutResult);
};
