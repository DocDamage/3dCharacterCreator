#include "CharacterCreatorGameMode.h"

#include "CharacterCreatorHUD.h"

ACharacterCreatorGameMode::ACharacterCreatorGameMode()
{
    HUDClass = ACharacterCreatorHUD::StaticClass();
    DefaultPawnClass = nullptr;
}
