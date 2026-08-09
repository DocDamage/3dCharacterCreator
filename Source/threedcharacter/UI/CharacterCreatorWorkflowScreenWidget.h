#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/CharacterCreatorSession.h"
#include "UI/CharacterCreatorUIFramework.h"
#include "CharacterCreatorWorkflowScreenWidget.generated.h"

class UCanvasPanel;
class UImage;
class UTextBlock;
class UCharacterCreatorCommandButtonWidget;
class ACharacterCreatorPreviewActor;
enum class ECharacterCreatorPreviewState : uint8;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterCreatorWorkflowModalRequested, FName);

UCLASS()
class THREEDCHARACTER_API UCharacterCreatorWorkflowScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeWithSession(UCharacterCreatorSession* InSession);
    void InitializeWithPreviewActor(ACharacterCreatorPreviewActor* InPreviewActor);
    void SetWorkflowScreen(ECharacterCreatorScreen InScreen);
    void FocusFirstControl();

    FOnCharacterCreatorWorkflowModalRequested OnModalRequested;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void BuildLayout();
    void BuildNavigation(UCanvasPanel* Canvas);
    void BuildPreview(UCanvasPanel* Canvas);
    void BuildInspector(UCanvasPanel* Canvas);
    void BuildOutfitInspector(UCanvasPanel* Canvas);
    void BuildHairInspector(UCanvasPanel* Canvas);
    void BuildMaterialsInspector(UCanvasPanel* Canvas);
    void BuildWeaponsInspector(UCanvasPanel* Canvas);
    UCharacterCreatorCommandButtonWidget* AddCommandButton(UCanvasPanel* Canvas, const FString& Label, FName CommandId, const FVector2D& Position, const FVector2D& Size, ECharacterCreatorButtonStyle Style);
    void ApplyAppearance(const FCharacterAppearanceState& NewAppearance);
    void ApplyPreviewRenderTarget();
    void ApplyPreviewState(ECharacterCreatorPreviewState NewState, const FText& Message);
    void ApplyMaterialPreset(const TArray<FLinearColor>& Colors);
    FText GetWorkflowTitle() const;
    FText GetWorkflowSubtitle() const;
    FText GetSelectionSummary(const FCharacterAppearanceState& Appearance) const;
    FString GetAssetLabel(const FSoftObjectPath& AssetPath) const;

    void HandleCommand(FName CommandId);

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
    TObjectPtr<UTextBlock> SelectionSummaryText;

    UPROPERTY()
    TObjectPtr<UTextBlock> EditStatusText;

    UPROPERTY()
    TArray<TObjectPtr<UCharacterCreatorCommandButtonWidget>> CommandButtons;

    ECharacterCreatorScreen WorkflowScreen = ECharacterCreatorScreen::OutfitAndArmor;
    bool bLayoutBuilt = false;
};
