#include "UI/CharacterCreatorSession.h"

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

float UCharacterCreatorSession::GetParameterValue(ECharacterCreatorParameter Parameter) const
{
    return AppearanceState.GetParameterValue(Parameter);
}

void UCharacterCreatorSession::SetAppearanceState(const FCharacterAppearanceState& NewState, bool bMarkDirty)
{
    AppearanceState = NewState;
    AppearanceState.Version = FCharacterAppearanceState::CurrentVersion;
    AppearanceState.bHasUnsavedChanges = bMarkDirty;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::ResetAppearance()
{
    AppearanceState = FCharacterAppearanceState();
    AppearanceState.bHasUnsavedChanges = true;
    OnAppearanceChanged.Broadcast(AppearanceState);
}

void UCharacterCreatorSession::SetActivePreset(const FCharacterPreset& NewPreset)
{
    ActivePreset = NewPreset;
    OnPresetChanged.Broadcast(ActivePreset);
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

void UCharacterCreatorSession::SetStatusMessage(const FText& NewMessage)
{
    if (StatusMessage.EqualTo(NewMessage))
    {
        return;
    }

    StatusMessage = NewMessage;
    OnStatusChanged.Broadcast(StatusMessage);
}

void UCharacterCreatorSession::Shutdown()
{
    OnScreenChanged.Clear();
    OnStatusChanged.Clear();
    OnAppearanceChanged.Clear();
    OnPresetChanged.Clear();
}
