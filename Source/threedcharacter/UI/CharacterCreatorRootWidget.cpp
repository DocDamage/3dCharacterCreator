#include "UI/CharacterCreatorRootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
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
    if (WidgetTree && !bLayoutBuilt)
    {
        BuildLayout();
        bLayoutBuilt = true;
    }
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
        if (Session)
        {
            PreviewActor->ApplyPerformanceSettings(Session->GetSettings());
        }
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
    UE_LOG(LogTemp, Display, TEXT("Character Creator UMG layout built: session=%d root=%d screens=%d"), Session != nullptr, WidgetTree && WidgetTree->RootWidget != nullptr, ScreenSwitcher ? ScreenSwitcher->GetNumWidgets() : 0);

    if (Session)
    {
        Session->OnScreenChanged.AddUObject(this, &UCharacterCreatorRootWidget::ApplyScreen);
        Session->OnStatusChanged.AddUObject(this, &UCharacterCreatorRootWidget::ApplyStatus);
        Session->OnAppearanceChanged.AddUObject(this, &UCharacterCreatorRootWidget::ApplyAppearance);
        Session->OnSettingsChanged.AddUObject(this, &UCharacterCreatorRootWidget::ApplySettings);
        ApplyScreen(Session->GetScreen());
        ApplyStatus(Session->GetStatusMessage());
        ApplyAppearance(Session->GetAppearanceStateNative());
        ApplySettings(Session->GetSettings());
    }

    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.RemoveAll(this);
        PreviewActor->OnPreviewStateChanged.AddUObject(this, &UCharacterCreatorRootWidget::ApplyPreviewState);
        PreviewActor->ApplyPerformanceSettings(Session->GetSettings());
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
        Session->OnSettingsChanged.RemoveAll(this);
    }

    if (MaterialsScreen)
    {
        MaterialsScreen->OnModalRequested.RemoveAll(this);
    }

    for (UCharacterCreatorUtilityWorkspaceWidget* UtilityScreen : UtilityScreens)
    {
        if (UtilityScreen) UtilityScreen->OnModalRequested.RemoveAll(this);
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

    RootScaleBox = WidgetTree->ConstructWidget<UScaleBox>();
    RootScaleBox->SetStretch(EStretch::ScaleToFit);
    WidgetTree->RootWidget = RootScaleBox;

    USizeBox* DesignSize = WidgetTree->ConstructWidget<USizeBox>();
    DesignSize->SetWidthOverride(1440.0f);
    DesignSize->SetHeightOverride(810.0f);
    RootScaleBox->AddChild(DesignSize);

    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    DesignSize->AddChild(RootCanvas);
    ShellCanvas = RootCanvas;

    ScreenSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>();
    Place(RootCanvas, ScreenSwitcher, FVector2D::ZeroVector, FVector2D(1440.0f, 810.0f));

    GlobalOverlay = WidgetTree->ConstructWidget<UCanvasPanel>();
    Place(RootCanvas, GlobalOverlay, FVector2D::ZeroVector, FVector2D(1440.0f, 810.0f));

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
    MaterialsScreen->OnModalRequested.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowModalRequested);
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
        ECharacterCreatorScreen::ProjectBrowser,
        ECharacterCreatorScreen::PhysicsSetup,
        ECharacterCreatorScreen::GameplayTest,
        ECharacterCreatorScreen::PreviewStudio,
        ECharacterCreatorScreen::PortraitStudio,
        ECharacterCreatorScreen::LODPerformance,
        ECharacterCreatorScreen::AssetBrowser,
        ECharacterCreatorScreen::ImportWizard,
        ECharacterCreatorScreen::Settings,
        ECharacterCreatorScreen::ValidationExport
    };
    UtilityScreens.Reset();
    for (const ECharacterCreatorScreen UtilityScreenId : UtilityWorkspaceScreens)
    {
        UCharacterCreatorUtilityWorkspaceWidget* UtilityScreen = WidgetTree->ConstructWidget<UCharacterCreatorUtilityWorkspaceWidget>();
        UtilityScreen->SetWorkspaceScreen(UtilityScreenId);
        UtilityScreen->InitializeWithSession(Session);
        UtilityScreen->InitializeWithPreviewActor(PreviewActor);
        UtilityScreen->OnModalRequested.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowModalRequested);
        UtilityScreens.Add(UtilityScreen);
        ScreenSwitcher->AddChild(UtilityScreen);
    }

    BuildDashboard(DashboardScreen);
    BuildCharacterCreator(CharacterScreen);

    if (GlobalOverlay)
    {
        using namespace CharacterCreatorUI;
        AddPanel(WidgetTree, GlobalOverlay, TEXT("GlobalAssetTray"), FVector2D(220.0f, 770.0f), FVector2D(1220.0f, 40.0f), SurfaceMuted);
        AddLabel(WidgetTree, GlobalOverlay, TEXT("GlobalTrayLabel"), TEXT("ASSET TRAY"), FVector2D(244.0f, 782.0f), FVector2D(100.0f, 14.0f), 9, Muted);

        GlobalActionButtons.Reset();
        const TArray<TTuple<const TCHAR*, const TCHAR*, ECharacterCreatorButtonStyle, FVector2D>> GlobalActions = {
            {TEXT("BROWSER"), TEXT("global_browser"), ECharacterCreatorButtonStyle::Ghost, FVector2D(944.0f, 14.0f)},
            {TEXT("SAVE"), TEXT("global_save"), ECharacterCreatorButtonStyle::Accent, FVector2D(1030.0f, 14.0f)},
            {TEXT("REVERT"), TEXT("global_revert"), ECharacterCreatorButtonStyle::Secondary, FVector2D(1116.0f, 14.0f)},
            {TEXT("SETTINGS"), TEXT("global_settings"), ECharacterCreatorButtonStyle::Ghost, FVector2D(1202.0f, 14.0f)},
            {TEXT("GAMEPAD"), TEXT("global_gamepad"), ECharacterCreatorButtonStyle::Ghost, FVector2D(1290.0f, 14.0f)}
        };
        for (const TTuple<const TCHAR*, const TCHAR*, ECharacterCreatorButtonStyle, FVector2D>& Action : GlobalActions)
        {
            UCharacterCreatorCommandButtonWidget* Button = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, Action.Get<0>(), FName(Action.Get<1>()), Action.Get<2>(), 9);
            Button->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
            GlobalActionButtons.Add(Button);
            Place(GlobalOverlay, Button, Action.Get<3>(), FVector2D(80.0f, 28.0f));
        }

        UCharacterCreatorCommandButtonWidget* AssetTrayButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("OPEN ASSET BROWSER"), FName(TEXT("tray_assets")), ECharacterCreatorButtonStyle::Secondary, 9);
        AssetTrayButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
        Place(GlobalOverlay, AssetTrayButton, FVector2D(360.0f, 776.0f), FVector2D(170.0f, 28.0f));
        UCharacterCreatorCommandButtonWidget* ImportTrayButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("IMPORT"), FName(TEXT("tray_import")), ECharacterCreatorButtonStyle::Secondary, 9);
        ImportTrayButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
        Place(GlobalOverlay, ImportTrayButton, FVector2D(540.0f, 776.0f), FVector2D(100.0f, 28.0f));
        UCharacterCreatorCommandButtonWidget* ExportTrayButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("VALIDATE / EXPORT"), FName(TEXT("export")), ECharacterCreatorButtonStyle::Primary, 9);
        ExportTrayButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
        Place(GlobalOverlay, ExportTrayButton, FVector2D(650.0f, 776.0f), FVector2D(170.0f, 28.0f));
    }
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

    UCharacterCreatorCommandButtonWidget* RandomizeCharacterButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("RANDOMIZE CHARACTER"), FName(TEXT("randomize")), ECharacterCreatorButtonStyle::Secondary, 11);
    RandomizeCharacterButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
    Place(Screen, RandomizeCharacterButton, FVector2D(924.0f, 460.0f), FVector2D(132.0f, 32.0f));

    UCharacterCreatorCommandButtonWidget* PresetManagerButton = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, TEXT("PRESET MANAGER"), FName(TEXT("preset_manager")), ECharacterCreatorButtonStyle::Secondary, 11);
    PresetManagerButton->OnCommand.AddUObject(this, &UCharacterCreatorRootWidget::HandleWorkflowCommand);
    Place(Screen, PresetManagerButton, FVector2D(1066.0f, 460.0f), FVector2D(132.0f, 32.0f));

    AddLabel(WidgetTree, Screen, TEXT("QuickActionHint"), TEXT("Start from a template or bring in an existing asset."), FVector2D(924.0f, 578.0f), FVector2D(258.0f, 18.0f), 9, Muted);
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
    case ECharacterCreatorScreen::ProjectBrowser:
        ScreenIndex = 13;
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
        ScreenIndex = 14;
        break;
    case ECharacterCreatorScreen::GameplayTest:
        ScreenIndex = 15;
        break;
    case ECharacterCreatorScreen::PreviewStudio:
        ScreenIndex = 16;
        break;
    case ECharacterCreatorScreen::PortraitStudio:
        ScreenIndex = 17;
        break;
    case ECharacterCreatorScreen::LODPerformance:
        ScreenIndex = 18;
        break;
    case ECharacterCreatorScreen::AssetBrowser:
        ScreenIndex = 19;
        break;
    case ECharacterCreatorScreen::ImportWizard:
        ScreenIndex = 20;
        break;
    case ECharacterCreatorScreen::Settings:
        ScreenIndex = 21;
        break;
    case ECharacterCreatorScreen::ValidationExport:
        ScreenIndex = 22;
        break;
    default:
        ScreenIndex = 1;
        break;
    }
    ScreenSwitcher->SetActiveWidgetIndex(ScreenIndex);
    BuildFocusGraphForActiveScreen();

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
    case ECharacterCreatorScreen::ProjectBrowser:
    case ECharacterCreatorScreen::PhysicsSetup:
    case ECharacterCreatorScreen::GameplayTest:
    case ECharacterCreatorScreen::PreviewStudio:
    case ECharacterCreatorScreen::PortraitStudio:
    case ECharacterCreatorScreen::LODPerformance:
    case ECharacterCreatorScreen::AssetBrowser:
    case ECharacterCreatorScreen::ImportWizard:
    case ECharacterCreatorScreen::Settings:
    case ECharacterCreatorScreen::ValidationExport:
    {
        const int32 UtilityIndex = NewScreen == ECharacterCreatorScreen::ProjectBrowser
            ? 0
            : static_cast<int32>(NewScreen) - static_cast<int32>(ECharacterCreatorScreen::PhysicsSetup) + 1;
        if (UtilityScreens.IsValidIndex(UtilityIndex) && UtilityScreens[UtilityIndex])
        {
            UCharacterCreatorUtilityWorkspaceWidget* FocusTarget = UtilityScreens[UtilityIndex];
            FocusTarget->FocusFirstControl();
            LastFocusWidget = FocusTarget;
        }
        break;
    }
    default:
        break;
    }

    if (LastFocusWidget.IsValid())
    {
        MoveFocusByDelta(0);
    }
}

FReply UCharacterCreatorRootWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    if (Session && Session->GetSettings().bGamepadEnabled && Session->GetSettings().bDPadNavigation)
    {
        if (Key == EKeys::Gamepad_DPad_Left)
        {
            return MoveFocusByDirection(FName(TEXT("left"))) ? FReply::Handled() : FReply::Unhandled();
        }
        if (Key == EKeys::Gamepad_DPad_Right)
        {
            return MoveFocusByDirection(FName(TEXT("right"))) ? FReply::Handled() : FReply::Unhandled();
        }
        if (Key == EKeys::Gamepad_DPad_Up)
        {
            return MoveFocusByDirection(FName(TEXT("up"))) ? FReply::Handled() : FReply::Unhandled();
        }
        if (Key == EKeys::Gamepad_DPad_Down)
        {
            return MoveFocusByDirection(FName(TEXT("down"))) ? FReply::Handled() : FReply::Unhandled();
        }
    }

    if (Key == EKeys::Escape)
    {
        if (ModalManager && ModalManager->CloseTopModal())
        {
            BuildFocusGraphForActiveScreen();
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
        if (ModalManager && ModalManager->HasOpenModal())
        {
            return FReply::Handled();
        }

        const TArray<ECharacterCreatorScreen> ScreenOrder = {
            ECharacterCreatorScreen::Dashboard,
            ECharacterCreatorScreen::ProjectBrowser,
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
            ECharacterCreatorScreen::Settings,
            ECharacterCreatorScreen::ValidationExport
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

    if (Session && Key == EKeys::Gamepad_FaceButton_Bottom && (!ModalManager || !ModalManager->HasOpenModal()))
    {
        Session->ApplyAppearanceChanges();
        return FReply::Handled();
    }

    if (Session && Key == EKeys::Gamepad_FaceButton_Right)
    {
        if (ModalManager && ModalManager->HasOpenModal())
        {
            if (ModalManager->CloseTopModal())
            {
                BuildFocusGraphForActiveScreen();
            }
            return FReply::Handled();
        }
        Session->RevertAppearanceChanges();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UCharacterCreatorRootWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
    {
        if (ModalManager && ModalManager->CloseTopModal())
        {
            BuildFocusGraphForActiveScreen();
            return FReply::Handled();
        }
    }
    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UCharacterCreatorRootWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
    if (Session && Session->GetSettings().bGamepadEnabled)
    {
        if (ModalManager && ModalManager->HasOpenModal())
        {
            if (Session->GetSettings().bAnalogNavigation && FMath::Abs(InAnalogEvent.GetAnalogValue()) > 0.75f &&
                (InAnalogEvent.GetKey() == EKeys::Gamepad_LeftX || InAnalogEvent.GetKey() == EKeys::Gamepad_LeftY))
            {
                const bool bHorizontal = InAnalogEvent.GetKey() == EKeys::Gamepad_LeftX;
                const bool bPositive = InAnalogEvent.GetAnalogValue() > 0.0f;
                const FName Direction = bHorizontal
                    ? (bPositive ? FName(TEXT("right")) : FName(TEXT("left")))
                    : (bPositive ? FName(TEXT("up")) : FName(TEXT("down")));
                return MoveFocusByDirection(Direction) ? FReply::Handled() : FReply::Unhandled();
            }
            return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
        }

        HandleGamepadCameraInput(InAnalogEvent);
        if (InAnalogEvent.GetKey() == EKeys::Gamepad_LeftX || InAnalogEvent.GetKey() == EKeys::Gamepad_LeftY)
        {
            const float Value = InAnalogEvent.GetAnalogValue();
            if (Session->GetSettings().bAnalogNavigation && FMath::Abs(Value) > 0.75f)
            {
                const bool bHorizontal = InAnalogEvent.GetKey() == EKeys::Gamepad_LeftX;
                const bool bPositive = Value > 0.0f;
                const FName Direction = bHorizontal
                    ? (bPositive ? FName(TEXT("right")) : FName(TEXT("left")))
                    : (bPositive ? FName(TEXT("up")) : FName(TEXT("down")));
                return MoveFocusByDirection(Direction) ? FReply::Handled() : FReply::Unhandled();
            }
        }
    }
    return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

void UCharacterCreatorRootWidget::ApplyStatus(const FText& NewStatus)
{
    if (DashboardStatusText && !NewStatus.IsEmpty())
    {
        DashboardStatusText->SetText(NewStatus);
        if (Session && Session->GetSettings().bHighContrast)
        {
            DashboardStatusText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        }
    }

    if (CharacterStatusText && !NewStatus.IsEmpty())
    {
        CharacterStatusText->SetText(NewStatus);
        if (Session && Session->GetSettings().bHighContrast)
        {
            CharacterStatusText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        }
    }
}

void UCharacterCreatorRootWidget::ApplySettings(const FCharacterCreatorSettings& NewSettings)
{
    if (PreviewActor)
    {
        PreviewActor->ApplyPerformanceSettings(NewSettings);
    }

    if (RootScaleBox)
    {
        RootScaleBox->SetRenderScale(FVector2D(NewSettings.UIScale));
    }

    if (WidgetTree)
    {
        TArray<UWidget*> AllWidgets;
        WidgetTree->GetAllWidgets(AllWidgets);
        const FLinearColor AccessibleTextColor = NewSettings.bHighContrast ? FLinearColor::White : CharacterCreatorUI::Text;
        for (UWidget* Widget : AllWidgets)
        {
            if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
            {
                TextBlock->SetRenderScale(FVector2D(NewSettings.TextScale));
                if (NewSettings.bHighContrast)
                {
                    TextBlock->SetColorAndOpacity(FSlateColor(AccessibleTextColor));
                }
            }
        }
    }
}

void UCharacterCreatorRootWidget::BuildFocusGraphForActiveScreen()
{
    ActiveFocusWidgets.Reset();
    if (!WidgetTree)
    {
        return;
    }

    TArray<UWidget*> AllWidgets;
    WidgetTree->GetAllWidgets(AllWidgets);
    for (UWidget* Widget : AllWidgets)
    {
        const UButton* Button = Cast<UButton>(Widget);
        const USlider* Slider = Cast<USlider>(Widget);
        const bool bFocusable = (Button && Button->GetIsFocusable()) || (Slider && Slider->IsFocusable);
        if (Widget && bFocusable && Widget->GetVisibility() == ESlateVisibility::Visible)
        {
            ActiveFocusWidgets.Add(Widget);
        }
    }

    TArray<UWidget*> FocusGraphWidgets;
    for (UWidget* Widget : ActiveFocusWidgets)
    {
        FocusGraphWidgets.Add(Widget);
    }
    FCharacterCreatorUIFactory::ConfigureFocusGraph(FocusGraphWidgets);

    if (Session)
    {
        TArray<FVector2D> Centers;
        Centers.Reserve(ActiveFocusWidgets.Num());
        for (UWidget* Widget : ActiveFocusWidgets)
        {
            FVector2D Center = Widget->GetCachedGeometry().GetAbsolutePosition() + (Widget->GetCachedGeometry().GetLocalSize() * 0.5f);
            if (Widget->GetCachedGeometry().GetLocalSize().IsNearlyZero())
            {
                if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
                {
                    Center = CanvasSlot->GetPosition() + (CanvasSlot->GetSize() * 0.5f);
                }
            }
            Centers.Add(Center);
        }

        auto FindNearest = [&Centers](int32 FromIndex, bool bHorizontal, float Sign) -> int32
        {
            int32 BestIndex = INDEX_NONE;
            float BestScore = TNumericLimits<float>::Max();
            for (int32 CandidateIndex = 0; CandidateIndex < Centers.Num(); ++CandidateIndex)
            {
                if (CandidateIndex == FromIndex)
                {
                    continue;
                }
                const FVector2D Delta = Centers[CandidateIndex] - Centers[FromIndex];
                const float Primary = (bHorizontal ? Delta.X : Delta.Y) * Sign;
                if (Primary <= 1.0f)
                {
                    continue;
                }
                const float Cross = FMath::Abs(bHorizontal ? Delta.Y : Delta.X);
                const float Score = Primary + (Cross * 0.35f);
                if (Score < BestScore)
                {
                    BestScore = Score;
                    BestIndex = CandidateIndex;
                }
            }
            return BestIndex;
        };

        TArray<FCharacterCreatorFocusGraphNode> Graph;
        for (int32 Index = 0; Index < ActiveFocusWidgets.Num(); ++Index)
        {
            FCharacterCreatorFocusGraphNode& Node = Graph.AddDefaulted_GetRef();
            Node.Id = ActiveFocusWidgets[Index]->GetFName();
            const int32 LeftIndex = FindNearest(Index, true, -1.0f);
            const int32 RightIndex = FindNearest(Index, true, 1.0f);
            const int32 UpIndex = FindNearest(Index, false, -1.0f);
            const int32 DownIndex = FindNearest(Index, false, 1.0f);
            Node.Left = LeftIndex != INDEX_NONE ? ActiveFocusWidgets[LeftIndex]->GetFName() : (Index > 0 ? ActiveFocusWidgets[Index - 1]->GetFName() : NAME_None);
            Node.Right = RightIndex != INDEX_NONE ? ActiveFocusWidgets[RightIndex]->GetFName() : (Index + 1 < ActiveFocusWidgets.Num() ? ActiveFocusWidgets[Index + 1]->GetFName() : NAME_None);
            Node.Up = UpIndex != INDEX_NONE ? ActiveFocusWidgets[UpIndex]->GetFName() : Node.Left;
            Node.Down = DownIndex != INDEX_NONE ? ActiveFocusWidgets[DownIndex]->GetFName() : Node.Right;

            if (LeftIndex != INDEX_NONE) ActiveFocusWidgets[Index]->SetNavigationRuleExplicit(EUINavigation::Left, ActiveFocusWidgets[LeftIndex]);
            if (RightIndex != INDEX_NONE) ActiveFocusWidgets[Index]->SetNavigationRuleExplicit(EUINavigation::Right, ActiveFocusWidgets[RightIndex]);
            if (UpIndex != INDEX_NONE) ActiveFocusWidgets[Index]->SetNavigationRuleExplicit(EUINavigation::Up, ActiveFocusWidgets[UpIndex]);
            if (DownIndex != INDEX_NONE) ActiveFocusWidgets[Index]->SetNavigationRuleExplicit(EUINavigation::Down, ActiveFocusWidgets[DownIndex]);
        }
        Session->SetFocusGraph(Graph);
    }
}

bool UCharacterCreatorRootWidget::MoveFocusByDelta(int32 Delta)
{
    if (ActiveFocusWidgets.Num() == 0)
    {
        BuildFocusGraphForActiveScreen();
    }
    if (ActiveFocusWidgets.Num() == 0)
    {
        return false;
    }

    int32 CurrentIndex = LastFocusWidget.IsValid() ? ActiveFocusWidgets.IndexOfByKey(LastFocusWidget.Get()) : INDEX_NONE;
    if (CurrentIndex == INDEX_NONE)
    {
        CurrentIndex = 0;
    }
    CurrentIndex = (CurrentIndex + Delta + ActiveFocusWidgets.Num()) % ActiveFocusWidgets.Num();
    for (UWidget* Widget : ActiveFocusWidgets)
    {
        if (UCharacterCreatorButtonWidget* Button = Cast<UCharacterCreatorButtonWidget>(Widget))
        {
            Button->SetFocusVisual(false);
        }
    }
    LastFocusWidget = ActiveFocusWidgets[CurrentIndex];
    if (UCharacterCreatorButtonWidget* Button = Cast<UCharacterCreatorButtonWidget>(ActiveFocusWidgets[CurrentIndex]))
    {
        Button->SetFocusVisual(true);
    }
    UCharacterCreatorUIHelpers::FocusWidget(ActiveFocusWidgets[CurrentIndex]);
    return true;
}

bool UCharacterCreatorRootWidget::MoveFocusByDirection(FName Direction)
{
    if (ActiveFocusWidgets.Num() == 0)
    {
        BuildFocusGraphForActiveScreen();
    }
    if (ActiveFocusWidgets.Num() == 0)
    {
        return false;
    }

    int32 CurrentIndex = LastFocusWidget.IsValid() ? ActiveFocusWidgets.IndexOfByKey(LastFocusWidget.Get()) : INDEX_NONE;
    if (CurrentIndex == INDEX_NONE)
    {
        CurrentIndex = 0;
    }

    int32 TargetIndex = INDEX_NONE;
    if (Session)
    {
        for (const FCharacterCreatorFocusGraphNode& Node : Session->GetFocusGraph())
        {
            if (Node.Id != ActiveFocusWidgets[CurrentIndex]->GetFName())
            {
                continue;
            }
            const FName TargetId = Direction == FName(TEXT("left")) ? Node.Left
                : Direction == FName(TEXT("right")) ? Node.Right
                : Direction == FName(TEXT("up")) ? Node.Up
                : Node.Down;
            for (int32 Index = 0; Index < ActiveFocusWidgets.Num(); ++Index)
            {
                if (ActiveFocusWidgets[Index]->GetFName() == TargetId)
                {
                    TargetIndex = Index;
                    break;
                }
            }
            break;
        }
    }

    if (TargetIndex == INDEX_NONE)
    {
        TargetIndex = (CurrentIndex + ((Direction == FName(TEXT("left")) || Direction == FName(TEXT("up"))) ? -1 : 1) + ActiveFocusWidgets.Num()) % ActiveFocusWidgets.Num();
    }

    for (UWidget* Widget : ActiveFocusWidgets)
    {
        if (UCharacterCreatorButtonWidget* Button = Cast<UCharacterCreatorButtonWidget>(Widget))
        {
            Button->SetFocusVisual(false);
        }
    }
    LastFocusWidget = ActiveFocusWidgets[TargetIndex];
    if (UCharacterCreatorButtonWidget* Button = Cast<UCharacterCreatorButtonWidget>(ActiveFocusWidgets[TargetIndex]))
    {
        Button->SetFocusVisual(true);
    }
    UCharacterCreatorUIHelpers::FocusWidget(ActiveFocusWidgets[TargetIndex]);
    return true;
}

void UCharacterCreatorRootWidget::HandleGamepadCameraInput(const FAnalogInputEvent& AnalogEvent)
{
    if (!Session || !PreviewActor || !Session->GetSettings().bAnalogNavigation || Session->GetSettings().bReducedMotion)
    {
        return;
    }

    const FKey Key = AnalogEvent.GetKey();
    if (Key != EKeys::Gamepad_RightX && Key != EKeys::Gamepad_RightY)
    {
        return;
    }

    const ECharacterCreatorScreen Screen = Session->GetScreen();
    if (Screen != ECharacterCreatorScreen::CharacterCreator && Screen != ECharacterCreatorScreen::PreviewStudio && Screen != ECharacterCreatorScreen::PortraitStudio)
    {
        return;
    }

    FCharacterCreatorPreviewStudioState StudioState = Session->GetPreviewStudioState();
    const float Sensitivity = Session->GetSettings().CameraSensitivity * 3.0f;
    const float AnalogValue = AnalogEvent.GetAnalogValue();
    if (Key == EKeys::Gamepad_RightX)
    {
        StudioState.OrbitYaw += AnalogValue * Sensitivity;
    }
    else
    {
        const float Direction = Session->GetSettings().bInvertCameraY ? 1.0f : -1.0f;
        StudioState.OrbitPitch += AnalogValue * Sensitivity * Direction;
    }
    StudioState.bOrbitEnabled = true;
    Session->SetPreviewStudioState(StudioState);
    PreviewActor->SetCameraOrbit(StudioState.OrbitYaw, StudioState.OrbitPitch, StudioState.Zoom);
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
        DashboardPreviewStatusText->SetColorAndOpacity(FSlateColor(Session && Session->GetSettings().bHighContrast ? FLinearColor::White : StateColor));
    }

    if (CharacterPreviewStatusText)
    {
        CharacterPreviewStatusText->SetText(Message);
        CharacterPreviewStatusText->SetColorAndOpacity(FSlateColor(Session && Session->GetSettings().bHighContrast ? FLinearColor::White : StateColor));
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

    const int32 ModalSerial = ++ModalInstanceSerial;
    UCharacterCreatorModalWidget* Modal = WidgetTree->ConstructWidget<UCharacterCreatorModalWidget>();
    if (!Modal)
    {
        return;
    }

    Modal->Rename(*FString::Printf(TEXT("%s_%d"), *DialogId.ToString(), ModalSerial));
    Modal->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f));
    UWidget* PreviousFocus = LastFocusWidget.Get();

    UCanvasPanel* ModalCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    Modal->AddChild(ModalCanvas);

    const bool bColorPicker = DialogId == FName(TEXT("color_picker"));
    const bool bTextPrompt = DialogId == FName(TEXT("project_new"))
        || DialogId == FName(TEXT("project_save_as"))
        || DialogId == FName(TEXT("project_rename"))
        || DialogId == FName(TEXT("project_duplicate"))
        || DialogId == FName(TEXT("import_source"))
        || DialogId == FName(TEXT("import_destination"))
        || DialogId == FName(TEXT("export_destination"));
    const int32 ActionCount = FMath::Max(1, Actions.Num());
    const float DialogHeight = bColorPicker
        ? FMath::Clamp(380.0f + (static_cast<float>(ActionCount) * 48.0f), 500.0f, 700.0f)
        : FMath::Clamp((bTextPrompt ? 290.0f : 230.0f) + (static_cast<float>(ActionCount) * 48.0f), 280.0f, 530.0f);
    const FVector2D DialogSize(560.0f, DialogHeight);
    const FVector2D DialogPosition = UCharacterCreatorUIHelpers::ClampPopupPosition(
        FVector2D((1440.0f - DialogSize.X) * 0.5f, (810.0f - DialogSize.Y) * 0.5f),
        DialogSize,
        FVector2D(1440.0f, 810.0f));

    UCharacterCreatorPanelWidget* DialogPanel = FCharacterCreatorUIFactory::MakePanel(WidgetTree, CharacterCreatorUI::SurfaceRaised);
    FCharacterCreatorUIFactory::Place(ModalCanvas, DialogPanel, DialogPosition, DialogSize);

    UCanvasPanel* DialogContent = WidgetTree->ConstructWidget<UCanvasPanel>();
    DialogPanel->AddChild(DialogContent);

    FCharacterCreatorUIFactory::AddLabel(WidgetTree, DialogContent, FString::Printf(TEXT("DialogTitle_%d"), ModalSerial), Title.ToUpper(), FVector2D(28.0f, 24.0f), FVector2D(500.0f, 28.0f), 16, CharacterCreatorUI::Gold);
    UTextBlock* DialogMessageText = FCharacterCreatorUIFactory::MakeLabel(WidgetTree, Message, 11, CharacterCreatorUI::Text);
    DialogMessageText->SetAutoWrapText(true);
    FCharacterCreatorUIFactory::Place(DialogContent, DialogMessageText, FVector2D(28.0f, 68.0f), FVector2D(500.0f, 88.0f));

    TArray<TPair<FName, FString>> DialogActions = Actions;
    if (DialogActions.Num() == 0)
    {
        DialogActions.Add(TPair<FName, FString>(FName(TEXT("dialog_close")), TEXT("CLOSE")));
    }

    TArray<UWidget*> ModalFocusWidgets;
    UWidget* FirstAction = nullptr;
    ModalTextInput = nullptr;

    if (bTextPrompt)
    {
        FString InitialValue;
        if (Session)
        {
            InitialValue = Session->GetSettings().ProjectName;
            if (DialogId == FName(TEXT("project_duplicate"))) InitialValue += TEXT(" Copy");
            if (DialogId == FName(TEXT("project_new"))) InitialValue = TEXT("New Character Project");
            if (DialogId == FName(TEXT("import_source"))) InitialValue = Session->GetSettings().ImportSourceDirectory;
            if (DialogId == FName(TEXT("import_destination"))) InitialValue = Session->GetSettings().ImportDestinationDirectory;
            if (DialogId == FName(TEXT("export_destination"))) InitialValue = Session->GetSettings().ExportDestinationDirectory;
        }
        ModalTextInput = WidgetTree->ConstructWidget<UEditableTextBox>();
        ModalTextInput->SetText(FText::FromString(InitialValue));
        ModalTextInput->SetHintText(FText::FromString(TEXT("Project name")));
        ModalTextInput->SetIsReadOnly(false);
        FCharacterCreatorUIFactory::Place(DialogContent, ModalTextInput, FVector2D(28.0f, 164.0f), FVector2D(500.0f, 38.0f));
        ModalFocusWidgets.Add(ModalTextInput);
        FirstAction = ModalTextInput;
    }

    if (bColorPicker)
    {
        PendingColorPickerTarget = ECharacterCreatorColorTarget::Skin;
        PendingColorPickerColor = Session ? Session->GetColorTarget(PendingColorPickerTarget) : FLinearColor::White;
        ColorPickerSliders.Reset();

        const TArray<TTuple<const TCHAR*, ECharacterCreatorParameter, float>> Channels = {
            {TEXT("RED"), ECharacterCreatorParameter::Height, PendingColorPickerColor.R},
            {TEXT("GREEN"), ECharacterCreatorParameter::ShoulderWidth, PendingColorPickerColor.G},
            {TEXT("BLUE"), ECharacterCreatorParameter::ArmLength, PendingColorPickerColor.B}
        };
        for (int32 ChannelIndex = 0; ChannelIndex < Channels.Num(); ++ChannelIndex)
        {
            const float ChannelY = 164.0f + (static_cast<float>(ChannelIndex) * 48.0f);
            FCharacterCreatorUIFactory::AddLabel(
                WidgetTree,
                DialogContent,
                FString::Printf(TEXT("ColorPickerChannel_%d_%d"), ModalSerial, ChannelIndex),
                Channels[ChannelIndex].Get<0>(),
                FVector2D(28.0f, ChannelY),
                FVector2D(72.0f, 28.0f),
                10,
                CharacterCreatorUI::Muted);
            UCharacterCreatorSliderWidget* ChannelSlider = FCharacterCreatorUIFactory::MakeSlider(
                WidgetTree,
                Channels[ChannelIndex].Get<1>(),
                Channels[ChannelIndex].Get<2>());
            ChannelSlider->OnParameterValueChanged.AddUObject(this, &UCharacterCreatorRootWidget::HandleColorPickerChannelChanged);
            FCharacterCreatorUIFactory::Place(DialogContent, ChannelSlider, FVector2D(112.0f, ChannelY + 4.0f), FVector2D(384.0f, 24.0f));
            ColorPickerSliders.Add(ChannelSlider);
            ModalFocusWidgets.Add(ChannelSlider);
        }
        if (ColorPickerSliders.Num() > 0)
        {
            FirstAction = ColorPickerSliders[0];
        }
    }

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

        const float ActionY = (bColorPicker ? 320.0f : bTextPrompt ? 224.0f : 174.0f) + (static_cast<float>(ActionIndex) * 44.0f);
        FCharacterCreatorUIFactory::Place(DialogContent, ActionButton, FVector2D(28.0f, ActionY), FVector2D(500.0f, 34.0f));
        if (!FirstAction)
        {
            FirstAction = ActionButton;
        }
        ModalFocusWidgets.Add(ActionButton);
    }

    FCharacterCreatorUIFactory::ConfigureFocusGraph(ModalFocusWidgets);
    ActiveFocusWidgets.Reset();
    for (UWidget* Widget : ModalFocusWidgets)
    {
        ActiveFocusWidgets.Add(Widget);
    }
    LastFocusWidget = FirstAction;

    FCharacterCreatorUIFactory::Place(ModalOverlay, Modal, FVector2D::ZeroVector, FVector2D(1440.0f, 810.0f));
    ModalDialogs.Add(Modal);

    UWidget* ReturnFocus = PreviousFocus;
    if (!ReturnFocus)
    {
        ReturnFocus = this;
    }

    if (ModalManager->OpenModal(Modal, ReturnFocus) && FirstAction)
    {
        UCharacterCreatorUIHelpers::FocusWidget(FirstAction);
        if (UCharacterCreatorButtonWidget* FocusButton = Cast<UCharacterCreatorButtonWidget>(FirstAction))
        {
            FocusButton->SetFocusVisual(true);
        }
    }
}

void UCharacterCreatorRootWidget::CloseTopModalAndRestoreFocus()
{
    if (ModalManager && ModalManager->CloseTopModal())
    {
        ModalTextInput = nullptr;
        BuildFocusGraphForActiveScreen();
    }
}

void UCharacterCreatorRootWidget::HandleModalCommand(FName CommandId)
{
    if (CommandId == FName(TEXT("dialog_close"))
        || CommandId == FName(TEXT("dialog_cancel"))
        || CommandId == FName(TEXT("dialog_new_cancel")))
    {
        CloseTopModalAndRestoreFocus();
        return;
    }

    if (CommandId == FName(TEXT("dialog_project_new_confirm"))
        || CommandId == FName(TEXT("dialog_project_save_as_confirm"))
        || CommandId == FName(TEXT("dialog_project_rename_confirm"))
        || CommandId == FName(TEXT("dialog_project_duplicate_confirm")))
    {
        const FString Value = ModalTextInput ? ModalTextInput->GetText().ToString().TrimStartAndEnd() : FString();
        bool bSucceeded = false;
        bool bNeedsUnsavedConfirmation = false;
        if (Session)
        {
            if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
            {
                const FString SelectedSlot = Session->GetProjectBrowserState().SelectedSlotName;
                if (CommandId == FName(TEXT("dialog_project_new_confirm"))) bSucceeded = Subsystem->CreateProject(Value);
                if (CommandId == FName(TEXT("dialog_project_save_as_confirm"))) bSucceeded = Subsystem->SaveCurrentProjectAs(Value);
                if (CommandId == FName(TEXT("dialog_project_rename_confirm"))) bSucceeded = Subsystem->RenameProject(SelectedSlot, Value);
                if (CommandId == FName(TEXT("dialog_project_duplicate_confirm"))) bSucceeded = Subsystem->DuplicateProject(SelectedSlot, Value);
                bNeedsUnsavedConfirmation = Session->GetProjectBrowserState().bUnsavedConfirmationRequired;
            }
        }
        CloseTopModalAndRestoreFocus();
        if (bNeedsUnsavedConfirmation)
        {
            HandleWorkflowModalRequested(FName(TEXT("unsaved_project")));
        }
        else if (bSucceeded && Session)
        {
            Session->SetScreen(ECharacterCreatorScreen::ProjectBrowser);
        }
        return;
    }

    if (CommandId == FName(TEXT("dialog_import_source_confirm")) || CommandId == FName(TEXT("dialog_import_destination_confirm")) || CommandId == FName(TEXT("dialog_export_destination_confirm")))
    {
        if (Session && ModalTextInput)
        {
            FString Directory = ModalTextInput->GetText().ToString().TrimStartAndEnd();
            if (!Directory.IsEmpty()) Directory = FPaths::ConvertRelativePathToFull(Directory);
            if (Directory.IsEmpty())
            {
                Session->SetStatusMessage(FText::FromString(TEXT("A destination folder cannot be empty.")));
            }
            else
            {
                FCharacterCreatorSettings Settings = Session->GetSettings();
                if (CommandId == FName(TEXT("dialog_import_source_confirm"))) Settings.ImportSourceDirectory = Directory;
                if (CommandId == FName(TEXT("dialog_import_destination_confirm"))) Settings.ImportDestinationDirectory = Directory;
                if (CommandId == FName(TEXT("dialog_export_destination_confirm"))) Settings.ExportDestinationDirectory = Directory;
                Session->SetSettings(Settings);
                Session->SetStatusMessage(FText::FromString(FString::Printf(TEXT("Folder set to %s"), *Directory)));
            }
        }
        CloseTopModalAndRestoreFocus();
        return;
    }

    if (CommandId == FName(TEXT("dialog_project_delete_confirm")))
    {
        if (Session)
        {
            if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
            {
                Subsystem->DeleteProject(Session->GetProjectBrowserState().SelectedSlotName);
            }
        }
        CloseTopModalAndRestoreFocus();
        return;
    }

    if (CommandId == FName(TEXT("dialog_project_recover")) || CommandId == FName(TEXT("dialog_project_recovery_dismiss")))
    {
        if (Session)
        {
            if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
            {
                if (CommandId == FName(TEXT("dialog_project_recover"))) Subsystem->RecoverAutosave();
                else Subsystem->DismissAutosaveRecovery();
            }
        }
        CloseTopModalAndRestoreFocus();
        return;
    }

    if (CommandId == FName(TEXT("dialog_unsaved_save")) || CommandId == FName(TEXT("dialog_unsaved_discard")) || CommandId == FName(TEXT("dialog_unsaved_cancel")))
    {
        if (Session)
        {
            if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
            {
                const ECharacterCreatorUnsavedDecision Decision = CommandId == FName(TEXT("dialog_unsaved_save"))
                    ? ECharacterCreatorUnsavedDecision::Save
                    : CommandId == FName(TEXT("dialog_unsaved_discard")) ? ECharacterCreatorUnsavedDecision::Discard : ECharacterCreatorUnsavedDecision::Cancel;
                Subsystem->ResolvePendingProjectChange(Decision);
            }
        }
        CloseTopModalAndRestoreFocus();
        return;
    }

    if (CommandId == FName(TEXT("dialog_new_confirm")))
    {
        if (Session)
        {
            Session->ResetAppearance();
            Session->SetScreen(ECharacterCreatorScreen::CharacterCreator);
        }
        CloseTopModalAndRestoreFocus();
        return;
    }

    if (CommandId == FName(TEXT("dialog_color_apply_skin"))
        || CommandId == FName(TEXT("dialog_color_apply_hair"))
        || CommandId == FName(TEXT("dialog_color_apply_primary"))
        || CommandId == FName(TEXT("dialog_color_apply_secondary")))
    {
        if (Session)
        {
            const ECharacterCreatorColorTarget Target = CommandId == FName(TEXT("dialog_color_apply_skin"))
                ? ECharacterCreatorColorTarget::Skin
                : CommandId == FName(TEXT("dialog_color_apply_hair"))
                    ? ECharacterCreatorColorTarget::Hair
                    : CommandId == FName(TEXT("dialog_color_apply_primary"))
                        ? ECharacterCreatorColorTarget::PrimaryOutfit
                        : ECharacterCreatorColorTarget::SecondaryOutfit;
            Session->SetColorTarget(Target, PendingColorPickerColor);
            Session->SetStatusMessage(FText::FromString(TEXT("Custom RGB color applied; press APPLY to commit it.")));
        }
        CloseTopModalAndRestoreFocus();
        return;
    }

    if (CommandId == FName(TEXT("dialog_save_template")))
    {
        if (Session)
        {
            Session->CreatePresetFromCurrent(FText::FromString(TEXT("Saved Template")), FText::FromString(TEXT("Created from the active character workspace.")));
            Session->SetStatusMessage(FText::FromString(TEXT("Saved the active character as a template")));
        }
        CloseTopModalAndRestoreFocus();
        return;
    }

    if (CommandId == FName(TEXT("dialog_import_validate")))
    {
        if (Session)
        {
            if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
            {
                FCharacterCreatorImportProgress Progress;
                Subsystem->ScanAssetDirectory(Session->GetSettings().ImportSourceDirectory, FString(), FString(), Progress);
            }
        }
        CloseTopModalAndRestoreFocus();
        return;
    }

    if (CommandId == FName(TEXT("dialog_import_open")))
    {
        CloseTopModalAndRestoreFocus();
        if (Session)
        {
            Session->SetScreen(ECharacterCreatorScreen::ImportWizard);
        }
        return;
    }

    if (CommandId == FName(TEXT("dialog_import_dependencies")))
    {
        CloseTopModalAndRestoreFocus();
        OpenModalDialog(FName(TEXT("dependency_warning")), TEXT("DEPENDENCY REVIEW"), TEXT("Some imported assets may reference external materials, skeletons, or animation dependencies. Validate the content folder and copy dependencies before applying the import."), { TPair<FName, FString>(FName(TEXT("dialog_import_open")), TEXT("OPEN IMPORT WIZARD")), TPair<FName, FString>(FName(TEXT("dialog_close")), TEXT("CANCEL")) });
        return;
    }

    if (CommandId == FName(TEXT("dialog_import_conflicts")))
    {
        CloseTopModalAndRestoreFocus();
        OpenModalDialog(FName(TEXT("conflict_resolution")), TEXT("CONFLICT POLICY"), TEXT("Choose what to do when an imported file already exists in the destination Content folder."), { TPair<FName, FString>(FName(TEXT("dialog_conflict_keep_both")), TEXT("KEEP BOTH")), TPair<FName, FString>(FName(TEXT("dialog_conflict_replace")), TEXT("OVERWRITE")), TPair<FName, FString>(FName(TEXT("dialog_close")), TEXT("CANCEL")) });
        return;
    }

    if (CommandId == FName(TEXT("dialog_conflict_keep_both")) || CommandId == FName(TEXT("dialog_conflict_replace")))
    {
        if (Session) Session->SetStatusMessage(CommandId == FName(TEXT("dialog_conflict_keep_both")) ? FText::FromString(TEXT("Import conflict policy: keep both")) : FText::FromString(TEXT("Import conflict policy: overwrite existing")));
        CloseTopModalAndRestoreFocus();
        return;
    }

    if (CommandId == FName(TEXT("dialog_export_full")) || CommandId == FName(TEXT("dialog_export_metadata")))
    {
        bool bQueued = false;
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
                    bQueued = Subsystem->StartExportCurrentDeliverables(Profile, FString());
                }
                else
                {
                    Profile.bGenerateBlueprint = true;
                    Profile.bGenerateDataAsset = true;
                    Profile.bGeneratePackage = true;
                    bQueued = Subsystem->StartExportCurrentDeliverables(Profile, FString());
                }
            }
        }
        CloseTopModalAndRestoreFocus();
        OpenModalDialog(bQueued ? FName(TEXT("export_queued")) : FName(TEXT("export_error")), bQueued ? TEXT("EXPORT QUEUED") : TEXT("EXPORT BLOCKED"), bQueued ? TEXT("Export is queued. Open Validation + Export to monitor its status, cancel before generation begins, review partial failures, or open the selected output folder.") : TEXT("Export validation found a blocking issue. Open Validation + Export to review the issue list and apply safe fixes."), { TPair<FName, FString>(FName(TEXT("dialog_close")), bQueued ? TEXT("CONTINUE") : TEXT("REVIEW LATER")) });
        return;
    }

    if (CommandId == FName(TEXT("dialog_preset_randomize")))
    {
        if (Session)
        {
            Session->RandomizeAppearance(false);
            Session->SetScreen(ECharacterCreatorScreen::CharacterCreator);
        }
        CloseTopModalAndRestoreFocus();
        return;
    }

    if (CommandId == FName(TEXT("dialog_preset_default")))
    {
        if (Session)
        {
            Session->RestoreDefaultPreset();
            Session->SetScreen(ECharacterCreatorScreen::CharacterCreator);
        }
        CloseTopModalAndRestoreFocus();
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
        CloseTopModalAndRestoreFocus();
        return;
    }
}

void UCharacterCreatorRootWidget::HandleNewCharacterClicked()
{
    HandleWorkflowModalRequested(FName(TEXT("project_new")));
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
            Subsystem->SaveCurrentProject();
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
    else if (CommandId == FName(TEXT("global_browser")))
    {
        Session->SetScreen(ECharacterCreatorScreen::ProjectBrowser);
    }
    else if (CommandId == FName(TEXT("global_save")))
    {
        HandleSaveCharacterClicked();
    }
    else if (CommandId == FName(TEXT("global_revert")))
    {
        HandleRevertCharacterClicked();
    }
    else if (CommandId == FName(TEXT("global_settings")))
    {
        Session->SetScreen(ECharacterCreatorScreen::Settings);
    }
    else if (CommandId == FName(TEXT("global_gamepad")))
    {
        OpenModalDialog(FName(TEXT("gamepad_overlay")), TEXT("GAMEPAD CONTROLS"), TEXT("D-PAD or left stick navigates. A confirms. B cancels or reverts. LB / RB switches workspaces. Right stick orbits the preview camera."), { TPair<FName, FString>(FName(TEXT("dialog_close")), TEXT("CLOSE")) });
    }
    else if (CommandId == FName(TEXT("tray_assets")))
    {
        Session->SetScreen(ECharacterCreatorScreen::AssetBrowser);
    }
    else if (CommandId == FName(TEXT("tray_import")))
    {
        Session->SetScreen(ECharacterCreatorScreen::ImportWizard);
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
    else if (CommandId == FName(TEXT("randomize")))
    {
        Session->RandomizeAppearance(false);
        Session->SetScreen(ECharacterCreatorScreen::CharacterCreator);
    }
    else if (CommandId == FName(TEXT("preset_manager")))
    {
        OpenModalDialog(
            FName(TEXT("preset_manager")),
            TEXT("PRESET MANAGER"),
            FString::Printf(TEXT("%d presets available. Compare, merge, duplicate, rename, or randomize from the active session."), Session->GetPresets().Num()),
            {
                TPair<FName, FString>(FName(TEXT("dialog_preset_randomize")), TEXT("RANDOMIZE ACTIVE")),
                TPair<FName, FString>(FName(TEXT("dialog_preset_default")), TEXT("RESTORE DEFAULT")),
                TPair<FName, FString>(FName(TEXT("dialog_cancel")), TEXT("CLOSE"))
            });
    }
    else if (CommandId == FName(TEXT("export")))
    {
        Session->SetScreen(ECharacterCreatorScreen::ValidationExport);
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

void UCharacterCreatorRootWidget::HandleWorkflowModalRequested(FName DialogId)
{
    if (DialogId == FName(TEXT("color_picker")))
    {
        OpenModalDialog(
            FName(TEXT("color_picker")),
            TEXT("COLOR PICKER"),
            TEXT("Tune an RGB color, then choose which material target receives it. The change remains an unapplied session edit until Apply is pressed."),
            {
                TPair<FName, FString>(FName(TEXT("dialog_color_apply_skin")), TEXT("APPLY TO SKIN")),
                TPair<FName, FString>(FName(TEXT("dialog_color_apply_hair")), TEXT("APPLY TO HAIR")),
                TPair<FName, FString>(FName(TEXT("dialog_color_apply_primary")), TEXT("APPLY TO PRIMARY OUTFIT")),
                TPair<FName, FString>(FName(TEXT("dialog_color_apply_secondary")), TEXT("APPLY TO SECONDARY OUTFIT")),
                TPair<FName, FString>(FName(TEXT("dialog_cancel")), TEXT("CANCEL"))
            });
    }
    else if (DialogId == FName(TEXT("project_new")))
    {
        OpenModalDialog(DialogId, TEXT("NEW PROJECT"), TEXT("Create a named project. It is written immediately and becomes the active autosave target."), { {FName(TEXT("dialog_project_new_confirm")), TEXT("CREATE PROJECT")}, {FName(TEXT("dialog_cancel")), TEXT("CANCEL")} });
    }
    else if (DialogId == FName(TEXT("project_save_as")))
    {
        OpenModalDialog(DialogId, TEXT("SAVE PROJECT AS"), TEXT("Save the current character under a new project name without overwriting the active project."), { {FName(TEXT("dialog_project_save_as_confirm")), TEXT("SAVE AS")}, {FName(TEXT("dialog_cancel")), TEXT("CANCEL")} });
    }
    else if (DialogId == FName(TEXT("project_rename")))
    {
        OpenModalDialog(DialogId, TEXT("RENAME PROJECT"), TEXT("Change the selected project's display name. Existing backups and its stable save identity are retained."), { {FName(TEXT("dialog_project_rename_confirm")), TEXT("RENAME")}, {FName(TEXT("dialog_cancel")), TEXT("CANCEL")} });
    }
    else if (DialogId == FName(TEXT("project_duplicate")))
    {
        OpenModalDialog(DialogId, TEXT("DUPLICATE PROJECT"), TEXT("Create an independent copy of the selected project and its character state."), { {FName(TEXT("dialog_project_duplicate_confirm")), TEXT("DUPLICATE")}, {FName(TEXT("dialog_cancel")), TEXT("CANCEL")} });
    }
    else if (DialogId == FName(TEXT("project_delete")))
    {
        OpenModalDialog(DialogId, TEXT("DELETE PROJECT"), TEXT("Delete the selected project, its autosave snapshot, and retained backups. This cannot be undone."), { {FName(TEXT("dialog_project_delete_confirm")), TEXT("DELETE PROJECT")}, {FName(TEXT("dialog_cancel")), TEXT("CANCEL")} });
    }
    else if (DialogId == FName(TEXT("project_recover")))
    {
        OpenModalDialog(DialogId, TEXT("AUTOSAVE RECOVERY"), TEXT("A newer recovery snapshot is available. Recover it as unsaved changes, or retain it and dismiss this prompt."), { {FName(TEXT("dialog_project_recover")), TEXT("RECOVER")}, {FName(TEXT("dialog_project_recovery_dismiss")), TEXT("DISMISS")}, {FName(TEXT("dialog_cancel")), TEXT("CANCEL")} });
    }
    else if (DialogId == FName(TEXT("unsaved_project")))
    {
        OpenModalDialog(DialogId, TEXT("UNSAVED CHANGES"), TEXT("Save the active project before switching, discard the edits, or cancel the project change."), { {FName(TEXT("dialog_unsaved_save")), TEXT("SAVE AND CONTINUE")}, {FName(TEXT("dialog_unsaved_discard")), TEXT("DISCARD AND CONTINUE")}, {FName(TEXT("dialog_unsaved_cancel")), TEXT("CANCEL")} });
    }
    else if (DialogId == FName(TEXT("import_source")))
    {
        OpenModalDialog(DialogId, TEXT("IMPORT SOURCE"), TEXT("Enter the extracted Unreal Content folder to analyze. AssetRegistry dependencies are available when the folder is mounted under this project Content directory."), { {FName(TEXT("dialog_import_source_confirm")), TEXT("USE SOURCE")}, {FName(TEXT("dialog_cancel")), TEXT("CANCEL")} });
    }
    else if (DialogId == FName(TEXT("import_destination")))
    {
        OpenModalDialog(DialogId, TEXT("IMPORT DESTINATION"), TEXT("Enter the destination Content directory. Raw Unreal packages retain their internal package references, so preserve their relative Content paths when moving between projects."), { {FName(TEXT("dialog_import_destination_confirm")), TEXT("USE DESTINATION")}, {FName(TEXT("dialog_cancel")), TEXT("CANCEL")} });
    }
    else if (DialogId == FName(TEXT("export_destination")))
    {
        OpenModalDialog(DialogId, TEXT("EXPORT DESTINATION"), TEXT("Enter the folder for manifests and staged Unreal package files. The folder is created on export if necessary."), { {FName(TEXT("dialog_export_destination_confirm")), TEXT("USE DESTINATION")}, {FName(TEXT("dialog_cancel")), TEXT("CANCEL")} });
    }
}

void UCharacterCreatorRootWidget::HandleColorPickerChannelChanged(ECharacterCreatorParameter Parameter, float Value)
{
    switch (Parameter)
    {
    case ECharacterCreatorParameter::Height:
        PendingColorPickerColor.R = Value;
        break;
    case ECharacterCreatorParameter::ShoulderWidth:
        PendingColorPickerColor.G = Value;
        break;
    case ECharacterCreatorParameter::ArmLength:
        PendingColorPickerColor.B = Value;
        break;
    default:
        break;
    }
    PendingColorPickerColor.A = 1.0f;
}

void UCharacterCreatorRootWidget::HandleOpenProjectClicked()
{
    if (Session)
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            Subsystem->RefreshProjectBrowser();
        }
        Session->SetScreen(ECharacterCreatorScreen::ProjectBrowser);
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
            TPair<FName, FString>(FName(TEXT("dialog_import_dependencies")), TEXT("REVIEW DEPENDENCIES")),
            TPair<FName, FString>(FName(TEXT("dialog_import_conflicts")), TEXT("REVIEW CONFLICT POLICY")),
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
