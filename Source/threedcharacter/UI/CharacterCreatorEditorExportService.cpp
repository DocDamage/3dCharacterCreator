#include "UI/CharacterCreatorEditorExportService.h"

#if WITH_EDITOR

#include "AssetToolsModule.h"
#include "Animation/AnimSequenceBase.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "CharacterCreatorGeneratedAssets.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "Factories/DataAssetFactory.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "HAL/FileManager.h"
#include "IKRigEditor/Public/RigEditor/IKRigController.h"
#include "IKRigEditor/Public/RigEditor/IKRigDefinitionFactory.h"
#include "IKRigEditor/Public/RetargetEditor/IKRetargetBatchOperation.h"
#include "IKRigEditor/Public/RetargetEditor/IKRetargetFactory.h"
#include "IKRigEditor/Public/RetargetEditor/IKRetargeterController.h"
#include "IKRig/Public/Retargeter/IKRetargetChainMapping.h"
#include "IKRig/Public/Retargeter/IKRetargeter.h"
#include "IKRig/Public/Rig/IKRigDefinition.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
    FString MakeSafeAssetName(const FText& DisplayName)
    {
        FString Name = DisplayName.ToString().TrimStartAndEnd();
        if (Name.IsEmpty())
        {
            Name = TEXT("ActiveCharacter");
        }
        Name = FPaths::MakeValidFileName(Name, TEXT('_'));
        Name.ReplaceInline(TEXT(" "), TEXT("_"));
        return Name;
    }

    FString MakeObjectPath(const FString& PackagePath, const FString& AssetName)
    {
        // AssetTools creates one package per asset beneath the requested folder:
        // /Game/Folder/AssetName.AssetName.
        return FString::Printf(TEXT("%s/%s.%s"), *PackagePath, *AssetName, *AssetName);
    }

    bool IsPackageAvailable(const FSoftObjectPath& AssetPath)
    {
        return AssetPath.ResolveObject() != nullptr
            || (!AssetPath.GetLongPackageName().IsEmpty() && FPackageName::DoesPackageExist(AssetPath.GetLongPackageName()));
    }

    UObject* CreateOrLoadAsset(IAssetTools& AssetTools, const FString& PackagePath, const FString& AssetName, UFactory* Factory, UClass* AssetClass)
    {
        const FString ObjectPath = MakeObjectPath(PackagePath, AssetName);
        if (UObject* Existing = StaticLoadObject(AssetClass, nullptr, *ObjectPath))
        {
            Existing->Modify();
            return Existing;
        }
        return AssetTools.CreateAsset(AssetName, PackagePath, AssetClass, Factory);
    }

    bool SaveAssetPackage(UObject* Asset, FString& OutFilename)
    {
        if (!Asset || !Asset->GetOutermost())
        {
            return false;
        }

        UPackage* Package = Asset->GetOutermost();
        Package->SetDirtyFlag(true);
        OutFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Package, Asset, *OutFilename, SaveArgs);
    }

    bool EnsureMannyMaterialFallbacks()
    {
        UMaterialInterface* ParentMaterial = Cast<UMaterialInterface>(StaticLoadObject(
            UMaterialInterface::StaticClass(),
            nullptr,
            TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
        if (!ParentMaterial)
        {
            return false;
        }

        IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
        const FString PackagePath = TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Materials/Instances/Manny");
        const TArray<FString> MaterialNames = { TEXT("MI_Manny_01"), TEXT("MI_Manny_02") };
        bool bAllAvailable = true;
        for (const FString& MaterialName : MaterialNames)
        {
            const FString ObjectPath = MakeObjectPath(PackagePath, MaterialName);
            const FString PackageName = FPackageName::ObjectPathToPackageName(ObjectPath);
            const bool bPackageExists = FPackageName::DoesPackageExist(PackageName);
            if (bPackageExists && StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *ObjectPath))
            {
                continue;
            }

            UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
            Factory->InitialParent = ParentMaterial;
            UObject* FallbackMaterial = AssetTools.CreateAsset(
                MaterialName,
                PackagePath,
                UMaterialInstanceConstant::StaticClass(),
                Factory);
            FString SavedFilename;
            if (!FallbackMaterial || !SaveAssetPackage(FallbackMaterial, SavedFilename))
            {
                bAllAvailable = false;
            }
        }
        return bAllAvailable;
    }

    void StagePackageFiles(const FString& PackageFilename, const FString& DestinationDirectory, TArray<FString>& OutFiles)
    {
        if (PackageFilename.IsEmpty() || DestinationDirectory.IsEmpty())
        {
            return;
        }

        IFileManager::Get().MakeDirectory(*DestinationDirectory, true);
        const FString SourceStem = FPaths::GetBaseFilename(PackageFilename);
        const FString SourceDirectory = FPaths::GetPath(PackageFilename);
        const FString DestinationStem = FPaths::Combine(DestinationDirectory, SourceStem);
        const TArray<FString> Extensions = { TEXT("uasset"), TEXT("uexp"), TEXT("ubulk") };

        for (const FString& Extension : Extensions)
        {
            const FString Source = FPaths::Combine(SourceDirectory, SourceStem + TEXT(".") + Extension);
            if (!IFileManager::Get().FileExists(*Source))
            {
                continue;
            }
            const FString Destination = DestinationStem + TEXT(".") + Extension;
            if (IFileManager::Get().Copy(*Destination, *Source, true, true) == COPY_OK)
            {
                OutFiles.Add(Destination);
            }
        }
    }

    bool WriteExportManifest(
        const FCharacterAppearanceState& Appearance,
        const FCharacterPreset& Preset,
        const FCharacterCreatorExportProfile& Profile,
        const FCharacterCreatorRealExportResult& Result,
        const FString& DestinationDirectory)
    {
        if (DestinationDirectory.IsEmpty())
        {
            return true;
        }

        TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
        Root->SetStringField(TEXT("format"), TEXT("CharacterCreator.RealPackage.v1"));
        Root->SetStringField(TEXT("blueprint"), Result.BlueprintAsset.ToString());
        Root->SetStringField(TEXT("dataAsset"), Result.DataAsset.ToString());
        Root->SetStringField(TEXT("contentPath"), Result.GeneratedContentPath);
        Root->SetStringField(TEXT("presetId"), Preset.PresetId.ToString(EGuidFormats::DigitsWithHyphens));
        Root->SetStringField(TEXT("presetName"), Preset.DisplayName.ToString());
        Root->SetNumberField(TEXT("profileVersion"), Profile.Version);

        TArray<TSharedPtr<FJsonValue>> Files;
        for (const FString& File : Result.StagedFiles)
        {
            Files.Add(MakeShared<FJsonValueString>(File));
        }
        Root->SetArrayField(TEXT("stagedFiles"), Files);

        FString ManifestJson;
        const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ManifestJson);
        if (!FJsonSerializer::Serialize(Root.ToSharedRef(), Writer))
        {
            return false;
        }

        return FFileHelper::SaveStringToFile(ManifestJson, *FPaths::Combine(DestinationDirectory, TEXT("ActiveCharacter.package-manifest.json")));
    }
}

bool FCharacterCreatorEditorExportService::GenerateRetargetedAnimation(
    const FCharacterAppearanceState& Appearance,
    FSoftObjectPath& OutTargetAnimation,
    FString& OutErrorMessage)
{
    OutTargetAnimation.Reset();
    OutErrorMessage.Reset();

    // The extracted FAB sample omits two Manny material instances referenced by
    // the source mesh. Create project-local fallback packages at those exact
    // paths before loading the mesh so dependency validation is complete and
    // the real retarget path does not rely on missing vendor files.
    EnsureMannyMaterialFallbacks();

    UAnimSequenceBase* SourceAnimation = IsPackageAvailable(Appearance.Animation.SourceAnimation)
        ? Cast<UAnimSequenceBase>(Appearance.Animation.SourceAnimation.TryLoad())
        : nullptr;
    USkeletalMesh* SourceMesh = IsPackageAvailable(Appearance.Animation.SourceSkeleton)
        ? Cast<USkeletalMesh>(Appearance.Animation.SourceSkeleton.TryLoad())
        : nullptr;
    const FSoftObjectPath TargetMeshPath = Appearance.Assets.SkeletalMesh.ToSoftObjectPath();
    USkeletalMesh* TargetMesh = IsPackageAvailable(TargetMeshPath)
        ? Cast<USkeletalMesh>(TargetMeshPath.TryLoad())
        : nullptr;
    if (!SourceAnimation || !SourceMesh || !TargetMesh)
    {
        OutErrorMessage = FString::Printf(
            TEXT("Real retargeting requires loadable assets (animation=%s:%s, sourceMesh=%s:%s, targetMesh=%s:%s)."),
            *Appearance.Animation.SourceAnimation.ToString(),
            SourceAnimation ? TEXT("ok") : TEXT("missing"),
            *Appearance.Animation.SourceSkeleton.ToString(),
            SourceMesh ? TEXT("ok") : TEXT("missing"),
            *Appearance.Assets.SkeletalMesh.ToSoftObjectPath().ToString(),
            TargetMesh ? TEXT("ok") : TEXT("missing"));
        return false;
    }

    const FString SafeTargetName = MakeSafeAssetName(FText::FromString(TargetMesh->GetName()));
    const FString RetargetingPath = TEXT("/Game/CharacterCreator/Generated/Retargeting");
    const FString TargetRigName = FString::Printf(TEXT("IK_%s_Target"), *SafeTargetName);
    const FString RetargeterName = FString::Printf(TEXT("RTG_%s_FromSource"), *SafeTargetName);

    const FString TargetRigObjectPath = MakeObjectPath(RetargetingPath, TargetRigName);
    UIKRigDefinition* TargetRig = FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(TargetRigObjectPath))
        ? Cast<UIKRigDefinition>(StaticLoadObject(UIKRigDefinition::StaticClass(), nullptr, *TargetRigObjectPath))
        : nullptr;
    if (!TargetRig)
    {
        TargetRig = UIKRigDefinitionFactory::CreateNewIKRigAsset(RetargetingPath, TargetRigName);
    }

    if (!TargetRig)
    {
        OutErrorMessage = TEXT("Unable to create the target IK Rig asset.");
        return false;
    }

    UIKRigController* TargetRigController = UIKRigController::GetController(TargetRig);
    if (!TargetRigController || !TargetRigController->SetSkeletalMesh(TargetMesh) || !TargetRigController->ApplyAutoGeneratedRetargetDefinition())
    {
        OutErrorMessage = TEXT("Unable to build the target IK Rig definition from the target skeletal mesh.");
        return false;
    }
    TargetRig->MarkPackageDirty();
    FString IgnoredFilename;
    SaveAssetPackage(TargetRig, IgnoredFilename);

    const FString RetargeterObjectPath = MakeObjectPath(RetargetingPath, RetargeterName);
    UIKRetargeter* Retargeter = FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(RetargeterObjectPath))
        ? Cast<UIKRetargeter>(StaticLoadObject(UIKRetargeter::StaticClass(), nullptr, *RetargeterObjectPath))
        : nullptr;
    if (!Retargeter)
    {
        FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
        UIKRetargetFactory* RetargeterFactory = NewObject<UIKRetargetFactory>();
        Retargeter = Cast<UIKRetargeter>(AssetToolsModule.Get().CreateAsset(
            RetargeterName,
            RetargetingPath,
            UIKRetargeter::StaticClass(),
            RetargeterFactory));
    }
    if (!Retargeter)
    {
        OutErrorMessage = TEXT("Unable to create the IK Retargeter asset.");
        return false;
    }

    // Build a clean source rig from the selected real source mesh instead of
    // loading the extracted sample retargeter. The sample retargeter references
    // stale PoseAssets; the generated retargeter below remains a real saved
    // deliverable and avoids importing those unrelated vendor warnings.
    UIKRigDefinition* SourceRig = nullptr;
    if (!SourceRig)
    {
        const FString SourceRigName = FString::Printf(TEXT("IK_%s_Source"), *FPaths::GetBaseFilename(SourceMesh->GetName()));
        const FString SourceRigObjectPath = MakeObjectPath(RetargetingPath, SourceRigName);
        SourceRig = FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(SourceRigObjectPath))
            ? Cast<UIKRigDefinition>(StaticLoadObject(UIKRigDefinition::StaticClass(), nullptr, *SourceRigObjectPath))
            : nullptr;
        if (!SourceRig)
        {
            SourceRig = UIKRigDefinitionFactory::CreateNewIKRigAsset(RetargetingPath, SourceRigName);
        }
        UIKRigController* SourceRigController = UIKRigController::GetController(SourceRig);
        if (!SourceRigController || !SourceRigController->SetSkeletalMesh(SourceMesh) || !SourceRigController->ApplyAutoGeneratedRetargetDefinition())
        {
            OutErrorMessage = TEXT("Unable to build the source IK Rig definition from the source skeletal mesh.");
            return false;
        }
        SourceRig->MarkPackageDirty();
        SaveAssetPackage(SourceRig, IgnoredFilename);
    }

    UIKRetargeterController* RetargeterController = UIKRetargeterController::GetController(Retargeter);
    if (!RetargeterController)
    {
        OutErrorMessage = TEXT("Unable to access the generated IK Retargeter controller.");
        return false;
    }

    RetargeterController->SetIKRig(ERetargetSourceOrTarget::Source, SourceRig);
    RetargeterController->SetIKRig(ERetargetSourceOrTarget::Target, TargetRig);
    RetargeterController->SetPreviewMesh(ERetargetSourceOrTarget::Source, SourceMesh);
    RetargeterController->SetPreviewMesh(ERetargetSourceOrTarget::Target, TargetMesh);
    RetargeterController->RemoveAllOps();
    RetargeterController->AddDefaultOps();
    RetargeterController->AutoMapChains(EAutoMapChainType::Fuzzy, true);
    Retargeter->MarkPackageDirty();
    SaveAssetPackage(Retargeter, IgnoredFilename);

    TArray<FAssetData> AssetsToRetarget;
    AssetsToRetarget.Add(FAssetData(SourceAnimation));
    const TArray<FAssetData> RetargetedAssets = UIKRetargetBatchOperation::DuplicateAndRetarget(
        AssetsToRetarget,
        SourceMesh,
        TargetMesh,
        Retargeter,
        TEXT(""),
        TEXT(""),
        TEXT(""),
        TEXT("_Retargeted"),
        false,
        false);

    if (RetargetedAssets.Num() == 0 || !RetargetedAssets[0].IsValid())
    {
        OutErrorMessage = TEXT("The IK Retargeter did not produce a target animation asset.");
        return false;
    }

    OutTargetAnimation = RetargetedAssets[0].GetSoftObjectPath();
    if (OutTargetAnimation.IsNull())
    {
        OutErrorMessage = TEXT("The generated target animation asset did not resolve to a valid object path.");
        return false;
    }

    UObject* TargetAnimationAsset = RetargetedAssets[0].GetAsset();
    FString TargetAnimationFilename;
    if (!TargetAnimationAsset || !SaveAssetPackage(TargetAnimationAsset, TargetAnimationFilename))
    {
        OutErrorMessage = TEXT("The IK Retargeter created a target animation object but its Unreal package could not be saved.");
        return false;
    }
    return true;
}

bool FCharacterCreatorEditorExportService::GenerateDeliverables(
    const FCharacterAppearanceState& Appearance,
    const FCharacterPreset& Preset,
    const FCharacterCreatorExportProfile& Profile,
    const FString& DestinationDirectory,
    FCharacterCreatorRealExportResult& OutResult)
{
    OutResult = FCharacterCreatorRealExportResult();

    TArray<FCharacterCreatorValidationIssue> Issues;
    FCharacterCreatorExportService::ValidateAppearance(Appearance, Profile, Issues);
    if (FCharacterCreatorExportService::HasErrors(Issues))
    {
        const FCharacterCreatorValidationIssue* FirstError = Issues.FindByPredicate([](const FCharacterCreatorValidationIssue& Issue)
        {
            return Issue.Severity == ECharacterCreatorValidationSeverity::Error;
        });
        OutResult.ErrorMessage = FirstError ? FirstError->Remediation.ToString() : TEXT("Export validation failed");
        return false;
    }

    const FString SafeName = MakeSafeAssetName(Preset.DisplayName);
    const FString ContentPath = FString::Printf(TEXT("/Game/CharacterCreator/Generated/%s"), *SafeName);
    const FString BlueprintName = FString::Printf(TEXT("BP_%s"), *SafeName);
    const FString DataAssetName = FString::Printf(TEXT("DA_%s"), *SafeName);

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
    IAssetTools& AssetTools = AssetToolsModule.Get();

    UCharacterCreatorAppearanceDataAsset* DataAsset = nullptr;
    UBlueprint* Blueprint = nullptr;
    FString BlueprintFilename;
    FString DataAssetFilename;

    if (Profile.bGenerateDataAsset || Profile.bGenerateBlueprint || Profile.bGeneratePackage)
    {
        UDataAssetFactory* DataFactory = NewObject<UDataAssetFactory>();
        DataFactory->DataAssetClass = UCharacterCreatorAppearanceDataAsset::StaticClass();
        DataAsset = Cast<UCharacterCreatorAppearanceDataAsset>(CreateOrLoadAsset(AssetTools, ContentPath, DataAssetName, DataFactory, UCharacterCreatorAppearanceDataAsset::StaticClass()));
        if (!DataAsset)
        {
            OutResult.ErrorMessage = TEXT("Unable to create the generated character Data Asset.");
            return false;
        }
        DataAsset->PresetId = Preset.PresetId;
        DataAsset->DisplayName = Preset.DisplayName;
        DataAsset->Appearance = Appearance;
        DataAsset->ExportProfile = Profile;
        DataAsset->MarkPackageDirty();
        DataAsset->PostEditChange();
        if (!SaveAssetPackage(DataAsset, DataAssetFilename))
        {
            OutResult.ErrorMessage = TEXT("Unable to save the generated character Data Asset package.");
            return false;
        }
        OutResult.DataAsset = FSoftObjectPath(DataAsset);
    }

    if (Profile.bGenerateBlueprint || Profile.bGeneratePackage)
    {
        UBlueprintFactory* BlueprintFactory = NewObject<UBlueprintFactory>();
        BlueprintFactory->ParentClass = ACharacterCreatorGeneratedCharacter::StaticClass();
        BlueprintFactory->BlueprintType = BPTYPE_Normal;
        BlueprintFactory->bSkipClassPicker = true;
        Blueprint = Cast<UBlueprint>(CreateOrLoadAsset(AssetTools, ContentPath, BlueprintName, BlueprintFactory, UBlueprint::StaticClass()));
        if (!Blueprint)
        {
            OutResult.ErrorMessage = TEXT("Unable to create the generated character Blueprint.");
            return false;
        }

        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        if (Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(ACharacterCreatorGeneratedCharacter::StaticClass()))
        {
            ACharacterCreatorGeneratedCharacter* GeneratedCDO = Cast<ACharacterCreatorGeneratedCharacter>(Blueprint->GeneratedClass->GetDefaultObject());
            if (GeneratedCDO)
            {
                GeneratedCDO->CharacterSkeletalMesh = Appearance.Assets.SkeletalMesh;
                GeneratedCDO->CharacterAnimationInstance = Appearance.Assets.AnimationInstanceClass;
                GeneratedCDO->CharacterPhysicsAsset = Appearance.Assets.PhysicsAsset;
                GeneratedCDO->AppearanceData = DataAsset;
                GeneratedCDO->ApplyExportedAssetReferences();
            }
        }
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        Blueprint->MarkPackageDirty();
        Blueprint->PostEditChange();
        if (!SaveAssetPackage(Blueprint, BlueprintFilename))
        {
            OutResult.ErrorMessage = TEXT("Unable to save the generated character Blueprint package.");
            return false;
        }
        OutResult.BlueprintAsset = FSoftObjectPath(Blueprint);
    }

    OutResult.GeneratedContentPath = ContentPath;
    if (Profile.bGeneratePackage)
    {
        OutResult.StagedPackageDirectory = DestinationDirectory.IsEmpty()
            ? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CharacterCreator"), TEXT("Exports"), SafeName)
            : FPaths::Combine(DestinationDirectory, SafeName);
        StagePackageFiles(BlueprintFilename, OutResult.StagedPackageDirectory, OutResult.StagedFiles);
        StagePackageFiles(DataAssetFilename, OutResult.StagedPackageDirectory, OutResult.StagedFiles);
        if (OutResult.StagedFiles.Num() == 0)
        {
            OutResult.ErrorMessage = TEXT("Generated assets were saved but no package files could be staged.");
            return false;
        }
        if (!WriteExportManifest(Appearance, Preset, Profile, OutResult, OutResult.StagedPackageDirectory))
        {
            OutResult.ErrorMessage = TEXT("Generated package files were staged but the package manifest could not be written.");
            return false;
        }
    }

    OutResult.bSucceeded = true;
    return true;
}

#else

bool FCharacterCreatorEditorExportService::GenerateRetargetedAnimation(
    const FCharacterAppearanceState&,
    FSoftObjectPath& OutTargetAnimation,
    FString& OutErrorMessage)
{
    OutTargetAnimation.Reset();
    OutErrorMessage = TEXT("Real IK retargeting is only available in an editor build.");
    return false;
}

bool FCharacterCreatorEditorExportService::GenerateDeliverables(
    const FCharacterAppearanceState&,
    const FCharacterPreset&,
    const FCharacterCreatorExportProfile&,
    const FString&,
    FCharacterCreatorRealExportResult& OutResult)
{
    OutResult = FCharacterCreatorRealExportResult();
    OutResult.ErrorMessage = TEXT("Real Unreal deliverable generation is only available in an editor build.");
    return false;
}

#endif
