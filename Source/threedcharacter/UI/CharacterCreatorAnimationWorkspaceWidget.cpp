#include "UI/CharacterCreatorAnimationWorkspaceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

#include "CharacterCreatorPreviewActor.h"

namespace CharacterCreatorAnimationUI
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

void UCharacterCreatorAnimationWorkspaceWidget::InitializeWithSession(UCharacterCreatorSession* InSession)
{
    Session = InSession;
}

void UCharacterCreatorAnimationWorkspaceWidget::InitializeWithPreviewActor(ACharacterCreatorPreviewActor* InPreviewActor)
{
    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.RemoveAll(this);
    }

    PreviewActor = InPreviewActor;
    if (PreviewActor && IsConstructed())
    {
        PreviewActor->OnPreviewStateChanged.AddUObject(this, &UCharacterCreatorAnimationWorkspaceWidget::ApplyPreviewState);
        ApplyPreviewRenderTarget();
        ApplyPreviewState(PreviewActor->GetPreviewState(), PreviewMessageForState(PreviewActor->GetPreviewState()));
    }
}

void UCharacterCreatorAnimationWorkspaceWidget::SetWorkspaceScreen(ECharacterCreatorScreen InScreen)
{
    WorkspaceScreen = InScreen;
}

void UCharacterCreatorAnimationWorkspaceWidget::FocusFirstControl()
{
    if (CommandButtons.Num() > 0 && CommandButtons[0])
    {
        UCharacterCreatorUIHelpers::FocusWidget(CommandButtons[0]);
    }
}

void UCharacterCreatorAnimationWorkspaceWidget::NativeConstruct()
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
        Session->OnAppearanceChanged.AddUObject(this, &UCharacterCreatorAnimationWorkspaceWidget::ApplyAppearance);
        ApplyAppearance(Session->GetAppearanceStateNative());
    }

    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.RemoveAll(this);
        PreviewActor->OnPreviewStateChanged.AddUObject(this, &UCharacterCreatorAnimationWorkspaceWidget::ApplyPreviewState);
        ApplyPreviewRenderTarget();
        ApplyPreviewState(PreviewActor->GetPreviewState(), PreviewMessageForState(PreviewActor->GetPreviewState()));
    }
}

void UCharacterCreatorAnimationWorkspaceWidget::NativeDestruct()
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

void UCharacterCreatorAnimationWorkspaceWidget::BuildLayout()
{
    using namespace CharacterCreatorAnimationUI;

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = RootCanvas;
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, RootCanvas, TEXT("AnimationBackground"), FVector2D::ZeroVector, FVector2D(1440.0f, 810.0f), Palette.Ink);
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, RootCanvas, TEXT("AnimationTopBar"), FVector2D(0.0f, 0.0f), FVector2D(1440.0f, 64.0f), Palette.Surface);
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, RootCanvas, TEXT("AnimationAccent"), FVector2D(0.0f, 60.0f), FVector2D(1440.0f, 4.0f), Palette.Blue);
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, RootCanvas, TEXT("AnimationRail"), FVector2D(0.0f, 64.0f), FVector2D(220.0f, 746.0f), Palette.SurfaceMuted);
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, RootCanvas, TEXT("AnimationWorkspace"), FVector2D(220.0f, 64.0f), FVector2D(760.0f, 746.0f), Palette.Ink);
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, RootCanvas, TEXT("AnimationInspector"), FVector2D(980.0f, 64.0f), FVector2D(460.0f, 746.0f), Palette.SurfaceMuted);
    AddLabel(WidgetTree, RootCanvas, TEXT("AnimationBrand"), TEXT("CHARACTER CREATOR"), FVector2D(28.0f, 17.0f), FVector2D(280.0f, 20.0f), 16, Palette.Gold);
    AddLabel(WidgetTree, RootCanvas, TEXT("AnimationWorkspaceName"), GetWorkspaceTitle().ToString().ToUpper(), FVector2D(28.0f, 39.0f), FVector2D(380.0f, 16.0f), 9, Palette.Muted);

    BuildNavigation(RootCanvas);
    BuildPreview(RootCanvas);
    BuildInspector(RootCanvas);
}

void UCharacterCreatorAnimationWorkspaceWidget::BuildNavigation(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorAnimationUI;
    AddLabel(WidgetTree, Canvas, TEXT("AnimationHeading"), TEXT("ANIMATION WORKSPACES"), FVector2D(28.0f, 94.0f), FVector2D(180.0f, 18.0f), 10, Palette.Muted);

    const TArray<TTuple<const TCHAR*, const TCHAR*>> NavigationEntries = {
        {TEXT("BODY / FACE"), TEXT("body")},
        {TEXT("OUTFIT / ARMOR"), TEXT("outfit")},
        {TEXT("HAIR / GROOMING"), TEXT("hair")},
        {TEXT("MATERIALS / COLOR"), TEXT("materials")},
        {TEXT("WEAPONS / IK"), TEXT("weapons")},
        {TEXT("ANIMATION OVERVIEW"), TEXT("animation_overview")},
        {TEXT("LOCOMOTION SETUP"), TEXT("locomotion")},
        {TEXT("BLEND SPACE ASSISTANT"), TEXT("blend_space")},
        {TEXT("ANIMATION BLUEPRINT"), TEXT("animation_blueprint")},
        {TEXT("MONTAGE / COMBOS"), TEXT("montage")},
        {TEXT("RETARGETING"), TEXT("retargeting")},
        {TEXT("SKELETON / SOCKETS"), TEXT("skeleton")}
    };

    float Y = 122.0f;
    for (const TTuple<const TCHAR*, const TCHAR*>& Entry : NavigationEntries)
    {
        AddCommandButton(Canvas, Entry.Get<0>(), FName(Entry.Get<1>()), FVector2D(16.0f, Y), FVector2D(188.0f, 32.0f), ECharacterCreatorButtonStyle::Ghost);
        Y += 35.0f;
    }

    AddLabel(WidgetTree, Canvas, TEXT("AnimationHint"), TEXT("Source assets stay separate until retargeted."), FVector2D(28.0f, 710.0f), FVector2D(175.0f, 36.0f), 9, Palette.Muted);
}

void UCharacterCreatorAnimationWorkspaceWidget::BuildPreview(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorAnimationUI;
    AddLabel(WidgetTree, Canvas, TEXT("AnimationTitle"), GetWorkspaceTitle().ToString().ToUpper(), FVector2D(252.0f, 92.0f), FVector2D(560.0f, 30.0f), 22, Palette.Text);
    AddLabel(WidgetTree, Canvas, TEXT("AnimationSubtitle"), GetWorkspaceSubtitle().ToString(), FVector2D(252.0f, 128.0f), FVector2D(640.0f, 20.0f), 11, Palette.Muted);
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, Canvas, TEXT("AnimationPreviewPanel"), FVector2D(252.0f, 178.0f), FVector2D(680.0f, 494.0f), Palette.Surface);
    AddLabel(WidgetTree, Canvas, TEXT("AnimationPreviewLabel"), TEXT("LIVE PREVIEW  •  SIDEKICK TARGET"), FVector2D(278.0f, 204.0f), FVector2D(300.0f, 18.0f), 10, Palette.Muted);
    PreviewStatusText = FCharacterCreatorUIFactory::MakeLabel(WidgetTree, TEXT("LOADING PREVIEW"), 10, Palette.Gold);
    Place(Canvas, PreviewStatusText, FVector2D(278.0f, 226.0f), FVector2D(380.0f, 18.0f));
    PreviewImage = WidgetTree->ConstructWidget<UImage>();
    Place(Canvas, PreviewImage, FVector2D(496.0f, 244.0f), FVector2D(190.0f, 310.0f));
    AddLabel(WidgetTree, Canvas, TEXT("AnimationPreviewMeta"), TEXT("NO SOURCE ANIMATION IS PLAYED ON SIDEKICK UNTIL TARGET READY"), FVector2D(278.0f, 620.0f), FVector2D(620.0f, 18.0f), 9, Palette.Gold);
}

void UCharacterCreatorAnimationWorkspaceWidget::BuildInspector(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorAnimationUI;
    AddLabel(WidgetTree, Canvas, TEXT("AnimationInspectorHeading"), GetWorkspaceTitle().ToString().ToUpper(), FVector2D(1012.0f, 96.0f), FVector2D(350.0f, 20.0f), 10, Palette.Muted);
    AddLabel(WidgetTree, Canvas, TEXT("AnimationInspectorHint"), TEXT("Choose a source, rig, or target step."), FVector2D(1012.0f, 124.0f), FVector2D(360.0f, 20.0f), 10, Palette.Muted);

    switch (WorkspaceScreen)
    {
    case ECharacterCreatorScreen::AnimationOverview:
        AddCommandButton(Canvas, TEXT("USE MANNY IDLE SOURCE"), FName(TEXT("source_idle")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("USE TALK SOURCE"), FName(TEXT("source_talk")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("CLEAR SOURCE"), FName(TEXT("source_clear")), FVector2D(1012.0f, 286.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Ghost);
        break;
    case ECharacterCreatorScreen::LocomotionSetup:
        AddCommandButton(Canvas, TEXT("USE MANNY WALK"), FName(TEXT("source_walk")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("USE MANNY RUN"), FName(TEXT("source_run")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("LOAD WALK / RUN BLEND SPACE"), FName(TEXT("blend_space_asset")), FVector2D(1012.0f, 286.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        break;
    case ECharacterCreatorScreen::BlendSpaceAssistant:
        AddCommandButton(Canvas, TEXT("SELECT WALK / RUN BLEND SPACE"), FName(TEXT("blend_space_asset")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("BEGIN RETARGET PASS"), FName(TEXT("retarget_begin")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::AnimationBlueprintWorkspace:
        AddCommandButton(Canvas, TEXT("SELECT MANNY ABP SOURCE"), FName(TEXT("abp_manny")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("GENERATE SIDEKICK ABP PROFILE"), FName(TEXT("abp_generate")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("CLEAR ANIMATION BLUEPRINT"), FName(TEXT("abp_clear")), FVector2D(1012.0f, 286.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Ghost);
        break;
    case ECharacterCreatorScreen::MontageComboBuilder:
        AddCommandButton(Canvas, TEXT("USE COMBO HANDS SOURCE"), FName(TEXT("source_combo")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("MARK MONTAGE READY"), FName(TEXT("montage_ready")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("BUILD ANIMATION SET"), FName(TEXT("animation_set_build")), FVector2D(1012.0f, 286.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::RetargetingAssistant:
        AddCommandButton(Canvas, TEXT("LOAD MANNEQUIN RETARGET RIG"), FName(TEXT("retarget_rig")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("EXECUTE RETARGETING"), FName(TEXT("retarget_begin")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("CLEAR RETARGETER"), FName(TEXT("retarget_clear")), FVector2D(1012.0f, 286.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Ghost);
        break;
    case ECharacterCreatorScreen::SkeletonRigSocketInspector:
        AddCommandButton(Canvas, TEXT("INSPECT SIDEKICK SKELETON"), FName(TEXT("inspect_sidekick")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("INSPECT MANNY SOURCE"), FName(TEXT("inspect_manny")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    default:
        break;
    }

    FCharacterCreatorUIFactory::AddPanel(WidgetTree, Canvas, TEXT("AnimationStatePanel"), FVector2D(1012.0f, 420.0f), FVector2D(352.0f, 110.0f), Palette.Surface);
    AddLabel(WidgetTree, Canvas, TEXT("AnimationStateLabel"), TEXT("ANIMATION STATE"), FVector2D(1030.0f, 434.0f), FVector2D(180.0f, 14.0f), 9, Palette.Muted);
    AnimationSummaryText = FCharacterCreatorUIFactory::MakeLabel(WidgetTree, TEXT("Source ready"), 10, Palette.Text);
    Place(Canvas, AnimationSummaryText, FVector2D(1030.0f, 456.0f), FVector2D(318.0f, 58.0f));
    FCharacterCreatorUIFactory::AddPanel(WidgetTree, Canvas, TEXT("AnimationEditPanel"), FVector2D(1012.0f, 548.0f), FVector2D(352.0f, 58.0f), Palette.Surface);
    AddLabel(WidgetTree, Canvas, TEXT("AnimationEditLabel"), TEXT("SESSION"), FVector2D(1030.0f, 560.0f), FVector2D(100.0f, 14.0f), 9, Palette.Muted);
    EditStatusText = FCharacterCreatorUIFactory::MakeLabel(WidgetTree, TEXT("Live edit"), 11, Palette.Gold);
    Place(Canvas, EditStatusText, FVector2D(1030.0f, 580.0f), FVector2D(300.0f, 18.0f));
    AddCommandButton(Canvas, TEXT("REVERT"), FName(TEXT("revert")), FVector2D(1012.0f, 620.0f), FVector2D(160.0f, 36.0f), ECharacterCreatorButtonStyle::Secondary);
    AddCommandButton(Canvas, TEXT("APPLY CHANGES"), FName(TEXT("apply")), FVector2D(1188.0f, 620.0f), FVector2D(176.0f, 36.0f), ECharacterCreatorButtonStyle::Accent);
}

UCharacterCreatorCommandButtonWidget* UCharacterCreatorAnimationWorkspaceWidget::AddCommandButton(UCanvasPanel* Canvas, const FString& Label, FName CommandId, const FVector2D& Position, const FVector2D& Size, ECharacterCreatorButtonStyle Style)
{
    UCharacterCreatorCommandButtonWidget* Button = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, Label, CommandId, Style, 10);
    Button->OnCommand.AddUObject(this, &UCharacterCreatorAnimationWorkspaceWidget::HandleCommand);
    CommandButtons.Add(Button);
    CharacterCreatorAnimationUI::Place(Canvas, Button, Position, Size);
    return Button;
}

void UCharacterCreatorAnimationWorkspaceWidget::ApplyAppearance(const FCharacterAppearanceState& NewAppearance)
{
    if (AnimationSummaryText)
    {
        AnimationSummaryText->SetText(GetAnimationSummary(NewAppearance));
    }
    if (EditStatusText)
    {
        EditStatusText->SetText(NewAppearance.bHasUnsavedChanges ? FText::FromString(TEXT("LIVE EDIT • NOT APPLIED")) : FText::FromString(TEXT("APPLIED TO SESSION")));
        EditStatusText->SetColorAndOpacity(FSlateColor(NewAppearance.bHasUnsavedChanges ? CharacterCreatorAnimationUI::Palette.Gold : CharacterCreatorAnimationUI::Palette.Success));
    }
}

void UCharacterCreatorAnimationWorkspaceWidget::ApplyPreviewRenderTarget()
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

void UCharacterCreatorAnimationWorkspaceWidget::ApplyPreviewState(ECharacterCreatorPreviewState NewState, const FText& Message)
{
    if (!PreviewStatusText)
    {
        return;
    }
    const FCharacterCreatorStylePalette& Palette = CharacterCreatorAnimationUI::Palette;
    PreviewStatusText->SetText(Message);
    PreviewStatusText->SetColorAndOpacity(FSlateColor(NewState == ECharacterCreatorPreviewState::Ready ? Palette.Success : NewState == ECharacterCreatorPreviewState::Failed ? Palette.Danger : Palette.Gold));
    ApplyPreviewRenderTarget();
}

FText UCharacterCreatorAnimationWorkspaceWidget::GetWorkspaceTitle() const
{
    switch (WorkspaceScreen)
    {
    case ECharacterCreatorScreen::AnimationOverview: return FText::FromString(TEXT("ANIMATION OVERVIEW"));
    case ECharacterCreatorScreen::LocomotionSetup: return FText::FromString(TEXT("LOCOMOTION SETUP"));
    case ECharacterCreatorScreen::BlendSpaceAssistant: return FText::FromString(TEXT("BLEND SPACE ASSISTANT"));
    case ECharacterCreatorScreen::AnimationBlueprintWorkspace: return FText::FromString(TEXT("ANIMATION BLUEPRINT"));
    case ECharacterCreatorScreen::MontageComboBuilder: return FText::FromString(TEXT("MONTAGE + COMBO BUILDER"));
    case ECharacterCreatorScreen::RetargetingAssistant: return FText::FromString(TEXT("RETARGETING ASSISTANT"));
    case ECharacterCreatorScreen::SkeletonRigSocketInspector: return FText::FromString(TEXT("SKELETON + RIG INSPECTOR"));
    default: return FText::FromString(TEXT("ANIMATION WORKSPACE"));
    }
}

FText UCharacterCreatorAnimationWorkspaceWidget::GetWorkspaceSubtitle() const
{
    switch (WorkspaceScreen)
    {
    case ECharacterCreatorScreen::RetargetingAssistant:
        return FText::FromString(TEXT("Prepare Manny source assets for a Sidekick target retarget pass."));
    case ECharacterCreatorScreen::SkeletonRigSocketInspector:
        return FText::FromString(TEXT("Inspect the source and target skeleton contracts before mapping."));
    default:
        return FText::FromString(TEXT("Use imported FAB source assets without bypassing target compatibility."));
    }
}

FString UCharacterCreatorAnimationWorkspaceWidget::GetAssetLabel(const FSoftObjectPath& AssetPath) const
{
    return AssetPath.IsNull() ? TEXT("None") : AssetPath.GetAssetName();
}

FText UCharacterCreatorAnimationWorkspaceWidget::GetAnimationSummary(const FCharacterAppearanceState& Appearance) const
{
    const TCHAR* StateLabel = TEXT("Unassigned");
    switch (Appearance.Animation.State)
    {
    case ECharacterCreatorAnimationState::SourceReady: StateLabel = TEXT("Source ready"); break;
    case ECharacterCreatorAnimationState::Retargeting: StateLabel = TEXT("Retargeting required"); break;
    case ECharacterCreatorAnimationState::TargetReady: StateLabel = TEXT("Target ready"); break;
    case ECharacterCreatorAnimationState::Failed: StateLabel = TEXT("Failed"); break;
    default: break;
    }

    return FText::FromString(FString::Printf(TEXT("%s\nSource: %s\nTarget: %s"), StateLabel, *GetAssetLabel(Appearance.Animation.SourceAnimation), *GetAssetLabel(Appearance.Animation.TargetAnimation)));
}

void UCharacterCreatorAnimationWorkspaceWidget::HandleCommand(FName CommandId)
{
    if (!Session)
    {
        return;
    }

    if (IsCommand(CommandId, TEXT("body"))) { Session->SetScreen(ECharacterCreatorScreen::CharacterCreator); return; }
    if (IsCommand(CommandId, TEXT("outfit"))) { Session->SetScreen(ECharacterCreatorScreen::OutfitAndArmor); return; }
    if (IsCommand(CommandId, TEXT("hair"))) { Session->SetScreen(ECharacterCreatorScreen::HairAndGrooming); return; }
    if (IsCommand(CommandId, TEXT("materials"))) { Session->SetScreen(ECharacterCreatorScreen::MaterialsAndColor); return; }
    if (IsCommand(CommandId, TEXT("weapons"))) { Session->SetScreen(ECharacterCreatorScreen::WeaponsAndIK); return; }
    if (IsCommand(CommandId, TEXT("animation_overview"))) { Session->SetScreen(ECharacterCreatorScreen::AnimationOverview); return; }
    if (IsCommand(CommandId, TEXT("locomotion"))) { Session->SetScreen(ECharacterCreatorScreen::LocomotionSetup); return; }
    if (IsCommand(CommandId, TEXT("blend_space"))) { Session->SetScreen(ECharacterCreatorScreen::BlendSpaceAssistant); return; }
    if (IsCommand(CommandId, TEXT("animation_blueprint"))) { Session->SetScreen(ECharacterCreatorScreen::AnimationBlueprintWorkspace); return; }
    if (IsCommand(CommandId, TEXT("montage"))) { Session->SetScreen(ECharacterCreatorScreen::MontageComboBuilder); return; }
    if (IsCommand(CommandId, TEXT("retargeting"))) { Session->SetScreen(ECharacterCreatorScreen::RetargetingAssistant); return; }
    if (IsCommand(CommandId, TEXT("skeleton"))) { Session->SetScreen(ECharacterCreatorScreen::SkeletonRigSocketInspector); return; }
    if (IsCommand(CommandId, TEXT("apply"))) { Session->ApplyAppearanceChanges(); return; }
    if (IsCommand(CommandId, TEXT("revert"))) { Session->RevertAppearanceChanges(); return; }

    const FSoftObjectPath MannySkeleton(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"));
    if (IsCommand(CommandId, TEXT("source_idle"))) { Session->SetAnimationSource(FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Animations/Manny/MM_Idle.MM_Idle")), MannySkeleton); return; }
    if (IsCommand(CommandId, TEXT("source_talk"))) { Session->SetAnimationSource(FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Animations/AS_Talk.AS_Talk")), MannySkeleton); return; }
    if (IsCommand(CommandId, TEXT("source_walk"))) { Session->SetAnimationSource(FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Animations/Manny/MM_Walk_Fwd.MM_Walk_Fwd")), MannySkeleton); return; }
    if (IsCommand(CommandId, TEXT("source_run"))) { Session->SetAnimationSource(FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Animations/Manny/MM_Run_Fwd.MM_Run_Fwd")), MannySkeleton); return; }
    if (IsCommand(CommandId, TEXT("source_combo"))) { Session->SetAnimationSource(FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Animations/AS_ComboHands.AS_ComboHands")), MannySkeleton); return; }
    if (IsCommand(CommandId, TEXT("source_clear"))) { Session->SetAnimationSource(FSoftObjectPath(), MannySkeleton); return; }
    if (IsCommand(CommandId, TEXT("blend_space_asset")))
    {
        const FSoftObjectPath BlendSpacePath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Animations/Manny/BS_MM_WalkRun.BS_MM_WalkRun"));
        Session->SetAnimationSource(BlendSpacePath, MannySkeleton);
        FCharacterCreatorBlendSpaceState BlendSpace;
        BlendSpace.AssetPath = BlendSpacePath;
        BlendSpace.bConfigured = true;
        Session->SetBlendSpaceState(BlendSpace);
        Session->SetStatusMessage(FText::FromString(TEXT("Manny blend space selected; retarget it before Sidekick playback")));
        return;
    }
    if (IsCommand(CommandId, TEXT("abp_manny")))
    {
        FCharacterCreatorAnimationBlueprintState BlueprintState;
        BlueprintState.SourceBlueprint = FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny"));
        BlueprintState.EnabledLayers = { FName(TEXT("Locomotion")), FName(TEXT("Additive")), FName(TEXT("WeaponOverlay")) };
        Session->SetAnimationBlueprintState(BlueprintState);
        Session->SetStatusMessage(FText::FromString(TEXT("Manny animation blueprint selected as a source workspace asset")));
        return;
    }
    if (IsCommand(CommandId, TEXT("abp_generate")))
    {
        FCharacterCreatorAnimationBlueprintState BlueprintState = Session->GetAnimationBlueprintState();
        if (BlueprintState.SourceBlueprint.IsNull())
        {
            BlueprintState.SourceBlueprint = FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny"));
        }
        BlueprintState.GeneratedBlueprint = FSoftObjectPath(TEXT("/Game/CharacterCreator/Generated/Animation/ABP_SidekickGenerated.ABP_SidekickGenerated"));
        BlueprintState.bGenerated = true;
        Session->SetAnimationBlueprintState(BlueprintState);
        Session->SetStatusMessage(FText::FromString(TEXT("Sidekick animation blueprint profile generated")));
        return;
    }
    if (IsCommand(CommandId, TEXT("abp_clear")))
    {
        Session->SetAnimationBlueprintState(FCharacterCreatorAnimationBlueprintState());
        Session->SetStatusMessage(FText::FromString(TEXT("Animation blueprint source cleared")));
        return;
    }
    if (IsCommand(CommandId, TEXT("montage_ready")))
    {
        FCharacterCreatorMontageComboState MontageState;
        MontageState.MontagePath = FSoftObjectPath(TEXT("/Game/CharacterCreator/Generated/Animation/Montage_Combo.Montage_Combo"));
        MontageState.Sections = { FName(TEXT("Attack01")), FName(TEXT("Attack02")), FName(TEXT("Recover")) };
        MontageState.bAuthoringReady = true;
        Session->SetMontageComboState(MontageState);
        Session->SetStatusMessage(FText::FromString(TEXT("Combo source is ready for montage authoring")));
        return;
    }
    if (IsCommand(CommandId, TEXT("animation_set_build")))
    {
        Session->SetAnimationSetClip(FName(TEXT("Idle")), FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Animations/Manny/MM_Idle.MM_Idle")));
        Session->SetAnimationSetClip(FName(TEXT("Walk")), FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Animations/Manny/MM_Walk_Fwd.MM_Walk_Fwd")));
        Session->SetAnimationSetClip(FName(TEXT("Run")), FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Animations/Manny/MM_Run_Fwd.MM_Run_Fwd")));
        Session->SetStatusMessage(FText::FromString(TEXT("Locomotion animation set built from Manny source clips")));
        return;
    }
    if (IsCommand(CommandId, TEXT("retarget_rig")))
    {
        Session->SetAnimationRetargeter(FSoftObjectPath(TEXT("/Game/FreeAnimationsPack/Demo/Characters/Mannequins/Rigs/RTG_Mannequin.RTG_Mannequin")));
        return;
    }
    if (IsCommand(CommandId, TEXT("retarget_begin")))
    {
        Session->ExecuteAnimationRetarget();
        return;
    }
    if (IsCommand(CommandId, TEXT("retarget_clear")))
    {
        Session->SetAnimationRetargeter(FSoftObjectPath());
        return;
    }
    if (IsCommand(CommandId, TEXT("inspect_sidekick")))
    {
        Session->InspectSkeletons();
        return;
    }
    if (IsCommand(CommandId, TEXT("inspect_manny")))
    {
        Session->SetStatusMessage(FText::FromString(TEXT("Inspecting source skeleton: SK_Mannequin")));
    }
}
