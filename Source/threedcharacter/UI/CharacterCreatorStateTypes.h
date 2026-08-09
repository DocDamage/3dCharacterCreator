#pragma once

#include "CoreMinimal.h"
#include "CharacterCreatorStateTypes.generated.h"

UENUM(BlueprintType)
enum class ECharacterCreatorDeliverable : uint8
{
    Manifest,
    Blueprint,
    DataAsset,
    Package
};

UENUM(BlueprintType)
enum class ECharacterCreatorUnsavedDecision : uint8
{
    Save,
    Discard,
    Cancel
};

UENUM(BlueprintType)
enum class ECharacterCreatorOperationState : uint8
{
    Idle,
    Queued,
    Running,
    Completed,
    CompletedWithErrors,
    Cancelled,
    Failed
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorExportProgress
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    ECharacterCreatorOperationState State = ECharacterCreatorOperationState::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    float Progress = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    FText Stage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    TArray<ECharacterCreatorDeliverable> Requested;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    TArray<ECharacterCreatorDeliverable> Succeeded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    TArray<ECharacterCreatorDeliverable> Failed;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings")
    int32 Version = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|General")
    FString ProjectName = TEXT("Character Creator Project");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|UI")
    float UIScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|UI")
    float TextScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Accessibility")
    bool bHighContrast = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Accessibility")
    bool bReducedMotion = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Accessibility")
    bool bColorSafeStatus = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Input")
    bool bGamepadEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Input")
    bool bDPadNavigation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Input")
    bool bAnalogNavigation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Controller")
    bool bInvertCameraY = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Controller")
    float CameraSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Performance")
    int32 TargetFrameRate = 60;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Performance")
    bool bUsePreviewLOD = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Performance")
    bool bAsyncAssetLoading = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Persistence")
    bool bAutosaveEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Persistence")
    int32 AutosaveIntervalSeconds = 15;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Backup")
    bool bCreateBackups = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Backup")
    int32 MaxBackupCount = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|ImportExport")
    bool bImportOverwrite = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|ImportExport")
    bool bExportManifest = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|ImportExport")
    bool bExportBlueprint = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|ImportExport")
    bool bExportDataAsset = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|ImportExport")
    bool bExportPackage = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|Backup")
    FString BackupDirectory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|ImportExport")
    FString ImportSourceDirectory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|ImportExport")
    FString ImportDestinationDirectory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Settings|ImportExport")
    FString ExportDestinationDirectory;

    void Sanitize()
    {
        UIScale = FMath::Clamp(UIScale, 0.75f, 1.50f);
        TextScale = FMath::Clamp(TextScale, 0.80f, 1.75f);
        CameraSensitivity = FMath::Clamp(CameraSensitivity, 0.25f, 3.0f);
        TargetFrameRate = FMath::Clamp(TargetFrameRate, 30, 240);
        AutosaveIntervalSeconds = FMath::Clamp(AutosaveIntervalSeconds, 5, 300);
        MaxBackupCount = FMath::Clamp(MaxBackupCount, 1, 50);
        if (ProjectName.IsEmpty()) ProjectName = TEXT("Character Creator Project");
    }
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorProjectRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    FString SlotName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    FDateTime LastModifiedUtc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    int32 AssetCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    bool bAutosave = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    bool bHasUnsavedChanges = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    int32 BackupCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    bool bRecoveryAvailable = false;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorProjectBrowserState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    FString SearchQuery;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    FString SelectedSlotName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    FString ActiveSlotName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    FString PendingSlotName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    FString RecoverySlotName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    TArray<FCharacterCreatorProjectRecord> Projects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    bool bLoading = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    bool bUnsavedConfirmationRequired = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Project")
    bool bAutosaveRecoveryAvailable = false;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorExportHistoryEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    FDateTime TimestampUtc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    FString DestinationDirectory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    TArray<ECharacterCreatorDeliverable> Deliverables;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    TArray<ECharacterCreatorDeliverable> SuccessfulDeliverables;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    TArray<ECharacterCreatorDeliverable> FailedDeliverables;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    bool bSucceeded = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    bool bPartialSuccess = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Export")
    FText Summary;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorFocusGraphNode
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Input")
    FName Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Input")
    FName Up;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Input")
    FName Down;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Input")
    FName Left;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Input")
    FName Right;
};
