#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/CharacterCreatorSession.h"
#include "CharacterCreatorRootWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UScaleBox;
class USizeBox;
class UTextBlock;
class UWidget;
class UWidgetSwitcher;
class UCharacterCreatorSliderWidget;
class UCharacterCreatorWorkflowScreenWidget;
class UCharacterCreatorCommandButtonWidget;
class UCharacterCreatorAnimationWorkspaceWidget;
class UCharacterCreatorUtilityWorkspaceWidget;
class UCharacterCreatorModalManager;
class UCharacterCreatorModalWidget;
class UImage;
class ACharacterCreatorPreviewActor;
enum class ECharacterCreatorPreviewState : uint8;

UCLASS()
class THREEDCHARACTER_API UCharacterCreatorRootWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeWithSession(UCharacterCreatorSession* InSession);
    void InitializeWithPreviewActor(ACharacterCreatorPreviewActor* InPreviewActor);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void BuildLayout();
    void BuildDashboard(UCanvasPanel* Screen);
    void BuildCharacterCreator(UCanvasPanel* Screen);
    void ApplyScreen(ECharacterCreatorScreen NewScreen);
    void ApplyStatus(const FText& NewStatus);
    void ApplyAppearance(const FCharacterAppearanceState& NewAppearance);
    void HandleBodyParameterChanged(ECharacterCreatorParameter Parameter, float Value);
    void ApplyPreviewRenderTarget();
    void ApplyPreviewState(ECharacterCreatorPreviewState NewState, const FText& Message);
    void HandleWorkflowCommand(FName CommandId);
    void OpenModalDialog(FName DialogId, const FString& Title, const FString& Message, const TArray<TPair<FName, FString>>& Actions);
    void HandleModalCommand(FName CommandId);

    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    UFUNCTION()
    void HandleNewCharacterClicked();

    UFUNCTION()
    void HandleBackToDashboardClicked();

    UFUNCTION()
    void HandleSaveCharacterClicked();

    UFUNCTION()
    void HandleRevertCharacterClicked();

    UFUNCTION()
    void HandleOpenProjectClicked();

    UFUNCTION()
    void HandleImportAssetClicked();

    UFUNCTION()
    void HandleOnboardingClicked();

    UFUNCTION()
    void HandleSaveTemplateClicked();

    UPROPERTY()
    TObjectPtr<UCharacterCreatorSession> Session;

    UPROPERTY()
    TObjectPtr<UWidgetSwitcher> ScreenSwitcher;

    UPROPERTY()
    TObjectPtr<UCanvasPanel> DashboardScreen;

    UPROPERTY()
    TObjectPtr<UCanvasPanel> CharacterScreen;

    UPROPERTY()
    TObjectPtr<UCharacterCreatorWorkflowScreenWidget> OutfitScreen;

    UPROPERTY()
    TObjectPtr<UCharacterCreatorWorkflowScreenWidget> HairScreen;

    UPROPERTY()
    TObjectPtr<UCharacterCreatorWorkflowScreenWidget> MaterialsScreen;

    UPROPERTY()
    TObjectPtr<UCharacterCreatorWorkflowScreenWidget> WeaponsScreen;

    UPROPERTY()
    TArray<TObjectPtr<UCharacterCreatorAnimationWorkspaceWidget>> AnimationScreens;

    UPROPERTY()
    TArray<TObjectPtr<UCharacterCreatorUtilityWorkspaceWidget>> UtilityScreens;

    UPROPERTY()
    TObjectPtr<UButton> NewCharacterButton;

    UPROPERTY()
    TObjectPtr<UButton> BackToDashboardButton;

    UPROPERTY()
    TObjectPtr<UButton> SaveCharacterButton;

    UPROPERTY()
    TObjectPtr<UButton> RevertCharacterButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> DashboardStatusText;

    UPROPERTY()
    TObjectPtr<UTextBlock> CharacterStatusText;

    UPROPERTY()
    TObjectPtr<UTextBlock> DashboardPreviewStatusText;

    UPROPERTY()
    TObjectPtr<UTextBlock> CharacterPreviewStatusText;

    UPROPERTY()
    TObjectPtr<UImage> DashboardPreviewImage;

    UPROPERTY()
    TObjectPtr<UImage> CharacterPreviewImage;

    UPROPERTY()
    TObjectPtr<ACharacterCreatorPreviewActor> PreviewActor;

    UPROPERTY()
    TObjectPtr<UCharacterCreatorModalManager> ModalManager;

    UPROPERTY()
    TObjectPtr<UCanvasPanel> ModalOverlay;

    UPROPERTY()
    TArray<TObjectPtr<UCharacterCreatorModalWidget>> ModalDialogs;

    UPROPERTY()
    TArray<TObjectPtr<UCharacterCreatorSliderWidget>> BodyParameterSliders;

    UPROPERTY()
    TArray<TObjectPtr<UTextBlock>> BodyParameterValueLabels;

    bool bLayoutBuilt = false;
    bool bRefreshingAppearance = false;
    TWeakObjectPtr<UWidget> LastFocusWidget;
};
