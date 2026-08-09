#include "UI/CharacterCreatorUIFramework.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateColor.h"

const FCharacterCreatorStylePalette& FCharacterCreatorUIStyle::GetPalette()
{
    static const FCharacterCreatorStylePalette Palette;
    return Palette;
}

void UCharacterCreatorPanelWidget::SetPanelStyle(ECharacterCreatorPanelStyle Style)
{
    const FCharacterCreatorStylePalette& Palette = FCharacterCreatorUIStyle::GetPalette();
    switch (Style)
    {
    case ECharacterCreatorPanelStyle::Base:
        SetPanelColor(Palette.Surface);
        break;
    case ECharacterCreatorPanelStyle::Raised:
        SetPanelColor(Palette.SurfaceRaised);
        break;
    case ECharacterCreatorPanelStyle::Muted:
        SetPanelColor(Palette.SurfaceMuted);
        break;
    case ECharacterCreatorPanelStyle::Overlay:
        SetPanelColor(Palette.Ink);
        break;
    default:
        SetPanelColor(Palette.Surface);
        break;
    }
}

void UCharacterCreatorPanelWidget::SetPanelColor(const FLinearColor& Color)
{
    SetBrushColor(Color);
}

UCharacterCreatorButtonWidget::UCharacterCreatorButtonWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InitIsFocusable(true);
}

void UCharacterCreatorButtonWidget::SetButtonStyle(ECharacterCreatorButtonStyle Style)
{
    const FCharacterCreatorStylePalette& Palette = FCharacterCreatorUIStyle::GetPalette();
    switch (Style)
    {
    case ECharacterCreatorButtonStyle::Primary:
        SetBackgroundColor(Palette.Blue);
        SetColorAndOpacity(FLinearColor::White);
        break;
    case ECharacterCreatorButtonStyle::Secondary:
        SetBackgroundColor(Palette.SurfaceRaised);
        SetColorAndOpacity(FLinearColor::White);
        break;
    case ECharacterCreatorButtonStyle::Ghost:
        SetBackgroundColor(Palette.SurfaceMuted);
        SetColorAndOpacity(Palette.Muted);
        break;
    case ECharacterCreatorButtonStyle::Accent:
        SetBackgroundColor(Palette.Gold);
        SetColorAndOpacity(Palette.Ink);
        break;
    default:
        SetBackgroundColor(Palette.SurfaceRaised);
        SetColorAndOpacity(FLinearColor::White);
        break;
    }
    UnfocusedBackgroundColor = GetBackgroundColor();
    UnfocusedForegroundColor = GetColorAndOpacity();
}

void UCharacterCreatorButtonWidget::SetFocusVisual(bool bFocused)
{
    const FCharacterCreatorStylePalette& Palette = FCharacterCreatorUIStyle::GetPalette();
    SetBackgroundColor(bFocused ? Palette.Blue : UnfocusedBackgroundColor);
    SetColorAndOpacity(bFocused ? Palette.Text : UnfocusedForegroundColor);
}

UCharacterCreatorCommandButtonWidget::UCharacterCreatorCommandButtonWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    OnClicked.AddDynamic(this, &UCharacterCreatorCommandButtonWidget::HandleClicked);
}

void UCharacterCreatorCommandButtonWidget::HandleClicked()
{
    OnCommand.Broadcast(CommandId);
}

UCharacterCreatorSliderWidget::UCharacterCreatorSliderWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    IsFocusable = true;
    OnValueChanged.AddDynamic(this, &UCharacterCreatorSliderWidget::HandleValueChanged);
}

void UCharacterCreatorSliderWidget::HandleValueChanged(float NewValue)
{
    OnParameterValueChanged.Broadcast(Parameter, NewValue);
}

void UCharacterCreatorTabButtonWidget::SetSelected(bool bInSelected)
{
    bSelected = bInSelected;
    SetButtonStyle(bSelected ? ECharacterCreatorButtonStyle::Primary : ECharacterCreatorButtonStyle::Secondary);
}

void UCharacterCreatorModalWidget::SetModalOpen(bool bOpen)
{
    SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UCharacterCreatorModalManager::Initialize(UUserWidget* InHostWidget)
{
    HostWidget = InHostWidget;
    CloseAllModals();
}

bool UCharacterCreatorModalManager::OpenModal(UCharacterCreatorModalWidget* Modal, UWidget* ReturnFocusWidget)
{
    if (!Modal || ModalStack.Contains(Modal))
    {
        return false;
    }

    ModalStack.Add(Modal);
    FocusStack.Add(ReturnFocusWidget);
    Modal->SetModalOpen(true);
    Modal->SetKeyboardFocus();
    return true;
}

bool UCharacterCreatorModalManager::CloseModal(UCharacterCreatorModalWidget* Modal)
{
    const int32 ModalIndex = ModalStack.Find(Modal);
    if (ModalIndex == INDEX_NONE)
    {
        return false;
    }

    const bool bWasTopModal = ModalIndex == ModalStack.Num() - 1;
    TWeakObjectPtr<UWidget> ReturnFocus = FocusStack.IsValidIndex(ModalIndex) ? FocusStack[ModalIndex] : nullptr;

    if (ModalStack.IsValidIndex(ModalIndex))
    {
        ModalStack[ModalIndex]->SetModalOpen(false);
    }
    ModalStack.RemoveAt(ModalIndex);
    FocusStack.RemoveAt(ModalIndex);

    if (bWasTopModal)
    {
        if (UWidget* FocusWidget = ReturnFocus.Get())
        {
            UCharacterCreatorUIHelpers::FocusWidget(FocusWidget);
        }
    }
    else if (ModalStack.Num() > 0 && ModalStack.Last())
    {
        ModalStack.Last()->SetKeyboardFocus();
    }

    return true;
}

bool UCharacterCreatorModalManager::CloseTopModal()
{
    return ModalStack.Num() > 0 && CloseModal(ModalStack.Last());
}

void UCharacterCreatorModalManager::CloseAllModals()
{
    for (UCharacterCreatorModalWidget* Modal : ModalStack)
    {
        if (Modal)
        {
            Modal->SetModalOpen(false);
        }
    }
    ModalStack.Reset();
    FocusStack.Reset();
}

void UCharacterCreatorModalManager::RestoreFocusForIndex(int32 Index)
{
    if (FocusStack.IsValidIndex(Index))
    {
        if (UWidget* FocusWidget = FocusStack[Index].Get())
        {
            UCharacterCreatorUIHelpers::FocusWidget(FocusWidget);
        }
    }
}

UCanvasPanelSlot* FCharacterCreatorUIFactory::Place(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, const FVector2D& Size)
{
    if (!Canvas || !Widget)
    {
        return nullptr;
    }

    UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
    Slot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
    Slot->SetAlignment(FVector2D::ZeroVector);
    Slot->SetPosition(Position);
    Slot->SetSize(Size);
    return Slot;
}

UTextBlock* FCharacterCreatorUIFactory::MakeLabel(UWidgetTree* Tree, const FString& Value, int32 FontSize, const FLinearColor& Color)
{
    UTextBlock* Result = Tree->ConstructWidget<UTextBlock>();
    Result->SetText(FText::FromString(Value));
    FSlateFontInfo Font = Result->GetFont();
    Font.Size = FontSize;
    Result->SetFont(Font);
    Result->SetColorAndOpacity(FSlateColor(Color));
    return Result;
}

UCharacterCreatorPanelWidget* FCharacterCreatorUIFactory::MakePanel(UWidgetTree* Tree, const FLinearColor& Color)
{
    UCharacterCreatorPanelWidget* Result = Tree->ConstructWidget<UCharacterCreatorPanelWidget>();
    Result->SetPanelColor(Color);
    return Result;
}

UCharacterCreatorButtonWidget* FCharacterCreatorUIFactory::MakeButton(UWidgetTree* Tree, const FString& Value, ECharacterCreatorButtonStyle Style, int32 FontSize)
{
    UCharacterCreatorButtonWidget* Result = Tree->ConstructWidget<UCharacterCreatorButtonWidget>();
    Result->SetButtonStyle(Style);
    Result->AddChild(MakeLabel(Tree, Value, FontSize, FCharacterCreatorUIStyle::GetPalette().Text));
    return Result;
}

UCharacterCreatorCommandButtonWidget* FCharacterCreatorUIFactory::MakeCommandButton(UWidgetTree* Tree, const FString& Value, FName CommandId, ECharacterCreatorButtonStyle Style, int32 FontSize)
{
    UCharacterCreatorCommandButtonWidget* Result = Tree->ConstructWidget<UCharacterCreatorCommandButtonWidget>();
    Result->SetCommandId(CommandId);
    Result->SetButtonStyle(Style);
    Result->AddChild(MakeLabel(Tree, Value, FontSize, FCharacterCreatorUIStyle::GetPalette().Text));
    return Result;
}

UCharacterCreatorSliderWidget* FCharacterCreatorUIFactory::MakeSlider(UWidgetTree* Tree, ECharacterCreatorParameter Parameter, float Value)
{
    UCharacterCreatorSliderWidget* Result = Tree->ConstructWidget<UCharacterCreatorSliderWidget>();
    Result->SetParameter(Parameter);
    Result->SetMinValue(0.0f);
    Result->SetMaxValue(1.0f);
    Result->SetStepSize(0.01f);
    Result->SetValue(Value);
    return Result;
}

void FCharacterCreatorUIFactory::AddLabel(UWidgetTree* Tree, UCanvasPanel* Canvas, const FString& Name, const FString& Value, const FVector2D& Position, const FVector2D& Size, int32 FontSize, const FLinearColor& Color)
{
    UTextBlock* TextWidget = MakeLabel(Tree, Value, FontSize, Color);
    TextWidget->Rename(*Name);
    Place(Canvas, TextWidget, Position, Size);
}

void FCharacterCreatorUIFactory::AddPanel(UWidgetTree* Tree, UCanvasPanel* Canvas, const FString& Name, const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color)
{
    UCharacterCreatorPanelWidget* Border = MakePanel(Tree, Color);
    Border->Rename(*Name);
    Place(Canvas, Border, Position, Size);
}

void FCharacterCreatorUIFactory::ConfigureFocusGraph(const TArray<UWidget*>& Widgets, bool bLoop)
{
    TArray<UWidget*> Focusable;
    for (UWidget* Widget : Widgets)
    {
        const UButton* Button = Cast<UButton>(Widget);
        const USlider* Slider = Cast<USlider>(Widget);
        const bool bFocusable = (Button && Button->GetIsFocusable()) || (Slider && Slider->IsFocusable);
        if (Widget && bFocusable && Widget->GetVisibility() != ESlateVisibility::Collapsed && Widget->GetVisibility() != ESlateVisibility::Hidden)
        {
            Focusable.Add(Widget);
        }
    }

    for (int32 Index = 0; Index < Focusable.Num(); ++Index)
    {
        UWidget* Previous = Index > 0 ? Focusable[Index - 1] : (bLoop && Focusable.Num() > 1 ? Focusable.Last() : nullptr);
        UWidget* Next = Index + 1 < Focusable.Num() ? Focusable[Index + 1] : (bLoop && Focusable.Num() > 1 ? Focusable[0] : nullptr);
        if (Previous) Focusable[Index]->SetNavigationRuleExplicit(EUINavigation::Previous, Previous);
        if (Next) Focusable[Index]->SetNavigationRuleExplicit(EUINavigation::Next, Next);
    }
}

void UCharacterCreatorUIHelpers::FocusWidget(UWidget* Widget)
{
    if (Widget)
    {
        Widget->SetKeyboardFocus();
    }
}

FVector2D UCharacterCreatorUIHelpers::ClampPopupPosition(const FVector2D& DesiredPosition, const FVector2D& PopupSize, const FVector2D& ViewportSize)
{
    const FVector2D MaximumPosition(
        FMath::Max(0.0f, ViewportSize.X - PopupSize.X),
        FMath::Max(0.0f, ViewportSize.Y - PopupSize.Y));

    return FVector2D(
        FMath::Clamp(DesiredPosition.X, 0.0f, MaximumPosition.X),
        FMath::Clamp(DesiredPosition.Y, 0.0f, MaximumPosition.Y));
}
