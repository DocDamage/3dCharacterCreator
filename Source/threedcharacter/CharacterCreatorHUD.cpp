#include "CharacterCreatorHUD.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "CharacterCreatorPreviewActor.h"
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

    UWorld* World = GetWorld();
    if (World)
    {
        const FTransform PreviewTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 100.0f));
        PreviewActor = World->SpawnActorDeferred<ACharacterCreatorPreviewActor>(
            ACharacterCreatorPreviewActor::StaticClass(),
            PreviewTransform,
            this,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

        if (PreviewActor)
        {
            PreviewActor->InitializeWithSession(Session);
            PreviewActor->FinishSpawning(PreviewTransform);
        }
    }

    RootWidget = CreateWidget<UCharacterCreatorRootWidget>(PlayerController, UCharacterCreatorRootWidget::StaticClass());
    if (!RootWidget)
    {
        return;
    }

    RootWidget->InitializeWithPreviewActor(PreviewActor);
    RootWidget->InitializeWithSession(Session);
    if (!RootWidget->AddToPlayerScreen(0))
    {
        RootWidget->AddToViewport(0);
    }
    RootWidget->SetVisibility(ESlateVisibility::Visible);
    UE_LOG(LogTemp, Display, TEXT("Character Creator root widget attached: constructed=%d inViewport=%d"), RootWidget->IsConstructed(), RootWidget->IsInViewport());

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

    if (PreviewActor)
    {
        PreviewActor->Destroy();
        PreviewActor = nullptr;
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
