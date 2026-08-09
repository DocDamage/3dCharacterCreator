#include "UI/CharacterCreatorSubsystem.h"

#include "UI/CharacterCreatorSession.h"

void UCharacterCreatorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Session = NewObject<UCharacterCreatorSession>(this, TEXT("CharacterCreatorSession"));
}

void UCharacterCreatorSubsystem::Deinitialize()
{
    if (Session)
    {
        Session->Shutdown();
        Session = nullptr;
    }

    Super::Deinitialize();
}
