#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CharacterCreatorSession.generated.h"

UENUM(BlueprintType)
enum class ECharacterCreatorScreen : uint8
{
    Dashboard,
    CharacterCreator
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterCreatorScreenChanged, ECharacterCreatorScreen);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterCreatorStatusChanged, const FText&);

UCLASS()
class THREEDCHARACTER_API UCharacterCreatorSession : public UObject
{
    GENERATED_BODY()

public:
    void SetScreen(ECharacterCreatorScreen NewScreen);
    ECharacterCreatorScreen GetScreen() const { return CurrentScreen; }

    void SetStatusMessage(const FText& NewMessage);
    const FText& GetStatusMessage() const { return StatusMessage; }

    void Shutdown();

    FOnCharacterCreatorScreenChanged OnScreenChanged;
    FOnCharacterCreatorStatusChanged OnStatusChanged;

private:
    ECharacterCreatorScreen CurrentScreen = ECharacterCreatorScreen::Dashboard;
    FText StatusMessage;
};
