#include "CharacterCreatorPreviewActor.h"

#include "Camera/CameraComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Animation/MorphTarget.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/Paths.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace CharacterCreatorPreview
{
    const TCHAR* FallbackSkeletalMeshPath = TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube");
}

ACharacterCreatorPreviewActor::ACharacterCreatorPreviewActor()
{
    PrimaryActorTick.bCanEverTick = false;

    PreviewRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewRoot"));
    SetRootComponent(PreviewRoot);

    CharacterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
    CharacterMesh->SetupAttachment(PreviewRoot);
    CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CharacterMesh->SetGenerateOverlapEvents(false);
    CharacterMesh->SetCastShadow(true);

    OutfitMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("OutfitMesh"));
    OutfitMesh->SetupAttachment(PreviewRoot);
    OutfitMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OutfitMesh->SetGenerateOverlapEvents(false);
    OutfitMesh->SetCastShadow(true);
    OutfitMesh->SetLeaderPoseComponent(CharacterMesh);

    HairMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HairMesh"));
    HairMesh->SetupAttachment(PreviewRoot);
    HairMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HairMesh->SetGenerateOverlapEvents(false);
    HairMesh->SetCastShadow(true);
    HairMesh->SetLeaderPoseComponent(CharacterMesh);

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(CharacterMesh, TEXT("hand_r"));
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMesh->SetGenerateOverlapEvents(false);
    WeaponMesh->SetCastShadow(true);
    WeaponMesh->SetVisibility(false);

    PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
    PreviewCamera->SetupAttachment(PreviewRoot);
    PreviewCamera->SetRelativeLocation(FVector(0.0f, -420.0f, 105.0f));
    PreviewCamera->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    PreviewCamera->FieldOfView = 28.0f;

    SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
    SceneCapture->SetupAttachment(PreviewRoot);
    SceneCapture->SetRelativeLocation(FVector(0.0f, -420.0f, 105.0f));
    SceneCapture->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    SceneCapture->FOVAngle = 28.0f;
    SceneCapture->bCaptureEveryFrame = true;
    SceneCapture->bCaptureOnMovement = true;
    SceneCapture->CaptureSource = SCS_FinalColorLDR;

    KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));
    KeyLight->SetupAttachment(PreviewRoot);
    KeyLight->SetRelativeLocation(FVector(120.0f, -160.0f, 260.0f));
    KeyLight->Intensity = 2400.0f;
    KeyLight->AttenuationRadius = 900.0f;
    KeyLight->LightColor = FColor(255, 240, 220);

    FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
    FillLight->SetupAttachment(PreviewRoot);
    FillLight->SetRelativeLocation(FVector(-60.0f, 180.0f, 150.0f));
    FillLight->Intensity = 1200.0f;
    FillLight->AttenuationRadius = 700.0f;
    FillLight->LightColor = FColor(180, 210, 255);

    RimLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RimLight"));
    RimLight->SetupAttachment(PreviewRoot);
    RimLight->SetRelativeLocation(FVector(-140.0f, 20.0f, 260.0f));
    RimLight->Intensity = 1800.0f;
    RimLight->AttenuationRadius = 800.0f;
    RimLight->LightColor = FColor(210, 180, 255);
}

void ACharacterCreatorPreviewActor::SetCameraMode(ECharacterCreatorPreviewCameraMode NewMode)
{
    FVector CameraLocation(0.0f, -420.0f, 105.0f);
    float FieldOfView = 28.0f;

    switch (NewMode)
    {
    case ECharacterCreatorPreviewCameraMode::ThreeQuarter:
        CameraLocation = FVector(300.0f, -360.0f, 115.0f);
        break;
    case ECharacterCreatorPreviewCameraMode::Side:
        CameraLocation = FVector(420.0f, 0.0f, 105.0f);
        break;
    case ECharacterCreatorPreviewCameraMode::Portrait:
        CameraLocation = FVector(0.0f, -320.0f, 155.0f);
        FieldOfView = 32.0f;
        break;
    case ECharacterCreatorPreviewCameraMode::Front:
    default:
        break;
    }

    const FRotator AimRotation = (FVector(0.0f, 0.0f, 105.0f) - CameraLocation).Rotation();
    if (PreviewCamera)
    {
        PreviewCamera->SetRelativeLocation(CameraLocation);
        PreviewCamera->SetRelativeRotation(AimRotation);
        PreviewCamera->FieldOfView = FieldOfView;
    }
    if (SceneCapture)
    {
        SceneCapture->SetRelativeLocation(CameraLocation);
        SceneCapture->SetRelativeRotation(AimRotation);
        SceneCapture->FOVAngle = FieldOfView;
    }
}

void ACharacterCreatorPreviewActor::SetCameraOrbit(float Yaw, float Pitch, float Zoom)
{
    const float SafeYaw = FMath::Clamp(Yaw, -180.0f, 180.0f);
    const float SafePitch = FMath::Clamp(Pitch, -80.0f, 80.0f);
    const float SafeZoom = FMath::Clamp(Zoom, 0.5f, 2.0f);
    const FVector Target(0.0f, 0.0f, 105.0f);
    const FVector CameraOffset = FRotator(SafePitch, SafeYaw, 0.0f).RotateVector(FVector(0.0f, -420.0f, 0.0f));
    const FVector CameraLocation = Target + CameraOffset;
    const FRotator AimRotation = (Target - CameraLocation).Rotation();
    const float FieldOfView = 28.0f / SafeZoom;
    if (PreviewCamera)
    {
        PreviewCamera->SetRelativeLocation(CameraLocation);
        PreviewCamera->SetRelativeRotation(AimRotation);
        PreviewCamera->FieldOfView = FieldOfView;
    }
    if (SceneCapture)
    {
        SceneCapture->SetRelativeLocation(CameraLocation);
        SceneCapture->SetRelativeRotation(AimRotation);
        SceneCapture->FOVAngle = FieldOfView;
    }
}

bool ACharacterCreatorPreviewActor::CapturePortrait(const FString& OutputPath, int32 Width, int32 Height, FString& OutErrorMessage)
{
    OutErrorMessage.Reset();
    if (!SceneCapture || !PreviewRenderTarget || OutputPath.IsEmpty())
    {
        OutErrorMessage = TEXT("The live preview render target is not ready.");
        return false;
    }

    const int32 SafeWidth = FMath::Clamp(Width, 128, 4096);
    const int32 SafeHeight = FMath::Clamp(Height, 128, 4096);
    const FString OutputDirectory = FPaths::GetPath(OutputPath);
    const FString OutputFilename = FPaths::GetBaseFilename(OutputPath);
    if (OutputDirectory.IsEmpty() || OutputFilename.IsEmpty())
    {
        OutErrorMessage = TEXT("Choose a complete portrait output path.");
        return false;
    }

    IFileManager::Get().MakeDirectory(*OutputDirectory, true);
    const int32 PreviousWidth = PreviewRenderTarget->SizeX;
    const int32 PreviousHeight = PreviewRenderTarget->SizeY;
    PreviewRenderTarget->ResizeTarget(SafeWidth, SafeHeight);
    PreviewRenderTarget->UpdateResourceImmediate(true);
    SceneCapture->CaptureScene();
    UKismetRenderingLibrary::ExportRenderTarget(this, PreviewRenderTarget, OutputDirectory, OutputFilename);
    PreviewRenderTarget->ResizeTarget(PreviousWidth, PreviousHeight);
    PreviewRenderTarget->UpdateResourceImmediate(true);
    SceneCapture->CaptureScene();

    const FString WrittenPath = FPaths::Combine(OutputDirectory, OutputFilename + TEXT(".png"));
    if (!IFileManager::Get().FileExists(*WrittenPath) || IFileManager::Get().FileSize(*WrittenPath) <= 0)
    {
        OutErrorMessage = FString::Printf(TEXT("The render target export did not create %s."), *WrittenPath);
        return false;
    }
    if (Session)
    {
        Session->PreparePortraitCapture(WrittenPath, SafeWidth, SafeHeight, FName(TEXT("PNG")));
        Session->SetStatusMessage(FText::FromString(FString::Printf(TEXT("Portrait captured to %s"), *WrittenPath)));
    }
    return true;
}

void ACharacterCreatorPreviewActor::InitializeWithSession(UCharacterCreatorSession* InSession)
{
    if (Session)
    {
        Session->OnAppearanceChanged.RemoveAll(this);
    }

    Session = InSession;
    if (Session)
    {
        Session->OnAppearanceChanged.AddUObject(this, &ACharacterCreatorPreviewActor::ApplyAppearance);
    }

    if (bHasBegunPlay)
    {
        StartPreview();
    }
}

void ACharacterCreatorPreviewActor::ApplyPerformanceSettings(const FCharacterCreatorSettings& Settings)
{
    const int32 ForcedPreviewLOD = Settings.bUsePreviewLOD ? 1 : 0;
    if (CharacterMesh)
    {
        CharacterMesh->SetForcedLOD(ForcedPreviewLOD);
    }
    if (OutfitMesh)
    {
        OutfitMesh->SetForcedLOD(ForcedPreviewLOD);
    }
    if (HairMesh)
    {
        HairMesh->SetForcedLOD(ForcedPreviewLOD);
    }

    if (Settings.bReducedMotion && PreviewCamera)
    {
        PreviewCamera->PostProcessSettings.bOverride_MotionBlurAmount = true;
        PreviewCamera->PostProcessSettings.MotionBlurAmount = 0.0f;
    }
}

void ACharacterCreatorPreviewActor::BeginPlay()
{
    Super::BeginPlay();
    bHasBegunPlay = true;

    if (Session)
    {
        StartPreview();
    }
}

void ACharacterCreatorPreviewActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Session)
    {
        Session->OnAppearanceChanged.RemoveAll(this);
    }

    if (AssetLoadHandle.IsValid())
    {
        AssetLoadHandle->ReleaseHandle();
        AssetLoadHandle.Reset();
    }

    Super::EndPlay(EndPlayReason);
}

void ACharacterCreatorPreviewActor::StartPreview()
{
    if (bPreviewStarted)
    {
        return;
    }

    bPreviewStarted = true;

    PreviewRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("CharacterCreatorPreviewRenderTarget"));
    PreviewRenderTarget->ClearColor = FLinearColor(0.015f, 0.025f, 0.035f, 1.0f);
    PreviewRenderTarget->InitAutoFormat(512, 768);
    PreviewRenderTarget->UpdateResourceImmediate(true);
    SceneCapture->TextureTarget = PreviewRenderTarget;

    const FCharacterAppearanceState Appearance = Session ? Session->GetAppearanceStateNative() : FCharacterAppearanceState();
    PendingAppearance = Appearance;
    ApplyAppearance(Appearance);
}

void ACharacterCreatorPreviewActor::RequestCharacterAssets(const FCharacterAppearanceState& Appearance)
{
    if (bRequestingAssets || AssetLoadHandle.IsValid())
    {
        return;
    }

    if (Appearance.Assets.SkeletalMesh.IsNull())
    {
        UseFallbackMesh(FText::FromString(TEXT("No character mesh assigned. Using the engine fallback preview.")), false);
        return;
    }

    ActiveAssetPath = Appearance.Assets.SkeletalMesh.ToSoftObjectPath();
    ActiveOutfitAssetPath = Appearance.Loadout.OutfitMesh;
    ActiveHairAssetPath = Appearance.Loadout.HairMesh;
    ActiveWeaponAssetPath = Appearance.Loadout.WeaponMesh;
    bUsingFallbackMesh = false;

    TArray<FSoftObjectPath> AssetPaths;
    AssetPaths.Add(Appearance.Assets.SkeletalMesh.ToSoftObjectPath());

    if (!Appearance.Assets.AnimationInstanceClass.IsNull())
    {
        AssetPaths.Add(Appearance.Assets.AnimationInstanceClass.ToSoftObjectPath());
    }

    if (!Appearance.Assets.PreviewAnimation.IsNull())
    {
        AssetPaths.Add(Appearance.Assets.PreviewAnimation.ToSoftObjectPath());
    }

    if (!Appearance.Assets.BaseMaterial.IsNull())
    {
        AssetPaths.Add(Appearance.Assets.BaseMaterial.ToSoftObjectPath());
    }

    if (!Appearance.Loadout.OutfitMesh.IsNull())
    {
        AssetPaths.Add(Appearance.Loadout.OutfitMesh);
    }

    if (!Appearance.Loadout.HairMesh.IsNull())
    {
        AssetPaths.Add(Appearance.Loadout.HairMesh);
    }

    if (!Appearance.Loadout.WeaponMesh.IsNull())
    {
        AssetPaths.Add(Appearance.Loadout.WeaponMesh);
    }

    SetPreviewState(ECharacterCreatorPreviewState::Loading, FText::FromString(TEXT("Loading character assets...")));
    bRequestingAssets = true;
    if (Session)
    {
        Session->SetAssetLoadState(ECharacterCreatorAssetLoadState::Loading);
    }

    AssetLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        AssetPaths,
        FStreamableDelegate::CreateUObject(this, &ACharacterCreatorPreviewActor::HandleCharacterAssetsLoaded));
    bRequestingAssets = false;

    if (!AssetLoadHandle.IsValid())
    {
        UseFallbackMesh(FText::FromString(TEXT("Character assets could not start loading. Using the fallback preview.")), true);
    }
}

void ACharacterCreatorPreviewActor::HandleCharacterAssetsLoaded()
{
    AssetLoadHandle.Reset();

    if (USkeletalMesh* LoadedMesh = PendingAppearance.Assets.SkeletalMesh.Get())
    {
        CharacterMesh->SetSkeletalMesh(LoadedMesh);
        ApplyLoadedAssets(PendingAppearance);
        ApplyAppearance(PendingAppearance);
        SetPreviewState(ECharacterCreatorPreviewState::Ready, FText::FromString(TEXT("Character preview ready")));
        if (Session)
        {
            Session->SetAssetLoadState(ECharacterCreatorAssetLoadState::Loaded);
        }
        return;
    }

    UseFallbackMesh(FText::FromString(TEXT("Character mesh failed to load. Using the fallback preview.")), true);
}

void ACharacterCreatorPreviewActor::ApplyAppearance(const FCharacterAppearanceState& Appearance)
{
    PendingAppearance = Appearance;

    if (!bPreviewStarted)
    {
        return;
    }

    const FSoftObjectPath RequestedAssetPath = Appearance.Assets.SkeletalMesh.ToSoftObjectPath();
    const bool bLoadoutChanged = Appearance.Loadout.OutfitMesh != ActiveOutfitAssetPath
        || Appearance.Loadout.HairMesh != ActiveHairAssetPath
        || Appearance.Loadout.WeaponMesh != ActiveWeaponAssetPath;
    if (RequestedAssetPath != ActiveAssetPath || bLoadoutChanged)
    {
        bUsingFallbackMesh = false;
        ActiveAssetPath = RequestedAssetPath;
        ActiveOutfitAssetPath = Appearance.Loadout.OutfitMesh;
        ActiveHairAssetPath = Appearance.Loadout.HairMesh;
        ActiveWeaponAssetPath = Appearance.Loadout.WeaponMesh;
    }

    if (!Appearance.Assets.SkeletalMesh.IsNull() && !bUsingFallbackMesh)
    {
        if (Appearance.Assets.SkeletalMesh.Get())
        {
            if (CharacterMesh->GetSkeletalMeshAsset() != Appearance.Assets.SkeletalMesh.Get())
            {
                CharacterMesh->SetSkeletalMesh(Appearance.Assets.SkeletalMesh.Get());
                ApplyLoadedAssets(Appearance);
            }
        }
        else if (!AssetLoadHandle.IsValid() && !bRequestingAssets)
        {
            RequestCharacterAssets(Appearance);
        }
    }
    else if (!CharacterMesh->GetSkeletalMeshAsset())
    {
        RequestCharacterAssets(Appearance);
    }

    if (bLoadoutChanged && CharacterMesh->GetSkeletalMeshAsset())
    {
        ApplyLoadedAssets(Appearance);
    }

    const bool bMissingOutfit = !Appearance.Loadout.OutfitMesh.IsNull() && !Appearance.Loadout.OutfitMesh.ResolveObject();
    const bool bMissingHair = !Appearance.Loadout.HairMesh.IsNull() && !Appearance.Loadout.HairMesh.ResolveObject();
    const bool bMissingWeapon = !Appearance.Loadout.WeaponMesh.IsNull() && !Appearance.Loadout.WeaponMesh.ResolveObject();
    if ((bMissingOutfit || bMissingHair || bMissingWeapon) && !AssetLoadHandle.IsValid() && !bRequestingAssets && !bUsingFallbackMesh)
    {
        RequestCharacterAssets(Appearance);
    }

    const float WidthScale = 0.88f + (Appearance.ShoulderWidth * 0.24f);
    const float DepthScale = 0.90f + (Appearance.ShoulderWidth * 0.18f);
    const float HeightScale = 0.86f + (Appearance.Height * 0.28f);
    CharacterMesh->SetRelativeScale3D(FVector(WidthScale, DepthScale, HeightScale));

    const TPair<ECharacterCreatorParameter, float> MorphValues[] = {
        {ECharacterCreatorParameter::Height, Appearance.Height},
        {ECharacterCreatorParameter::ShoulderWidth, Appearance.ShoulderWidth},
        {ECharacterCreatorParameter::ArmLength, Appearance.ArmLength},
        {ECharacterCreatorParameter::LegLength, Appearance.LegLength},
        {ECharacterCreatorParameter::HeadScale, Appearance.HeadScale},
        {ECharacterCreatorParameter::BrowHeight, Appearance.BrowHeight},
        {ECharacterCreatorParameter::JawWidth, Appearance.JawWidth},
        {ECharacterCreatorParameter::NoseWidth, Appearance.NoseWidth},
        {ECharacterCreatorParameter::EyeSize, Appearance.EyeSize},
        {ECharacterCreatorParameter::MouthWidth, Appearance.MouthWidth}
    };

    for (const TPair<ECharacterCreatorParameter, float>& MorphValue : MorphValues)
    {
        const FName MorphTargetName = GetMorphTargetName(Appearance, MorphValue.Key);
        if (!MorphTargetName.IsNone())
        {
            const float CenteredMorphWeight = FMath::Clamp((MorphValue.Value - 0.5f) * 2.0f, -1.0f, 1.0f);
            CharacterMesh->SetMorphTarget(MorphTargetName, CenteredMorphWeight);
        }
    }

    const TPair<ECharacterCreatorFaceAdvancedParameter, float> AdvancedMorphValues[] = {
        {ECharacterCreatorFaceAdvancedParameter::CheekWidth, Appearance.AdvancedFace.CheekWidth},
        {ECharacterCreatorFaceAdvancedParameter::ChinDepth, Appearance.AdvancedFace.ChinDepth},
        {ECharacterCreatorFaceAdvancedParameter::EyeSpacing, Appearance.AdvancedFace.EyeSpacing},
        {ECharacterCreatorFaceAdvancedParameter::EyeHeight, Appearance.AdvancedFace.EyeHeight},
        {ECharacterCreatorFaceAdvancedParameter::LipFullness, Appearance.AdvancedFace.LipFullness},
        {ECharacterCreatorFaceAdvancedParameter::EarSize, Appearance.AdvancedFace.EarSize},
        {ECharacterCreatorFaceAdvancedParameter::NasolabialDepth, Appearance.AdvancedFace.NasolabialDepth}
    };

    for (const TPair<ECharacterCreatorFaceAdvancedParameter, float>& MorphValue : AdvancedMorphValues)
    {
        ECharacterCreatorParameter CandidateParameter = ECharacterCreatorParameter::JawWidth;
        switch (MorphValue.Key)
        {
        case ECharacterCreatorFaceAdvancedParameter::CheekWidth: CandidateParameter = ECharacterCreatorParameter::ShoulderWidth; break;
        case ECharacterCreatorFaceAdvancedParameter::ChinDepth: CandidateParameter = ECharacterCreatorParameter::JawWidth; break;
        case ECharacterCreatorFaceAdvancedParameter::EyeSpacing:
        case ECharacterCreatorFaceAdvancedParameter::EyeHeight: CandidateParameter = ECharacterCreatorParameter::EyeSize; break;
        case ECharacterCreatorFaceAdvancedParameter::LipFullness: CandidateParameter = ECharacterCreatorParameter::MouthWidth; break;
        case ECharacterCreatorFaceAdvancedParameter::EarSize: CandidateParameter = ECharacterCreatorParameter::HeadScale; break;
        case ECharacterCreatorFaceAdvancedParameter::NasolabialDepth: CandidateParameter = ECharacterCreatorParameter::NoseWidth; break;
        default: break;
        }

        const FName MorphTargetName = GetMorphTargetName(Appearance, CandidateParameter);
        if (!MorphTargetName.IsNone())
        {
            const float CenteredMorphWeight = FMath::Clamp((MorphValue.Value - 0.5f) * 2.0f, -1.0f, 1.0f);
            CharacterMesh->SetMorphTarget(MorphTargetName, CenteredMorphWeight);
        }
    }

    ApplyMaterialParameters(Appearance);

    ECharacterCreatorPreviewCameraMode CameraMode = ECharacterCreatorPreviewCameraMode::Front;
    if (Appearance.PreviewTesting.Studio.CameraMode == FName(TEXT("ThreeQuarter"))) CameraMode = ECharacterCreatorPreviewCameraMode::ThreeQuarter;
    if (Appearance.PreviewTesting.Studio.CameraMode == FName(TEXT("Side"))) CameraMode = ECharacterCreatorPreviewCameraMode::Side;
    if (Appearance.PreviewTesting.Studio.CameraMode == FName(TEXT("Portrait"))) CameraMode = ECharacterCreatorPreviewCameraMode::Portrait;
    SetCameraMode(CameraMode);
    const float Zoom = FMath::Clamp(Appearance.PreviewTesting.Studio.Zoom, 0.5f, 2.0f);
    if (PreviewCamera)
    {
        PreviewCamera->FieldOfView /= Zoom;
    }
    if (SceneCapture)
    {
        SceneCapture->FOVAngle /= Zoom;
    }

    if (Appearance.PreviewTesting.Studio.LightingProfile == FName(TEXT("Dramatic")))
    {
        KeyLight->Intensity = 3000.0f;
        FillLight->Intensity = 650.0f;
        RimLight->Intensity = 2400.0f;
    }
    else if (Appearance.PreviewTesting.Studio.LightingProfile == FName(TEXT("Soft")))
    {
        KeyLight->Intensity = 1700.0f;
        FillLight->Intensity = 1500.0f;
        RimLight->Intensity = 900.0f;
    }
    else
    {
        KeyLight->Intensity = 2400.0f;
        FillLight->Intensity = 1200.0f;
        RimLight->Intensity = 1800.0f;
    }
}

void ACharacterCreatorPreviewActor::ApplyLoadedAssets(const FCharacterAppearanceState& Appearance)
{
    if (UClass* AnimationClass = Appearance.Assets.AnimationInstanceClass.Get())
    {
        CharacterMesh->SetAnimInstanceClass(AnimationClass);
    }

    if (UAnimSequenceBase* TargetAnimation = Cast<UAnimSequenceBase>(Appearance.Animation.TargetAnimation.ResolveObject()))
    {
        CharacterMesh->PlayAnimation(TargetAnimation, true);
    }
    else if (UAnimSequenceBase* PreviewAnimation = Appearance.Assets.PreviewAnimation.Get())
    {
        CharacterMesh->PlayAnimation(PreviewAnimation, true);
    }

    if (OutfitMesh)
    {
        USkeletalMesh* LoadedOutfit = Cast<USkeletalMesh>(Appearance.Loadout.OutfitMesh.ResolveObject());
        OutfitMesh->SetSkeletalMesh(LoadedOutfit);
        OutfitMesh->SetVisibility(Appearance.Loadout.bOutfitEnabled && LoadedOutfit != nullptr);
    }

    if (HairMesh)
    {
        USkeletalMesh* LoadedHair = Cast<USkeletalMesh>(Appearance.Loadout.HairMesh.ResolveObject());
        HairMesh->SetSkeletalMesh(LoadedHair);
        HairMesh->SetVisibility(Appearance.Loadout.bHairEnabled && LoadedHair != nullptr);
    }

    if (WeaponMesh)
    {
        UStaticMesh* LoadedWeapon = Cast<UStaticMesh>(Appearance.Loadout.WeaponMesh.ResolveObject());
        WeaponMesh->SetStaticMesh(LoadedWeapon);
        const FCharacterCreatorWeaponSetup MainHandWeapon = Appearance.Equipment.GetWeapon(ECharacterCreatorWeaponSlot::MainHand);
        const FName SocketName = MainHandWeapon.SocketName.IsNone() ? Appearance.IK.RightHandSocket : MainHandWeapon.SocketName;
        if (CharacterMesh && !SocketName.IsNone())
        {
            WeaponMesh->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
        }
        WeaponMesh->SetRelativeLocation(MainHandWeapon.GripLocation);
        WeaponMesh->SetRelativeRotation(MainHandWeapon.GripRotation);
        WeaponMesh->SetRelativeScale3D(MainHandWeapon.GripScale);
        WeaponMesh->SetVisibility(Appearance.Loadout.bWeaponEnabled && MainHandWeapon.bEnabled && LoadedWeapon != nullptr);
    }
}

void ACharacterCreatorPreviewActor::ApplyMaterialParameters(const FCharacterAppearanceState& Appearance)
{
    if (UMaterialInterface* BaseMaterial = Appearance.Assets.BaseMaterial.Get())
    {
        const int32 MaterialCount = CharacterMesh->GetNumMaterials();
        for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
        {
            CharacterMesh->SetMaterial(MaterialIndex, BaseMaterial);
        }
    }

    const int32 MaterialCount = CharacterMesh->GetNumMaterials();
    for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
    {
        UMaterialInstanceDynamic* DynamicMaterial = CharacterMesh->CreateDynamicMaterialInstance(MaterialIndex);
        if (DynamicMaterial)
        {
            DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Appearance.SkinColor);
            DynamicMaterial->SetVectorParameterValue(TEXT("Base Color"), Appearance.SkinColor);
            DynamicMaterial->SetScalarParameterValue(TEXT("SkinRoughness"), Appearance.Grooming.SkinRoughness);
            DynamicMaterial->SetScalarParameterValue(TEXT("SkinDetail"), Appearance.Grooming.SkinDetail);
        }
    }

    const auto ApplyColorToComponent = [&Appearance](UMeshComponent* Component, const FLinearColor& Color)
    {
        if (!Component)
        {
            return;
        }

        const int32 ComponentMaterialCount = Component->GetNumMaterials();
        for (int32 MaterialIndex = 0; MaterialIndex < ComponentMaterialCount; ++MaterialIndex)
        {
            if (UMaterialInstanceDynamic* DynamicMaterial = Component->CreateDynamicMaterialInstance(MaterialIndex))
            {
                DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
                DynamicMaterial->SetVectorParameterValue(TEXT("Base Color"), Color);
                DynamicMaterial->SetScalarParameterValue(TEXT("HairLength"), Appearance.Grooming.HairLength);
                DynamicMaterial->SetScalarParameterValue(TEXT("HairDensity"), Appearance.Grooming.HairDensity);
                DynamicMaterial->SetScalarParameterValue(TEXT("HairRoughness"), Appearance.Grooming.HairRoughness);
            }
        }
    };

    ApplyColorToComponent(OutfitMesh, Appearance.PrimaryOutfitColor);
    ApplyColorToComponent(HairMesh, Appearance.HairColor);
    ApplyColorToComponent(WeaponMesh, Appearance.SecondaryOutfitColor);

    const auto ApplyMaterialState = [](UMeshComponent* Component, const FCharacterCreatorMaterialSlotState& State)
    {
        if (!Component)
        {
            return;
        }

        for (int32 MaterialIndex = 0; MaterialIndex < Component->GetNumMaterials(); ++MaterialIndex)
        {
            if (UMaterialInstanceDynamic* DynamicMaterial = Component->CreateDynamicMaterialInstance(MaterialIndex))
            {
                DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), State.Tint);
                DynamicMaterial->SetVectorParameterValue(TEXT("Base Color"), State.Tint);
                DynamicMaterial->SetScalarParameterValue(TEXT("Metallic"), State.Metallic);
                DynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), State.Roughness);
                DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), State.EmissiveStrength);
                DynamicMaterial->SetScalarParameterValue(TEXT("PatternScale"), State.PatternScale);
            }
        }
    };

    if (const FCharacterCreatorMaterialSlotState* SkinState = Appearance.Equipment.MaterialSlots.Find(FName(TEXT("Skin"))))
    {
        ApplyMaterialState(CharacterMesh, *SkinState);
    }
    if (const FCharacterCreatorMaterialSlotState* HairState = Appearance.Equipment.MaterialSlots.Find(FName(TEXT("Hair"))))
    {
        ApplyMaterialState(HairMesh, *HairState);
    }
    if (const FCharacterCreatorMaterialSlotState* OutfitState = Appearance.Equipment.MaterialSlots.Find(FName(TEXT("PrimaryOutfit"))))
    {
        ApplyMaterialState(OutfitMesh, *OutfitState);
    }
    if (const FCharacterCreatorMaterialSlotState* SecondaryOutfitState = Appearance.Equipment.MaterialSlots.Find(FName(TEXT("SecondaryOutfit"))))
    {
        ApplyMaterialState(WeaponMesh, *SecondaryOutfitState);
    }
}

void ACharacterCreatorPreviewActor::UseFallbackMesh(const FText& Reason, bool bFailedLoad)
{
    USkeletalMesh* FallbackMesh = LoadObject<USkeletalMesh>(nullptr, CharacterCreatorPreview::FallbackSkeletalMeshPath);
    if (FallbackMesh)
    {
        bUsingFallbackMesh = true;
        ActiveAssetPath = PendingAppearance.Assets.SkeletalMesh.ToSoftObjectPath();
        CharacterMesh->SetSkeletalMesh(FallbackMesh);
        const FSoftObjectPath RequestedAssetPath = PendingAppearance.Assets.SkeletalMesh.ToSoftObjectPath();
        const bool bHasRequestedAsset = !RequestedAssetPath.IsNull();
        if (!bHasRequestedAsset)
        {
            ApplyAppearance(PendingAppearance);
        }
        SetPreviewState(bFailedLoad ? ECharacterCreatorPreviewState::Failed : ECharacterCreatorPreviewState::UsingFallback, Reason);
        if (Session)
        {
            Session->SetAssetLoadState(bFailedLoad ? ECharacterCreatorAssetLoadState::Failed : ECharacterCreatorAssetLoadState::Missing);
        }
        return;
    }

    SetPreviewState(ECharacterCreatorPreviewState::Failed, FText::FromString(TEXT("No preview mesh is available.")));
    if (Session)
    {
        Session->SetAssetLoadState(ECharacterCreatorAssetLoadState::Failed);
    }
}

void ACharacterCreatorPreviewActor::SetPreviewState(ECharacterCreatorPreviewState NewState, const FText& Message)
{
    PreviewState = NewState;
    OnPreviewStateChanged.Broadcast(PreviewState, Message);
}

FName ACharacterCreatorPreviewActor::GetMorphTargetName(const FCharacterAppearanceState& Appearance, ECharacterCreatorParameter Parameter) const
{
    if (const FName* ConfiguredName = Appearance.Assets.MorphTargetNames.Find(Parameter))
    {
        if (HasMorphTarget(*ConfiguredName))
        {
            return *ConfiguredName;
        }
    }

    TArray<FString> CandidateTokens;
    switch (Parameter)
    {
    case ECharacterCreatorParameter::Height:
        CandidateTokens = { TEXT("height"), TEXT("body") };
        break;
    case ECharacterCreatorParameter::ShoulderWidth:
        CandidateTokens = { TEXT("shoulder"), TEXT("width") };
        break;
    case ECharacterCreatorParameter::ArmLength:
        CandidateTokens = { TEXT("arm"), TEXT("length") };
        break;
    case ECharacterCreatorParameter::LegLength:
        CandidateTokens = { TEXT("leg"), TEXT("length") };
        break;
    case ECharacterCreatorParameter::HeadScale:
        CandidateTokens = { TEXT("head"), TEXT("skull") };
        break;
    case ECharacterCreatorParameter::BrowHeight:
        CandidateTokens = { TEXT("brow"), TEXT("eyebrow") };
        break;
    case ECharacterCreatorParameter::JawWidth:
        CandidateTokens = { TEXT("jaw"), TEXT("chin") };
        break;
    case ECharacterCreatorParameter::NoseWidth:
        CandidateTokens = { TEXT("nose"), TEXT("nostril") };
        break;
    case ECharacterCreatorParameter::EyeSize:
        CandidateTokens = { TEXT("eye"), TEXT("iris") };
        break;
    case ECharacterCreatorParameter::MouthWidth:
        CandidateTokens = { TEXT("mouth"), TEXT("lip") };
        break;
    default:
        break;
    }

    if (CharacterMesh && CharacterMesh->GetSkeletalMeshAsset())
    {
        const TArray<TObjectPtr<UMorphTarget>>& MorphTargets = CharacterMesh->GetSkeletalMeshAsset()->GetMorphTargets();
        for (const TObjectPtr<UMorphTarget>& MorphTarget : MorphTargets)
        {
            if (!MorphTarget)
            {
                continue;
            }

            const FString MorphName = MorphTarget->GetName().ToLower();
            for (const FString& CandidateToken : CandidateTokens)
            {
                if (MorphName.Contains(CandidateToken))
                {
                    return MorphTarget->GetFName();
                }
            }
        }
    }

    return NAME_None;
}

bool ACharacterCreatorPreviewActor::HasMorphTarget(FName MorphTargetName) const
{
    if (MorphTargetName.IsNone() || !CharacterMesh || !CharacterMesh->GetSkeletalMeshAsset())
    {
        return false;
    }

    const TArray<TObjectPtr<UMorphTarget>>& MorphTargets = CharacterMesh->GetSkeletalMeshAsset()->GetMorphTargets();
    return MorphTargets.ContainsByPredicate([MorphTargetName](const TObjectPtr<UMorphTarget>& MorphTarget)
    {
        return MorphTarget && MorphTarget->GetFName() == MorphTargetName;
    });
}
