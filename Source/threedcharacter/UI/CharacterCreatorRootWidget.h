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

UCLASS()
class THREEDCHARACTER_API UCharacterCreatorRootWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeWithSession(UCharacterCreatorSession* InSession);

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

    UFUNCTION()
    void HandleNewCharacterClicked();

    UFUNCTION()
    void HandleBackToDashboardClicked();

    UFUNCTION()
    void HandleSaveCharacterClicked();

    UFUNCTION()
    void HandleOpenProjectClicked();

    UFUNCTION()
    void HandleImportAssetClicked();

    UPROPERTY()
    TObjectPtr<UCharacterCreatorSession> Session;

    UPROPERTY()
    TObjectPtr<UWidgetSwitcher> ScreenSwitcher;

    UPROPERTY()
    TObjectPtr<UCanvasPanel> DashboardScreen;

    UPROPERTY()
    TObjectPtr<UCanvasPanel> CharacterScreen;

    UPROPERTY()
    TObjectPtr<UButton> NewCharacterButton;

    UPROPERTY()
    TObjectPtr<UButton> BackToDashboardButton;

    UPROPERTY()
    TObjectPtr<UButton> SaveCharacterButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> DashboardStatusText;

    UPROPERTY()
    TObjectPtr<UTextBlock> CharacterStatusText;

    UPROPERTY()
    TArray<TObjectPtr<UCharacterCreatorSliderWidget>> BodyParameterSliders;

    UPROPERTY()
    TArray<TObjectPtr<UTextBlock>> BodyParameterValueLabels;

    bool bLayoutBuilt = false;
    bool bRefreshingAppearance = false;
};
