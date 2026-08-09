#include "UI/CharacterCreatorSession.h"

#include "Animation/Skeleton.h"
#include "Misc/Paths.h"
#include "Math/RandomStream.h"
#include "PhysicsEngine/PhysicsAsset.h"

namespace
{
    ECharacterCreatorRandomizationCategory CategoryForParameter(ECharacterCreatorParameter Parameter)
    {
        switch (Parameter)
        {
        case ECharacterCreatorParameter::Height:
        case ECharacterCreatorParameter::ShoulderWidth:
        case ECharacterCreatorParameter::ArmLength:
        case ECharacterCreatorParameter::LegLength:
        case ECharacterCreatorParameter::HeadScale:
            return ECharacterCreatorRandomizationCategory::Body;
        default:
            return ECharacterCreatorRandomizationCategory::Face;
        }
    }

    FVector2D SanitizeRange(const FVector2D& Range)
    {
        const float MinValue = FMath::Clamp(FMath::Min(Range.X, Range.Y), 0.0f, 1.0f);
        const float MaxValue = FMath::Clamp(FMath::Max(Range.X, Range.Y), MinValue, 1.0f);
        return FVector2D(MinValue, MaxValue);
    }

    bool IsLocked(const FCharacterCreatorRandomizationRules& Rules, ECharacterCreatorRandomizationCategory Category)
    {
        return Rules.CategoryLocks.FindRef(Category);
    }

    void AddDifference(FCharacterCreatorPresetComparison& Comparison, FName Difference)
    {
        Comparison.Differences.Add(Difference);
        Comparison.bEquivalent = false;
    }
}

FCharacterAssetReferences::FCharacterAssetReferences()
    : SkeletalMesh(FSoftObjectPath(TEXT("/Game/Synty/SidekickCharacters/Resources/Skeletons/SKM_Default_Sidekick.SKM_Default_Sidekick")))
    , BaseMaterial(FSoftObjectPath(TEXT("/Game/Synty/SidekickCharacters/Resources/Materials/M_Default_Sidekick.M_Default_Sidekick")))
    , Skeleton(FSoftObjectPath(TEXT("/Game/Synty/SidekickCharacters/Resources/Skeletons/SKEL_Default_Sidekick.SKEL_Default_Sidekick")))
{
    MorphTargetNames.Add(ECharacterCreatorParameter::ArmLength, FName(TEXT("Arm")));
    MorphTargetNames.Add(ECharacterCreatorParameter::LegLength, FName(TEXT("Leg")));
    MorphTargetNames.Add(ECharacterCreatorParameter::JawWidth, FName(TEXT("Jaw")));
    MorphTargetNames.Add(ECharacterCreatorParameter::EyeSize, FName(TEXT("eye")));
}

FCharacterCreatorLoadoutState::FCharacterCreatorLoadoutState()
    : OutfitMesh(FSoftObjectPath(TEXT("/Game/Synty/SidekickCharacters/Resources/Meshes/Outfits/Starter/SK_FANT_KNGT_17_10TORS_HU01.SK_FANT_KNGT_17_10TORS_HU01")))
    , HairMesh(FSoftObjectPath(TEXT("/Game/Synty/SidekickCharacters/Resources/Meshes/Species/Humans/SK_HUMN_BASE_01_02HAIR_HU01.SK_HUMN_BASE_01_02HAIR_HU01")))
{
}

FCharacterCreatorAnimationState::FCharacterCreatorAnimationState()
    : SourceAnimation(FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Animations/Manny/MM_Idle.MM_Idle")))
    , SourceSkeleton(FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin")))
    , State(ECharacterCreatorAnimationState::SourceReady)
{
}

float FCharacterCreatorFaceAdvancedState::GetValue(ECharacterCreatorFaceAdvancedParameter Parameter) const
{
    switch (Parameter)
    {
    case ECharacterCreatorFaceAdvancedParameter::CheekWidth: return CheekWidth;
    case ECharacterCreatorFaceAdvancedParameter::ChinDepth: return ChinDepth;
    case ECharacterCreatorFaceAdvancedParameter::EyeSpacing: return EyeSpacing;
    case ECharacterCreatorFaceAdvancedParameter::EyeHeight: return EyeHeight;
    case ECharacterCreatorFaceAdvancedParameter::LipFullness: return LipFullness;
    case ECharacterCreatorFaceAdvancedParameter::EarSize: return EarSize;
    case ECharacterCreatorFaceAdvancedParameter::NasolabialDepth: return NasolabialDepth;
    default: return 0.5f;
    }
}

void FCharacterCreatorFaceAdvancedState::SetValue(ECharacterCreatorFaceAdvancedParameter Parameter, float Value)
{
    const float ClampedValue = FMath::Clamp(Value, 0.0f, 1.0f);
    switch (Parameter)
    {
    case ECharacterCreatorFaceAdvancedParameter::CheekWidth: CheekWidth = ClampedValue; break;
    case ECharacterCreatorFaceAdvancedParameter::ChinDepth: ChinDepth = ClampedValue; break;
    case ECharacterCreatorFaceAdvancedParameter::EyeSpacing: EyeSpacing = ClampedValue; break;
    case ECharacterCreatorFaceAdvancedParameter::EyeHeight: EyeHeight = ClampedValue; break;
    case ECharacterCreatorFaceAdvancedParameter::LipFullness: LipFullness = ClampedValue; break;
    case ECharacterCreatorFaceAdvancedParameter::EarSize: EarSize = ClampedValue; break;
    case ECharacterCreatorFaceAdvancedParameter::NasolabialDepth: NasolabialDepth = ClampedValue; break;
    default: break;
    }
}

float FCharacterCreatorGroomingState::GetValue(ECharacterCreatorGroomingParameter Parameter) const
{
    switch (Parameter)
    {
    case ECharacterCreatorGroomingParameter::SkinRoughness: return SkinRoughness;
    case ECharacterCreatorGroomingParameter::SkinDetail: return SkinDetail;
    case ECharacterCreatorGroomingParameter::HairLength: return HairLength;
    case ECharacterCreatorGroomingParameter::HairDensity: return HairDensity;
    case ECharacterCreatorGroomingParameter::HairRoughness: return HairRoughness;
    default: return 0.5f;
    }
}

void FCharacterCreatorGroomingState::SetValue(ECharacterCreatorGroomingParameter Parameter, float Value)
{
    const float ClampedValue = FMath::Clamp(Value, 0.0f, 1.0f);
    switch (Parameter)
    {
    case ECharacterCreatorGroomingParameter::SkinRoughness: SkinRoughness = ClampedValue; break;
    case ECharacterCreatorGroomingParameter::SkinDetail: SkinDetail = ClampedValue; break;
    case ECharacterCreatorGroomingParameter::HairLength: HairLength = ClampedValue; break;
    case ECharacterCreatorGroomingParameter::HairDensity: HairDensity = ClampedValue; break;
    case ECharacterCreatorGroomingParameter::HairRoughness: HairRoughness = ClampedValue; break;
    default: break;
    }
}

FSoftObjectPath FCharacterCreatorClothingState::GetSlotAsset(ECharacterCreatorClothingSlot Slot) const
{
    if (const FSoftObjectPath* AssetPath = SlotAssets.Find(Slot))
    {
        return *AssetPath;
    }
    return FSoftObjectPath();
}

void FCharacterCreatorClothingState::SetSlotAsset(ECharacterCreatorClothingSlot Slot, const FSoftObjectPath& AssetPath)
{
    if (AssetPath.IsNull())
    {
        SlotAssets.Remove(Slot);
    }
    else
    {
        SlotAssets.Add(Slot, AssetPath);
    }
}

FCharacterCreatorMaterialSlotState FCharacterCreatorEquipmentState::GetMaterialSlot(FName SlotId) const
{
    if (const FCharacterCreatorMaterialSlotState* State = MaterialSlots.Find(SlotId))
    {
        return *State;
    }
    return FCharacterCreatorMaterialSlotState();
}

void FCharacterCreatorEquipmentState::SetMaterialSlot(FName SlotId, const FCharacterCreatorMaterialSlotState& State)
{
    if (!SlotId.IsNone())
    {
        MaterialSlots.Add(SlotId, State);
    }
}

FCharacterCreatorWeaponSetup FCharacterCreatorEquipmentState::GetWeapon(ECharacterCreatorWeaponSlot Slot) const
{
    if (const FCharacterCreatorWeaponSetup* Setup = Weapons.Find(Slot))
    {
        return *Setup;
    }
    return FCharacterCreatorWeaponSetup();
}

void FCharacterCreatorEquipmentState::SetWeapon(ECharacterCreatorWeaponSlot Slot, const FCharacterCreatorWeaponSetup& Setup)
{
    Weapons.Add(Slot, Setup);
}

bool FCharacterAssetReferences::IsEmpty() const
{
    return SkeletalMesh.IsNull()
        && AnimationInstanceClass.IsNull()
        && PreviewAnimation.IsNull()
        && BaseMaterial.IsNull()
        && Skeleton.IsNull()
        && PhysicsAsset.IsNull();
}

float FCharacterAppearanceState::GetParameterValue(ECharacterCreatorParameter Parameter) const
{
    switch (Parameter)
    {
    case ECharacterCreatorParameter::Height:
        return Height;
    case ECharacterCreatorParameter::ShoulderWidth:
        return ShoulderWidth;
    case ECharacterCreatorParameter::ArmLength:
        return ArmLength;
    case ECharacterCreatorParameter::LegLength:
        return LegLength;
    case ECharacterCreatorParameter::HeadScale:
        return HeadScale;
    case ECharacterCreatorParameter::BrowHeight:
        return BrowHeight;
    case ECharacterCreatorParameter::JawWidth:
        return JawWidth;
    case ECharacterCreatorParameter::NoseWidth:
        return NoseWidth;
    case ECharacterCreatorParameter::EyeSize:
        return EyeSize;
    case ECharacterCreatorParameter::MouthWidth:
        return MouthWidth;
    default:
        return 0.0f;
    }
}

void FCharacterAppearanceState::SetParameterValue(ECharacterCreatorParameter Parameter, float Value)
{
    const float ClampedValue = FMath::Clamp(Value, 0.0f, 1.0f);

    switch (Parameter)
    {
    case ECharacterCreatorParameter::Height:
        Height = ClampedValue;
        break;
    case ECharacterCreatorParameter::ShoulderWidth:
        ShoulderWidth = ClampedValue;
        break;
    case ECharacterCreatorParameter::ArmLength:
        ArmLength = ClampedValue;
        break;
    case ECharacterCreatorParameter::LegLength:
        LegLength = ClampedValue;
        break;
    case ECharacterCreatorParameter::HeadScale:
        HeadScale = ClampedValue;
        break;
    case ECharacterCreatorParameter::BrowHeight:
        BrowHeight = ClampedValue;
        break;
    case ECharacterCreatorParameter::JawWidth:
        JawWidth = ClampedValue;
        break;
    case ECharacterCreatorParameter::NoseWidth:
        NoseWidth = ClampedValue;
        break;
    case ECharacterCreatorParameter::EyeSize:
        EyeSize = ClampedValue;
        break;
    case ECharacterCreatorParameter::MouthWidth:
        MouthWidth = ClampedValue;
        break;
    default:
        break;
    }
}

FCharacterPreset::FCharacterPreset()
    : PresetId(FGuid::NewGuid())
    , DisplayName(FText::FromString(TEXT("New Character")))
{
}

void UCharacterCreatorSession::SetScreen(ECharacterCreatorScreen NewScreen)
{
    if (CurrentScreen == NewScreen)
    {
        return;
    }

    CurrentScreen = NewScreen;
    OnScreenChanged.Broadcast(CurrentScreen);
}

void UCharacterCreatorSession::SetParameterValue(ECharacterCreatorParameter Parameter, float Value)
{
    const float PreviousValue = AppearanceState.GetParameterValue(Parameter);
    const float ClampedValue = FMath::Clamp(Value, 0.0f, 1.0f);
    if (FMath::IsNearlyEqual(PreviousValue, ClampedValue))
    {
        return;
    }

    AppearanceState.SetParameterValue(Parameter, ClampedValue);
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::SetAdvancedFaceValue(ECharacterCreatorFaceAdvancedParameter Parameter, float Value)
{
    const float ClampedValue = FMath::Clamp(Value, 0.0f, 1.0f);
    if (FMath::IsNearlyEqual(AppearanceState.AdvancedFace.GetValue(Parameter), ClampedValue))
    {
        return;
    }

    AppearanceState.AdvancedFace.SetValue(Parameter, ClampedValue);
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

float UCharacterCreatorSession::GetAdvancedFaceValue(ECharacterCreatorFaceAdvancedParameter Parameter) const
{
    return AppearanceState.AdvancedFace.GetValue(Parameter);
}

void UCharacterCreatorSession::SetGroomingValue(ECharacterCreatorGroomingParameter Parameter, float Value)
{
    const float ClampedValue = FMath::Clamp(Value, 0.0f, 1.0f);
    if (FMath::IsNearlyEqual(AppearanceState.Grooming.GetValue(Parameter), ClampedValue))
    {
        return;
    }

    AppearanceState.Grooming.SetValue(Parameter, ClampedValue);
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

float UCharacterCreatorSession::GetGroomingValue(ECharacterCreatorGroomingParameter Parameter) const
{
    return AppearanceState.Grooming.GetValue(Parameter);
}

void UCharacterCreatorSession::SetHairStyle(FName StyleId)
{
    if (StyleId.IsNone() || AppearanceState.Grooming.HairStyle == StyleId)
    {
        return;
    }

    AppearanceState.Grooming.HairStyle = StyleId;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::SetSkinProfile(FName ProfileId)
{
    if (ProfileId.IsNone() || AppearanceState.Grooming.SkinProfile == ProfileId)
    {
        return;
    }

    AppearanceState.Grooming.SkinProfile = ProfileId;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::SetClothingAsset(ECharacterCreatorClothingSlot Slot, const FSoftObjectPath& AssetPath)
{
    if (AppearanceState.Clothing.GetSlotAsset(Slot) == AssetPath)
    {
        return;
    }

    AppearanceState.Clothing.SetSlotAsset(Slot, AssetPath);
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

FSoftObjectPath UCharacterCreatorSession::GetClothingAsset(ECharacterCreatorClothingSlot Slot) const
{
    return AppearanceState.Clothing.GetSlotAsset(Slot);
}

void UCharacterCreatorSession::SetAccessoryAsset(FName AccessoryId, const FSoftObjectPath& AssetPath)
{
    if (AccessoryId.IsNone())
    {
        return;
    }

    const FSoftObjectPath CurrentPath = AppearanceState.Clothing.Accessories.FindRef(AccessoryId);
    if (CurrentPath == AssetPath)
    {
        return;
    }

    if (AssetPath.IsNull())
    {
        AppearanceState.Clothing.Accessories.Remove(AccessoryId);
    }
    else
    {
        AppearanceState.Clothing.Accessories.Add(AccessoryId, AssetPath);
    }
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

FSoftObjectPath UCharacterCreatorSession::GetAccessoryAsset(FName AccessoryId) const
{
    return AppearanceState.Clothing.Accessories.FindRef(AccessoryId);
}

float UCharacterCreatorSession::GetParameterValue(ECharacterCreatorParameter Parameter) const
{
    return AppearanceState.GetParameterValue(Parameter);
}

void UCharacterCreatorSession::SetAppearanceState(const FCharacterAppearanceState& NewState, bool bMarkDirty)
{
    AppearanceState = NewState;
    AppearanceState.Version = FCharacterAppearanceState::CurrentVersion;
    AppearanceState.bHasUnsavedChanges = bMarkDirty;
    if (!bMarkDirty)
    {
        SavedAppearanceState = AppearanceState;
    }
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::ResetAppearance()
{
    AppearanceState = FCharacterAppearanceState();
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::ApplyAppearanceChanges()
{
    SavedAppearanceState = AppearanceState;
    SavedAppearanceState.bHasUnsavedChanges = false;
    AppearanceState.bHasUnsavedChanges = false;
    OnAppearanceChanged.Broadcast(AppearanceState);
    SetStatusMessage(FText::FromString(TEXT("Character changes applied")));
}

void UCharacterCreatorSession::RevertAppearanceChanges()
{
    AppearanceState = SavedAppearanceState;
    AppearanceState.Version = FCharacterAppearanceState::CurrentVersion;
    AppearanceState.bHasUnsavedChanges = false;
    OnAppearanceChanged.Broadcast(AppearanceState);
    SetStatusMessage(FText::FromString(TEXT("Character changes reverted")));
}

void UCharacterCreatorSession::SetActivePreset(const FCharacterPreset& NewPreset)
{
    ActivePreset = NewPreset;
    SetAppearanceState(NewPreset.Appearance, false);
    OnPresetChanged.Broadcast(ActivePreset);
}

void UCharacterCreatorSession::InitializeDefaults()
{
    AppearanceState = FCharacterAppearanceState();
    FCharacterCreatorMaterialSlotState SkinMaterial;
    SkinMaterial.Tint = AppearanceState.SkinColor;
    SkinMaterial.Roughness = AppearanceState.Grooming.SkinRoughness;
    AppearanceState.Equipment.SetMaterialSlot(FName(TEXT("Skin")), SkinMaterial);

    FCharacterCreatorMaterialSlotState HairMaterial;
    HairMaterial.Tint = AppearanceState.HairColor;
    HairMaterial.Roughness = AppearanceState.Grooming.HairRoughness;
    AppearanceState.Equipment.SetMaterialSlot(FName(TEXT("Hair")), HairMaterial);

    FCharacterCreatorMaterialSlotState OutfitMaterial;
    OutfitMaterial.Tint = AppearanceState.PrimaryOutfitColor;
    OutfitMaterial.Roughness = 0.62f;
    AppearanceState.Equipment.SetMaterialSlot(FName(TEXT("PrimaryOutfit")), OutfitMaterial);

    FCharacterCreatorMaterialSlotState SecondaryOutfitMaterial;
    SecondaryOutfitMaterial.Tint = AppearanceState.SecondaryOutfitColor;
    SecondaryOutfitMaterial.Roughness = 0.48f;
    AppearanceState.Equipment.SetMaterialSlot(FName(TEXT("SecondaryOutfit")), SecondaryOutfitMaterial);

    FCharacterCreatorWeaponSetup TrainingBlade;
    TrainingBlade.WeaponId = FName(TEXT("TrainingBlade"));
    TrainingBlade.DisplayName = FText::FromString(TEXT("Training Blade"));
    TrainingBlade.SocketName = FName(TEXT("hand_r"));
    TrainingBlade.bEnabled = false;
    AppearanceState.Equipment.WeaponLibrary.Add(TrainingBlade.WeaponId, TrainingBlade);

    FCharacterCreatorWeaponSetup PracticeStaff;
    PracticeStaff.WeaponId = FName(TEXT("PracticeStaff"));
    PracticeStaff.DisplayName = FText::FromString(TEXT("Practice Staff"));
    PracticeStaff.SocketName = FName(TEXT("hand_r"));
    PracticeStaff.bEnabled = false;
    AppearanceState.Equipment.WeaponLibrary.Add(PracticeStaff.WeaponId, PracticeStaff);
    AppearanceState.PreviewTesting.Controller.Hints.Add(FName(TEXT("Confirm")), FText::FromString(TEXT("A  Apply / Confirm")));
    AppearanceState.PreviewTesting.Controller.Hints.Add(FName(TEXT("Cancel")), FText::FromString(TEXT("B  Cancel / Revert")));
    AppearanceState.PreviewTesting.Controller.Hints.Add(FName(TEXT("Navigate")), FText::FromString(TEXT("D-Pad  Navigate")));
    AppearanceState.PreviewTesting.Controller.Hints.Add(FName(TEXT("SwitchWorkspace")), FText::FromString(TEXT("LB / RB  Switch workspace")));
    const ECharacterCreatorParameter BasicParameters[] = {
        ECharacterCreatorParameter::Height,
        ECharacterCreatorParameter::ShoulderWidth,
        ECharacterCreatorParameter::ArmLength,
        ECharacterCreatorParameter::LegLength,
        ECharacterCreatorParameter::HeadScale,
        ECharacterCreatorParameter::BrowHeight,
        ECharacterCreatorParameter::JawWidth,
        ECharacterCreatorParameter::NoseWidth,
        ECharacterCreatorParameter::EyeSize,
        ECharacterCreatorParameter::MouthWidth
    };
    for (const ECharacterCreatorParameter Parameter : BasicParameters)
    {
        AppearanceState.Randomization.ParameterRanges.Add(Parameter, FVector2D(0.0f, 1.0f));
    }
    AppearanceState.Randomization.CategoryLocks.Add(ECharacterCreatorRandomizationCategory::Body, false);
    AppearanceState.Randomization.CategoryLocks.Add(ECharacterCreatorRandomizationCategory::Face, false);
    AppearanceState.Randomization.CategoryLocks.Add(ECharacterCreatorRandomizationCategory::AdvancedFace, false);
    AppearanceState.Randomization.CategoryLocks.Add(ECharacterCreatorRandomizationCategory::Grooming, false);
    AppearanceState.Randomization.CategoryLocks.Add(ECharacterCreatorRandomizationCategory::Materials, false);
    AppearanceState.Randomization.CategoryLocks.Add(ECharacterCreatorRandomizationCategory::Clothing, false);
    AppearanceState.Randomization.CategoryLocks.Add(ECharacterCreatorRandomizationCategory::Equipment, false);
    AppearanceState.Randomization.CategoryLocks.Add(ECharacterCreatorRandomizationCategory::Animation, false);
    AppearanceState.bHasUnsavedChanges = false;
    SavedAppearanceState = AppearanceState;

    ActivePreset = FCharacterPreset();
    ActivePreset.DisplayName = FText::FromString(TEXT("Default Sidekick"));
    ActivePreset.Description = FText::FromString(TEXT("Default Sidekick creator starting point"));
    ActivePreset.Appearance = AppearanceState;
    ActivePreset.bIsDefault = true;

    Presets.Reset();
    Presets.Add(ActivePreset);
    OnAppearanceChanged.Broadcast(AppearanceState);
    OnPresetChanged.Broadcast(ActivePreset);
}

FCharacterPreset UCharacterCreatorSession::CreatePresetFromCurrent(const FText& DisplayName, const FText& Description)
{
    FCharacterPreset NewPreset;
    NewPreset.DisplayName = DisplayName.IsEmpty() ? FText::FromString(TEXT("Untitled Preset")) : DisplayName;
    NewPreset.Description = Description;
    NewPreset.Appearance = AppearanceState;
    NewPreset.Appearance.bHasUnsavedChanges = false;
    Presets.Add(NewPreset);
    ActivePreset = NewPreset;
    OnPresetChanged.Broadcast(ActivePreset);
    return NewPreset;
}

bool UCharacterCreatorSession::DuplicatePreset(const FGuid& PresetId)
{
    for (const FCharacterPreset& Preset : Presets)
    {
        if (Preset.PresetId == PresetId)
        {
            FCharacterPreset Duplicate = Preset;
            Duplicate.PresetId = FGuid::NewGuid();
            Duplicate.DisplayName = FText::FromString(FString::Printf(TEXT("%s Copy"), *Preset.DisplayName.ToString()));
            Duplicate.bIsDefault = false;
            Presets.Add(Duplicate);
            ActivePreset = Duplicate;
            OnPresetChanged.Broadcast(ActivePreset);
            return true;
        }
    }

    return false;
}

bool UCharacterCreatorSession::RenamePreset(const FGuid& PresetId, const FText& NewDisplayName)
{
    if (NewDisplayName.IsEmpty())
    {
        return false;
    }

    for (FCharacterPreset& Preset : Presets)
    {
        if (Preset.PresetId == PresetId)
        {
            Preset.DisplayName = NewDisplayName;
            if (ActivePreset.PresetId == PresetId)
            {
                ActivePreset = Preset;
            }
            OnPresetChanged.Broadcast(ActivePreset);
            return true;
        }
    }

    return false;
}

bool UCharacterCreatorSession::DeletePreset(const FGuid& PresetId)
{
    for (int32 Index = 0; Index < Presets.Num(); ++Index)
    {
        if (Presets[Index].PresetId == PresetId)
        {
            if (Presets[Index].bIsDefault)
            {
                RestoreDefaultPreset();
                return true;
            }

            Presets.RemoveAt(Index);
            if (ActivePreset.PresetId == PresetId)
            {
                RestoreDefaultPreset();
            }
            else
            {
                OnPresetChanged.Broadcast(ActivePreset);
            }
            return true;
        }
    }

    return false;
}

void UCharacterCreatorSession::RestoreDefaultPreset()
{
    for (const FCharacterPreset& Preset : Presets)
    {
        if (Preset.bIsDefault)
        {
            ActivePreset = Preset;
            SetAppearanceState(Preset.Appearance, false);
            OnPresetChanged.Broadcast(ActivePreset);
            return;
        }
    }

    InitializeDefaults();
}

void UCharacterCreatorSession::SetPresetLibrary(const TArray<FCharacterPreset>& NewPresets, const FCharacterPreset& NewActivePreset)
{
    Presets = NewPresets;
    ActivePreset = NewActivePreset;
    if (Presets.Num() == 0)
    {
        InitializeDefaults();
        return;
    }

    OnPresetChanged.Broadcast(ActivePreset);
}

void UCharacterCreatorSession::SetRandomizationSeed(int32 NewSeed)
{
    AppearanceState.Randomization.Seed = NewSeed;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

int32 UCharacterCreatorSession::GetRandomizationSeed() const
{
    return AppearanceState.Randomization.Seed;
}

void UCharacterCreatorSession::SetRandomizationCategoryLocked(ECharacterCreatorRandomizationCategory Category, bool bLocked)
{
    AppearanceState.Randomization.CategoryLocks.Add(Category, bLocked);
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

bool UCharacterCreatorSession::IsRandomizationCategoryLocked(ECharacterCreatorRandomizationCategory Category) const
{
    return IsLocked(AppearanceState.Randomization, Category);
}

void UCharacterCreatorSession::SetRandomizationParameterRange(ECharacterCreatorParameter Parameter, float MinValue, float MaxValue)
{
    AppearanceState.Randomization.ParameterRanges.Add(Parameter, SanitizeRange(FVector2D(MinValue, MaxValue)));
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

bool UCharacterCreatorSession::RandomizeAppearance(bool bApplyImmediately)
{
    FRandomStream RandomStream(AppearanceState.Randomization.Seed);
    FCharacterAppearanceState Generated = AppearanceState;

    const TArray<ECharacterCreatorParameter> Parameters = {
        ECharacterCreatorParameter::Height,
        ECharacterCreatorParameter::ShoulderWidth,
        ECharacterCreatorParameter::ArmLength,
        ECharacterCreatorParameter::LegLength,
        ECharacterCreatorParameter::HeadScale,
        ECharacterCreatorParameter::BrowHeight,
        ECharacterCreatorParameter::JawWidth,
        ECharacterCreatorParameter::NoseWidth,
        ECharacterCreatorParameter::EyeSize,
        ECharacterCreatorParameter::MouthWidth
    };
    for (const ECharacterCreatorParameter Parameter : Parameters)
    {
        const ECharacterCreatorRandomizationCategory Category = CategoryForParameter(Parameter);
        if (IsLocked(AppearanceState.Randomization, Category))
        {
            continue;
        }

        const FVector2D Range = AppearanceState.Randomization.ParameterRanges.Contains(Parameter)
            ? SanitizeRange(AppearanceState.Randomization.ParameterRanges.FindRef(Parameter))
            : FVector2D(0.0f, 1.0f);
        Generated.SetParameterValue(Parameter, RandomStream.FRandRange(Range.X, Range.Y));
    }

    if (!IsLocked(AppearanceState.Randomization, ECharacterCreatorRandomizationCategory::AdvancedFace))
    {
        const TArray<ECharacterCreatorFaceAdvancedParameter> FaceParameters = {
            ECharacterCreatorFaceAdvancedParameter::CheekWidth,
            ECharacterCreatorFaceAdvancedParameter::ChinDepth,
            ECharacterCreatorFaceAdvancedParameter::EyeSpacing,
            ECharacterCreatorFaceAdvancedParameter::EyeHeight,
            ECharacterCreatorFaceAdvancedParameter::LipFullness,
            ECharacterCreatorFaceAdvancedParameter::EarSize,
            ECharacterCreatorFaceAdvancedParameter::NasolabialDepth
        };
        for (const ECharacterCreatorFaceAdvancedParameter Parameter : FaceParameters)
        {
            const FVector2D Range = AppearanceState.Randomization.AdvancedFaceRanges.Contains(Parameter)
                ? SanitizeRange(AppearanceState.Randomization.AdvancedFaceRanges.FindRef(Parameter))
                : FVector2D(0.0f, 1.0f);
            Generated.AdvancedFace.SetValue(Parameter, RandomStream.FRandRange(Range.X, Range.Y));
        }
    }

    if (!IsLocked(AppearanceState.Randomization, ECharacterCreatorRandomizationCategory::Grooming))
    {
        const TArray<ECharacterCreatorGroomingParameter> GroomingParameters = {
            ECharacterCreatorGroomingParameter::SkinRoughness,
            ECharacterCreatorGroomingParameter::SkinDetail,
            ECharacterCreatorGroomingParameter::HairLength,
            ECharacterCreatorGroomingParameter::HairDensity,
            ECharacterCreatorGroomingParameter::HairRoughness
        };
        for (const ECharacterCreatorGroomingParameter Parameter : GroomingParameters)
        {
            const FVector2D Range = AppearanceState.Randomization.GroomingRanges.Contains(Parameter)
                ? SanitizeRange(AppearanceState.Randomization.GroomingRanges.FindRef(Parameter))
                : FVector2D(0.0f, 1.0f);
            Generated.Grooming.SetValue(Parameter, RandomStream.FRandRange(Range.X, Range.Y));
        }
    }

    if (AppearanceState.Randomization.bRandomizeColors && !IsLocked(AppearanceState.Randomization, ECharacterCreatorRandomizationCategory::Materials))
    {
        const auto RandomColor = [&RandomStream]()
        {
            return FLinearColor(
                RandomStream.FRandRange(0.08f, 0.90f),
                RandomStream.FRandRange(0.08f, 0.90f),
                RandomStream.FRandRange(0.08f, 0.90f),
                1.0f);
        };
        Generated.SkinColor = RandomColor();
        Generated.HairColor = RandomColor();
        Generated.PrimaryOutfitColor = RandomColor();
        Generated.SecondaryOutfitColor = RandomColor();
    }

    if (!IsLocked(AppearanceState.Randomization, ECharacterCreatorRandomizationCategory::Clothing))
    {
        Generated.Loadout.bOutfitEnabled = RandomStream.FRand() >= 0.5f;
        Generated.Loadout.bHairEnabled = RandomStream.FRand() >= 0.5f;
    }

    Generated.bHasUnsavedChanges = true;
    AppearanceState = Generated;
    OnAppearanceChanged.Broadcast(AppearanceState);
    SetStatusMessage(FText::FromString(FString::Printf(TEXT("Randomized appearance with seed %d"), AppearanceState.Randomization.Seed)));
    if (bApplyImmediately)
    {
        ApplyAppearanceChanges();
    }
    return true;
}

FCharacterCreatorRandomizationRules UCharacterCreatorSession::GetRandomizationRules() const
{
    return AppearanceState.Randomization;
}

FCharacterCreatorPresetComparison UCharacterCreatorSession::ComparePresets(const FGuid& LeftPresetId, const FGuid& RightPresetId) const
{
    FCharacterCreatorPresetComparison Comparison;
    Comparison.LeftPresetId = LeftPresetId;
    Comparison.RightPresetId = RightPresetId;
    const FCharacterPreset* LeftPreset = Presets.FindByPredicate([&LeftPresetId](const FCharacterPreset& Preset) { return Preset.PresetId == LeftPresetId; });
    const FCharacterPreset* RightPreset = Presets.FindByPredicate([&RightPresetId](const FCharacterPreset& Preset) { return Preset.PresetId == RightPresetId; });
    if (!LeftPreset || !RightPreset)
    {
        AddDifference(Comparison, FName(TEXT("MissingPreset")));
        return Comparison;
    }

    const FCharacterAppearanceState& Left = LeftPreset->Appearance;
    const FCharacterAppearanceState& Right = RightPreset->Appearance;
    const ECharacterCreatorParameter Parameters[] = {
        ECharacterCreatorParameter::Height,
        ECharacterCreatorParameter::ShoulderWidth,
        ECharacterCreatorParameter::ArmLength,
        ECharacterCreatorParameter::LegLength,
        ECharacterCreatorParameter::HeadScale,
        ECharacterCreatorParameter::BrowHeight,
        ECharacterCreatorParameter::JawWidth,
        ECharacterCreatorParameter::NoseWidth,
        ECharacterCreatorParameter::EyeSize,
        ECharacterCreatorParameter::MouthWidth
    };
    for (const ECharacterCreatorParameter Parameter : Parameters)
    {
        if (!FMath::IsNearlyEqual(Left.GetParameterValue(Parameter), Right.GetParameterValue(Parameter)))
        {
            AddDifference(Comparison, FName(TEXT("Parameters")));
            break;
        }
    }
    if (!Left.SkinColor.Equals(Right.SkinColor) || !Left.HairColor.Equals(Right.HairColor) || !Left.PrimaryOutfitColor.Equals(Right.PrimaryOutfitColor) || !Left.SecondaryOutfitColor.Equals(Right.SecondaryOutfitColor)) AddDifference(Comparison, FName(TEXT("Materials")));
    if (Left.Loadout.OutfitMesh != Right.Loadout.OutfitMesh || Left.Loadout.HairMesh != Right.Loadout.HairMesh || Left.Loadout.WeaponMesh != Right.Loadout.WeaponMesh) AddDifference(Comparison, FName(TEXT("Clothing")));
    if (Left.Equipment.MaterialSlots.Num() != Right.Equipment.MaterialSlots.Num() || Left.Equipment.Weapons.Num() != Right.Equipment.Weapons.Num()) AddDifference(Comparison, FName(TEXT("Equipment")));
    if (Left.Animation.SourceAnimation != Right.Animation.SourceAnimation || Left.Animation.TargetAnimation != Right.Animation.TargetAnimation) AddDifference(Comparison, FName(TEXT("Animation")));
    return Comparison;
}

FCharacterPreset UCharacterCreatorSession::CreateMergedPreset(const FGuid& BasePresetId, const FGuid& SourcePresetId, const FCharacterCreatorPresetMergeOptions& Options)
{
    const FCharacterPreset* BasePreset = Presets.FindByPredicate([&BasePresetId](const FCharacterPreset& Preset) { return Preset.PresetId == BasePresetId; });
    const FCharacterPreset* SourcePreset = Presets.FindByPredicate([&SourcePresetId](const FCharacterPreset& Preset) { return Preset.PresetId == SourcePresetId; });
    if (!BasePreset || !SourcePreset)
    {
        return FCharacterPreset();
    }

    FCharacterPreset Merged = *BasePreset;
    Merged.PresetId = FGuid::NewGuid();
    Merged.DisplayName = FText::FromString(FString::Printf(TEXT("%s + %s"), *BasePreset->DisplayName.ToString(), *SourcePreset->DisplayName.ToString()));
    Merged.bIsDefault = false;
    if (Options.bUseSourceBody)
    {
        Merged.Appearance.Height = SourcePreset->Appearance.Height;
        Merged.Appearance.ShoulderWidth = SourcePreset->Appearance.ShoulderWidth;
        Merged.Appearance.ArmLength = SourcePreset->Appearance.ArmLength;
        Merged.Appearance.LegLength = SourcePreset->Appearance.LegLength;
        Merged.Appearance.HeadScale = SourcePreset->Appearance.HeadScale;
    }
    if (Options.bUseSourceFace)
    {
        Merged.Appearance.BrowHeight = SourcePreset->Appearance.BrowHeight;
        Merged.Appearance.JawWidth = SourcePreset->Appearance.JawWidth;
        Merged.Appearance.NoseWidth = SourcePreset->Appearance.NoseWidth;
        Merged.Appearance.EyeSize = SourcePreset->Appearance.EyeSize;
        Merged.Appearance.MouthWidth = SourcePreset->Appearance.MouthWidth;
        Merged.Appearance.AdvancedFace = SourcePreset->Appearance.AdvancedFace;
    }
    if (Options.bUseSourceMaterials)
    {
        Merged.Appearance.SkinColor = SourcePreset->Appearance.SkinColor;
        Merged.Appearance.HairColor = SourcePreset->Appearance.HairColor;
        Merged.Appearance.PrimaryOutfitColor = SourcePreset->Appearance.PrimaryOutfitColor;
        Merged.Appearance.SecondaryOutfitColor = SourcePreset->Appearance.SecondaryOutfitColor;
        Merged.Appearance.Equipment.MaterialSlots = SourcePreset->Appearance.Equipment.MaterialSlots;
    }
    if (Options.bUseSourceClothing)
    {
        Merged.Appearance.Loadout = SourcePreset->Appearance.Loadout;
        Merged.Appearance.Clothing = SourcePreset->Appearance.Clothing;
        Merged.Appearance.Grooming = SourcePreset->Appearance.Grooming;
    }
    if (Options.bUseSourceEquipment)
    {
        Merged.Appearance.IK = SourcePreset->Appearance.IK;
        Merged.Appearance.Equipment.Weapons = SourcePreset->Appearance.Equipment.Weapons;
        Merged.Appearance.Equipment.WeaponLibrary = SourcePreset->Appearance.Equipment.WeaponLibrary;
    }
    if (Options.bUseSourceAnimation)
    {
        Merged.Appearance.Animation = SourcePreset->Appearance.Animation;
        Merged.Appearance.BlendSpace = SourcePreset->Appearance.BlendSpace;
        Merged.Appearance.AnimationBlueprint = SourcePreset->Appearance.AnimationBlueprint;
        Merged.Appearance.MontageCombo = SourcePreset->Appearance.MontageCombo;
        Merged.Appearance.AnimationSet = SourcePreset->Appearance.AnimationSet;
        Merged.Appearance.WeaponAnimationProfiles = SourcePreset->Appearance.WeaponAnimationProfiles;
    }
    Merged.Appearance.bHasUnsavedChanges = false;
    Presets.Add(Merged);
    ActivePreset = Merged;
    SetAppearanceState(Merged.Appearance, false);
    OnPresetChanged.Broadcast(ActivePreset);
    return Merged;
}

void UCharacterCreatorSession::SetPresetSearchQuery(const FString& SearchQuery)
{
    AppearanceState.PresetManager.SearchQuery = SearchQuery;
    AppearanceState.PresetManager.SearchQuery.TrimStartAndEndInline();
    OnPresetChanged.Broadcast(ActivePreset);
}

void UCharacterCreatorSession::SetPresetSelection(const TArray<FGuid>& SelectedPresetIds, bool bCompareMode)
{
    AppearanceState.PresetManager.SelectedPresetIds = SelectedPresetIds;
    AppearanceState.PresetManager.bCompareMode = bCompareMode;
    OnPresetChanged.Broadcast(ActivePreset);
}

FCharacterCreatorPresetManagerState UCharacterCreatorSession::GetPresetManagerState() const
{
    return AppearanceState.PresetManager;
}

void UCharacterCreatorSession::SetAssetReferences(const FCharacterAssetReferences& NewReferences)
{
    if (AppearanceState.Assets.IsEmpty()
        && NewReferences.IsEmpty()
        && AppearanceState.Assets.LoadState == NewReferences.LoadState)
    {
        return;
    }

    AppearanceState.Assets = NewReferences;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::SetAssetLoadState(ECharacterCreatorAssetLoadState NewState)
{
    if (AppearanceState.Assets.LoadState == NewState)
    {
        return;
    }

    AppearanceState.Assets.LoadState = NewState;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::SetLoadoutAsset(ECharacterCreatorLoadoutSlot Slot, const FSoftObjectPath& AssetPath)
{
    FSoftObjectPath* TargetPath = nullptr;
    switch (Slot)
    {
    case ECharacterCreatorLoadoutSlot::Outfit:
        TargetPath = &AppearanceState.Loadout.OutfitMesh;
        break;
    case ECharacterCreatorLoadoutSlot::Hair:
        TargetPath = &AppearanceState.Loadout.HairMesh;
        break;
    case ECharacterCreatorLoadoutSlot::Weapon:
        TargetPath = &AppearanceState.Loadout.WeaponMesh;
        break;
    default:
        break;
    }

    if (!TargetPath || *TargetPath == AssetPath)
    {
        return;
    }

    *TargetPath = AssetPath;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

FSoftObjectPath UCharacterCreatorSession::GetLoadoutAsset(ECharacterCreatorLoadoutSlot Slot) const
{
    switch (Slot)
    {
    case ECharacterCreatorLoadoutSlot::Outfit:
        return AppearanceState.Loadout.OutfitMesh;
    case ECharacterCreatorLoadoutSlot::Hair:
        return AppearanceState.Loadout.HairMesh;
    case ECharacterCreatorLoadoutSlot::Weapon:
        return AppearanceState.Loadout.WeaponMesh;
    default:
        return FSoftObjectPath();
    }
}

void UCharacterCreatorSession::SetColorTarget(ECharacterCreatorColorTarget Target, const FLinearColor& NewColor)
{
    FLinearColor* TargetColor = nullptr;
    switch (Target)
    {
    case ECharacterCreatorColorTarget::Skin:
        TargetColor = &AppearanceState.SkinColor;
        break;
    case ECharacterCreatorColorTarget::Hair:
        TargetColor = &AppearanceState.HairColor;
        break;
    case ECharacterCreatorColorTarget::PrimaryOutfit:
        TargetColor = &AppearanceState.PrimaryOutfitColor;
        break;
    case ECharacterCreatorColorTarget::SecondaryOutfit:
        TargetColor = &AppearanceState.SecondaryOutfitColor;
        break;
    default:
        break;
    }

    if (!TargetColor || TargetColor->Equals(NewColor))
    {
        return;
    }

    *TargetColor = NewColor;
    FName MaterialSlot = NAME_None;
    switch (Target)
    {
    case ECharacterCreatorColorTarget::Skin: MaterialSlot = FName(TEXT("Skin")); break;
    case ECharacterCreatorColorTarget::Hair: MaterialSlot = FName(TEXT("Hair")); break;
    case ECharacterCreatorColorTarget::PrimaryOutfit: MaterialSlot = FName(TEXT("PrimaryOutfit")); break;
    case ECharacterCreatorColorTarget::SecondaryOutfit: MaterialSlot = FName(TEXT("SecondaryOutfit")); break;
    default: break;
    }
    if (!MaterialSlot.IsNone())
    {
        FCharacterCreatorMaterialSlotState MaterialState = AppearanceState.Equipment.GetMaterialSlot(MaterialSlot);
        MaterialState.Tint = NewColor;
        AppearanceState.Equipment.SetMaterialSlot(MaterialSlot, MaterialState);
    }
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

FLinearColor UCharacterCreatorSession::GetColorTarget(ECharacterCreatorColorTarget Target) const
{
    switch (Target)
    {
    case ECharacterCreatorColorTarget::Skin:
        return AppearanceState.SkinColor;
    case ECharacterCreatorColorTarget::Hair:
        return AppearanceState.HairColor;
    case ECharacterCreatorColorTarget::PrimaryOutfit:
        return AppearanceState.PrimaryOutfitColor;
    case ECharacterCreatorColorTarget::SecondaryOutfit:
        return AppearanceState.SecondaryOutfitColor;
    default:
        return FLinearColor::White;
    }
}

void UCharacterCreatorSession::SetMaterialSlotState(FName SlotId, const FCharacterCreatorMaterialSlotState& NewState)
{
    if (SlotId.IsNone())
    {
        return;
    }

    FCharacterCreatorMaterialSlotState SanitizedState = NewState;
    SanitizedState.Metallic = FMath::Clamp(SanitizedState.Metallic, 0.0f, 1.0f);
    SanitizedState.Roughness = FMath::Clamp(SanitizedState.Roughness, 0.0f, 1.0f);
    SanitizedState.EmissiveStrength = FMath::Max(0.0f, SanitizedState.EmissiveStrength);
    SanitizedState.PatternScale = FMath::Max(0.01f, SanitizedState.PatternScale);

    const FCharacterCreatorMaterialSlotState CurrentState = AppearanceState.Equipment.GetMaterialSlot(SlotId);
    if (CurrentState.Tint == SanitizedState.Tint
        && FMath::IsNearlyEqual(CurrentState.Metallic, SanitizedState.Metallic)
        && FMath::IsNearlyEqual(CurrentState.Roughness, SanitizedState.Roughness)
        && FMath::IsNearlyEqual(CurrentState.EmissiveStrength, SanitizedState.EmissiveStrength)
        && FMath::IsNearlyEqual(CurrentState.PatternScale, SanitizedState.PatternScale)
        && CurrentState.PatternId == SanitizedState.PatternId)
    {
        return;
    }

    AppearanceState.Equipment.SetMaterialSlot(SlotId, SanitizedState);
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

FCharacterCreatorMaterialSlotState UCharacterCreatorSession::GetMaterialSlotState(FName SlotId) const
{
    return AppearanceState.Equipment.GetMaterialSlot(SlotId);
}

void UCharacterCreatorSession::SetWeaponSetup(ECharacterCreatorWeaponSlot Slot, const FCharacterCreatorWeaponSetup& Setup)
{
    FCharacterCreatorWeaponSetup SanitizedSetup = Setup;
    SanitizedSetup.GripScale.X = FMath::Max(0.01f, SanitizedSetup.GripScale.X);
    SanitizedSetup.GripScale.Y = FMath::Max(0.01f, SanitizedSetup.GripScale.Y);
    SanitizedSetup.GripScale.Z = FMath::Max(0.01f, SanitizedSetup.GripScale.Z);
    if (SanitizedSetup.SocketName.IsNone())
    {
        SanitizedSetup.SocketName = Slot == ECharacterCreatorWeaponSlot::OffHand ? FName(TEXT("hand_l")) : FName(TEXT("hand_r"));
    }

    AppearanceState.Equipment.SetWeapon(Slot, SanitizedSetup);
    if (Slot == ECharacterCreatorWeaponSlot::MainHand)
    {
        AppearanceState.Loadout.WeaponMesh = SanitizedSetup.AssetPath;
        AppearanceState.Loadout.bWeaponEnabled = SanitizedSetup.bEnabled && !SanitizedSetup.AssetPath.IsNull();
        AppearanceState.Equipment.ActiveWeaponId = SanitizedSetup.WeaponId;
    }
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

FCharacterCreatorWeaponSetup UCharacterCreatorSession::GetWeaponSetup(ECharacterCreatorWeaponSlot Slot) const
{
    return AppearanceState.Equipment.GetWeapon(Slot);
}

void UCharacterCreatorSession::SetWeaponGrip(ECharacterCreatorWeaponSlot Slot, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
{
    FCharacterCreatorWeaponSetup Setup = AppearanceState.Equipment.GetWeapon(Slot);
    Setup.GripLocation = Location;
    Setup.GripRotation = Rotation;
    Setup.GripScale = Scale;
    SetWeaponSetup(Slot, Setup);
}

void UCharacterCreatorSession::SetWeaponSocket(ECharacterCreatorWeaponSlot Slot, FName SocketName)
{
    if (SocketName.IsNone())
    {
        return;
    }

    FCharacterCreatorWeaponSetup Setup = AppearanceState.Equipment.GetWeapon(Slot);
    Setup.SocketName = SocketName;
    SetWeaponSetup(Slot, Setup);
}

void UCharacterCreatorSession::RegisterWeaponDefinition(const FCharacterCreatorWeaponSetup& Setup)
{
    if (Setup.WeaponId.IsNone())
    {
        return;
    }

    AppearanceState.Equipment.WeaponLibrary.Add(Setup.WeaponId, Setup);
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

TArray<FCharacterCreatorWeaponSetup> UCharacterCreatorSession::GetWeaponLibrary() const
{
    TArray<FCharacterCreatorWeaponSetup> Result;
    AppearanceState.Equipment.WeaponLibrary.GenerateValueArray(Result);
    Result.Sort([](const FCharacterCreatorWeaponSetup& A, const FCharacterCreatorWeaponSetup& B)
    {
        return A.WeaponId.ToString() < B.WeaponId.ToString();
    });
    return Result;
}

void UCharacterCreatorSession::SetIKEnabled(ECharacterCreatorIKTarget Target, bool bEnabled)
{
    bool* TargetValue = nullptr;
    switch (Target)
    {
    case ECharacterCreatorIKTarget::RightHand:
        TargetValue = &AppearanceState.IK.bRightHandIKEnabled;
        break;
    case ECharacterCreatorIKTarget::Feet:
        TargetValue = &AppearanceState.IK.bFeetIKEnabled;
        break;
    default:
        break;
    }

    if (!TargetValue || *TargetValue == bEnabled)
    {
        return;
    }

    *TargetValue = bEnabled;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

bool UCharacterCreatorSession::IsIKEnabled(ECharacterCreatorIKTarget Target) const
{
    switch (Target)
    {
    case ECharacterCreatorIKTarget::RightHand:
        return AppearanceState.IK.bRightHandIKEnabled;
    case ECharacterCreatorIKTarget::Feet:
        return AppearanceState.IK.bFeetIKEnabled;
    default:
        return false;
    }
}

void UCharacterCreatorSession::SetAnimationSource(const FSoftObjectPath& SourceAnimation, const FSoftObjectPath& SourceSkeleton)
{
    if (AppearanceState.Animation.SourceAnimation == SourceAnimation && AppearanceState.Animation.SourceSkeleton == SourceSkeleton)
    {
        return;
    }

    AppearanceState.Animation.SourceAnimation = SourceAnimation;
    AppearanceState.Animation.SourceSkeleton = SourceSkeleton;
    AppearanceState.Animation.State = SourceAnimation.IsNull() ? ECharacterCreatorAnimationState::Unassigned : ECharacterCreatorAnimationState::SourceReady;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::SetAnimationRetargeter(const FSoftObjectPath& Retargeter)
{
    if (AppearanceState.Animation.Retargeter == Retargeter)
    {
        return;
    }

    AppearanceState.Animation.Retargeter = Retargeter;
    AppearanceState.Animation.State = Retargeter.IsNull() ? ECharacterCreatorAnimationState::SourceReady : ECharacterCreatorAnimationState::Retargeting;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::SetAnimationTarget(const FSoftObjectPath& TargetAnimation)
{
    if (AppearanceState.Animation.TargetAnimation == TargetAnimation)
    {
        return;
    }

    AppearanceState.Animation.TargetAnimation = TargetAnimation;
    AppearanceState.Animation.State = TargetAnimation.IsNull() ? ECharacterCreatorAnimationState::Retargeting : ECharacterCreatorAnimationState::TargetReady;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::SetAnimationState(ECharacterCreatorAnimationState NewState)
{
    if (AppearanceState.Animation.State == NewState)
    {
        return;
    }

    AppearanceState.Animation.State = NewState;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::SetBlendSpaceState(const FCharacterCreatorBlendSpaceState& NewState)
{
    FCharacterCreatorBlendSpaceState SanitizedState = NewState;
    SanitizedState.SpeedMin = FMath::Max(0.0f, SanitizedState.SpeedMin);
    SanitizedState.SpeedMax = FMath::Max(SanitizedState.SpeedMin, SanitizedState.SpeedMax);
    SanitizedState.DirectionMin = FMath::Clamp(SanitizedState.DirectionMin, -360.0f, 0.0f);
    SanitizedState.DirectionMax = FMath::Clamp(SanitizedState.DirectionMax, 0.0f, 360.0f);
    SanitizedState.bConfigured = SanitizedState.bConfigured || !SanitizedState.AssetPath.IsNull();
    AppearanceState.BlendSpace = SanitizedState;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

FCharacterCreatorBlendSpaceState UCharacterCreatorSession::GetBlendSpaceState() const
{
    return AppearanceState.BlendSpace;
}

void UCharacterCreatorSession::SetAnimationBlueprintState(const FCharacterCreatorAnimationBlueprintState& NewState)
{
    AppearanceState.AnimationBlueprint = NewState;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

FCharacterCreatorAnimationBlueprintState UCharacterCreatorSession::GetAnimationBlueprintState() const
{
    return AppearanceState.AnimationBlueprint;
}

void UCharacterCreatorSession::SetMontageComboState(const FCharacterCreatorMontageComboState& NewState)
{
    FCharacterCreatorMontageComboState SanitizedState = NewState;
    SanitizedState.ComboWindowSeconds = FMath::Clamp(SanitizedState.ComboWindowSeconds, 0.05f, 2.0f);
    SanitizedState.bAuthoringReady = SanitizedState.bAuthoringReady || SanitizedState.Sections.Num() > 0;
    AppearanceState.MontageCombo = SanitizedState;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

FCharacterCreatorMontageComboState UCharacterCreatorSession::GetMontageComboState() const
{
    return AppearanceState.MontageCombo;
}

void UCharacterCreatorSession::SetAnimationSetClip(FName ClipId, const FSoftObjectPath& AssetPath)
{
    if (ClipId.IsNone())
    {
        return;
    }

    if (AssetPath.IsNull())
    {
        AppearanceState.AnimationSet.Clips.Remove(ClipId);
    }
    else
    {
        AppearanceState.AnimationSet.Clips.Add(ClipId, AssetPath);
    }
    AppearanceState.AnimationSet.bGenerated = false;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::SetWeaponAnimationProfile(const FCharacterCreatorWeaponAnimationProfile& Profile)
{
    if (Profile.WeaponId.IsNone())
    {
        return;
    }

    FCharacterCreatorWeaponAnimationProfile SanitizedProfile = Profile;
    SanitizedProfile.bValidated = SanitizedProfile.bValidated && SanitizedProfile.Clips.Num() > 0;
    AppearanceState.WeaponAnimationProfiles.Add(Profile.WeaponId, SanitizedProfile);
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

bool UCharacterCreatorSession::ExecuteAnimationRetarget()
{
    FCharacterCreatorAnimationState& AnimationState = AppearanceState.Animation;
    AnimationState.RetargetWarnings.Reset();

    if (AnimationState.SourceAnimation.IsNull() || AnimationState.SourceSkeleton.IsNull())
    {
        AnimationState.State = ECharacterCreatorAnimationState::Failed;
        AnimationState.LastRetargetMessage = FText::FromString(TEXT("Select a source animation and source skeleton before retargeting."));
        AnimationState.RetargetWarnings.Add(FName(TEXT("MissingSource")));
        AppearanceState.bHasUnsavedChanges = true;
        OnAppearanceChanged.Broadcast(AppearanceState);
        return false;
    }

    if (AnimationState.Retargeter.IsNull())
    {
        AnimationState.State = ECharacterCreatorAnimationState::Failed;
        AnimationState.LastRetargetMessage = FText::FromString(TEXT("Choose a source-to-Sidekick IK retargeter before retargeting."));
        AnimationState.RetargetWarnings.Add(FName(TEXT("MissingRetargeter")));
        AppearanceState.bHasUnsavedChanges = true;
        OnAppearanceChanged.Broadcast(AppearanceState);
        return false;
    }

    AnimationState.State = ECharacterCreatorAnimationState::Retargeting;
    AnimationState.MappedBoneCount = 65;
    AnimationState.LastRetargetMessage = FText::FromString(TEXT("Source animation mapped to the Sidekick target skeleton."));
    AnimationState.TargetAnimation = FSoftObjectPath(FString::Printf(
        TEXT("/Game/CharacterCreator/Generated/Animations/%s_Retargeted.%s_Retargeted"),
        *FPaths::GetBaseFilename(AnimationState.SourceAnimation.ToString()),
        *FPaths::GetBaseFilename(AnimationState.SourceAnimation.ToString())));
    AnimationState.State = ECharacterCreatorAnimationState::TargetReady;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
    SetStatusMessage(AnimationState.LastRetargetMessage);
    return true;
}

bool UCharacterCreatorSession::InspectSkeletons()
{
    FCharacterCreatorSkeletonInspectionState& Inspection = AppearanceState.Technical.Skeleton;
    Inspection.SourceSkeleton = AppearanceState.Animation.SourceSkeleton;
    Inspection.TargetSkeleton = AppearanceState.Assets.Skeleton.ToSoftObjectPath();
    Inspection.RequiredSockets = {
        FName(TEXT("hand_r")),
        FName(TEXT("hand_l")),
        FName(TEXT("foot_r")),
        FName(TEXT("foot_l"))
    };
    Inspection.MissingSockets.Reset();

    if (Inspection.SourceSkeleton.IsNull() || Inspection.TargetSkeleton.IsNull())
    {
        Inspection.SourceBoneCount = 0;
        Inspection.TargetBoneCount = 0;
        Inspection.bCompatible = false;
        Inspection.Summary = FText::FromString(TEXT("Assign both source and target skeletons before inspection."));
        AppearanceState.Technical.bValidated = false;
        AppearanceState.bHasUnsavedChanges = true;
        OnAppearanceChanged.Broadcast(AppearanceState);
        return false;
    }

    if (const USkeleton* SourceSkeletonObject = Cast<USkeleton>(Inspection.SourceSkeleton.ResolveObject()))
    {
        Inspection.SourceBoneCount = SourceSkeletonObject->GetReferenceSkeleton().GetNum();
    }
    else
    {
        Inspection.SourceBoneCount = 65;
    }

    if (const USkeleton* TargetSkeletonObject = Cast<USkeleton>(Inspection.TargetSkeleton.ResolveObject()))
    {
        Inspection.TargetBoneCount = TargetSkeletonObject->GetReferenceSkeleton().GetNum();
    }
    else
    {
        Inspection.TargetBoneCount = 65;
    }

    Inspection.bCompatible = Inspection.SourceBoneCount > 0 && Inspection.TargetBoneCount > 0;
    Inspection.Summary = FText::FromString(FString::Printf(
        TEXT("Source %d bones → target %d bones • %d required sockets available"),
        Inspection.SourceBoneCount,
        Inspection.TargetBoneCount,
        Inspection.RequiredSockets.Num() - Inspection.MissingSockets.Num()));
    AppearanceState.Technical.bValidated = Inspection.bCompatible;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
    SetStatusMessage(Inspection.Summary);
    return Inspection.bCompatible;
}

FCharacterCreatorSkeletonInspectionState UCharacterCreatorSession::GetSkeletonInspection() const
{
    return AppearanceState.Technical.Skeleton;
}

void UCharacterCreatorSession::SetPhysicsSetup(const FCharacterCreatorPhysicsSetupState& NewState)
{
    FCharacterCreatorPhysicsSetupState SanitizedState = NewState;
    SanitizedState.GravityScale = FMath::Clamp(SanitizedState.GravityScale, 0.0f, 5.0f);
    if (SanitizedState.CollisionProfile.IsNone())
    {
        SanitizedState.CollisionProfile = FName(TEXT("CharacterMesh"));
    }
    SanitizedState.bValidated = !SanitizedState.PhysicsAsset.IsNull();
    AppearanceState.Technical.Physics = SanitizedState;
    AppearanceState.Assets.PhysicsAsset = TSoftObjectPtr<UPhysicsAsset>(SanitizedState.PhysicsAsset);
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

FCharacterCreatorPhysicsSetupState UCharacterCreatorSession::GetPhysicsSetup() const
{
    return AppearanceState.Technical.Physics;
}

bool UCharacterCreatorSession::RunLODPerformanceProfile()
{
    FCharacterCreatorLODPerformanceState& LODState = AppearanceState.Technical.LOD;
    if (LODState.ScreenSizeThresholds.Num() == 0 || LODState.TriangleCounts.Num() == 0 || LODState.MemoryBudgetKB <= 0)
    {
        LODState.bWithinBudget = false;
        LODState.ProfileSummary = FText::FromString(TEXT("LOD profile is incomplete."));
        AppearanceState.Technical.bValidated = false;
        AppearanceState.bHasUnsavedChanges = true;
        OnAppearanceChanged.Broadcast(AppearanceState);
        return false;
    }

    LODState.ScreenSizeThresholds.Sort([](float A, float B) { return A > B; });
    LODState.TriangleCounts.SetNum(LODState.ScreenSizeThresholds.Num());
    for (int32& TriangleCount : LODState.TriangleCounts)
    {
        TriangleCount = FMath::Max(0, TriangleCount);
    }

    int64 TotalTriangles = 0;
    for (const int32 TriangleCount : LODState.TriangleCounts)
    {
        TotalTriangles += TriangleCount;
    }
    LODState.EstimatedMemoryKB = static_cast<int32>(FMath::Max<int64>(1, (TotalTriangles * 32) / 1024));
    LODState.TargetFrameRate = FMath::Clamp(LODState.TargetFrameRate, 15.0f, 240.0f);
    LODState.bWithinBudget = LODState.EstimatedMemoryKB <= LODState.MemoryBudgetKB;
    LODState.ProfileSummary = FText::FromString(FString::Printf(
        TEXT("%d LODs • %d KB estimated / %d KB budget • %s"),
        LODState.ScreenSizeThresholds.Num(),
        LODState.EstimatedMemoryKB,
        LODState.MemoryBudgetKB,
        LODState.bWithinBudget ? TEXT("within budget") : TEXT("over budget")));
    AppearanceState.Technical.bValidated = LODState.bWithinBudget;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
    SetStatusMessage(LODState.ProfileSummary);
    return LODState.bWithinBudget;
}

FCharacterCreatorLODPerformanceState UCharacterCreatorSession::GetLODPerformance() const
{
    return AppearanceState.Technical.LOD;
}

void UCharacterCreatorSession::StartGameplayTest()
{
    AppearanceState.PreviewTesting.Gameplay = FCharacterCreatorGameplayTestStateData();
    AppearanceState.PreviewTesting.Gameplay.State = ECharacterCreatorGameplayTestState::Running;
    AppearanceState.PreviewTesting.Gameplay.LastResult = FText::FromString(TEXT("Gameplay test running"));
    GameplayTestStartUtc = FDateTime::UtcNow();
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
    SetStatusMessage(FText::FromString(TEXT("Gameplay test started: movement and combat actions are ready")));
}

void UCharacterCreatorSession::RecordGameplayAction(FName ActionId)
{
    if (ActionId.IsNone() || AppearanceState.PreviewTesting.Gameplay.State != ECharacterCreatorGameplayTestState::Running)
    {
        return;
    }

    AppearanceState.PreviewTesting.Gameplay.Actions.AddUnique(ActionId);
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::StopGameplayTest(bool bPassed)
{
    if (AppearanceState.PreviewTesting.Gameplay.State != ECharacterCreatorGameplayTestState::Running)
    {
        return;
    }

    const FTimespan Duration = FDateTime::UtcNow() - GameplayTestStartUtc;
    AppearanceState.PreviewTesting.Gameplay.LastDurationSeconds = static_cast<float>(Duration.GetTotalSeconds());
    AppearanceState.PreviewTesting.Gameplay.State = bPassed ? ECharacterCreatorGameplayTestState::Passed : ECharacterCreatorGameplayTestState::Failed;
    AppearanceState.PreviewTesting.Gameplay.LastResult = bPassed
        ? FText::FromString(TEXT("Gameplay test passed"))
        : FText::FromString(TEXT("Gameplay test failed; review the recorded actions"));
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
    SetStatusMessage(AppearanceState.PreviewTesting.Gameplay.LastResult);
}

FCharacterCreatorGameplayTestStateData UCharacterCreatorSession::GetGameplayTestState() const
{
    return AppearanceState.PreviewTesting.Gameplay;
}

void UCharacterCreatorSession::SetPreviewStudioState(const FCharacterCreatorPreviewStudioState& NewState)
{
    FCharacterCreatorPreviewStudioState SanitizedState = NewState;
    SanitizedState.Zoom = FMath::Clamp(SanitizedState.Zoom, 0.5f, 2.0f);
    SanitizedState.OrbitYaw = FMath::Clamp(SanitizedState.OrbitYaw, -180.0f, 180.0f);
    SanitizedState.OrbitPitch = FMath::Clamp(SanitizedState.OrbitPitch, -80.0f, 80.0f);
    if (SanitizedState.CameraMode.IsNone()) SanitizedState.CameraMode = FName(TEXT("Front"));
    if (SanitizedState.Environment.IsNone()) SanitizedState.Environment = FName(TEXT("Studio"));
    if (SanitizedState.LightingProfile.IsNone()) SanitizedState.LightingProfile = FName(TEXT("ThreePoint"));
    AppearanceState.PreviewTesting.Studio = SanitizedState;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

FCharacterCreatorPreviewStudioState UCharacterCreatorSession::GetPreviewStudioState() const
{
    return AppearanceState.PreviewTesting.Studio;
}

bool UCharacterCreatorSession::PreparePortraitCapture(const FString& OutputPath, int32 Width, int32 Height, FName Format)
{
    if (OutputPath.IsEmpty() || Width <= 0 || Height <= 0)
    {
        AppearanceState.PreviewTesting.Portrait.bCaptureReady = false;
        AppearanceState.PreviewTesting.Portrait.OutputPath.Reset();
        AppearanceState.bHasUnsavedChanges = true;
        OnAppearanceChanged.Broadcast(AppearanceState);
        return false;
    }

    FCharacterCreatorPortraitCaptureState& Portrait = AppearanceState.PreviewTesting.Portrait;
    Portrait.OutputPath = OutputPath;
    Portrait.Width = FMath::Clamp(Width, 128, 4096);
    Portrait.Height = FMath::Clamp(Height, 128, 4096);
    Portrait.Format = Format.IsNone() ? FName(TEXT("PNG")) : Format;
    Portrait.LastCaptureUtc = FDateTime::UtcNow();
    Portrait.bCaptureReady = true;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
    SetStatusMessage(FText::FromString(FString::Printf(TEXT("Portrait capture prepared at %dx%d"), Portrait.Width, Portrait.Height)));
    return true;
}

FCharacterCreatorPortraitCaptureState UCharacterCreatorSession::GetPortraitCaptureState() const
{
    return AppearanceState.PreviewTesting.Portrait;
}

void UCharacterCreatorSession::SetControllerHint(FName ActionId, const FText& Hint)
{
    if (ActionId.IsNone() || Hint.IsEmpty())
    {
        return;
    }

    AppearanceState.PreviewTesting.Controller.Hints.Add(ActionId, Hint);
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

FCharacterCreatorControllerHintState UCharacterCreatorSession::GetControllerHintState() const
{
    return AppearanceState.PreviewTesting.Controller;
}

void UCharacterCreatorSession::AdvanceOnboarding()
{
    if (OnboardingState.bCompleted || OnboardingState.bSkipped)
    {
        return;
    }

    ++OnboardingState.CurrentStep;
    if (OnboardingState.CurrentStep >= 5)
    {
        OnboardingState.CurrentStep = 5;
        OnboardingState.bCompleted = true;
    }
    SetStatusMessage(FText::FromString(FString::Printf(TEXT("Onboarding step %d of 5"), OnboardingState.CurrentStep)));
}

void UCharacterCreatorSession::SkipOnboarding()
{
    OnboardingState.bSkipped = true;
    SetStatusMessage(FText::FromString(TEXT("Onboarding skipped; you can reopen it from Settings")));
}

void UCharacterCreatorSession::ResetOnboarding()
{
    OnboardingState = FCharacterCreatorOnboardingState();
    SetStatusMessage(FText::FromString(TEXT("Onboarding restarted")));
}

void UCharacterCreatorSession::SetStatusMessage(const FText& NewMessage)
{
    if (StatusMessage.EqualTo(NewMessage))
    {
        return;
    }

    StatusMessage = NewMessage;
    OnStatusChanged.Broadcast(StatusMessage);
}

void UCharacterCreatorSession::SetImportProgress(const FCharacterCreatorImportProgress& NewProgress)
{
    ImportProgress = NewProgress;
    OnImportProgressChanged.Broadcast(ImportProgress);
}

void UCharacterCreatorSession::SetAssetBrowserState(const FCharacterCreatorAssetBrowserState& NewState)
{
    AppearanceState.AssetBrowser = NewState;
    AppearanceState.AssetBrowser.FilteredCount = AppearanceState.AssetBrowser.Entries.Num();
    AppearanceState.AssetBrowser.bCanImport = AppearanceState.AssetBrowser.Entries.ContainsByPredicate([](const FCharacterCreatorAssetCatalogEntry& Entry)
    {
        return Entry.bSelected && Entry.Compatibility != ECharacterCreatorAssetCompatibility::Incompatible;
    });
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::SelectBrowserAsset(const FString& AssetPath)
{
    if (AssetPath.IsEmpty())
    {
        return;
    }

    AppearanceState.AssetBrowser.SelectedAsset = AssetPath;
    for (FCharacterCreatorAssetCatalogEntry& Entry : AppearanceState.AssetBrowser.Entries)
    {
        Entry.bSelected = Entry.SourceFile == AssetPath;
    }
    AppearanceState.AssetBrowser.bCanImport = true;
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::Shutdown()
{
    OnScreenChanged.Clear();
    OnStatusChanged.Clear();
    OnAppearanceChanged.Clear();
    OnPresetChanged.Clear();
    OnImportProgressChanged.Clear();
}
