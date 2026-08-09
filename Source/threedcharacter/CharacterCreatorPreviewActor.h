#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Actor.h"
#include "UI/CharacterCreatorSession.h"
#include "CharacterCreatorPreviewActor.generated.h"

class UCameraComponent;
class UPointLightComponent;
class USceneCaptureComponent2D;
class USceneComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UTextureRenderTarget2D;

UENUM(BlueprintType)
enum class ECharacterCreatorPreviewState : uint8
{
    Uninitialized,
    Loading,
    Ready,
    UsingFallback,
    Failed
};

UENUM(BlueprintType)
enum class ECharacterCreatorPreviewCameraMode : uint8
{
    Front,
    ThreeQuarter,
    Side,
    Portrait
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCharacterCreatorPreviewStateChanged, ECharacterCreatorPreviewState, const FText&);

UCLASS()
class THREEDCHARACTER_API ACharacterCreatorPreviewActor : public AActor
{
    GENERATED_BODY()

public:
    ACharacterCreatorPreviewActor();

    void InitializeWithSession(UCharacterCreatorSession* InSession);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Preview")
    UTextureRenderTarget2D* GetPreviewRenderTarget() const { return PreviewRenderTarget; }

    UFUNCTION(BlueprintPure, Category = "Character Creator|Preview")
    ECharacterCreatorPreviewState GetPreviewState() const { return PreviewState; }

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Preview")
    void SetCameraMode(ECharacterCreatorPreviewCameraMode NewMode);

    FOnCharacterCreatorPreviewStateChanged OnPreviewStateChanged;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void StartPreview();
    void RequestCharacterAssets(const FCharacterAppearanceState& Appearance);
    void HandleCharacterAssetsLoaded();
    void ApplyAppearance(const FCharacterAppearanceState& Appearance);
    void ApplyLoadedAssets(const FCharacterAppearanceState& Appearance);
    void ApplyMaterialParameters(const FCharacterAppearanceState& Appearance);
    void UseFallbackMesh(const FText& Reason, bool bFailedLoad);
    void SetPreviewState(ECharacterCreatorPreviewState NewState, const FText& Message);
    FName GetMorphTargetName(const FCharacterAppearanceState& Appearance, ECharacterCreatorParameter Parameter) const;
    bool HasMorphTarget(FName MorphTargetName) const;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> PreviewRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkeletalMeshComponent> CharacterMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkeletalMeshComponent> OutfitMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkeletalMeshComponent> HairMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> WeaponMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> PreviewCamera;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneCaptureComponent2D> SceneCapture;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> KeyLight;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> FillLight;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> RimLight;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;

    UPROPERTY()
    TObjectPtr<UCharacterCreatorSession> Session;

    TSharedPtr<FStreamableHandle> AssetLoadHandle;
    FCharacterAppearanceState PendingAppearance;
    FSoftObjectPath ActiveAssetPath;
    FSoftObjectPath ActiveOutfitAssetPath;
    FSoftObjectPath ActiveHairAssetPath;
    FSoftObjectPath ActiveWeaponAssetPath;
    ECharacterCreatorPreviewState PreviewState = ECharacterCreatorPreviewState::Uninitialized;
    bool bHasBegunPlay = false;
    bool bPreviewStarted = false;
    bool bRequestingAssets = false;
    bool bUsingFallbackMesh = false;
};
