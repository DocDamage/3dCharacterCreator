#include "UI/CharacterCreatorRootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/Paths.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

#include "CharacterCreatorPreviewActor.h"
#include "UI/CharacterCreatorUIFramework.h"
#include "UI/CharacterCreatorAnimationWorkspaceWidget.h"
#include "UI/CharacterCreatorExportService.h"
#include "UI/CharacterCreatorSubsystem.h"
#include "UI/CharacterCreatorUtilityWorkspaceWidget.h"
#include "UI/CharacterCreatorWorkflowScreenWidget.h"

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

namespace
{
    FText PreviewMessageForState(ECharacterCreatorPreviewState State)
    {
        switch (State)
        {
        case ECharacterCreatorPreviewState::Loading:
            return FText::FromString(TEXT("Loading character assets..."));
        case ECharacterCreatorPreviewState::Ready:
            return FText::FromString(TEXT("Character preview ready"));
        case ECharacterCreatorPreviewState::UsingFallback:
            return FText::FromString(TEXT("Using fallback preview"));
        case ECharacterCreatorPreviewState::Failed:
            return FText::FromString(TEXT("Preview asset failed"));
        case ECharacterCreatorPreviewState::Uninitialized:
        default:
            return FText::FromString(TEXT("Initializing preview..."));
        }
    }
}

void UCharacterCreatorRootWidget::InitializeWithSession(UCharacterCreatorSession* InSession)
{
    Session = InSession;
}

void UCharacterCreatorRootWidget::InitializeWithPreviewActor(ACharacterCreatorPreviewActor* InPreviewActor)
{
    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.RemoveAll(this);
    }

    PreviewActor = InPreviewActor;
    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.AddUObject(this, &UCharacterCreatorRootWidget::ApplyPreviewState);
        ApplyPreviewRenderTarget();
        ApplyPreviewState(PreviewActor->GetPreviewState(), PreviewMessageForState(PreviewActor->GetPreviewState()));
    }
}

void UCharacterCreatorRootWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);

    ModalManager = NewObject<UCharacterCreatorModalManager>(this, TEXT("CharacterCreatorModalManager"));
    if (ModalManager)
    {
        ModalManager->Initialize(this);
    }

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

    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.RemoveAll(this);
        PreviewActor->OnPreviewStateChanged.AddUObject(this, &UCharacterCreatorRootWidget::ApplyPreviewState);
        ApplyPreviewRenderTarget();
        ApplyPreviewState(PreviewActor->GetPreviewState(), PreviewMessageForState(PreviewActor->GetPreviewState()));
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

    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.RemoveAll(this);
    }

    if (ModalManager)
    {
        ModalManager->CloseAllModals();
        ModalManager = nullptr;
    }

    ModalDialogs.Reset();
    ModalOverlay = nullptr;

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

    ModalOverlay = WidgetTree->ConstructWidget<UCanvasPanel>();
    Place(RootCanvas, ModalOverlay, FVector2D::ZeroVector, FVector2D(1440.0f, 810.0f));

    DashboardScreen = WidgetTree->ConstructWidget<UCanvasPanel>();
    CharacterScreen = WidgetTree->ConstructWidget<UCanvasPanel>();
    OutfitScreen = WidgetTree->ConstructWidget<UCharacterCreatorWorkflowScreenWidget>();
    HairScreen = WidgetTree->ConstructWidget<UCharacterCreatorWorkflowScreenWidget>();
    MaterialsScreen = WidgetTree->ConstructWidget<UCharacterCreatorWorkflowScreenWidget>();
    WeaponsScreen = WidgetTree->ConstructWidget<UCharacterCreatorWorkflowScreenWidget>();
    OutfitScreen->SetWorkflowScreen(ECharacterCreatorScreen::OutfitAndArmor);
    HairScreen->SetWorkflowScreen(ECharacterCreatorScreen::HairAndGrooming);
    MaterialsScreen->SetWorkflowScreen(ECharacterCreatorScreen::MaterialsAndColor);
    WeaponsScreen->SetWorkflowScreen(ECharacterCreatorScreen::WeaponsAndIK);
    OutfitScreen->InitializeWithSession(Session);
    HairScreen->InitializeWithSession(Session);
    MaterialsScreen->InitializeWithSession(Session);
    WeaponsScreen->InitializeWithSession(Session);
    OutfitScreen->InitializeWithPreviewActor(PreviewActor);
    HairScreen->InitializeWithPreviewActor(PreviewActor);
    MaterialsScreen->InitializeWithPreviewActor(PreviewActor);
    WeaponsScreen->InitializeWithPreviewActor(PreviewActor);
    ScreenSwitcher->AddChild(DashboardScreen);
    ScreenSwitcher->AddChild(CharacterScreen);
    ScreenSwitcher->AddChild(OutfitScreen);
    ScreenSwitcher->AddChild(HairScreen);
    ScreenSwitcher->AddChild(MaterialsScreen);
    ScreenSwitcher->AddChild(WeaponsScreen);

    const TArray<ECharacterCreatorScreen> AnimationWorkspaceScreens = {
        ECharacterCreatorScreen::AnimationOverview,
        ECharacterCreatorScreen::LocomotionSetup,
        ECharacterCreatorScreen::BlendSpaceAssistant,
        ECharacterCreatorScreen::AnimationBlueprintWorkspace,
        ECharacterCreatorScreen::MontageComboBuilder,
        ECharacterCreatorScreen::RetargetingAssistant,
        ECharacterCreatorScreen::SkeletonRigSocketInspector
    };
    AnimationScreens.Reset();
    for (const ECharacterCreatorScreen AnimationScreenId : AnimationWorkspaceScreens)
    {
        UCharacterCreatorAnimationWorkspaceWidget* AnimationScreen = WidgetTree->ConstructWidget<UCharacterCreatorAnimationWorkspaceWidget>();
        AnimationScreen->SetWorkspaceScreen(AnimationScreenId);
        AnimationScreen->InitializeWithSession(Session);
        AnimationScreen->InitializeWithPreviewActor(PreviewActor);
        AnimationScreens.Add(AnimationScreen);
        ScreenSwitcher->AddChild(AnimationScreen);
    }

    const TArray<ECharacterCreatorScreen> UtilityWorkspaceScreens = {
        ECharacterCreatorScreen::PhysicsSetup,
        ECharacterCreatorScreen::GameplayTest,
        ECharacterCreatorScreen::PreviewStudio,
        ECharacterCreatorScreen::PortraitStudio,
        ECharacterCreatorScreen::LODPerformance,
        ECharacterCreatorScreen::AssetBrowser,
        ECharacterCreatorScreen::ImportWizard,
        ECharacterCreatorScreen::Settings
    };
    UtilityScreens.Reset();
    for (const ECharacterCreatorScreen UtilityScreenId : UtilityWorkspaceScreens)
    {
        UCharacterCreatorUtilityWorkspaceWidget* UtilityScreen = WidgetTree->ConstructWidget<UCharacterCreatorUtilityWorkspaceWidget>();
        UtilityScreen->SetWorkspaceScreen(UtilityScreenId);
        UtilityScreen->InitializeWithSession(Session);
        UtilityScreen->InitializeWithPreviewActor(PreviewActor);
        UtilityScreens.Add(UtilityScreen);
        ScreenSwitcher->AddChild(UtilityScreen);
    }

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

    UCharacterCreatorCommandButtonWidget* DashboardCharacterNavigationButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("CHARACTER"), FName(TEXT("body")), ECharacterCreatorButtonStyle::Ghost, 10);
    DashboardCharacterNavigationButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
    Place(Screen, DashboardCharacterNavigationButton, FVector2D(16.0f, 164.0f), FVector2D(188.0f, 36.0f));

    UCharacterCreatorCommandButtonWidget* DashboardAnimationNavigationButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("ANIMATION"), FName(TEXT("animation_overview")), ECharacterCreatorButtonStyle::Ghost, 10);
    DashboardAnimationNavigationButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
    Place(Screen, DashboardAnimationNavigationButton, FVector2D(16.0f, 206.0f), FVector2D(188.0f, 36.0f));

    UCharacterCreatorCommandButtonWidget* DashboardExportNavigationButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("EXPORT"), FName(TEXT("export")), ECharacterCreatorButtonStyle::Ghost, 10);
    DashboardExportNavigationButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
    Place(Screen, DashboardExportNavigationButton, FVector2D(16.0f, 248.0f), FVector2D(188.0f, 36.0f));
    AddLabel(WidgetTree, Screen, TEXT("NavHint"), TEXT("Select a workspace to continue"), FVector2D(28.0f, 710.0f), FVector2D(170.0f, 36.0f), 9, Muted);

    AddLabel(WidgetTree, Screen, TEXT("DashboardTitle"), TEXT("MAIN DASHBOARD"), FVector2D(260.0f, 96.0f), FVector2D(480.0f, 34.0f), 24, Text);
    AddLabel(WidgetTree, Screen, TEXT("DashboardSubtitle"), TEXT("Build, preview, and ship a character-ready asset package."), FVector2D(260.0f, 132.0f), FVector2D(620.0f, 22.0f), 11, Muted);

    AddPanel(WidgetTree, Screen, TEXT("LivePreviewPanel"), FVector2D(260.0f, 178.0f), FVector2D(600.0f, 424.0f), Surface);
    AddLabel(WidgetTree, Screen, TEXT("LivePreviewEyebrow"), TEXT("LIVE PREVIEW"), FVector2D(286.0f, 204.0f), FVector2D(200.0f, 18.0f), 10, Muted);
    AddLabel(WidgetTree, Screen, TEXT("LivePreviewName"), TEXT("NOVA HERO"), FVector2D(286.0f, 230.0f), FVector2D(220.0f, 28.0f), 18, Gold);
    DashboardPreviewStatusText = Label(WidgetTree, TEXT("LOADING PREVIEW"), 10, Gold);
    Place(Screen, DashboardPreviewStatusText, FVector2D(286.0f, 264.0f), FVector2D(260.0f, 18.0f));
    AddPanel(WidgetTree, Screen, TEXT("PreviewStage"), FVector2D(458.0f, 244.0f), FVector2D(190.0f, 278.0f), SurfaceRaised);
    AddPanel(WidgetTree, Screen, TEXT("PreviewHead"), FVector2D(528.0f, 276.0f), FVector2D(50.0f, 62.0f), Gold);
    AddPanel(WidgetTree, Screen, TEXT("PreviewBody"), FVector2D(508.0f, 342.0f), FVector2D(90.0f, 126.0f), FLinearColor(0.34f, 0.27f, 0.19f, 1.0f));
    AddPanel(WidgetTree, Screen, TEXT("PreviewBase"), FVector2D(492.0f, 478.0f), FVector2D(122.0f, 16.0f), FLinearColor(0.20f, 0.14f, 0.10f, 1.0f));
    DashboardPreviewImage = WidgetTree->ConstructWidget<UImage>();
    Place(Screen, DashboardPreviewImage, FVector2D(458.0f, 244.0f), FVector2D(190.0f, 278.0f));
    AddLabel(WidgetTree, Screen, TEXT("PreviewFootnote"), TEXT("BODY / FACE / OUTFIT  •  24 ASSETS"), FVector2D(286.0f, 558.0f), FVector2D(360.0f, 18.0f), 9, Muted);

    AddPanel(WidgetTree, Screen, TEXT("QuickActionsPanel"), FVector2D(896.0f, 178.0f), FVector2D(330.0f, 424.0f), Surface);
    AddLabel(WidgetTree, Screen, TEXT("QuickActionsHeading"), TEXT("QUICK ACTIONS"), FVector2D(924.0f, 204.0f), FVector2D(240.0f, 20.0f), 10, Muted);

    NewCharacterButton = Button(WidgetTree, TEXT("NEW CHARACTER"), Blue, Text, 12);
    NewCharacterButton->OnClicked.AddDynamic(this, &UCharacterCreatorRootWidget::HandleNewCharacterClicked);
    Place(Screen, NewCharacterButton, FVector2D(924.0f, 244.0f), FVector2D(274.0f, 44.0f));

    UButton* ImportButton = Button(WidgetTree, TEXT("IMPORT ASSET"), SurfaceRaised, Text, 12);
    ImportButton->OnClicked.AddDynamic(this, &UCharacterCreatorRootWidget::HandleImportAssetClicked);
    Place(Screen, ImportButton, FVector2D(924.0f, 300.0f), FVector2D(274.0f, 44.0f));

    UButton* OnboardingButton = Button(WidgetTree, TEXT("START ONBOARDING"), SurfaceRaised, Text, 12);
    OnboardingButton->OnClicked.AddDynamic(this, &UCharacterCreatorRootWidget::HandleOnboardingClicked);
    Place(Screen, OnboardingButton, FVector2D(924.0f, 356.0f), FVector2D(274.0f, 44.0f));

    UButton* SaveTemplateButton = Button(WidgetTree, TEXT("SAVE AS TEMPLATE"), SurfaceRaised, Text, 12);
    SaveTemplateButton->OnClicked.AddDynamic(this, &UCharacterCreatorRootWidget::HandleSaveTemplateClicked);
    Place(Screen, SaveTemplateButton, FVector2D(924.0f, 412.0f), FVector2D(274.0f, 44.0f));

    AddLabel(WidgetTree, Screen, TEXT("QuickActionHint"), TEXT("Start from a template or bring in an existing asset."), FVector2D(924.0f, 464.0f), FVector2D(258.0f, 24.0f), 9, Muted);
    AddPanel(WidgetTree, Screen, TEXT("DashboardStatusPanel"), FVector2D(924.0f, 496.0f), FVector2D(274.0f, 74.0f), SurfaceMuted);
    AddLabel(WidgetTree, Screen, TEXT("DashboardStatusLabel"), TEXT("SYSTEM STATUS"), FVector2D(942.0f, 510.0f), FVector2D(180.0f, 14.0f), 9, Muted);
    DashboardStatusText = Label(WidgetTree, TEXT("Ready for a new character"), 11, Success);
    Place(Screen, DashboardStatusText, FVector2D(942.0f, 534.0f), FVector2D(230.0f, 20.0f));

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

    UCharacterCreatorCommandButtonWidget* OutfitNavigationButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("OUTFIT / ARMOR"), FName(TEXT("outfit")), ECharacterCreatorButtonStyle::Ghost, 10);
    OutfitNavigationButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
    Place(Screen, OutfitNavigationButton, FVector2D(16.0f, 164.0f), FVector2D(188.0f, 36.0f));

    UCharacterCreatorCommandButtonWidget* HairNavigationButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("HAIR / GROOMING"), FName(TEXT("hair")), ECharacterCreatorButtonStyle::Ghost, 10);
    HairNavigationButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
    Place(Screen, HairNavigationButton, FVector2D(16.0f, 206.0f), FVector2D(188.0f, 36.0f));

    UCharacterCreatorCommandButtonWidget* MaterialsNavigationButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("MATERIALS / COLOR"), FName(TEXT("materials")), ECharacterCreatorButtonStyle::Ghost, 10);
    MaterialsNavigationButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
    Place(Screen, MaterialsNavigationButton, FVector2D(16.0f, 248.0f), FVector2D(188.0f, 36.0f));

    UCharacterCreatorCommandButtonWidget* WeaponsNavigationButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("WEAPONS / IK"), FName(TEXT("weapons")), ECharacterCreatorButtonStyle::Ghost, 10);
    WeaponsNavigationButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
    Place(Screen, WeaponsNavigationButton, FVector2D(16.0f, 290.0f), FVector2D(188.0f, 36.0f));

    UCharacterCreatorCommandButtonWidget* AnimationNavigationButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("ANIMATION"), FName(TEXT("animation_overview")), ECharacterCreatorButtonStyle::Ghost, 10);
    AnimationNavigationButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
    Place(Screen, AnimationNavigationButton, FVector2D(16.0f, 332.0f), FVector2D(188.0f, 36.0f));

    AddLabel(WidgetTree, Screen, TEXT("CreatorTitle"), TEXT("BODY & FACE"), FVector2D(252.0f, 92.0f), FVector2D(360.0f, 30.0f), 22, Text);
    AddLabel(WidgetTree, Screen, TEXT("CreatorSubtitle"), TEXT("Shape the base proportions before layering outfit and materials."), FVector2D(252.0f, 128.0f), FVector2D(580.0f, 20.0f), 11, Muted);
    AddPanel(WidgetTree, Screen, TEXT("CreatorPreviewPanel"), FVector2D(252.0f, 178.0f), FVector2D(680.0f, 494.0f), Surface);
    AddLabel(WidgetTree, Screen, TEXT("CreatorPreviewLabel"), TEXT("LIVE PREVIEW  •  FRONT"), FVector2D(278.0f, 204.0f), FVector2D(260.0f, 18.0f), 10, Muted);
    CharacterPreviewStatusText = Label(WidgetTree, TEXT("LOADING PREVIEW"), 10, Gold);
    Place(Screen, CharacterPreviewStatusText, FVector2D(278.0f, 226.0f), FVector2D(300.0f, 18.0f));
    AddPanel(WidgetTree, Screen, TEXT("CreatorStage"), FVector2D(496.0f, 244.0f), FVector2D(190.0f, 310.0f), SurfaceRaised);
    AddPanel(WidgetTree, Screen, TEXT("CreatorHead"), FVector2D(566.0f, 278.0f), FVector2D(50.0f, 62.0f), Gold);
    AddPanel(WidgetTree, Screen, TEXT("CreatorBody"), FVector2D(546.0f, 344.0f), FVector2D(90.0f, 140.0f), FLinearColor(0.34f, 0.27f, 0.19f, 1.0f));
    AddPanel(WidgetTree, Screen, TEXT("CreatorBase"), FVector2D(530.0f, 494.0f), FVector2D(122.0f, 16.0f), FLinearColor(0.20f, 0.14f, 0.10f, 1.0f));
    CharacterPreviewImage = WidgetTree->ConstructWidget<UImage>();
    Place(Screen, CharacterPreviewImage, FVector2D(496.0f, 244.0f), FVector2D(190.0f, 310.0f));
    AddLabel(WidgetTree, Screen, TEXT("CreatorPreviewMeta"), TEXT("NOVA HERO  •  RIG STANDARD HUMANOID"), FVector2D(278.0f, 620.0f), FVector2D(440.0f, 18.0f), 9, Gold);

    AddLabel(WidgetTree, Screen, TEXT("InspectorHeading"), TEXT("BODY + FACE"), FVector2D(1012.0f, 96.0f), FVector2D(260.0f, 20.0f), 10, Muted);
    AddLabel(WidgetTree, Screen, TEXT("BodyHint"), TEXT("BODY"), FVector2D(1012.0f, 124.0f), FVector2D(120.0f, 20.0f), 10, Muted);
    AddLabel(WidgetTree, Screen, TEXT("FaceHint"), TEXT("FACE"), FVector2D(1190.0f, 124.0f), FVector2D(120.0f, 20.0f), 10, Muted);

    BodyParameterSliders.Reset();
    BodyParameterValueLabels.Reset();

    const FCharacterAppearanceState InitialAppearance = Session ? Session->GetAppearanceStateNative() : FCharacterAppearanceState();
    const TArray<TPair<ECharacterCreatorParameter, FString>> ParameterDefinitions = {
        {ECharacterCreatorParameter::Height, TEXT("Height")},
        {ECharacterCreatorParameter::ShoulderWidth, TEXT("Shoulder Width")},
        {ECharacterCreatorParameter::ArmLength, TEXT("Arm Length")},
        {ECharacterCreatorParameter::LegLength, TEXT("Leg Length")},
        {ECharacterCreatorParameter::HeadScale, TEXT("Head Scale")},
        {ECharacterCreatorParameter::BrowHeight, TEXT("Brow Height")},
        {ECharacterCreatorParameter::JawWidth, TEXT("Jaw Width")},
        {ECharacterCreatorParameter::NoseWidth, TEXT("Nose Width")},
        {ECharacterCreatorParameter::EyeSize, TEXT("Eye Size")},
        {ECharacterCreatorParameter::MouthWidth, TEXT("Mouth Width")}
    };

    for (int32 ParameterIndex = 0; ParameterIndex < ParameterDefinitions.Num(); ++ParameterIndex)
    {
        const TPair<ECharacterCreatorParameter, FString>& ParameterDefinition = ParameterDefinitions[ParameterIndex];
        const bool bFaceParameter = ParameterIndex >= 5;
        const float SliderX = bFaceParameter ? 1190.0f : 1012.0f;
        const float ValueX = bFaceParameter ? 1354.0f : 1154.0f;
        const float SliderY = 182.0f + static_cast<float>(ParameterIndex % 5) * 56.0f;
        const FString ParameterName = ParameterDefinition.Value;
        const float InitialValue = InitialAppearance.GetParameterValue(ParameterDefinition.Key);

        AddLabel(WidgetTree, Screen, TEXT("InspectorLabel_") + ParameterName, ParameterName.ToUpper(), FVector2D(SliderX, SliderY), FVector2D(140.0f, 18.0f), 9, Text);

        UTextBlock* ValueLabel = Label(WidgetTree, FString::Printf(TEXT("%.2f"), InitialValue), 10, Gold);
        ValueLabel->Rename(*(TEXT("InspectorValue_") + ParameterName));
        Place(Screen, ValueLabel, FVector2D(ValueX, SliderY), FVector2D(36.0f, 18.0f));
        BodyParameterValueLabels.Add(ValueLabel);

        UCharacterCreatorSliderWidget* Slider = FCharacterCreatorUIFactory::MakeSlider(WidgetTree, ParameterDefinition.Key, InitialValue);
        Slider->OnParameterValueChanged.AddUObject(this, &UCharacterCreatorRootWidget::HandleBodyParameterChanged);
        Place(Screen, Slider, FVector2D(SliderX, SliderY + 24.0f), FVector2D(154.0f, 18.0f));
        BodyParameterSliders.Add(Slider);
    }

    AddPanel(WidgetTree, Screen, TEXT("CreatorStatusPanel"), FVector2D(1012.0f, 500.0f), FVector2D(352.0f, 74.0f), Surface);
    AddLabel(WidgetTree, Screen, TEXT("CreatorStatusLabel"), TEXT("STATUS"), FVector2D(1030.0f, 514.0f), FVector2D(100.0f, 14.0f), 9, Muted);
    CharacterStatusText = Label(WidgetTree, TEXT("Unsaved changes"), 11, Gold);
    Place(Screen, CharacterStatusText, FVector2D(1030.0f, 536.0f), FVector2D(300.0f, 18.0f));

    BackToDashboardButton = Button(WidgetTree, TEXT("BACK TO DASHBOARD"), SurfaceRaised, Text, 11);
    BackToDashboardButton->OnClicked.AddDynamic(this, &UCharacterCreatorRootWidget::HandleBackToDashboardClicked);
    Place(Screen, BackToDashboardButton, FVector2D(252.0f, 714.0f), FVector2D(190.0f, 36.0f));

    SaveCharacterButton = Button(WidgetTree, TEXT("SAVE CHARACTER"), Gold, Ink, 11);
    SaveCharacterButton->OnClicked.AddDynamic(this, &UCharacterCreatorRootWidget::HandleSaveCharacterClicked);
    Place(Screen, SaveCharacterButton, FVector2D(1174.0f, 714.0f), FVector2D(190.0f, 36.0f));

    RevertCharacterButton = Button(WidgetTree, TEXT("REVERT"), SurfaceRaised, Text, 11);
    RevertCharacterButton->OnClicked.AddDynamic(this, &UCharacterCreatorRootWidget::HandleRevertCharacterClicked);
    Place(Screen, RevertCharacterButton, FVector2D(1000.0f, 714.0f), FVector2D(150.0f, 36.0f));
}

void UCharacterCreatorRootWidget::ApplyScreen(ECharacterCreatorScreen NewScreen)
{
    if (!ScreenSwitcher)
    {
        return;
    }

    int32 ScreenIndex = 0;
    switch (NewScreen)
    {
    case ECharacterCreatorScreen::Dashboard:
        ScreenIndex = 0;
        break;
    case ECharacterCreatorScreen::CharacterCreator:
        ScreenIndex = 1;
        break;
    case ECharacterCreatorScreen::OutfitAndArmor:
        ScreenIndex = 2;
        break;
    case ECharacterCreatorScreen::HairAndGrooming:
        ScreenIndex = 3;
        break;
    case ECharacterCreatorScreen::MaterialsAndColor:
        ScreenIndex = 4;
        break;
    case ECharacterCreatorScreen::WeaponsAndIK:
        ScreenIndex = 5;
        break;
    case ECharacterCreatorScreen::AnimationOverview:
        ScreenIndex = 6;
        break;
    case ECharacterCreatorScreen::LocomotionSetup:
        ScreenIndex = 7;
        break;
    case ECharacterCreatorScreen::BlendSpaceAssistant:
        ScreenIndex = 8;
        break;
    case ECharacterCreatorScreen::AnimationBlueprintWorkspace:
        ScreenIndex = 9;
        break;
    case ECharacterCreatorScreen::MontageComboBuilder:
        ScreenIndex = 10;
        break;
    case ECharacterCreatorScreen::RetargetingAssistant:
        ScreenIndex = 11;
        break;
    case ECharacterCreatorScreen::SkeletonRigSocketInspector:
        ScreenIndex = 12;
        break;
    case ECharacterCreatorScreen::PhysicsSetup:
        ScreenIndex = 13;
        break;
    case ECharacterCreatorScreen::GameplayTest:
        ScreenIndex = 14;
        break;
    case ECharacterCreatorScreen::PreviewStudio:
        ScreenIndex = 15;
        break;
    case ECharacterCreatorScreen::PortraitStudio:
        ScreenIndex = 16;
        break;
    case ECharacterCreatorScreen::LODPerformance:
        ScreenIndex = 17;
        break;
    case ECharacterCreatorScreen::AssetBrowser:
        ScreenIndex = 18;
        break;
    case ECharacterCreatorScreen::ImportWizard:
        ScreenIndex = 19;
        break;
    case ECharacterCreatorScreen::Settings:
        ScreenIndex = 20;
        break;
    default:
        ScreenIndex = 1;
        break;
    }
    ScreenSwitcher->SetActiveWidgetIndex(ScreenIndex);

    if (NewScreen == ECharacterCreatorScreen::CharacterCreator && NewCharacterButton)
    {
        if (BackToDashboardButton)
        {
            LastFocusWidget = BackToDashboardButton;
            UCharacterCreatorUIHelpers::FocusWidget(BackToDashboardButton);
        }
    }
    else if (NewCharacterButton)
    {
        LastFocusWidget = NewCharacterButton;
        UCharacterCreatorUIHelpers::FocusWidget(NewCharacterButton);
    }

    switch (NewScreen)
    {
    case ECharacterCreatorScreen::OutfitAndArmor:
        if (OutfitScreen) { OutfitScreen->FocusFirstControl(); LastFocusWidget = OutfitScreen; }
        break;
    case ECharacterCreatorScreen::HairAndGrooming:
        if (HairScreen) { HairScreen->FocusFirstControl(); LastFocusWidget = HairScreen; }
        break;
    case ECharacterCreatorScreen::MaterialsAndColor:
        if (MaterialsScreen) { MaterialsScreen->FocusFirstControl(); LastFocusWidget = MaterialsScreen; }
        break;
    case ECharacterCreatorScreen::WeaponsAndIK:
        if (WeaponsScreen) { WeaponsScreen->FocusFirstControl(); LastFocusWidget = WeaponsScreen; }
        break;
    case ECharacterCreatorScreen::AnimationOverview:
    case ECharacterCreatorScreen::LocomotionSetup:
    case ECharacterCreatorScreen::BlendSpaceAssistant:
    case ECharacterCreatorScreen::AnimationBlueprintWorkspace:
    case ECharacterCreatorScreen::MontageComboBuilder:
    case ECharacterCreatorScreen::RetargetingAssistant:
    case ECharacterCreatorScreen::SkeletonRigSocketInspector:
        if (AnimationScreens.IsValidIndex(static_cast<int32>(NewScreen) - static_cast<int32>(ECharacterCreatorScreen::AnimationOverview)) && AnimationScreens[static_cast<int32>(NewScreen) - static_cast<int32>(ECharacterCreatorScreen::AnimationOverview)])
        {
            UCharacterCreatorAnimationWorkspaceWidget* FocusTarget = AnimationScreens[static_cast<int32>(NewScreen) - static_cast<int32>(ECharacterCreatorScreen::AnimationOverview)];
            FocusTarget->FocusFirstControl();
            LastFocusWidget = FocusTarget;
        }
        break;
    case ECharacterCreatorScreen::PhysicsSetup:
    case ECharacterCreatorScreen::GameplayTest:
    case ECharacterCreatorScreen::PreviewStudio:
    case ECharacterCreatorScreen::PortraitStudio:
    case ECharacterCreatorScreen::LODPerformance:
    case ECharacterCreatorScreen::AssetBrowser:
    case ECharacterCreatorScreen::ImportWizard:
    case ECharacterCreatorScreen::Settings:
        if (UtilityScreens.IsValidIndex(static_cast<int32>(NewScreen) - static_cast<int32>(ECharacterCreatorScreen::PhysicsSetup)) && UtilityScreens[static_cast<int32>(NewScreen) - static_cast<int32>(ECharacterCreatorScreen::PhysicsSetup)])
        {
            UCharacterCreatorUtilityWorkspaceWidget* FocusTarget = UtilityScreens[static_cast<int32>(NewScreen) - static_cast<int32>(ECharacterCreatorScreen::PhysicsSetup)];
            FocusTarget->FocusFirstControl();
            LastFocusWidget = FocusTarget;
        }
        break;
    default:
        break;
    }
}

FReply UCharacterCreatorRootWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    if (Key == EKeys::Escape)
    {
        if (ModalManager && ModalManager->CloseTopModal())
        {
            return FReply::Handled();
        }

        if (Session && Session->GetScreen() != ECharacterCreatorScreen::Dashboard)
        {
            Session->SetScreen(ECharacterCreatorScreen::Dashboard);
            return FReply::Handled();
        }
    }

    if (Session && (Key == EKeys::Gamepad_LeftShoulder || Key == EKeys::Gamepad_RightShoulder))
    {
        const TArray<ECharacterCreatorScreen> ScreenOrder = {
            ECharacterCreatorScreen::Dashboard,
            ECharacterCreatorScreen::CharacterCreator,
            ECharacterCreatorScreen::OutfitAndArmor,
            ECharacterCreatorScreen::HairAndGrooming,
            ECharacterCreatorScreen::MaterialsAndColor,
            ECharacterCreatorScreen::WeaponsAndIK,
            ECharacterCreatorScreen::AnimationOverview,
            ECharacterCreatorScreen::LocomotionSetup,
            ECharacterCreatorScreen::BlendSpaceAssistant,
            ECharacterCreatorScreen::AnimationBlueprintWorkspace,
            ECharacterCreatorScreen::MontageComboBuilder,
            ECharacterCreatorScreen::RetargetingAssistant,
            ECharacterCreatorScreen::SkeletonRigSocketInspector,
            ECharacterCreatorScreen::PhysicsSetup,
            ECharacterCreatorScreen::GameplayTest,
            ECharacterCreatorScreen::PreviewStudio,
            ECharacterCreatorScreen::PortraitStudio,
            ECharacterCreatorScreen::LODPerformance,
            ECharacterCreatorScreen::AssetBrowser,
            ECharacterCreatorScreen::ImportWizard,
            ECharacterCreatorScreen::Settings
        };
        const int32 CurrentIndex = ScreenOrder.IndexOfByKey(Session->GetScreen());
        if (CurrentIndex != INDEX_NONE)
        {
            const int32 Direction = Key == EKeys::Gamepad_RightShoulder ? 1 : -1;
            const int32 NextIndex = (CurrentIndex + Direction + ScreenOrder.Num()) % ScreenOrder.Num();
            Session->SetScreen(ScreenOrder[NextIndex]);
            return FReply::Handled();
        }
    }

    if (Session && Key == EKeys::Gamepad_FaceButton_Bottom)
    {
        Session->ApplyAppearanceChanges();
        return FReply::Handled();
    }

    if (Session && Key == EKeys::Gamepad_FaceButton_Right)
    {
        Session->RevertAppearanceChanges();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
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

void UCharacterCreatorRootWidget::ApplyPreviewRenderTarget()
{
    if (!PreviewActor)
    {
        return;
    }

    UTextureRenderTarget2D* RenderTarget = PreviewActor->GetPreviewRenderTarget();
    if (!RenderTarget)
    {
        return;
    }

    FSlateBrush PreviewBrush;
    PreviewBrush.DrawAs = ESlateBrushDrawType::Image;
    PreviewBrush.SetResourceObject(RenderTarget);
    PreviewBrush.ImageSize = FVector2D(512.0f, 768.0f);
    PreviewBrush.TintColor = FSlateColor(FLinearColor::White);

    if (DashboardPreviewImage)
    {
        DashboardPreviewImage->SetBrush(PreviewBrush);
    }

    if (CharacterPreviewImage)
    {
        CharacterPreviewImage->SetBrush(PreviewBrush);
    }
}

void UCharacterCreatorRootWidget::ApplyPreviewState(ECharacterCreatorPreviewState NewState, const FText& Message)
{
    FLinearColor StateColor = CharacterCreatorUI::Gold;
    switch (NewState)
    {
    case ECharacterCreatorPreviewState::Ready:
        StateColor = CharacterCreatorUI::Success;
        break;
    case ECharacterCreatorPreviewState::Failed:
        StateColor = FLinearColor(0.820f, 0.360f, 0.390f, 1.0f);
        break;
    case ECharacterCreatorPreviewState::Loading:
    case ECharacterCreatorPreviewState::UsingFallback:
    case ECharacterCreatorPreviewState::Uninitialized:
    default:
        break;
    }

    if (DashboardPreviewStatusText)
    {
        DashboardPreviewStatusText->SetText(Message);
        DashboardPreviewStatusText->SetColorAndOpacity(FSlateColor(StateColor));
    }

    if (CharacterPreviewStatusText)
    {
        CharacterPreviewStatusText->SetText(Message);
        CharacterPreviewStatusText->SetColorAndOpacity(FSlateColor(StateColor));
    }

    ApplyPreviewRenderTarget();
}

void UCharacterCreatorRootWidget::OpenModalDialog(FName DialogId, const FString& Title, const FString& Message, const TArray<TPair<FName, FString>>& Actions)
{
    if (!WidgetTree || !ModalOverlay || !ModalManager)
    {
        return;
    }

    if (ModalManager->HasOpenModal())
    {
        ModalManager->CloseTopModal();
    }

    UCharacterCreatorModalWidget* Modal = WidgetTree->ConstructWidget<UCharacterCreatorModalWidget>();
    if (!Modal)
    {
        return;
    }

    Modal->Rename(*DialogId.ToString());
    Modal->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f));

    UCanvasPanel* ModalCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    Modal->AddChild(ModalCanvas);

    const int32 ActionCount = FMath::Max(1, Actions.Num());
    const float DialogHeight = FMath::Clamp(230.0f + (static_cast<float>(ActionCount) * 48.0f), 280.0f, 470.0f);
    const FVector2D DialogSize(560.0f, DialogHeight);
    const FVector2D DialogPosition = UCharacterCreatorUIHelpers::ClampPopupPosition(
        FVector2D((1440.0f - DialogSize.X) * 0.5f, (810.0f - DialogSize.Y) * 0.5f),
        DialogSize,
        FVector2D(1440.0f, 810.0f));

    UCharacterCreatorPanelWidget* DialogPanel = FCharacterCreatorUIFactory::MakePanel(WidgetTree, CharacterCreatorUI::SurfaceRaised);
    FCharacterCreatorUIFactory::Place(ModalCanvas, DialogPanel, DialogPosition, DialogSize);

    UCanvasPanel* DialogContent = WidgetTree->ConstructWidget<UCanvasPanel>();
    DialogPanel->AddChild(DialogContent);

    FCharacterCreatorUIFactory::AddLabel(WidgetTree, DialogContent, TEXT("DialogTitle"), Title.ToUpper(), FVector2D(28.0f, 24.0f), FVector2D(500.0f, 28.0f), 16, CharacterCreatorUI::Gold);
    UTextBlock* DialogMessageText = FCharacterCreatorUIFactory::MakeLabel(WidgetTree, Message, 11, CharacterCreatorUI::Text);
    DialogMessageText->SetAutoWrapText(true);
    FCharacterCreatorUIFactory::Place(DialogContent, DialogMessageText, FVector2D(28.0f, 68.0f), FVector2D(500.0f, 88.0f));

    TArray<TPair<FName, FString>> DialogActions = Actions;
    if (DialogActions.Num() == 0)
    {
        DialogActions.Add(TPair<FName, FString>(FName(TEXT("dialog_close")), TEXT("CLOSE")));
    }

    UWidget* FirstAction = nullptr;
    for (int32 ActionIndex = 0; ActionIndex < DialogActions.Num(); ++ActionIndex)
    {
        const TPair<FName, FString>& Action = DialogActions[ActionIndex];
        const bool bPrimary = ActionIndex == 0 && DialogActions.Num() > 1;
        UCharacterCreatorCommandButtonWidget* ActionButton = FCharacterCreatorUIFactory::MakeCommandButton(
            WidgetTree,
            Action.Value,
            Action.Key,
            bPrimary ? ECharacterCreatorButtonStyle::Primary : ECharacterCreatorButtonStyle::Secondary,
            11);
        ActionButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleModalCommand);

        const float ActionY = 174.0f + (static_cast<float>(ActionIndex) * 44.0f);
        FCharacterCreatorUIFactory::Place(DialogContent, ActionButton, FVector2D(28.0f, ActionY), FVector2D(500.0f, 34.0f));
        if (!FirstAction)
        {
            FirstAction = ActionButton;
        }
    }

    FCharacterCreatorUIFactory::Place(ModalOverlay, Modal, FVector2D::ZeroVector, FVector2D(1440.0f, 810.0f));
    ModalDialogs.Add(Modal);

    UWidget* ReturnFocus = LastFocusWidget.Get();
    if (!ReturnFocus)
    {
        ReturnFocus = this;
    }

    if (ModalManager->OpenModal(Modal, ReturnFocus) && FirstAction)
    {
        UCharacterCreatorUIHelpers::FocusWidget(FirstAction);
    }
}

void UCharacterCreatorRootWidget::HandleModalCommand(FName CommandId)
{
    if (CommandId == FName(TEXT("dialog_close"))
        || CommandId == FName(TEXT("dialog_cancel"))
        || CommandId == FName(TEXT("dialog_new_cancel")))
    {
        if (ModalManager)
        {
            ModalManager->CloseTopModal();
        }
        return;
    }

    if (CommandId == FName(TEXT("dialog_new_confirm")))
    {
        if (Session)
        {
            Session->ResetAppearance();
            Session->SetScreen(ECharacterCreatorScreen::CharacterCreator);
        }
        if (ModalManager)
        {
            ModalManager->CloseTopModal();
        }
        return;
    }

    if (CommandId == FName(TEXT("dialog_save_template")))
    {
        if (Session)
        {
            Session->CreatePresetFromCurrent(FText::FromString(TEXT("Saved Template")), FText::FromString(TEXT("Created from the active character workspace.")));
            Session->SetStatusMessage(FText::FromString(TEXT("Saved the active character as a template")));
        }
        if (ModalManager)
        {
            ModalManager->CloseTopModal();
        }
        return;
    }

    if (CommandId == FName(TEXT("dialog_import_validate")))
    {
        if (Session)
        {
            if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
            {
                FCharacterCreatorImportProgress Progress;
                Subsystem->ValidateImportDirectory(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("FreeAnimationsPack")), Progress);
            }
        }
        if (ModalManager)
        {
            ModalManager->CloseTopModal();
        }
        return;
    }

    if (CommandId == FName(TEXT("dialog_import_open")))
    {
        if (ModalManager)
        {
            ModalManager->CloseTopModal();
        }
        if (Session)
        {
            Session->SetScreen(ECharacterCreatorScreen::ImportWizard);
        }
        return;
    }

    if (CommandId == FName(TEXT("dialog_export_full")) || CommandId == FName(TEXT("dialog_export_metadata")))
    {
        if (Session)
        {
            if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
            {
                FCharacterCreatorExportProfile Profile;
                if (CommandId == FName(TEXT("dialog_export_metadata")))
                {
                    Profile.bIncludeMesh = false;
                    Profile.bIncludeMaterials = false;
                    Profile.bIncludeAnimations = false;
                    Profile.bIncludeMetadata = true;
                }
                Subsystem->ExportCurrentManifest(Profile, FString());
            }
        }
        if (ModalManager)
        {
            ModalManager->CloseTopModal();
        }
        return;
    }

    if (CommandId == FName(TEXT("dialog_onboarding_start")))
    {
        if (Session)
        {
            Session->ResetOnboarding();
            Session->AdvanceOnboarding();
            Session->SetScreen(ECharacterCreatorScreen::CharacterCreator);
        }
        if (ModalManager)
        {
            ModalManager->CloseTopModal();
        }
        return;
    }
}

void UCharacterCreatorRootWidget::HandleNewCharacterClicked()
{
    OpenModalDialog(
        FName(TEXT("new_character")),
        TEXT("NEW CHARACTER"),
        TEXT("Start a fresh Sidekick character? Unsaved changes remain available through the current session until you save or revert."),
        {
            TPair<FName, FString>(FName(TEXT("dialog_new_confirm")), TEXT("START NEW CHARACTER")),
            TPair<FName, FString>(FName(TEXT("dialog_new_cancel")), TEXT("CANCEL"))
        });
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
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            Subsystem->SaveAutosave();
        }
        else
        {
            Session->ApplyAppearanceChanges();
            Session->SetStatusMessage(FText::FromString(TEXT("Character changes applied to the active session")));
        }
    }
}

void UCharacterCreatorRootWidget::HandleRevertCharacterClicked()
{
    if (Session)
    {
        Session->RevertAppearanceChanges();
    }
}

void UCharacterCreatorRootWidget::HandleWorkflowCommand(FName CommandId)
{
    if (!Session)
    {
        return;
    }

    if (CommandId == FName(TEXT("body")))
    {
        Session->SetScreen(ECharacterCreatorScreen::CharacterCreator);
    }
    else if (CommandId == FName(TEXT("outfit")))
    {
        Session->SetScreen(ECharacterCreatorScreen::OutfitAndArmor);
    }
    else if (CommandId == FName(TEXT("hair")))
    {
        Session->SetScreen(ECharacterCreatorScreen::HairAndGrooming);
    }
    else if (CommandId == FName(TEXT("materials")))
    {
        Session->SetScreen(ECharacterCreatorScreen::MaterialsAndColor);
    }
    else if (CommandId == FName(TEXT("weapons")))
    {
        Session->SetScreen(ECharacterCreatorScreen::WeaponsAndIK);
    }
    else if (CommandId == FName(TEXT("animation_overview")))
    {
        Session->SetScreen(ECharacterCreatorScreen::AnimationOverview);
    }
    else if (CommandId == FName(TEXT("export")))
    {
        OpenModalDialog(
            FName(TEXT("export_options")),
            TEXT("EXPORT OPTIONS"),
            TEXT("Choose a manifest profile. Validation runs before writing the export file and reports the first actionable failure in the workspace status."),
            {
                TPair<FName, FString>(FName(TEXT("dialog_export_full")), TEXT("FULL CHARACTER MANIFEST")),
                TPair<FName, FString>(FName(TEXT("dialog_export_metadata")), TEXT("METADATA ONLY")),
                TPair<FName, FString>(FName(TEXT("dialog_cancel")), TEXT("CANCEL"))
            });
    }
}

void UCharacterCreatorRootWidget::HandleOpenProjectClicked()
{
    if (Session)
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            if (Subsystem->LoadAutosave())
            {
                Session->SetScreen(ECharacterCreatorScreen::CharacterCreator);
                return;
            }
        }

        Session->SetStatusMessage(FText::FromString(TEXT("No autosave exists yet; create a character first")));
        OpenModalDialog(
            FName(TEXT("load_error")),
            TEXT("NO SAVED PROJECT"),
            TEXT("There is no compatible autosave yet. Save the active character first, then use Open Project to restore it after a restart."),
            {
                TPair<FName, FString>(FName(TEXT("dialog_close")), TEXT("CLOSE"))
            });
    }
}

void UCharacterCreatorRootWidget::HandleImportAssetClicked()
{
    OpenModalDialog(
        FName(TEXT("asset_picker")),
        TEXT("IMPORT ASSET"),
        TEXT("The installed FAB content is available under /Game/FreeAnimationsPack. Validate its files first, or open the import wizard for the full progress view."),
        {
            TPair<FName, FString>(FName(TEXT("dialog_import_validate")), TEXT("VALIDATE FAB CONTENT")),
            TPair<FName, FString>(FName(TEXT("dialog_import_open")), TEXT("OPEN IMPORT WIZARD")),
            TPair<FName, FString>(FName(TEXT("dialog_cancel")), TEXT("CANCEL"))
        });
}

void UCharacterCreatorRootWidget::HandleOnboardingClicked()
{
    OpenModalDialog(
        FName(TEXT("onboarding")),
        TEXT("ONBOARDING"),
        TEXT("The guided setup will walk through the Sidekick preview, body and face controls, outfit selection, and save/export checkpoints."),
        {
            TPair<FName, FString>(FName(TEXT("dialog_onboarding_start")), TEXT("START TUTORIAL")),
            TPair<FName, FString>(FName(TEXT("dialog_cancel")), TEXT("CANCEL"))
        });
}

void UCharacterCreatorRootWidget::HandleSaveTemplateClicked()
{
    OpenModalDialog(
        FName(TEXT("save_template")),
        TEXT("SAVE AS TEMPLATE"),
        TEXT("Create a versioned preset from the current appearance, loadout, colors, and animation workspace metadata?"),
        {
            TPair<FName, FString>(FName(TEXT("dialog_save_template")), TEXT("SAVE TEMPLATE")),
            TPair<FName, FString>(FName(TEXT("dialog_cancel")), TEXT("CANCEL"))
        });
}
