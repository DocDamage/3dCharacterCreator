#include "UI/CharacterCreatorExportService.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    void AddIssue(TArray<FCharacterCreatorValidationIssue>& Issues, ECharacterCreatorValidationSeverity Severity, FName Code, const TCHAR* Message, const TCHAR* Remediation)
    {
        FCharacterCreatorValidationIssue& Issue = Issues.AddDefaulted_GetRef();
        Issue.Severity = Severity;
        Issue.Code = Code;
        Issue.Message = FText::FromString(Message);
        Issue.Remediation = FText::FromString(Remediation);
    }

    TSharedPtr<FJsonValue> ColorToJson(const FLinearColor& Color)
    {
        TSharedPtr<FJsonObject> ColorObject = MakeShared<FJsonObject>();
        ColorObject->SetNumberField(TEXT("r"), Color.R);
        ColorObject->SetNumberField(TEXT("g"), Color.G);
        ColorObject->SetNumberField(TEXT("b"), Color.B);
        ColorObject->SetNumberField(TEXT("a"), Color.A);
        return MakeShared<FJsonValueObject>(ColorObject);
    }

    void SetSoftPath(TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, const FSoftObjectPath& Path)
    {
        Object->SetStringField(FieldName, Path.IsNull() ? FString() : Path.ToString());
    }
}

void FCharacterCreatorExportService::ValidateAppearance(const FCharacterAppearanceState& Appearance, const FCharacterCreatorExportProfile& Profile, TArray<FCharacterCreatorValidationIssue>& OutIssues)
{
    OutIssues.Reset();

    if (Profile.Version <= 0)
    {
        AddIssue(OutIssues, ECharacterCreatorValidationSeverity::Error, TEXT("InvalidProfileVersion"), TEXT("The export profile version is invalid."), TEXT("Use the current export profile version."));
    }

    if (Profile.bIncludeMesh)
    {
        if (Appearance.Assets.SkeletalMesh.IsNull())
        {
            AddIssue(OutIssues, ECharacterCreatorValidationSeverity::Error, TEXT("MissingCharacterMesh"), TEXT("No character skeletal mesh is assigned."), TEXT("Assign a valid Sidekick skeletal mesh before exporting the mesh profile."));
        }
        else if (!Appearance.Assets.SkeletalMesh.ToSoftObjectPath().IsValid())
        {
            AddIssue(OutIssues, ECharacterCreatorValidationSeverity::Error, TEXT("InvalidCharacterMesh"), TEXT("The character skeletal mesh reference is not a valid asset path."), TEXT("Replace the mesh reference with an asset from the project Content folder."));
        }
    }

    if (Profile.bIncludeMaterials && Appearance.Assets.BaseMaterial.IsNull())
    {
        AddIssue(OutIssues, ECharacterCreatorValidationSeverity::Error, TEXT("MissingBaseMaterial"), TEXT("No base material is assigned."), TEXT("Assign a material before exporting the materials profile."));
    }

    if (Profile.bIncludeAnimations && Appearance.Assets.AnimationInstanceClass.IsNull() && Appearance.Assets.PreviewAnimation.IsNull())
    {
        AddIssue(OutIssues, ECharacterCreatorValidationSeverity::Warning, TEXT("NoAnimationPreview"), TEXT("No compatible animation preview is assigned."), TEXT("Retarget a FAB animation to the Sidekick skeleton before exporting animation data."));
    }

    if (!Appearance.Loadout.OutfitMesh.IsNull() && !Appearance.Loadout.OutfitMesh.IsValid())
    {
        AddIssue(OutIssues, ECharacterCreatorValidationSeverity::Error, TEXT("InvalidOutfitAsset"), TEXT("The selected outfit asset reference is invalid."), TEXT("Choose an outfit asset from the Sidekick Content folder or remove the selection."));
    }

    if (!Appearance.Loadout.HairMesh.IsNull() && !Appearance.Loadout.HairMesh.IsValid())
    {
        AddIssue(OutIssues, ECharacterCreatorValidationSeverity::Error, TEXT("InvalidHairAsset"), TEXT("The selected hair asset reference is invalid."), TEXT("Choose a hair asset from the Sidekick Content folder or remove the selection."));
    }

    if (Profile.bIncludeMetadata)
    {
        AddIssue(OutIssues, ECharacterCreatorValidationSeverity::Info, TEXT("MetadataReady"), TEXT("Character metadata is ready for export."), TEXT("No action required."));
    }
}

bool FCharacterCreatorExportService::HasErrors(const TArray<FCharacterCreatorValidationIssue>& Issues)
{
    return Issues.ContainsByPredicate([](const FCharacterCreatorValidationIssue& Issue)
    {
        return Issue.Severity == ECharacterCreatorValidationSeverity::Error;
    });
}

bool FCharacterCreatorExportService::BuildManifestJson(const FCharacterAppearanceState& Appearance, const FCharacterPreset& Preset, const FCharacterCreatorExportProfile& Profile, FString& OutJson)
{
    TArray<FCharacterCreatorValidationIssue> Issues;
    ValidateAppearance(Appearance, Profile, Issues);
    if (HasErrors(Issues))
    {
        return false;
    }

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(TEXT("profileVersion"), Profile.Version);
    Root->SetStringField(TEXT("presetId"), Preset.PresetId.ToString(EGuidFormats::DigitsWithHyphens));
    Root->SetStringField(TEXT("presetName"), Preset.DisplayName.ToString());

    TSharedPtr<FJsonObject> Assets = MakeShared<FJsonObject>();
    if (Profile.bIncludeMesh)
    {
        SetSoftPath(Assets, TEXT("skeletalMesh"), Appearance.Assets.SkeletalMesh.ToSoftObjectPath());
        SetSoftPath(Assets, TEXT("skeleton"), Appearance.Assets.Skeleton.ToSoftObjectPath());
        SetSoftPath(Assets, TEXT("physicsAsset"), Appearance.Assets.PhysicsAsset.ToSoftObjectPath());
    }
    if (Profile.bIncludeMaterials)
    {
        SetSoftPath(Assets, TEXT("baseMaterial"), Appearance.Assets.BaseMaterial.ToSoftObjectPath());
    }
    if (Profile.bIncludeAnimations)
    {
        SetSoftPath(Assets, TEXT("animationInstance"), Appearance.Assets.AnimationInstanceClass.ToSoftObjectPath());
        SetSoftPath(Assets, TEXT("previewAnimation"), Appearance.Assets.PreviewAnimation.ToSoftObjectPath());
        SetSoftPath(Assets, TEXT("sourceAnimation"), Appearance.Animation.SourceAnimation);
        SetSoftPath(Assets, TEXT("sourceSkeleton"), Appearance.Animation.SourceSkeleton);
        SetSoftPath(Assets, TEXT("retargeter"), Appearance.Animation.Retargeter);
        SetSoftPath(Assets, TEXT("targetAnimation"), Appearance.Animation.TargetAnimation);
    }
    SetSoftPath(Assets, TEXT("outfitMesh"), Appearance.Loadout.OutfitMesh);
    SetSoftPath(Assets, TEXT("hairMesh"), Appearance.Loadout.HairMesh);
    SetSoftPath(Assets, TEXT("weaponMesh"), Appearance.Loadout.WeaponMesh);
    Root->SetObjectField(TEXT("assets"), Assets);

    TSharedPtr<FJsonObject> Colors = MakeShared<FJsonObject>();
    Colors->SetField(TEXT("skin"), ColorToJson(Appearance.SkinColor));
    Colors->SetField(TEXT("hair"), ColorToJson(Appearance.HairColor));
    Colors->SetField(TEXT("primaryOutfit"), ColorToJson(Appearance.PrimaryOutfitColor));
    Colors->SetField(TEXT("secondaryOutfit"), ColorToJson(Appearance.SecondaryOutfitColor));
    Root->SetObjectField(TEXT("colors"), Colors);

    TSharedPtr<FJsonObject> Parameters = MakeShared<FJsonObject>();
    Parameters->SetNumberField(TEXT("height"), Appearance.Height);
    Parameters->SetNumberField(TEXT("shoulderWidth"), Appearance.ShoulderWidth);
    Parameters->SetNumberField(TEXT("armLength"), Appearance.ArmLength);
    Parameters->SetNumberField(TEXT("legLength"), Appearance.LegLength);
    Parameters->SetNumberField(TEXT("headScale"), Appearance.HeadScale);
    Parameters->SetNumberField(TEXT("browHeight"), Appearance.BrowHeight);
    Parameters->SetNumberField(TEXT("jawWidth"), Appearance.JawWidth);
    Parameters->SetNumberField(TEXT("noseWidth"), Appearance.NoseWidth);
    Parameters->SetNumberField(TEXT("eyeSize"), Appearance.EyeSize);
    Parameters->SetNumberField(TEXT("mouthWidth"), Appearance.MouthWidth);
    Root->SetObjectField(TEXT("parameters"), Parameters);

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    return FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
}

bool FCharacterCreatorExportService::BuildBlueprintDescriptor(const FCharacterAppearanceState& Appearance, const FCharacterPreset& Preset, FString& OutJson)
{
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("format"), TEXT("CharacterCreator.BlueprintDescriptor.v1"));
    Root->SetStringField(TEXT("assetName"), Preset.DisplayName.ToString().IsEmpty() ? TEXT("CharacterCreator_ActiveCharacter") : Preset.DisplayName.ToString());
    Root->SetStringField(TEXT("targetClass"), TEXT("CharacterCreatorGeneratedCharacter"));
    Root->SetStringField(TEXT("skeletalMesh"), Appearance.Assets.SkeletalMesh.ToSoftObjectPath().ToString());
    Root->SetStringField(TEXT("animationInstance"), Appearance.Assets.AnimationInstanceClass.ToSoftObjectPath().ToString());
    Root->SetStringField(TEXT("physicsAsset"), Appearance.Assets.PhysicsAsset.ToSoftObjectPath().ToString());
    Root->SetBoolField(TEXT("generated"), true);

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    return FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
}

bool FCharacterCreatorExportService::BuildDataAssetDescriptor(const FCharacterAppearanceState& Appearance, const FCharacterPreset& Preset, FString& OutJson)
{
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("format"), TEXT("CharacterCreator.DataAssetDescriptor.v1"));
    Root->SetStringField(TEXT("assetName"), Preset.DisplayName.ToString().IsEmpty() ? TEXT("DA_CharacterCreator_ActiveCharacter") : FString::Printf(TEXT("DA_%s"), *Preset.DisplayName.ToString()));
    Root->SetStringField(TEXT("class"), TEXT("CharacterCreatorAppearanceData"));
    Root->SetStringField(TEXT("presetId"), Preset.PresetId.ToString(EGuidFormats::DigitsWithHyphens));
    Root->SetNumberField(TEXT("height"), Appearance.Height);
    Root->SetNumberField(TEXT("shoulderWidth"), Appearance.ShoulderWidth);
    Root->SetNumberField(TEXT("headScale"), Appearance.HeadScale);
    Root->SetStringField(TEXT("baseMaterial"), Appearance.Assets.BaseMaterial.ToSoftObjectPath().ToString());
    Root->SetBoolField(TEXT("generated"), true);

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    return FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
}

bool FCharacterCreatorExportService::BuildPackageDescriptor(const FCharacterAppearanceState& Appearance, const FCharacterPreset& Preset, const FCharacterCreatorExportProfile& Profile, FString& OutJson)
{
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("format"), TEXT("CharacterCreator.PackageDescriptor.v1"));
    Root->SetStringField(TEXT("packageName"), Preset.DisplayName.ToString().IsEmpty() ? TEXT("CharacterCreator_ActiveCharacter") : Preset.DisplayName.ToString());
    Root->SetStringField(TEXT("contentRoot"), TEXT("/Game/CharacterCreator/Generated"));
    Root->SetBoolField(TEXT("includeMesh"), Profile.bIncludeMesh);
    Root->SetBoolField(TEXT("includeMaterials"), Profile.bIncludeMaterials);
    Root->SetBoolField(TEXT("includeAnimations"), Profile.bIncludeAnimations);
    Root->SetBoolField(TEXT("includeMetadata"), Profile.bIncludeMetadata);
    Root->SetStringField(TEXT("skeletalMesh"), Appearance.Assets.SkeletalMesh.ToSoftObjectPath().ToString());
    Root->SetStringField(TEXT("skeleton"), Appearance.Assets.Skeleton.ToSoftObjectPath().ToString());
    Root->SetStringField(TEXT("physicsAsset"), Appearance.Assets.PhysicsAsset.ToSoftObjectPath().ToString());
    Root->SetBoolField(TEXT("generated"), true);

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    return FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
}
