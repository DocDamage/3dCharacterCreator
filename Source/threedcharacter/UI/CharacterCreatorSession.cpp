#include "UI/CharacterCreatorSession.h"

void UCharacterCreatorSession::SetScreen(ECharacterCreatorScreen NewScreen)
{
    if (CurrentScreen == NewScreen)
    {
        return;
    }

    CurrentScreen = NewScreen;
    OnScreenChanged.Broadcast(CurrentScreen);
}

void UCharacterCreatorSession::SetStatusMessage(const FText& NewMessage)
{
    StatusMessage = NewMessage;
    OnStatusChanged.Broadcast(StatusMessage);
}

void UCharacterCreatorSession::Shutdown()
{
    OnScreenChanged.Clear();
    OnStatusChanged.Clear();
}
