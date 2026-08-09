#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CharacterCreatorHUD.generated.h"

class UCharacterCreatorRootWidget;
class UCharacterCreatorSession;
class UCharacterCreatorSubsystem;
class ACharacterCreatorPreviewActor;

UCLASS()
class THREEDCHARACTER_API ACharacterCreatorHUD : public AHUD
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY()
    TObjectPtr<UCharacterCreatorSession> Session;

    UPROPERTY()
    TObjectPtr<UCharacterCreatorSubsystem> CharacterCreatorSubsystem;

    UPROPERTY()
    TObjectPtr<ACharacterCreatorPreviewActor> PreviewActor;

    UPROPERTY()
    TObjectPtr<UCharacterCreatorRootWidget> RootWidget;
};
