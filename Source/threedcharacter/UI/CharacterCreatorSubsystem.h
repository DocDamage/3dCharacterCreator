#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CharacterCreatorSubsystem.generated.h"

class UCharacterCreatorSession;

UCLASS()
class THREEDCHARACTER_API UCharacterCreatorSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category = "Character Creator")
    UCharacterCreatorSession* GetSession() const { return Session; }

private:
    UPROPERTY()
    TObjectPtr<UCharacterCreatorSession> Session;
};
