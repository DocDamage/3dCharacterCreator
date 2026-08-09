#include "UI/CharacterCreatorRootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"

#include "UI/CharacterCreatorUIFramework.h"

namespace CharacterCreatorUI
{
    const FCharacterCreatorStylePalette& Palette = FCharacterCreatorUIStyle::GetPalette();
    const FLinearColor& Ink = Palette.Ink;
    const FLinearColor& Surface = Palette.Surface;
    const FLinearColor& SurfaceRaised = Palette.SurfaceRaised;
    const FLinearColor& SurfaceMuted = Palette.SurfaceMuted;
    const FLinearColor& Blue = Palette.Blue;
    const FLinearColor& Gold = Palette.Gold;
    const FLinearColor& Text = Palette.Text;
    const FLinearColor& Muted = Palette.Muted;
    const FLinearColor& Success = Palette.Success;

    UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, const FVector2D& Size)
    {
        return FCharacterCreatorUIFactory::Place(Canvas, Widget, Position, Size);
    }

    UTextBlock* Label(UWidgetTree* Tree, const FString& Value, int32 FontSize, const FLinearColor& Color)
    {
        return FCharacterCreatorUIFactory::MakeLabel(Tree, Value, FontSize, Color);
    }

    UCharacterCreatorPanelWidget* Panel(UWidgetTree* Tree, const FLinearColor& Color)
    {
        return FCharacterCreatorUIFactory::MakePanel(Tree, Color);
    }

    UCharacterCreatorButtonWidget* Button(UWidgetTree* Tree, const FString& Value, const FLinearColor& Background, const FLinearColor& Foreground, int32 FontSize = 12)
    {
        UCharacterCreatorButtonWidget* Result = Tree->ConstructWidget<UCharacterCreatorButtonWidget>();
        Result->SetBackgroundColor(Background);
        Result->SetColorAndOpacity(FLinearColor::White);
        Result->AddChild(FCharacterCreatorUIFactory::MakeLabel(Tree, Value, FontSize, Foreground));
        return Result;
    }

    void AddLabel(UWidgetTree* Tree, UCanvasPanel* Canvas, const FString& Name, const FString& Value, const FVector2D& Position, const FVector2D& Size, int32 FontSize, const FLinearColor& Color)
    {
        FCharacterCreatorUIFactory::AddLabel(Tree, Canvas, Name, Value, Position, Size, FontSize, Color);
    }

    void AddPanel(UWidgetTree* Tree, UCanvasPanel* Canvas, const FString& Name, const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color)
    {
        FCharacterCreatorUIFactory::AddPanel(Tree, Canvas, Name, Position, Size, Color);
    }
}

void UCharacterCreatorRootWidget::InitializeWithSession(UCharacterCreatorSession* InSession)
{
    Session = InSession;
}

void UCharacterCreatorRootWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);

    if (!bLayoutBuilt)
    {
        BuildLayout();
        bLayoutBuilt = true;
    }

    if (Session)
    {
        Session->OnScreenChanged.AddUObject(this, &UCharacterCreatorRootWidget::ApplyScreen);
        Session->OnStatusChanged.AddUObject(this, &UCharacterCreatorRootWidget::ApplyStatus);
        Session->OnAppearanceChanged.AddUObject(this, &UCharacterCreatorRootWidget::ApplyAppearance);
        ApplyScreen(Session->GetScreen());
        ApplyStatus(Session->GetStatusMessage());
        ApplyAppearance(Session->GetAppearanceStateNative());
    }
}

void UCharacterCreatorRootWidget::NativeDestruct()
{
    if (Session)
    {
        Session->OnScreenChanged.RemoveAll(this);
        Session->OnStatusChanged.RemoveAll(this);
        Session->OnAppearanceChanged.RemoveAll(this);
    }

    Super::NativeDestruct();
}

void UCharacterCreatorRootWidget::BuildLayout()
{
    using namespace CharacterCreatorUI;

    UScaleBox* ScaleBox = WidgetTree->ConstructWidget<UScaleBox>();
    ScaleBox->SetStretch(EStretch::ScaleToFit);
    WidgetTree->RootWidget = ScaleBox;

    USizeBox* DesignSize = WidgetTree->ConstructWidget<USizeBox>();
    DesignSize->SetWidthOverride(1440.0f);
    DesignSize->SetHeightOverride(810.0f);
    ScaleBox->AddChild(DesignSize);

    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    DesignSize->AddChild(RootCanvas);

    ScreenSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>();
    Place(RootCanvas, ScreenSwitcher, FVector2D::ZeroVector, FVector2D(1440.0f, 810.0f));

    DashboardScreen = WidgetTree->ConstructWidget<UCanvasPanel>();
    CharacterScreen = WidgetTree->ConstructWidget<UCanvasPanel>();
    ScreenSwitcher->AddChild(DashboardScreen);
    ScreenSwitcher->AddChild(CharacterScreen);

    BuildDashboard(DashboardScreen);
    BuildCharacterCreator(CharacterScreen);
}

void UCharacterCreatorRootWidget::BuildDashboard(UCanvasPanel* Screen)
{
    using namespace CharacterCreatorUI;

    AddPanel(WidgetTree, Screen, TEXT("DashboardBackground"), FVector2D::ZeroVector, FVector2D(1440.0f, 810.0f), Ink);
    AddPanel(WidgetTree, Screen, TEXT("DashboardTopBar"), FVector2D(0.0f, 0.0f), FVector2D(1440.0f, 64.0f), Surface);
    AddPanel(WidgetTree, Screen, TEXT("DashboardAccent"), FVector2D(0.0f, 60.0f), FVector2D(1440.0f, 4.0f), Blue);
    AddPanel(WidgetTree, Screen, TEXT("DashboardRail"), FVector2D(0.0f, 64.0f), FVector2D(220.0f, 746.0f), SurfaceMuted);
    AddPanel(WidgetTree, Screen, TEXT("DashboardContent"), FVector2D(220.0f, 64.0f), FVector2D(1220.0f, 746.0f), Ink);

    AddLabel(WidgetTree, Screen, TEXT("DashboardBrand"), TEXT("CHARACTER CREATOR"), FVector2D(28.0f, 17.0f), FVector2D(280.0f, 20.0f), 16, Gold);
    AddLabel(WidgetTree, Screen, TEXT("DashboardWorkspace"), TEXT("WORKSPACE / MAIN DASHBOARD"), FVector2D(28.0f, 39.0f), FVector2D(320.0f, 16.0f), 9, Muted);

    UButton* OpenProjectButton = Button(WidgetTree, TEXT("OPEN PROJECT"), SurfaceRaised, Text, 10);
    OpenProjectButton->OnClicked.AddDynamic(this, &UCharacterCreatorRootWidget::HandleOpenProjectClicked);
    Place(Screen, OpenProjectButton, FVector2D(1250.0f, 18.0f), FVector2D(150.0f, 28.0f));

    AddLabel(WidgetTree, Screen, TEXT("NavHeading"), TEXT("NAVIGATION"), FVector2D(28.0f, 94.0f), FVector2D(150.0f, 18.0f), 10, Muted);
    AddLabel(WidgetTree, Screen, TEXT("NavDashboard"), TEXT("DASHBOARD"), FVector2D(28.0f, 132.0f), FVector2D(150.0f, 20.0f), 12, Blue);
    AddLabel(WidgetTree, Screen, TEXT("NavCharacter"), TEXT("CHARACTER"), FVector2D(28.0f, 174.0f), FVector2D(150.0f, 20.0f), 12, Text);
    AddLabel(WidgetTree, Screen, TEXT("NavAnimation"), TEXT("ANIMATION"), FVector2D(28.0f, 216.0f), FVector2D(150.0f, 20.0f), 12, Muted);
    AddLabel(WidgetTree, Screen, TEXT("NavExport"), TEXT("EXPORT"), FVector2D(28.0f, 258.0f), FVector2D(150.0f, 20.0f), 12, Muted);
    AddPanel(WidgetTree, Screen, TEXT("NavActive"), FVector2D(16.0f, 122.0f), FVector2D(188.0f, 36.0f), SurfaceRaised);
    AddPanel(WidgetTree, Screen, TEXT("NavActiveAccent"), FVector2D(16.0f, 122.0f), FVector2D(4.0f, 36.0f), Blue);
    AddLabel(WidgetTree, Screen, TEXT("NavHint"), TEXT("Select a workspace to continue"), FVector2D(28.0f, 710.0f), FVector2D(170.0f, 36.0f), 9, Muted);

    AddLabel(WidgetTree, Screen, TEXT("DashboardTitle"), TEXT("MAIN DASHBOARD"), FVector2D(260.0f, 96.0f), FVector2D(480.0f, 34.0f), 24, Text);
    AddLabel(WidgetTree, Screen, TEXT("DashboardSubtitle"), TEXT("Build, preview, and ship a character-ready asset package."), FVector2D(260.0f, 132.0f), FVector2D(620.0f, 22.0f), 11, Muted);

    AddPanel(WidgetTree, Screen, TEXT("LivePreviewPanel"), FVector2D(260.0f, 178.0f), FVector2D(600.0f, 424.0f), Surface);
    AddLabel(WidgetTree, Screen, TEXT("LivePreviewEyebrow"), TEXT("LIVE PREVIEW"), FVector2D(286.0f, 204.0f), FVector2D(200.0f, 18.0f), 10, Muted);
    AddLabel(WidgetTree, Screen, TEXT("LivePreviewName"), TEXT("NOVA HERO"), FVector2D(286.0f, 230.0f), FVector2D(220.0f, 28.0f), 18, Gold);
    AddLabel(WidgetTree, Screen, TEXT("LivePreviewStatus"), TEXT("READY TO EDIT"), FVector2D(286.0f, 264.0f), FVector2D(220.0f, 18.0f), 10, Success);
    AddPanel(WidgetTree, Screen, TEXT("PreviewStage"), FVector2D(458.0f, 244.0f), FVector2D(190.0f, 278.0f), SurfaceRaised);
    AddPanel(WidgetTree, Screen, TEXT("PreviewHead"), FVector2D(528.0f, 276.0f), FVector2D(50.0f, 62.0f), Gold);
    AddPanel(WidgetTree, Screen, TEXT("PreviewBody"), FVector2D(508.0f, 342.0f), FVector2D(90.0f, 126.0f), FLinearColor(0.34f, 0.27f, 0.19f, 1.0f));
    AddPanel(WidgetTree, Screen, TEXT("PreviewBase"), FVector2D(492.0f, 478.0f), FVector2D(122.0f, 16.0f), FLinearColor(0.20f, 0.14f, 0.10f, 1.0f));
    AddLabel(WidgetTree, Screen, TEXT("PreviewFootnote"), TEXT("BODY / FACE / OUTFIT  •  24 ASSETS"), FVector2D(286.0f, 558.0f), FVector2D(360.0f, 18.0f), 9, Muted);

    AddPanel(WidgetTree, Screen, TEXT("QuickActionsPanel"), FVector2D(896.0f, 178.0f), FVector2D(330.0f, 424.0f), Surface);
    AddLabel(WidgetTree, Screen, TEXT("QuickActionsHeading"), TEXT("QUICK ACTIONS"), FVector2D(924.0f, 204.0f), FVector2D(240.0f, 20.0f), 10, Muted);

    NewCharacterButton = Button(WidgetTree, TEXT("NEW CHARACTER"), Blue, Text, 12);
    NewCharacterButton->OnClicked.AddDynamic(this, &UCharacterCreatorRootWidget::HandleNewCharacterClicked);
    Place(Screen, NewCharacterButton, FVector2D(924.0f, 244.0f), FVector2D(274.0f, 44.0f));

    UButton* ImportButton = Button(WidgetTree, TEXT("IMPORT ASSET"), SurfaceRaised, Text, 12);
    ImportButton->OnClicked.AddDynamic(this, &UCharacterCreatorRootWidget::HandleImportAssetClicked);
    Place(Screen, ImportButton, FVector2D(924.0f, 300.0f), FVector2D(274.0f, 44.0f));

    AddLabel(WidgetTree, Screen, TEXT("QuickActionHint"), TEXT("Start from a template or bring in an existing mesh."), FVector2D(924.0f, 364.0f), FVector2D(258.0f, 40.0f), 10, Muted);
    AddPanel(WidgetTree, Screen, TEXT("DashboardStatusPanel"), FVector2D(924.0f, 438.0f), FVector2D(274.0f, 74.0f), SurfaceMuted);
    AddLabel(WidgetTree, Screen, TEXT("DashboardStatusLabel"), TEXT("SYSTEM STATUS"), FVector2D(942.0f, 452.0f), FVector2D(180.0f, 14.0f), 9, Muted);
    DashboardStatusText = Label(WidgetTree, TEXT("Ready for a new character"), 11, Success);
    Place(Screen, DashboardStatusText, FVector2D(942.0f, 476.0f), FVector2D(230.0f, 20.0f));

    AddLabel(WidgetTree, Screen, TEXT("RecentHeading"), TEXT("RECENT PROJECTS"), FVector2D(260.0f, 650.0f), FVector2D(240.0f, 20.0f), 10, Muted);
    AddPanel(WidgetTree, Screen, TEXT("RecentOne"), FVector2D(260.0f, 682.0f), FVector2D(280.0f, 72.0f), Surface);
    AddLabel(WidgetTree, Screen, TEXT("RecentOneName"), TEXT("Nova Hero"), FVector2D(280.0f, 698.0f), FVector2D(180.0f, 18.0f), 12, Text);
    AddLabel(WidgetTree, Screen, TEXT("RecentOneMeta"), TEXT("Edited today  •  24 assets"), FVector2D(280.0f, 724.0f), FVector2D(200.0f, 16.0f), 9, Muted);
    AddPanel(WidgetTree, Screen, TEXT("RecentTwo"), FVector2D(560.0f, 682.0f), FVector2D(280.0f, 72.0f), Surface);
    AddLabel(WidgetTree, Screen, TEXT("RecentTwoName"), TEXT("Ranger Tech"), FVector2D(580.0f, 698.0f), FVector2D(180.0f, 18.0f), 12, Text);
    AddLabel(WidgetTree, Screen, TEXT("RecentTwoMeta"), TEXT("Edited yesterday  •  18 assets"), FVector2D(580.0f, 724.0f), FVector2D(210.0f, 16.0f), 9, Muted);
}

void UCharacterCreatorRootWidget::BuildCharacterCreator(UCanvasPanel* Screen)
{
    using namespace CharacterCreatorUI;

    AddPanel(WidgetTree, Screen, TEXT("CreatorBackground"), FVector2D::ZeroVector, FVector2D(1440.0f, 810.0f), Ink);
    AddPanel(WidgetTree, Screen, TEXT("CreatorTopBar"), FVector2D(0.0f, 0.0f), FVector2D(1440.0f, 64.0f), Surface);
    AddPanel(WidgetTree, Screen, TEXT("CreatorAccent"), FVector2D(0.0f, 60.0f), FVector2D(1440.0f, 4.0f), Gold);
    AddPanel(WidgetTree, Screen, TEXT("CreatorRail"), FVector2D(0.0f, 64.0f), FVector2D(220.0f, 746.0f), SurfaceMuted);
    AddPanel(WidgetTree, Screen, TEXT("CreatorWorkspace"), FVector2D(220.0f, 64.0f), FVector2D(760.0f, 746.0f), Ink);
    AddPanel(WidgetTree, Screen, TEXT("CreatorInspector"), FVector2D(980.0f, 64.0f), FVector2D(460.0f, 746.0f), SurfaceMuted);

    AddLabel(WidgetTree, Screen, TEXT("CreatorBrand"), TEXT("CHARACTER CREATOR"), FVector2D(28.0f, 17.0f), FVector2D(280.0f, 20.0f), 16, Gold);
    AddLabel(WidgetTree, Screen, TEXT("CreatorWorkspaceName"), TEXT("NEW CHARACTER / BODY + FACE"), FVector2D(28.0f, 39.0f), FVector2D(320.0f, 16.0f), 9, Muted);
    AddLabel(WidgetTree, Screen, TEXT("CreatorNavHeading"), TEXT("WORKFLOW"), FVector2D(28.0f, 94.0f), FVector2D(150.0f, 18.0f), 10, Muted);
    AddLabel(WidgetTree, Screen, TEXT("CreatorNavBody"), TEXT("BODY / FACE"), FVector2D(28.0f, 134.0f), FVector2D(160.0f, 20.0f), 12, Blue);
    AddLabel(WidgetTree, Screen, TEXT("CreatorNavOutfit"), TEXT("OUTFIT"), FVector2D(28.0f, 176.0f), FVector2D(160.0f, 20.0f), 12, Muted);
    AddLabel(WidgetTree, Screen, TEXT("CreatorNavHair"), TEXT("HAIR / GROOMING"), FVector2D(28.0f, 218.0f), FVector2D(160.0f, 20.0f), 12, Muted);
    AddLabel(WidgetTree, Screen, TEXT("CreatorNavMaterials"), TEXT("MATERIALS / COLOR"), FVector2D(28.0f, 260.0f), FVector2D(170.0f, 20.0f), 12, Muted);
    AddPanel(WidgetTree, Screen, TEXT("CreatorActiveNav"), FVector2D(16.0f, 122.0f), FVector2D(188.0f, 36.0f), SurfaceRaised);
    AddPanel(WidgetTree, Screen, TEXT("CreatorActiveAccent"), FVector2D(16.0f, 122.0f), FVector2D(4.0f, 36.0f), Gold);

    AddLabel(WidgetTree, Screen, TEXT("CreatorTitle"), TEXT("BODY & FACE"), FVector2D(252.0f, 92.0f), FVector2D(360.0f, 30.0f), 22, Text);
    AddLabel(WidgetTree, Screen, TEXT("CreatorSubtitle"), TEXT("Shape the base proportions before layering outfit and materials."), FVector2D(252.0f, 128.0f), FVector2D(580.0f, 20.0f), 11, Muted);
    AddPanel(WidgetTree, Screen, TEXT("CreatorPreviewPanel"), FVector2D(252.0f, 178.0f), FVector2D(680.0f, 494.0f), Surface);
    AddLabel(WidgetTree, Screen, TEXT("CreatorPreviewLabel"), TEXT("LIVE PREVIEW  •  FRONT"), FVector2D(278.0f, 204.0f), FVector2D(260.0f, 18.0f), 10, Muted);
    AddPanel(WidgetTree, Screen, TEXT("CreatorStage"), FVector2D(496.0f, 244.0f), FVector2D(190.0f, 310.0f), SurfaceRaised);
    AddPanel(WidgetTree, Screen, TEXT("CreatorHead"), FVector2D(566.0f, 278.0f), FVector2D(50.0f, 62.0f), Gold);
    AddPanel(WidgetTree, Screen, TEXT("CreatorBody"), FVector2D(546.0f, 344.0f), FVector2D(90.0f, 140.0f), FLinearColor(0.34f, 0.27f, 0.19f, 1.0f));
    AddPanel(WidgetTree, Screen, TEXT("CreatorBase"), FVector2D(530.0f, 494.0f), FVector2D(122.0f, 16.0f), FLinearColor(0.20f, 0.14f, 0.10f, 1.0f));
    AddLabel(WidgetTree, Screen, TEXT("CreatorPreviewMeta"), TEXT("NOVA HERO  •  RIG STANDARD HUMANOID"), FVector2D(278.0f, 620.0f), FVector2D(440.0f, 18.0f), 9, Gold);

    AddLabel(WidgetTree, Screen, TEXT("InspectorHeading"), TEXT("BODY PROPORTIONS"), FVector2D(1012.0f, 96.0f), FVector2D(260.0f, 20.0f), 10, Muted);
    AddLabel(WidgetTree, Screen, TEXT("InspectorHint"), TEXT("Adjust values and preview changes instantly."), FVector2D(1012.0f, 124.0f), FVector2D(350.0f, 20.0f), 10, Muted);

    BodyParameterSliders.Reset();
    BodyParameterValueLabels.Reset();

    const FCharacterAppearanceState InitialAppearance = Session ? Session->GetAppearanceStateNative() : FCharacterAppearanceState();
    const TArray<TPair<ECharacterCreatorParameter, FString>> ParameterDefinitions = {
        {ECharacterCreatorParameter::Height, TEXT("Height")},
        {ECharacterCreatorParameter::ShoulderWidth, TEXT("Shoulder Width")},
        {ECharacterCreatorParameter::ArmLength, TEXT("Arm Length")},
        {ECharacterCreatorParameter::LegLength, TEXT("Leg Length")},
        {ECharacterCreatorParameter::HeadScale, TEXT("Head Scale")}
    };

    float SliderY = 182.0f;
    for (const TPair<ECharacterCreatorParameter, FString>& ParameterDefinition : ParameterDefinitions)
    {
        const FString ParameterName = ParameterDefinition.Value;
        const float InitialValue = InitialAppearance.GetParameterValue(ParameterDefinition.Key);

        AddLabel(WidgetTree, Screen, TEXT("InspectorLabel_") + ParameterName, ParameterName.ToUpper(), FVector2D(1012.0f, SliderY), FVector2D(220.0f, 18.0f), 10, Text);

        UTextBlock* ValueLabel = Label(WidgetTree, FString::Printf(TEXT("%.2f"), InitialValue), 10, Gold);
        ValueLabel->Rename(*(TEXT("InspectorValue_") + ParameterName));
        Place(Screen, ValueLabel, FVector2D(1354.0f, SliderY), FVector2D(52.0f, 18.0f));
        BodyParameterValueLabels.Add(ValueLabel);

        UCharacterCreatorSliderWidget* Slider = FCharacterCreatorUIFactory::MakeSlider(WidgetTree, ParameterDefinition.Key, InitialValue);
        Slider->OnParameterValueChanged.AddUObject(this, &UCharacterCreatorRootWidget::HandleBodyParameterChanged);
        Place(Screen, Slider, FVector2D(1012.0f, SliderY + 24.0f), FVector2D(352.0f, 18.0f));
        BodyParameterSliders.Add(Slider);

        SliderY += 70.0f;
    }

    AddPanel(WidgetTree, Screen, TEXT("CreatorStatusPanel"), FVector2D(1012.0f, 548.0f), FVector2D(352.0f, 58.0f), Surface);
    AddLabel(WidgetTree, Screen, TEXT("CreatorStatusLabel"), TEXT("STATUS"), FVector2D(1030.0f, 560.0f), FVector2D(100.0f, 14.0f), 9, Muted);
    CharacterStatusText = Label(WidgetTree, TEXT("Unsaved changes"), 11, Gold);
    Place(Screen, CharacterStatusText, FVector2D(1030.0f, 580.0f), FVector2D(300.0f, 18.0f));

    BackToDashboardButton = Button(WidgetTree, TEXT("BACK TO DASHBOARD"), SurfaceRaised, Text, 11);
    BackToDashboardButton->OnClicked.AddDynamic(this, &UCharacterCreatorRootWidget::HandleBackToDashboardClicked);
    Place(Screen, BackToDashboardButton, FVector2D(252.0f, 714.0f), FVector2D(190.0f, 36.0f));

    SaveCharacterButton = Button(WidgetTree, TEXT("SAVE CHARACTER"), Gold, Ink, 11);
    SaveCharacterButton->OnClicked.AddDynamic(this, &UCharacterCreatorRootWidget::HandleSaveCharacterClicked);
    Place(Screen, SaveCharacterButton, FVector2D(1174.0f, 714.0f), FVector2D(190.0f, 36.0f));
}

void UCharacterCreatorRootWidget::ApplyScreen(ECharacterCreatorScreen NewScreen)
{
    if (!ScreenSwitcher)
    {
        return;
    }

    const int32 ScreenIndex = NewScreen == ECharacterCreatorScreen::Dashboard ? 0 : 1;
    ScreenSwitcher->SetActiveWidgetIndex(ScreenIndex);

    if (NewScreen == ECharacterCreatorScreen::CharacterCreator && NewCharacterButton)
    {
        if (BackToDashboardButton)
        {
            UCharacterCreatorUIHelpers::FocusWidget(BackToDashboardButton);
        }
    }
    else if (NewCharacterButton)
    {
        UCharacterCreatorUIHelpers::FocusWidget(NewCharacterButton);
    }
}

void UCharacterCreatorRootWidget::ApplyStatus(const FText& NewStatus)
{
    if (DashboardStatusText && !NewStatus.IsEmpty())
    {
        DashboardStatusText->SetText(NewStatus);
    }

    if (CharacterStatusText && !NewStatus.IsEmpty())
    {
        CharacterStatusText->SetText(NewStatus);
    }
}

void UCharacterCreatorRootWidget::ApplyAppearance(const FCharacterAppearanceState& NewAppearance)
{
    bRefreshingAppearance = true;

    const int32 ControlCount = FMath::Min(BodyParameterSliders.Num(), BodyParameterValueLabels.Num());
    for (int32 Index = 0; Index < ControlCount; ++Index)
    {
        if (UCharacterCreatorSliderWidget* Slider = BodyParameterSliders[Index])
        {
            const float Value = NewAppearance.GetParameterValue(Slider->GetParameter());
            Slider->SetValue(Value);

            if (UTextBlock* ValueLabel = BodyParameterValueLabels[Index])
            {
                ValueLabel->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), Value)));
            }
        }
    }

    bRefreshingAppearance = false;
}

void UCharacterCreatorRootWidget::HandleBodyParameterChanged(ECharacterCreatorParameter Parameter, float Value)
{
    if (!bRefreshingAppearance && Session)
    {
        Session->SetParameterValue(Parameter, Value);
    }
}

void UCharacterCreatorRootWidget::HandleNewCharacterClicked()
{
    if (Session)
    {
        Session->SetStatusMessage(FText::FromString(TEXT("New character workspace opened")));
        Session->SetScreen(ECharacterCreatorScreen::CharacterCreator);
    }
}

void UCharacterCreatorRootWidget::HandleBackToDashboardClicked()
{
    if (Session)
    {
        Session->SetStatusMessage(FText::FromString(TEXT("Ready for a new character")));
        Session->SetScreen(ECharacterCreatorScreen::Dashboard);
    }
}

void UCharacterCreatorRootWidget::HandleSaveCharacterClicked()
{
    if (Session)
    {
        Session->SetStatusMessage(FText::FromString(TEXT("Character saved to library")));
    }
}

void UCharacterCreatorRootWidget::HandleOpenProjectClicked()
{
    if (Session)
    {
        Session->SetStatusMessage(FText::FromString(TEXT("Project browser is ready for the next pass")));
    }
}

void UCharacterCreatorRootWidget::HandleImportAssetClicked()
{
    if (Session)
    {
        Session->SetStatusMessage(FText::FromString(TEXT("Import workflow is queued for the next pass")));
    }
}
