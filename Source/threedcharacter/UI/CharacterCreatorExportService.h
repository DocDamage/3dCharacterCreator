#pragma once

#include "CoreMinimal.h"
#include "UI/CharacterCreatorSession.h"
#include "UI/CharacterCreatorStateTypes.h"
#include "CharacterCreatorExportService.generated.h"

UENUM(BlueprintType)
enum class ECharacterCreatorValidationSeverity : uint8
{
    Info,
    Warning,
    Error
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorValidationIssue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Validation")
    ECharacterCreatorValidationSeverity Severity = ECharacterCreatorValidationSeverity::Info;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Validation")
    FName Code;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Validation")
    FText Message;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Validation")
    FText Remediation;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorExportProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    int32 Version = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    bool bIncludeMesh = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    bool bIncludeMaterials = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    bool bIncludeAnimations = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    bool bIncludeMetadata = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    bool bGenerateBlueprint = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    bool bGenerateDataAsset = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    bool bGeneratePackage = false;
};

struct THREEDCHARACTER_API FCharacterCreatorExportService
{
    static void ValidateAppearance(const FCharacterAppearanceState& Appearance, const FCharacterCreatorExportProfile& Profile, TArray<FCharacterCreatorValidationIssue>& OutIssues);
    static bool HasErrors(const TArray<FCharacterCreatorValidationIssue>& Issues);
    static bool BuildManifestJson(const FCharacterAppearanceState& Appearance, const FCharacterPreset& Preset, const FCharacterCreatorExportProfile& Profile, FString& OutJson);
    static bool BuildBlueprintDescriptor(const FCharacterAppearanceState& Appearance, const FCharacterPreset& Preset, FString& OutJson);
    static bool BuildDataAssetDescriptor(const FCharacterAppearanceState& Appearance, const FCharacterPreset& Preset, FString& OutJson);
    static bool BuildPackageDescriptor(const FCharacterAppearanceState& Appearance, const FCharacterPreset& Preset, const FCharacterCreatorExportProfile& Profile, FString& OutJson);
};
