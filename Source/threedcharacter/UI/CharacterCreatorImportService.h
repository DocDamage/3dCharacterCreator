#pragma once

#include "CoreMinimal.h"
#include "CharacterCreatorImportService.generated.h"

UENUM(BlueprintType)
enum class ECharacterCreatorImportState : uint8
{
    Idle,
    Validating,
    Ready,
    Failed
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorImportProgress
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    ECharacterCreatorImportState State = ECharacterCreatorImportState::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FString SourceDirectory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    int32 TotalFiles = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    int32 ValidFiles = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    int32 InvalidFiles = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    float Progress = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FText Message;
};

struct THREEDCHARACTER_API FCharacterCreatorImportService
{
    static bool ValidateDirectory(const FString& SourceDirectory, FCharacterCreatorImportProgress& OutProgress);
};
