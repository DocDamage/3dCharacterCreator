#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UI/CharacterCreatorImportService.h"
#include "CharacterCreatorSession.generated.h"

class UAnimInstance;
class UAnimSequenceBase;
class UMaterialInterface;
class UPhysicsAsset;
class USkeletalMesh;
class USkeleton;

UENUM(BlueprintType)
enum class ECharacterCreatorScreen : uint8
{
    Dashboard,
    CharacterCreator,
    OutfitAndArmor,
    HairAndGrooming,
    MaterialsAndColor,
    WeaponsAndIK,
    AnimationOverview,
    LocomotionSetup,
    BlendSpaceAssistant,
    AnimationBlueprintWorkspace,
    MontageComboBuilder,
    RetargetingAssistant,
    SkeletonRigSocketInspector,
    PhysicsSetup,
    GameplayTest,
    PreviewStudio,
    PortraitStudio,
    LODPerformance,
    AssetBrowser,
    ImportWizard,
    Settings
};

UENUM(BlueprintType)
enum class ECharacterCreatorParameter : uint8
{
    Height,
    ShoulderWidth,
    ArmLength,
    LegLength,
    HeadScale,
    BrowHeight,
    JawWidth,
    NoseWidth,
    EyeSize,
    MouthWidth
};

UENUM(BlueprintType)
enum class ECharacterCreatorAssetLoadState : uint8
{
    Unassigned,
    Loading,
    Loaded,
    Missing,
    Failed
};

UENUM(BlueprintType)
enum class ECharacterCreatorLoadoutSlot : uint8
{
    Outfit,
    Hair,
    Weapon
};

UENUM(BlueprintType)
enum class ECharacterCreatorColorTarget : uint8
{
    Skin,
    Hair,
    PrimaryOutfit,
    SecondaryOutfit
};

UENUM(BlueprintType)
enum class ECharacterCreatorIKTarget : uint8
{
    RightHand,
    Feet
};

UENUM(BlueprintType)
enum class ECharacterCreatorAnimationState : uint8
{
    Unassigned,
    SourceReady,
    Retargeting,
    TargetReady,
    Failed
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterAssetReferences
{
    GENERATED_BODY()

    FCharacterAssetReferences();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Assets")
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Assets")
    TSoftClassPtr<UAnimInstance> AnimationInstanceClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Assets")
    TSoftObjectPtr<UAnimSequenceBase> PreviewAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Assets")
    TSoftObjectPtr<UMaterialInterface> BaseMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Assets")
    TSoftObjectPtr<USkeleton> Skeleton;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Assets")
    TSoftObjectPtr<UPhysicsAsset> PhysicsAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Assets")
    ECharacterCreatorAssetLoadState LoadState = ECharacterCreatorAssetLoadState::Unassigned;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Assets")
    TMap<ECharacterCreatorParameter, FName> MorphTargetNames;

    bool IsEmpty() const;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorLoadoutState
{
    GENERATED_BODY()

    FCharacterCreatorLoadoutState();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Loadout")
    FSoftObjectPath OutfitMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Loadout")
    FSoftObjectPath HairMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Loadout")
    FSoftObjectPath WeaponMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Loadout")
    bool bOutfitEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Loadout")
    bool bHairEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Loadout")
    bool bWeaponEnabled = false;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorIKState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|IK")
    bool bRightHandIKEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|IK")
    bool bFeetIKEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|IK")
    float RightHandIKWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|IK")
    float FeetIKWeight = 1.0f;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorAnimationState
{
    GENERATED_BODY()

    FCharacterCreatorAnimationState();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FSoftObjectPath SourceAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FSoftObjectPath SourceSkeleton;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FSoftObjectPath Retargeter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FSoftObjectPath TargetAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    ECharacterCreatorAnimationState State = ECharacterCreatorAnimationState::Unassigned;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorOnboardingState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Onboarding")
    int32 CurrentStep = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Onboarding")
    bool bCompleted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Onboarding")
    bool bSkipped = false;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterAppearanceState
{
    GENERATED_BODY()

    static constexpr int32 CurrentVersion = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|State")
    int32 Version = CurrentVersion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|State")
    FCharacterAssetReferences Assets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|State")
    FCharacterCreatorLoadoutState Loadout;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|State")
    FCharacterCreatorIKState IK;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|State")
    FCharacterCreatorAnimationState Animation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Body")
    float Height = 0.56f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Body")
    float ShoulderWidth = 0.64f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Body")
    float ArmLength = 0.48f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Body")
    float LegLength = 0.60f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Body")
    float HeadScale = 0.42f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    float BrowHeight = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    float JawWidth = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    float NoseWidth = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    float EyeSize = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    float MouthWidth = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Materials")
    FLinearColor SkinColor = FLinearColor(0.34f, 0.27f, 0.19f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Materials")
    FLinearColor HairColor = FLinearColor(0.08f, 0.05f, 0.03f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Materials")
    FLinearColor PrimaryOutfitColor = FLinearColor(0.08f, 0.16f, 0.24f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Materials")
    FLinearColor SecondaryOutfitColor = FLinearColor(0.70f, 0.56f, 0.25f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|State")
    bool bHasUnsavedChanges = false;

    float GetParameterValue(ECharacterCreatorParameter Parameter) const;
    void SetParameterValue(ECharacterCreatorParameter Parameter, float Value);
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterPreset
{
    GENERATED_BODY()

    FCharacterPreset();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Preset", meta = (IgnoreForMemberInitializationTest))
    FGuid PresetId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Preset")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Preset")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Preset")
    int32 Version = FCharacterAppearanceState::CurrentVersion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Preset")
    FCharacterAppearanceState Appearance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Preset")
    bool bIsDefault = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterCreatorScreenChanged, ECharacterCreatorScreen);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterCreatorStatusChanged, const FText&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterCreatorAppearanceChanged, const FCharacterAppearanceState&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterCreatorPresetChanged, const FCharacterPreset&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterCreatorImportProgressChanged, const FCharacterCreatorImportProgress&);

UCLASS(BlueprintType)
class THREEDCHARACTER_API UCharacterCreatorSession : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Navigation")
    void SetScreen(ECharacterCreatorScreen NewScreen);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Navigation")
    ECharacterCreatorScreen GetScreen() const { return CurrentScreen; }

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Appearance")
    void SetParameterValue(ECharacterCreatorParameter Parameter, float Value);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Appearance")
    float GetParameterValue(ECharacterCreatorParameter Parameter) const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Appearance")
    void SetAppearanceState(const FCharacterAppearanceState& NewState, bool bMarkDirty = true);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Appearance")
    FCharacterAppearanceState GetAppearanceState() const { return AppearanceState; }

    const FCharacterAppearanceState& GetAppearanceStateNative() const { return AppearanceState; }

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Appearance")
    void ResetAppearance();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Appearance")
    void ApplyAppearanceChanges();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Appearance")
    void RevertAppearanceChanges();

    UFUNCTION(BlueprintPure, Category = "Character Creator|Appearance")
    bool HasUnsavedChanges() const { return AppearanceState.bHasUnsavedChanges; }

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Preset")
    void SetActivePreset(const FCharacterPreset& NewPreset);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Preset")
    FCharacterPreset GetActivePreset() const { return ActivePreset; }

    void InitializeDefaults();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Preset")
    FCharacterPreset CreatePresetFromCurrent(const FText& DisplayName, const FText& Description);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Preset")
    bool DuplicatePreset(const FGuid& PresetId);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Preset")
    bool RenamePreset(const FGuid& PresetId, const FText& NewDisplayName);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Preset")
    bool DeletePreset(const FGuid& PresetId);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Preset")
    void RestoreDefaultPreset();

    UFUNCTION(BlueprintPure, Category = "Character Creator|Preset")
    TArray<FCharacterPreset> GetPresets() const { return Presets; }

    void SetPresetLibrary(const TArray<FCharacterPreset>& NewPresets, const FCharacterPreset& NewActivePreset);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Assets")
    void SetAssetReferences(const FCharacterAssetReferences& NewReferences);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Assets")
    void SetAssetLoadState(ECharacterCreatorAssetLoadState NewState);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Loadout")
    void SetLoadoutAsset(ECharacterCreatorLoadoutSlot Slot, const FSoftObjectPath& AssetPath);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Loadout")
    FSoftObjectPath GetLoadoutAsset(ECharacterCreatorLoadoutSlot Slot) const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Materials")
    void SetColorTarget(ECharacterCreatorColorTarget Target, const FLinearColor& NewColor);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Materials")
    FLinearColor GetColorTarget(ECharacterCreatorColorTarget Target) const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|IK")
    void SetIKEnabled(ECharacterCreatorIKTarget Target, bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "Character Creator|IK")
    bool IsIKEnabled(ECharacterCreatorIKTarget Target) const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Animation")
    void SetAnimationSource(const FSoftObjectPath& SourceAnimation, const FSoftObjectPath& SourceSkeleton);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Animation")
    void SetAnimationRetargeter(const FSoftObjectPath& Retargeter);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Animation")
    void SetAnimationTarget(const FSoftObjectPath& TargetAnimation);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Animation")
    void SetAnimationState(ECharacterCreatorAnimationState NewState);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Onboarding")
    void AdvanceOnboarding();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Onboarding")
    void SkipOnboarding();

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Onboarding")
    void ResetOnboarding();

    UFUNCTION(BlueprintPure, Category = "Character Creator|Onboarding")
    FCharacterCreatorOnboardingState GetOnboardingState() const { return OnboardingState; }

    void SetOnboardingState(const FCharacterCreatorOnboardingState& NewState) { OnboardingState = NewState; }

    UFUNCTION(BlueprintPure, Category = "Character Creator|Assets")
    FCharacterAssetReferences GetAssetReferences() const { return AppearanceState.Assets; }

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Status")
    void SetStatusMessage(const FText& NewMessage);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Status")
    FText GetStatusMessage() const { return StatusMessage; }

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Import")
    void SetImportProgress(const FCharacterCreatorImportProgress& NewProgress);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Import")
    FCharacterCreatorImportProgress GetImportProgress() const { return ImportProgress; }

    void Shutdown();

    FOnCharacterCreatorScreenChanged OnScreenChanged;
    FOnCharacterCreatorStatusChanged OnStatusChanged;
    FOnCharacterCreatorAppearanceChanged OnAppearanceChanged;
    FOnCharacterCreatorPresetChanged OnPresetChanged;
    FOnCharacterCreatorImportProgressChanged OnImportProgressChanged;

private:
    ECharacterCreatorScreen CurrentScreen = ECharacterCreatorScreen::Dashboard;
    FText StatusMessage = FText::FromString(TEXT("Ready for a new character"));
    FCharacterAppearanceState AppearanceState;
    FCharacterAppearanceState SavedAppearanceState;
    FCharacterPreset ActivePreset;
    TArray<FCharacterPreset> Presets;
    FCharacterCreatorImportProgress ImportProgress;
    FCharacterCreatorOnboardingState OnboardingState;
};
