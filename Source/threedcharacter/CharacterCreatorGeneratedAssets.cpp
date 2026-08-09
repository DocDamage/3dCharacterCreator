#include "CharacterCreatorGeneratedAssets.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"

ACharacterCreatorGeneratedCharacter::ACharacterCreatorGeneratedCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    if (GetMesh())
    {
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
}

void ACharacterCreatorGeneratedCharacter::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyExportedAssetReferences();
}

void ACharacterCreatorGeneratedCharacter::ApplyAppearanceData(UCharacterCreatorAppearanceDataAsset* InAppearanceData)
{
    AppearanceData = InAppearanceData;
    if (InAppearanceData)
    {
        const FCharacterAssetReferences& Assets = InAppearanceData->Appearance.Assets;
        CharacterSkeletalMesh = Assets.SkeletalMesh;
        CharacterAnimationInstance = Assets.AnimationInstanceClass;
        CharacterPhysicsAsset = Assets.PhysicsAsset;
    }
    ApplyExportedAssetReferences();
}

void ACharacterCreatorGeneratedCharacter::ApplyExportedAssetReferences()
{
    USkeletalMeshComponent* CharacterMesh = GetMesh();
    if (!CharacterMesh)
    {
        return;
    }

    if (USkeletalMesh* LoadedMesh = CharacterSkeletalMesh.LoadSynchronous())
    {
        CharacterMesh->SetSkeletalMesh(LoadedMesh);
    }
    if (UClass* AnimationClass = CharacterAnimationInstance.LoadSynchronous())
    {
        CharacterMesh->SetAnimInstanceClass(AnimationClass);
    }
    if (UPhysicsAsset* PhysicsAsset = CharacterPhysicsAsset.LoadSynchronous())
    {
        CharacterMesh->SetPhysicsAsset(PhysicsAsset);
    }
}
