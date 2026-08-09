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

UENUM(BlueprintType)
enum class ECharacterCreatorAssetCompatibility : uint8
{
    Unknown,
    Compatible,
    Warning,
    Incompatible,
    Conflict
};

UENUM(BlueprintType)
enum class ECharacterCreatorImportConflictResolution : uint8
{
    Skip,
    Overwrite,
    KeepBoth
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorAssetCatalogEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FString SourceFile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FString RelativePath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FString AssetName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FString Category;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FString Extension;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    int64 FileSize = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    ECharacterCreatorAssetCompatibility Compatibility = ECharacterCreatorAssetCompatibility::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    TArray<FText> DependencyWarnings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    bool bSelected = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    bool bConflict = false;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorImportOptions
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FString DestinationContentDirectory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    bool bOverwriteExisting = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    bool bCopyDependencies = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    ECharacterCreatorImportConflictResolution ConflictResolution = ECharacterCreatorImportConflictResolution::Skip;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorAssetBrowserState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    TArray<FCharacterCreatorAssetCatalogEntry> Entries;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FString SearchQuery;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FString CategoryFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FString SelectedAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    int32 FilteredCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    bool bCanImport = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FText LastScanMessage;
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
    int32 CompatibleFiles = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    int32 WarningFiles = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    int32 DependencyWarnings = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    float Progress = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Import")
    FText Message;
};

struct THREEDCHARACTER_API FCharacterCreatorImportService
{
    static bool ValidateDirectory(const FString& SourceDirectory, FCharacterCreatorImportProgress& OutProgress);
    static bool ScanDirectory(const FString& SourceDirectory, const FString& SearchQuery, const FString& CategoryFilter, TArray<FCharacterCreatorAssetCatalogEntry>& OutEntries, FCharacterCreatorImportProgress& OutProgress);
    static ECharacterCreatorAssetCompatibility AnalyzeCompatibility(const FString& FilePath, TArray<FText>& OutWarnings);
    static bool ImportAssets(const TArray<FCharacterCreatorAssetCatalogEntry>& Entries, const FCharacterCreatorImportOptions& Options, FCharacterCreatorImportProgress& OutProgress);
};
