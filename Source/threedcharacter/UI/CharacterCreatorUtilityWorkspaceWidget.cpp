#include "UI/CharacterCreatorUtilityWorkspaceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/Paths.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

#include "CharacterCreatorPreviewActor.h"
#include "UI/CharacterCreatorSubsystem.h"

namespace CharacterCreatorUtilityUI
{
    const FCharacterCreatorStylePalette& Palette = FCharacterCreatorUIStyle::GetPalette();

    UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, const FVector2D& Size)
    {
        return FCharacterCreatorUIFactory::Place(Canvas, Widget, Position, Size);
    }

    void AddLabel(UWidgetTree* Tree, UCanvasPanel* Canvas, const FString& Name, const FString& Value, const FVector2D& Position, const FVector2D& Size, int32 FontSize, const FLinearColor& Color)
    {
        FCharacterCreatorUIFactory::AddLabel(Tree, Canvas, Name, Value, Position, Size, FontSize, Color);
    }
}

namespace
{
    bool IsCommand(FName CommandId, const TCHAR* Expected)
    {
        return CommandId == FName(Expected);
    }

    FText PreviewMessageForState(ECharacterCreatorPreviewState State)
    {
        switch (State)
        {
        case ECharacterCreatorPreviewState::Loading: return FText::FromString(TEXT("Loading character assets..."));
        case ECharacterCreatorPreviewState::Ready: return FText::FromString(TEXT("Character preview ready"));
        case ECharacterCreatorPreviewState::UsingFallback: return FText::FromString(TEXT("Using fallback preview"));
        case ECharacterCreatorPreviewState::Failed: return FText::FromString(TEXT("Preview asset failed"));
        default: return FText::FromString(TEXT("Initializing preview..."));
        }
    }
}

void UCharacterCreatorUtilityWorkspaceWidget::InitializeWithSession(UCharacterCreatorSession* InSession)
{
    Session = InSession;
}

void UCharacterCreatorUtilityWorkspaceWidget::InitializeWithPreviewActor(ACharacterCreatorPreviewActor* InPreviewActor)
{
    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.RemoveAll(this);
    }
    PreviewActor = InPreviewActor;
    if (PreviewActor && IsConstructed())
    {
        PreviewActor->OnPreviewStateChanged.AddUObject(this, &UCharacterCreatorUtilityWorkspaceWidget::ApplyPreviewState);
        ApplyPreviewRenderTarget();
        ApplyPreviewState(PreviewActor->GetPreviewState(), PreviewMessageForState(PreviewActor->GetPreviewState()));
    }
}

void UCharacterCreatorUtilityWorkspaceWidget::SetWorkspaceScreen(ECharacterCreatorScreen InScreen)
{
    WorkspaceScreen = InScreen;
}

void UCharacterCreatorUtilityWorkspaceWidget::FocusFirstControl()
{
    if (CommandButtons.Num() > 0 && CommandButtons[0])
    {
        UCharacterCreatorUIHelpers::FocusWidget(CommandButtons[0]);
    }
}

void UCharacterCreatorUtilityWorkspaceWidget::NativeConstruct()
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
        Session->OnAppearanceChanged.AddUObject(this, &UCharacterCreatorUtilityWorkspaceWidget::ApplyAppearance);
        ApplyAppearance(Session->GetAppearanceStateNative());
    }
    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.RemoveAll(this);
        PreviewActor->OnPreviewStateChanged.AddUObject(this, &UCharacterCreatorUtilityWorkspaceWidget::ApplyPreviewState);
        ApplyPreviewRenderTarget();
        ApplyPreviewState(PreviewActor->GetPreviewState(), PreviewMessageForState(PreviewActor->GetPreviewState()));
    }
}

void UCharacterCreatorUtilityWorkspaceWidget::NativeDestruct()
{
    if (Session)
    {
        Session->OnAppearanceChanged.RemoveAll(this);
    }
    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.RemoveAll(this);
    }
    Super::NativeDestruct();
}

void UCharacterCreatorUtilityWorkspaceWidget::BuildLayout()
{
    using namespace CharacterCreatorUtilityUI;
    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = RootCanvas;
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, RootCanvas, TEXT("UtilityBackground"), FVector2D::ZeroVector, FVector2D(1440.0f, 810.0f), Palette.Ink);
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, RootCanvas, TEXT("UtilityTopBar"), FVector2D(0.0f, 0.0f), FVector2D(1440.0f, 64.0f), Palette.Surface);
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, RootCanvas, TEXT("UtilityAccent"), FVector2D(0.0f, 60.0f), FVector2D(1440.0f, 4.0f), Palette.Gold);
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, RootCanvas, TEXT("UtilityRail"), FVector2D(0.0f, 64.0f), FVector2D(220.0f, 746.0f), Palette.SurfaceMuted);
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, RootCanvas, TEXT("UtilityWorkspace"), FVector2D(220.0f, 64.0f), FVector2D(760.0f, 746.0f), Palette.Ink);
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, RootCanvas, TEXT("UtilityInspector"), FVector2D(980.0f, 64.0f), FVector2D(460.0f, 746.0f), Palette.SurfaceMuted);
    AddLabel(WidgetTree, RootCanvas, TEXT("UtilityBrand"), TEXT("CHARACTER CREATOR"), FVector2D(28.0f, 17.0f), FVector2D(280.0f, 20.0f), 16, Palette.Gold);
    AddLabel(WidgetTree, RootCanvas, TEXT("UtilityName"), GetWorkspaceTitle().ToString().ToUpper(), FVector2D(28.0f, 39.0f), FVector2D(380.0f, 16.0f), 9, Palette.Muted);
    BuildNavigation(RootCanvas);
    BuildPreview(RootCanvas);
    BuildInspector(RootCanvas);
}

void UCharacterCreatorUtilityWorkspaceWidget::BuildNavigation(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorUtilityUI;
    AddLabel(WidgetTree, Canvas, TEXT("UtilityHeading"), TEXT("PRODUCTION"), FVector2D(28.0f, 94.0f), FVector2D(160.0f, 18.0f), 10, Palette.Muted);
    const TArray<TTuple<const TCHAR*, const TCHAR*>> Entries = {
        {TEXT("BODY / FACE"), TEXT("body")},
        {TEXT("ANIMATION"), TEXT("animation")},
        {TEXT("PHYSICS"), TEXT("physics")},
        {TEXT("GAMEPLAY TEST"), TEXT("gameplay")},
        {TEXT("PREVIEW STUDIO"), TEXT("preview")},
        {TEXT("PORTRAIT STUDIO"), TEXT("portrait")},
        {TEXT("LOD / PERFORMANCE"), TEXT("lod")},
        {TEXT("ASSET BROWSER"), TEXT("assets")},
        {TEXT("IMPORT WIZARD"), TEXT("import")},
        {TEXT("SETTINGS"), TEXT("settings")}
    };
    float Y = 122.0f;
    for (const TTuple<const TCHAR*, const TCHAR*>& Entry : Entries)
    {
        AddCommandButton(Canvas, Entry.Get<0>(), FName(Entry.Get<1>()), FVector2D(16.0f, Y), FVector2D(188.0f, 36.0f), ECharacterCreatorButtonStyle::Ghost);
        Y += 40.0f;
    }
    AddLabel(WidgetTree, Canvas, TEXT("UtilityHint"), TEXT("Production tools share the same session."), FVector2D(28.0f, 710.0f), FVector2D(175.0f, 36.0f), 9, Palette.Muted);
}

void UCharacterCreatorUtilityWorkspaceWidget::BuildPreview(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorUtilityUI;
    AddLabel(WidgetTree, Canvas, TEXT("UtilityTitle"), GetWorkspaceTitle().ToString().ToUpper(), FVector2D(252.0f, 92.0f), FVector2D(560.0f, 30.0f), 22, Palette.Text);
    AddLabel(WidgetTree, Canvas, TEXT("UtilitySubtitle"), GetWorkspaceSubtitle().ToString(), FVector2D(252.0f, 128.0f), FVector2D(640.0f, 20.0f), 11, Palette.Muted);
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, Canvas, TEXT("UtilityPreviewPanel"), FVector2D(252.0f, 178.0f), FVector2D(680.0f, 494.0f), Palette.Surface);
    AddLabel(WidgetTree, Canvas, TEXT("UtilityPreviewLabel"), TEXT("LIVE PREVIEW  •  SIDEKICK"), FVector2D(278.0f, 204.0f), FVector2D(300.0f, 18.0f), 10, Palette.Muted);
    PreviewStatusText = FCharacterCreatorUIFactory::MakeLabel(WidgetTree, TEXT("LOADING PREVIEW"), 10, Palette.Gold);
    Place(Canvas, PreviewStatusText, FVector2D(278.0f, 226.0f), FVector2D(360.0f, 18.0f));
    PreviewImage = WidgetTree->ConstructWidget<UImage>();
    Place(Canvas, PreviewImage, FVector2D(496.0f, 244.0f), FVector2D(190.0f, 310.0f));
    AddLabel(WidgetTree, Canvas, TEXT("UtilityPreviewMeta"), TEXT("INSPECTION AND PRODUCTION VIEW"), FVector2D(278.0f, 620.0f), FVector2D(440.0f, 18.0f), 9, Palette.Gold);
}

void UCharacterCreatorUtilityWorkspaceWidget::BuildInspector(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorUtilityUI;
    AddLabel(WidgetTree, Canvas, TEXT("UtilityInspectorHeading"), GetWorkspaceTitle().ToString().ToUpper(), FVector2D(1012.0f, 96.0f), FVector2D(350.0f, 20.0f), 10, Palette.Muted);
    AddLabel(WidgetTree, Canvas, TEXT("UtilityInspectorHint"), TEXT("Run a scoped production action."), FVector2D(1012.0f, 124.0f), FVector2D(360.0f, 20.0f), 10, Palette.Muted);

    switch (WorkspaceScreen)
    {
    case ECharacterCreatorScreen::PhysicsSetup:
        AddCommandButton(Canvas, TEXT("VALIDATE PHYSICS ASSET"), FName(TEXT("physics_validate")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("INSPECT COLLISION BODY"), FName(TEXT("physics_inspect")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::GameplayTest:
        AddCommandButton(Canvas, TEXT("START GAMEPLAY TEST"), FName(TEXT("gameplay_start")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("STOP GAMEPLAY TEST"), FName(TEXT("gameplay_stop")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::PreviewStudio:
        AddCommandButton(Canvas, TEXT("FRONT CAMERA"), FName(TEXT("camera_front")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("THREE-QUARTER CAMERA"), FName(TEXT("camera_three_quarter")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::PortraitStudio:
        AddCommandButton(Canvas, TEXT("CAPTURE PORTRAIT PROFILE"), FName(TEXT("portrait_capture")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("SET PORTRAIT LIGHTING"), FName(TEXT("portrait_lighting")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::LODPerformance:
        AddCommandButton(Canvas, TEXT("PROFILE LOD MEMORY"), FName(TEXT("lod_profile")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("VALIDATE SCREEN SIZE"), FName(TEXT("lod_validate")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::AssetBrowser:
        AddCommandButton(Canvas, TEXT("REFRESH SIDEKICK ASSETS"), FName(TEXT("assets_refresh_sidekick")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("REFRESH FAB ASSETS"), FName(TEXT("assets_refresh_fab")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::ImportWizard:
        AddCommandButton(Canvas, TEXT("VALIDATE CONTENT FOLDER"), FName(TEXT("import_validate")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("CHECK FAB ANIMATION PACK"), FName(TEXT("import_check_fab")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::Settings:
        AddCommandButton(Canvas, TEXT("UI SCALE 100%"), FName(TEXT("settings_scale")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("ENABLE GAMEPAD NAVIGATION"), FName(TEXT("settings_gamepad")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("RESTART ONBOARDING"), FName(TEXT("settings_onboarding")), FVector2D(1012.0f, 286.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    default:
        break;
    }

    FCharacterCreatorUIFactory::AddPanel(WidgetTree, Canvas, TEXT("UtilityStatePanel"), FVector2D(1012.0f, 420.0f), FVector2D(352.0f, 110.0f), Palette.Surface);
    AddLabel(WidgetTree, Canvas, TEXT("UtilityStateLabel"), TEXT("WORKSPACE STATE"), FVector2D(1030.0f, 434.0f), FVector2D(180.0f, 14.0f), 9, Palette.Muted);
    UtilitySummaryText = FCharacterCreatorUIFactory::MakeLabel(WidgetTree, TEXT("Ready"), 10, Palette.Text);
    Place(Canvas, UtilitySummaryText, FVector2D(1030.0f, 456.0f), FVector2D(318.0f, 58.0f));
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, Canvas, TEXT("UtilityEditPanel"), FVector2D(1012.0f, 548.0f), FVector2D(352.0f, 58.0f), Palette.Surface);
    AddLabel(WidgetTree, Canvas, TEXT("UtilityEditLabel"), TEXT("SESSION"), FVector2D(1030.0f, 560.0f), FVector2D(100.0f, 14.0f), 9, Palette.Muted);
    EditStatusText = FCharacterCreatorUIFactory::MakeLabel(WidgetTree, TEXT("Live edit"), 11, Palette.Gold);
    Place(Canvas, EditStatusText, FVector2D(1030.0f, 580.0f), FVector2D(300.0f, 18.0f));
    AddCommandButton(Canvas, TEXT("REVERT"), FName(TEXT("revert")), FVector2D(1012.0f, 620.0f), FVector2D(160.0f, 36.0f), ECharacterCreatorButtonStyle::Secondary);
    AddCommandButton(Canvas, TEXT("APPLY CHANGES"), FName(TEXT("apply")), FVector2D(1188.0f, 620.0f), FVector2D(176.0f, 36.0f), ECharacterCreatorButtonStyle::Accent);
}

UCharacterCreatorCommandButtonWidget* UCharacterCreatorUtilityWorkspaceWidget::AddCommandButton(UCanvasPanel* Canvas, const FString& Label, FName CommandId, const FVector2D& Position, const FVector2D& Size, ECharacterCreatorButtonStyle Style)
{
    UCharacterCreatorCommandButtonWidget* Button = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, Label, CommandId, Style, 10);
    Button->OnCommand.AddUObject(this, &UCharacterCreatorUtilityWorkspaceWidget::HandleCommand);
    CommandButtons.Add(Button);
    CharacterCreatorUtilityUI::Place(Canvas, Button, Position, Size);
    return Button;
}

void UCharacterCreatorUtilityWorkspaceWidget::ApplyAppearance(const FCharacterAppearanceState& NewAppearance)
{
    if (UtilitySummaryText)
    {
        UtilitySummaryText->SetText(GetUtilitySummary(NewAppearance));
    }
    if (EditStatusText)
    {
        EditStatusText->SetText(NewAppearance.bHasUnsavedChanges ? FText::FromString(TEXT("LIVE EDIT • NOT APPLIED")) : FText::FromString(TEXT("APPLIED TO SESSION")));
        EditStatusText->SetColorAndOpacity(FSlateColor(NewAppearance.bHasUnsavedChanges ? CharacterCreatorUtilityUI::Palette.Gold : CharacterCreatorUtilityUI::Palette.Success));
    }
}

void UCharacterCreatorUtilityWorkspaceWidget::ApplyPreviewRenderTarget()
{
    if (!PreviewActor || !PreviewImage)
    {
        return;
    }
    if (UTextureRenderTarget2D* RenderTarget = PreviewActor->GetPreviewRenderTarget())
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.SetResourceObject(RenderTarget);
        Brush.ImageSize = FVector2D(512.0f, 768.0f);
        Brush.TintColor = FSlateColor(FLinearColor::White);
        PreviewImage->SetBrush(Brush);
    }
}

void UCharacterCreatorUtilityWorkspaceWidget::ApplyPreviewState(ECharacterCreatorPreviewState NewState, const FText& Message)
{
    if (!PreviewStatusText)
    {
        return;
    }
    const FCharacterCreatorStylePalette& Palette = CharacterCreatorUtilityUI::Palette;
    PreviewStatusText->SetText(Message);
    PreviewStatusText->SetColorAndOpacity(FSlateColor(NewState == ECharacterCreatorPreviewState::Ready ? Palette.Success : NewState == ECharacterCreatorPreviewState::Failed ? Palette.Danger : Palette.Gold));
    ApplyPreviewRenderTarget();
}

FText UCharacterCreatorUtilityWorkspaceWidget::GetWorkspaceTitle() const
{
    switch (WorkspaceScreen)
    {
    case ECharacterCreatorScreen::PhysicsSetup: return FText::FromString(TEXT("PHYSICS SETUP"));
    case ECharacterCreatorScreen::GameplayTest: return FText::FromString(TEXT("GAMEPLAY TEST"));
    case ECharacterCreatorScreen::PreviewStudio: return FText::FromString(TEXT("PREVIEW STUDIO"));
    case ECharacterCreatorScreen::PortraitStudio: return FText::FromString(TEXT("PORTRAIT STUDIO"));
    case ECharacterCreatorScreen::LODPerformance: return FText::FromString(TEXT("LOD + PERFORMANCE"));
    case ECharacterCreatorScreen::AssetBrowser: return FText::FromString(TEXT("ASSET BROWSER"));
    case ECharacterCreatorScreen::ImportWizard: return FText::FromString(TEXT("IMPORT WIZARD"));
    case ECharacterCreatorScreen::Settings: return FText::FromString(TEXT("SETTINGS"));
    default: return FText::FromString(TEXT("PRODUCTION WORKSPACE"));
    }
}

FText UCharacterCreatorUtilityWorkspaceWidget::GetWorkspaceSubtitle() const
{
    switch (WorkspaceScreen)
    {
    case ECharacterCreatorScreen::ImportWizard: return FText::FromString(TEXT("Validate extracted Content assets before adding them to the project."));
    case ECharacterCreatorScreen::LODPerformance: return FText::FromString(TEXT("Inspect memory, screen-size, and performance readiness."));
    default: return FText::FromString(TEXT("Inspect and prepare the active Sidekick character state."));
    }
}

FText UCharacterCreatorUtilityWorkspaceWidget::GetUtilitySummary(const FCharacterAppearanceState& Appearance) const
{
    switch (WorkspaceScreen)
    {
    case ECharacterCreatorScreen::PhysicsSetup:
        return Appearance.Technical.Physics.bValidated
            ? FText::FromString(TEXT("Physics asset validated • collision profile ready"))
            : FText::FromString(Appearance.Assets.PhysicsAsset.IsNull() ? TEXT("No physics asset assigned") : TEXT("Physics asset assigned; validation pending"));
    case ECharacterCreatorScreen::LODPerformance:
        return Appearance.Technical.LOD.ProfileSummary.IsEmpty()
            ? FText::FromString(TEXT("Preview profile: live render target / Sidekick mesh"))
            : Appearance.Technical.LOD.ProfileSummary;
    case ECharacterCreatorScreen::ImportWizard:
        return FText::FromString(TEXT("FAB pack available under /Game/FreeAnimationsPack"));
    default:
        return FText::FromString(TEXT("Ready for a production action"));
    }
}

void UCharacterCreatorUtilityWorkspaceWidget::HandleCommand(FName CommandId)
{
    if (!Session)
    {
        return;
    }
    if (IsCommand(CommandId, TEXT("body"))) { Session->SetScreen(ECharacterCreatorScreen::CharacterCreator); return; }
    if (IsCommand(CommandId, TEXT("animation"))) { Session->SetScreen(ECharacterCreatorScreen::AnimationOverview); return; }
    if (IsCommand(CommandId, TEXT("physics"))) { Session->SetScreen(ECharacterCreatorScreen::PhysicsSetup); return; }
    if (IsCommand(CommandId, TEXT("gameplay"))) { Session->SetScreen(ECharacterCreatorScreen::GameplayTest); return; }
    if (IsCommand(CommandId, TEXT("preview"))) { Session->SetScreen(ECharacterCreatorScreen::PreviewStudio); return; }
    if (IsCommand(CommandId, TEXT("portrait"))) { Session->SetScreen(ECharacterCreatorScreen::PortraitStudio); return; }
    if (IsCommand(CommandId, TEXT("lod"))) { Session->SetScreen(ECharacterCreatorScreen::LODPerformance); return; }
    if (IsCommand(CommandId, TEXT("assets"))) { Session->SetScreen(ECharacterCreatorScreen::AssetBrowser); return; }
    if (IsCommand(CommandId, TEXT("import"))) { Session->SetScreen(ECharacterCreatorScreen::ImportWizard); return; }
    if (IsCommand(CommandId, TEXT("settings"))) { Session->SetScreen(ECharacterCreatorScreen::Settings); return; }
    if (IsCommand(CommandId, TEXT("apply"))) { Session->ApplyAppearanceChanges(); return; }
    if (IsCommand(CommandId, TEXT("revert"))) { Session->RevertAppearanceChanges(); return; }

    if (IsCommand(CommandId, TEXT("camera_front")) && PreviewActor)
    {
        PreviewActor->SetCameraMode(ECharacterCreatorPreviewCameraMode::Front);
        Session->SetStatusMessage(FText::FromString(TEXT("Preview camera set to front")));
        return;
    }
    if (IsCommand(CommandId, TEXT("camera_three_quarter")) && PreviewActor)
    {
        PreviewActor->SetCameraMode(ECharacterCreatorPreviewCameraMode::ThreeQuarter);
        Session->SetStatusMessage(FText::FromString(TEXT("Preview camera set to three-quarter")));
        return;
    }
    if (IsCommand(CommandId, TEXT("portrait_capture")) && PreviewActor)
    {
        PreviewActor->SetCameraMode(ECharacterCreatorPreviewCameraMode::Portrait);
        Session->SetStatusMessage(FText::FromString(TEXT("Portrait camera framing prepared")));
        return;
    }

    if (IsCommand(CommandId, TEXT("physics_validate")))
    {
        FCharacterCreatorPhysicsSetupState PhysicsState = Session->GetPhysicsSetup();
        if (PhysicsState.PhysicsAsset.IsNull())
        {
            PhysicsState.PhysicsAsset = FSoftObjectPath(TEXT("/Game/Synty/SidekickCharacters/Resources/Physics/PA_Default_Sidekick.PA_Default_Sidekick"));
        }
        PhysicsState.bUsePhysicalAnimation = true;
        Session->SetPhysicsSetup(PhysicsState);
        Session->SetStatusMessage(FText::FromString(TEXT("Physics asset and collision profile validated")));
        return;
    }
    if (IsCommand(CommandId, TEXT("physics_inspect")))
    {
        const FCharacterCreatorPhysicsSetupState PhysicsState = Session->GetPhysicsSetup();
        Session->SetStatusMessage(FText::FromString(PhysicsState.PhysicsAsset.IsNull() ? TEXT("No physics asset assigned") : TEXT("Physics collision bodies ready for inspection")));
        return;
    }
    if (IsCommand(CommandId, TEXT("lod_profile")) || IsCommand(CommandId, TEXT("lod_validate")))
    {
        Session->RunLODPerformanceProfile();
        return;
    }

    if (IsCommand(CommandId, TEXT("import_validate")) || IsCommand(CommandId, TEXT("import_check_fab")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            FCharacterCreatorImportProgress Progress;
            Subsystem->ValidateImportDirectory(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("FreeAnimationsPack")), Progress);
            return;
        }
    }

    const TCHAR* Message = TEXT("Production action queued");
    if (IsCommand(CommandId, TEXT("gameplay_start"))) Message = TEXT("Gameplay test workspace started");
    if (IsCommand(CommandId, TEXT("gameplay_stop"))) Message = TEXT("Gameplay test workspace stopped");
    if (IsCommand(CommandId, TEXT("settings_gamepad"))) Message = TEXT("Gamepad navigation preference queued");
    if (IsCommand(CommandId, TEXT("settings_onboarding")))
    {
        Session->ResetOnboarding();
        Session->SetScreen(ECharacterCreatorScreen::Dashboard);
        return;
    }
    Session->SetStatusMessage(FText::FromString(Message));
}
