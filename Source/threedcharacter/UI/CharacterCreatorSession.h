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
enum class ECharacterCreatorFaceAdvancedParameter : uint8
{
    CheekWidth,
    ChinDepth,
    EyeSpacing,
    EyeHeight,
    LipFullness,
    EarSize,
    NasolabialDepth
};

UENUM(BlueprintType)
enum class ECharacterCreatorGroomingParameter : uint8
{
    SkinRoughness,
    SkinDetail,
    HairLength,
    HairDensity,
    HairRoughness
};

UENUM(BlueprintType)
enum class ECharacterCreatorClothingSlot : uint8
{
    Head,
    Torso,
    Legs,
    Feet,
    Gloves,
    Cape
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
enum class ECharacterCreatorWeaponSlot : uint8
{
    MainHand,
    OffHand,
    Back
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|IK")
    FName RightHandSocket = TEXT("hand_r");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|IK")
    FName LeftHandSocket = TEXT("hand_l");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|IK")
    FName LeftFootSocket = TEXT("foot_l");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|IK")
    FName RightFootSocket = TEXT("foot_r");
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorMaterialSlotState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Materials")
    FLinearColor Tint = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Materials")
    float Metallic = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Materials")
    float Roughness = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Materials")
    float EmissiveStrength = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Materials")
    float PatternScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Materials")
    FName PatternId = TEXT("None");
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorWeaponSetup
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Weapons")
    FName WeaponId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Weapons")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Weapons")
    FSoftObjectPath AssetPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Weapons")
    FName SocketName = TEXT("hand_r");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Weapons")
    FVector GripLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Weapons")
    FRotator GripRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Weapons")
    FVector GripScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Weapons")
    bool bEnabled = true;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorEquipmentState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Materials")
    TMap<FName, FCharacterCreatorMaterialSlotState> MaterialSlots;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Weapons")
    TMap<ECharacterCreatorWeaponSlot, FCharacterCreatorWeaponSetup> Weapons;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Weapons")
    TMap<FName, FCharacterCreatorWeaponSetup> WeaponLibrary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Weapons")
    FName ActiveWeaponId = NAME_None;

    FCharacterCreatorMaterialSlotState GetMaterialSlot(FName SlotId) const;
    void SetMaterialSlot(FName SlotId, const FCharacterCreatorMaterialSlotState& State);
    FCharacterCreatorWeaponSetup GetWeapon(ECharacterCreatorWeaponSlot Slot) const;
    void SetWeapon(ECharacterCreatorWeaponSlot Slot, const FCharacterCreatorWeaponSetup& Setup);
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    int32 MappedBoneCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    TArray<FName> RetargetWarnings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FText LastRetargetMessage;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorBlendSpaceState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FSoftObjectPath AssetPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    float SpeedMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    float SpeedMax = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    float DirectionMin = -180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    float DirectionMax = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    bool bConfigured = false;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorAnimationBlueprintState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FSoftObjectPath SourceBlueprint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FSoftObjectPath GeneratedBlueprint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    TArray<FName> EnabledLayers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    bool bLocomotionEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    bool bGenerated = false;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorMontageComboState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FSoftObjectPath MontagePath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    TArray<FName> Sections;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    float ComboWindowSeconds = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    bool bAuthoringReady = false;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorAnimationSetState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FName SetId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    TMap<FName, FSoftObjectPath> Clips;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    bool bGenerated = false;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorWeaponAnimationProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FName WeaponId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    TMap<FName, FSoftObjectPath> Clips;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    bool bValidated = false;
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorFaceAdvancedState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    float CheekWidth = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    float ChinDepth = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    float EyeSpacing = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    float EyeHeight = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    float LipFullness = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    float EarSize = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    float NasolabialDepth = 0.50f;

    float GetValue(ECharacterCreatorFaceAdvancedParameter Parameter) const;
    void SetValue(ECharacterCreatorFaceAdvancedParameter Parameter, float Value);
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorGroomingState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Grooming")
    float SkinRoughness = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Grooming")
    float SkinDetail = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Grooming")
    float HairLength = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Grooming")
    float HairDensity = 0.70f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Grooming")
    float HairRoughness = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Grooming")
    FName HairStyle = TEXT("BaseHair01");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Grooming")
    FName SkinProfile = TEXT("Natural");

    float GetValue(ECharacterCreatorGroomingParameter Parameter) const;
    void SetValue(ECharacterCreatorGroomingParameter Parameter, float Value);
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorClothingState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Clothing")
    TMap<ECharacterCreatorClothingSlot, FSoftObjectPath> SlotAssets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Clothing")
    TMap<FName, FSoftObjectPath> Accessories;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Clothing")
    bool bUseLayeredClothing = true;

    FSoftObjectPath GetSlotAsset(ECharacterCreatorClothingSlot Slot) const;
    void SetSlotAsset(ECharacterCreatorClothingSlot Slot, const FSoftObjectPath& AssetPath);
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

    static constexpr int32 CurrentVersion = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|State")
    int32 Version = CurrentVersion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|State")
    FCharacterAssetReferences Assets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|State")
    FCharacterCreatorLoadoutState Loadout;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|State")
    FCharacterCreatorIKState IK;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Equipment")
    FCharacterCreatorEquipmentState Equipment;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|State")
    FCharacterCreatorAnimationState Animation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FCharacterCreatorBlendSpaceState BlendSpace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FCharacterCreatorAnimationBlueprintState AnimationBlueprint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FCharacterCreatorMontageComboState MontageCombo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    FCharacterCreatorAnimationSetState AnimationSet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Animation")
    TMap<FName, FCharacterCreatorWeaponAnimationProfile> WeaponAnimationProfiles;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Face")
    FCharacterCreatorFaceAdvancedState AdvancedFace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Grooming")
    FCharacterCreatorGroomingState Grooming;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Clothing")
    FCharacterCreatorClothingState Clothing;

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

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Face")
    void SetAdvancedFaceValue(ECharacterCreatorFaceAdvancedParameter Parameter, float Value);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Face")
    float GetAdvancedFaceValue(ECharacterCreatorFaceAdvancedParameter Parameter) const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Grooming")
    void SetGroomingValue(ECharacterCreatorGroomingParameter Parameter, float Value);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Grooming")
    float GetGroomingValue(ECharacterCreatorGroomingParameter Parameter) const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Grooming")
    void SetHairStyle(FName StyleId);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Grooming")
    void SetSkinProfile(FName ProfileId);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Clothing")
    void SetClothingAsset(ECharacterCreatorClothingSlot Slot, const FSoftObjectPath& AssetPath);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Clothing")
    FSoftObjectPath GetClothingAsset(ECharacterCreatorClothingSlot Slot) const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Clothing")
    void SetAccessoryAsset(FName AccessoryId, const FSoftObjectPath& AssetPath);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Clothing")
    FSoftObjectPath GetAccessoryAsset(FName AccessoryId) const;

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

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Materials")
    void SetMaterialSlotState(FName SlotId, const FCharacterCreatorMaterialSlotState& NewState);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Materials")
    FCharacterCreatorMaterialSlotState GetMaterialSlotState(FName SlotId) const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Weapons")
    void SetWeaponSetup(ECharacterCreatorWeaponSlot Slot, const FCharacterCreatorWeaponSetup& Setup);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Weapons")
    FCharacterCreatorWeaponSetup GetWeaponSetup(ECharacterCreatorWeaponSlot Slot) const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Weapons")
    void SetWeaponGrip(ECharacterCreatorWeaponSlot Slot, const FVector& Location, const FRotator& Rotation, const FVector& Scale);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Weapons")
    void SetWeaponSocket(ECharacterCreatorWeaponSlot Slot, FName SocketName);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Weapons")
    void RegisterWeaponDefinition(const FCharacterCreatorWeaponSetup& Setup);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Weapons")
    TArray<FCharacterCreatorWeaponSetup> GetWeaponLibrary() const;

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

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Animation")
    void SetBlendSpaceState(const FCharacterCreatorBlendSpaceState& NewState);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Animation")
    FCharacterCreatorBlendSpaceState GetBlendSpaceState() const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Animation")
    void SetAnimationBlueprintState(const FCharacterCreatorAnimationBlueprintState& NewState);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Animation")
    FCharacterCreatorAnimationBlueprintState GetAnimationBlueprintState() const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Animation")
    void SetMontageComboState(const FCharacterCreatorMontageComboState& NewState);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Animation")
    FCharacterCreatorMontageComboState GetMontageComboState() const;

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Animation")
    void SetAnimationSetClip(FName ClipId, const FSoftObjectPath& AssetPath);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Animation")
    void SetWeaponAnimationProfile(const FCharacterCreatorWeaponAnimationProfile& Profile);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Animation")
    bool ExecuteAnimationRetarget();

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
