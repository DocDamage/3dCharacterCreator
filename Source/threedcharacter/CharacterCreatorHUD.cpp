#include "CharacterCreatorHUD.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "UI/CharacterCreatorRootWidget.h"
#include "UI/CharacterCreatorSession.h"
#include "UI/CharacterCreatorSubsystem.h"

void ACharacterCreatorHUD::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PlayerController = GetOwningPlayerController();
    if (!PlayerController)
    {
        return;
    }

    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    CharacterCreatorSubsystem = GameInstance ? GameInstance->GetSubsystem<UCharacterCreatorSubsystem>() : nullptr;
    Session = CharacterCreatorSubsystem ? CharacterCreatorSubsystem->GetSession() : nullptr;
    if (!Session)
    {
        return;
    }

    RootWidget = CreateWidget<UCharacterCreatorRootWidget>(PlayerController, UCharacterCreatorRootWidget::StaticClass());
    if (!RootWidget)
    {
        return;
    }

    RootWidget->InitializeWithSession(Session);
    RootWidget->AddToViewport(0);

    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(RootWidget->TakeWidget());
    PlayerController->SetInputMode(InputMode);
    PlayerController->bShowMouseCursor = true;
}

void ACharacterCreatorHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (RootWidget)
    {
        RootWidget->RemoveFromParent();
        RootWidget = nullptr;
    }

    if (Session)
    {
        Session = nullptr;
    }

    CharacterCreatorSubsystem = nullptr;

    if (APlayerController* PlayerController = GetOwningPlayerController())
    {
        FInputModeGameOnly InputMode;
        PlayerController->SetInputMode(InputMode);
        PlayerController->bShowMouseCursor = false;
    }

    Super::EndPlay(EndPlayReason);
}
