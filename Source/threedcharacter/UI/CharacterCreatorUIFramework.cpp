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
}

UCharacterCreatorSliderWidget::UCharacterCreatorSliderWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
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
