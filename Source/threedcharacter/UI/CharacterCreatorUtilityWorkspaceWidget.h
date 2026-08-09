#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/CharacterCreatorSession.h"
#include "UI/CharacterCreatorUIFramework.h"
#include "CharacterCreatorUtilityWorkspaceWidget.generated.h"

class UCanvasPanel;
class UImage;
class UTextBlock;
class UCharacterCreatorCommandButtonWidget;
class ACharacterCreatorPreviewActor;
enum class ECharacterCreatorPreviewState : uint8;

UCLASS()
class THREEDCHARACTER_API UCharacterCreatorUtilityWorkspaceWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeWithSession(UCharacterCreatorSession* InSession);
    void InitializeWithPreviewActor(ACharacterCreatorPreviewActor* InPreviewActor);
    void SetWorkspaceScreen(ECharacterCreatorScreen InScreen);
    void FocusFirstControl();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void BuildLayout();
    void BuildNavigation(UCanvasPanel* Canvas);
    void BuildPreview(UCanvasPanel* Canvas);
    void BuildInspector(UCanvasPanel* Canvas);
    UCharacterCreatorCommandButtonWidget* AddCommandButton(UCanvasPanel* Canvas, const FString& Label, FName CommandId, const FVector2D& Position, const FVector2D& Size, ECharacterCreatorButtonStyle Style);
    void ApplyAppearance(const FCharacterAppearanceState& NewAppearance);
    void ApplyStatus(const FText& NewStatus);
    void ApplyPreviewRenderTarget();
    void ApplyPreviewState(ECharacterCreatorPreviewState NewState, const FText& Message);
    void HandleCommand(FName CommandId);
    FText GetWorkspaceTitle() const;
    FText GetWorkspaceSubtitle() const;
    FText GetUtilitySummary(const FCharacterAppearanceState& Appearance) const;

    UPROPERTY()
    TObjectPtr<UCharacterCreatorSession> Session;

    UPROPERTY()
    TObjectPtr<ACharacterCreatorPreviewActor> PreviewActor;

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UImage> PreviewImage;

    UPROPERTY()
    TObjectPtr<UTextBlock> PreviewStatusText;

    UPROPERTY()
    TObjectPtr<UTextBlock> UtilitySummaryText;

    UPROPERTY()
    TObjectPtr<UTextBlock> EditStatusText;

    UPROPERTY()
    TArray<TObjectPtr<UCharacterCreatorCommandButtonWidget>> CommandButtons;

    ECharacterCreatorScreen WorkspaceScreen = ECharacterCreatorScreen::PreviewStudio;
    bool bLayoutBuilt = false;
};
