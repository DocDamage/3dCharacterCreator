#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Character.h"
#include "UI/CharacterCreatorExportService.h"
#include "CharacterCreatorGeneratedAssets.generated.h"

class UAnimInstance;
class UPhysicsAsset;
class USkeletalMesh;

/**
 * Runtime-readable character data produced by the editor export pipeline.
 * Keeping the complete session snapshot here makes the generated deliverable
 * useful to Blueprints and packaged builds without depending on JSON parsing.
 */
UCLASS(BlueprintType)
class THREEDCHARACTER_API UCharacterCreatorAppearanceDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Creator|Export")
    FGuid PresetId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Creator|Export")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Creator|Export")
    FCharacterAppearanceState Appearance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Creator|Export")
    FCharacterCreatorExportProfile ExportProfile;
};

/**
 * Blueprint parent used by generated character deliverables. It is intentionally
 * small: the generated Blueprint owns the exported defaults while this class
 * provides a stable runtime application path.
 */
UCLASS(Blueprintable)
class THREEDCHARACTER_API ACharacterCreatorGeneratedCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ACharacterCreatorGeneratedCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Creator|Export")
    TSoftObjectPtr<USkeletalMesh> CharacterSkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Creator|Export")
    TSoftClassPtr<UAnimInstance> CharacterAnimationInstance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Creator|Export")
    TSoftObjectPtr<UPhysicsAsset> CharacterPhysicsAsset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Creator|Export")
    TSoftObjectPtr<UCharacterCreatorAppearanceDataAsset> AppearanceData;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Export")
    void ApplyAppearanceData(UCharacterCreatorAppearanceDataAsset* InAppearanceData);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Export")
    void ApplyExportedAssetReferences();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
};
