#include "UI/CharacterCreatorWorkflowScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

#include "CharacterCreatorPreviewActor.h"
#include "UI/CharacterCreatorUIFramework.h"

namespace CharacterCreatorWorkflowUI
{
    const FCharacterCreatorStylePalette& Palette = FCharacterCreatorUIStyle::GetPalette();

    UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, const FVector2D& Size)
    {
        return FCharacterCreatorUIFactory::Place(Canvas, Widget, Position, Size);
    }

    UTextBlock* Label(UWidgetTree* Tree, const FString& Value, int32 FontSize, const FLinearColor& Color)
    {
        return FCharacterCreatorUIFactory::MakeLabel(Tree, Value, FontSize, Color);
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

    bool IsCommand(FName CommandId, const TCHAR* Expected)
    {
        return CommandId == FName(Expected);
    }
}

void UCharacterCreatorWorkflowScreenWidget::InitializeWithSession(UCharacterCreatorSession* InSession)
{
    Session = InSession;
}

void UCharacterCreatorWorkflowScreenWidget::InitializeWithPreviewActor(ACharacterCreatorPreviewActor* InPreviewActor)
{
    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.RemoveAll(this);
    }

    PreviewActor = InPreviewActor;
    if (PreviewActor && IsConstructed())
    {
        PreviewActor->OnPreviewStateChanged.AddUObject(this, &UCharacterCreatorWorkflowScreenWidget::ApplyPreviewState);
        ApplyPreviewRenderTarget();
        ApplyPreviewState(PreviewActor->GetPreviewState(), PreviewMessageForState(PreviewActor->GetPreviewState()));
    }
}

void UCharacterCreatorWorkflowScreenWidget::SetWorkflowScreen(ECharacterCreatorScreen InScreen)
{
    WorkflowScreen = InScreen;
}

void UCharacterCreatorWorkflowScreenWidget::FocusFirstControl()
{
    if (CommandButtons.Num() > 0 && CommandButtons[0])
    {
        UCharacterCreatorUIHelpers::FocusWidget(CommandButtons[0]);
    }
}

void UCharacterCreatorWorkflowScreenWidget::NativeConstruct()
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
        Session->OnAppearanceChanged.AddUObject(this, &UCharacterCreatorWorkflowScreenWidget::ApplyAppearance);
        ApplyAppearance(Session->GetAppearanceStateNative());
    }

    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.RemoveAll(this);
        PreviewActor->OnPreviewStateChanged.AddUObject(this, &UCharacterCreatorWorkflowScreenWidget::ApplyPreviewState);
        ApplyPreviewRenderTarget();
        ApplyPreviewState(PreviewActor->GetPreviewState(), PreviewMessageForState(PreviewActor->GetPreviewState()));
    }
}

void UCharacterCreatorWorkflowScreenWidget::NativeDestruct()
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

void UCharacterCreatorWorkflowScreenWidget::BuildLayout()
{
    using namespace CharacterCreatorWorkflowUI;

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = RootCanvas;

    AddPanel(WidgetTree, RootCanvas, TEXT("WorkflowBackground"), FVector2D::ZeroVector, FVector2D(1440.0f, 810.0f), Palette.Ink);
    AddPanel(WidgetTree, RootCanvas, TEXT("WorkflowTopBar"), FVector2D(0.0f, 0.0f), FVector2D(1440.0f, 64.0f), Palette.Surface);
    AddPanel(WidgetTree, RootCanvas, TEXT("WorkflowAccent"), FVector2D(0.0f, 60.0f), FVector2D(1440.0f, 4.0f), Palette.Gold);
    AddPanel(WidgetTree, RootCanvas, TEXT("WorkflowRail"), FVector2D(0.0f, 64.0f), FVector2D(220.0f, 746.0f), Palette.SurfaceMuted);
    AddPanel(WidgetTree, RootCanvas, TEXT("WorkflowWorkspace"), FVector2D(220.0f, 64.0f), FVector2D(760.0f, 746.0f), Palette.Ink);
    AddPanel(WidgetTree, RootCanvas, TEXT("WorkflowInspector"), FVector2D(980.0f, 64.0f), FVector2D(460.0f, 746.0f), Palette.SurfaceMuted);

    AddLabel(WidgetTree, RootCanvas, TEXT("WorkflowBrand"), TEXT("CHARACTER CREATOR"), FVector2D(28.0f, 17.0f), FVector2D(280.0f, 20.0f), 16, Palette.Gold);
    AddLabel(WidgetTree, RootCanvas, TEXT("WorkflowName"), GetWorkflowTitle().ToString().ToUpper(), FVector2D(28.0f, 39.0f), FVector2D(360.0f, 16.0f), 9, Palette.Muted);

    BuildNavigation(RootCanvas);
    BuildPreview(RootCanvas);
    BuildInspector(RootCanvas);
}

void UCharacterCreatorWorkflowScreenWidget::BuildNavigation(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorWorkflowUI;

    AddLabel(WidgetTree, Canvas, TEXT("WorkflowHeading"), TEXT("WORKFLOW"), FVector2D(28.0f, 94.0f), FVector2D(150.0f, 18.0f), 10, Palette.Muted);

    const TArray<TTuple<const TCHAR*, const TCHAR*>> NavigationEntries = {
        {TEXT("BODY / FACE"), TEXT("body")},
        {TEXT("OUTFIT / ARMOR"), TEXT("outfit")},
        {TEXT("HAIR / GROOMING"), TEXT("hair")},
        {TEXT("MATERIALS / COLOR"), TEXT("materials")},
        {TEXT("WEAPONS / IK"), TEXT("weapons")},
        {TEXT("ANIMATION"), TEXT("animation_overview")}
    };

    float Y = 122.0f;
    for (const TTuple<const TCHAR*, const TCHAR*>& NavigationEntry : NavigationEntries)
    {
        UCharacterCreatorCommandButtonWidget* Button = AddCommandButton(Canvas, NavigationEntry.Get<0>(), FName(NavigationEntry.Get<1>()), FVector2D(16.0f, Y), FVector2D(188.0f, 36.0f), ECharacterCreatorButtonStyle::Ghost);
        (void)Button;
        Y += 42.0f;
    }

    AddLabel(WidgetTree, Canvas, TEXT("WorkflowHint"), TEXT("Edits stay live until applied."), FVector2D(28.0f, 710.0f), FVector2D(170.0f, 36.0f), 9, Palette.Muted);
}

void UCharacterCreatorWorkflowScreenWidget::BuildPreview(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorWorkflowUI;

    AddLabel(WidgetTree, Canvas, TEXT("WorkflowTitle"), GetWorkflowTitle().ToString().ToUpper(), FVector2D(252.0f, 92.0f), FVector2D(500.0f, 30.0f), 22, Palette.Text);
    AddLabel(WidgetTree, Canvas, TEXT("WorkflowSubtitle"), GetWorkflowSubtitle().ToString(), FVector2D(252.0f, 128.0f), FVector2D(620.0f, 20.0f), 11, Palette.Muted);
    AddPanel(WidgetTree, Canvas, TEXT("WorkflowPreviewPanel"), FVector2D(252.0f, 178.0f), FVector2D(680.0f, 494.0f), Palette.Surface);
    AddLabel(WidgetTree, Canvas, TEXT("WorkflowPreviewLabel"), TEXT("LIVE PREVIEW  •  FRONT"), FVector2D(278.0f, 204.0f), FVector2D(260.0f, 18.0f), 10, Palette.Muted);

    PreviewStatusText = Label(WidgetTree, TEXT("LOADING PREVIEW"), 10, Palette.Gold);
    Place(Canvas, PreviewStatusText, FVector2D(278.0f, 226.0f), FVector2D(360.0f, 18.0f));

    PreviewImage = WidgetTree->ConstructWidget<UImage>();
    Place(Canvas, PreviewImage, FVector2D(496.0f, 244.0f), FVector2D(190.0f, 310.0f));
    AddLabel(WidgetTree, Canvas, TEXT("WorkflowPreviewMeta"), TEXT("SIDEKICK  •  LIVE LOADOUT PREVIEW"), FVector2D(278.0f, 620.0f), FVector2D(440.0f, 18.0f), 9, Palette.Gold);
}

void UCharacterCreatorWorkflowScreenWidget::BuildInspector(UCanvasPanel* Canvas)
{
    switch (WorkflowScreen)
    {
    case ECharacterCreatorScreen::OutfitAndArmor:
        BuildOutfitInspector(Canvas);
        break;
    case ECharacterCreatorScreen::HairAndGrooming:
        BuildHairInspector(Canvas);
        break;
    case ECharacterCreatorScreen::MaterialsAndColor:
        BuildMaterialsInspector(Canvas);
        break;
    case ECharacterCreatorScreen::WeaponsAndIK:
        BuildWeaponsInspector(Canvas);
        break;
    default:
        break;
    }

    using namespace CharacterCreatorWorkflowUI;
    AddPanel(WidgetTree, Canvas, TEXT("EditStatusPanel"), FVector2D(1012.0f, 520.0f), FVector2D(352.0f, 74.0f), Palette.Surface);
    AddLabel(WidgetTree, Canvas, TEXT("EditStatusLabel"), TEXT("EDIT STATE"), FVector2D(1030.0f, 534.0f), FVector2D(130.0f, 14.0f), 9, Palette.Muted);
    EditStatusText = Label(WidgetTree, TEXT("Live edit"), 11, Palette.Gold);
    Place(Canvas, EditStatusText, FVector2D(1030.0f, 554.0f), FVector2D(300.0f, 18.0f));

    UCharacterCreatorCommandButtonWidget* RevertButton = AddCommandButton(Canvas, TEXT("REVERT"), FName(TEXT("revert")), FVector2D(1012.0f, 620.0f), FVector2D(160.0f, 36.0f), ECharacterCreatorButtonStyle::Secondary);
    UCharacterCreatorCommandButtonWidget* ApplyButton = AddCommandButton(Canvas, TEXT("APPLY CHANGES"), FName(TEXT("apply")), FVector2D(1188.0f, 620.0f), FVector2D(176.0f, 36.0f), ECharacterCreatorButtonStyle::Accent);
    (void)RevertButton;
    (void)ApplyButton;
}

void UCharacterCreatorWorkflowScreenWidget::BuildOutfitInspector(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorWorkflowUI;
    AddLabel(WidgetTree, Canvas, TEXT("InspectorHeading"), TEXT("OUTFIT + ARMOR"), FVector2D(1012.0f, 96.0f), FVector2D(260.0f, 20.0f), 10, Palette.Muted);
    AddLabel(WidgetTree, Canvas, TEXT("InspectorHint"), TEXT("Select a modular Sidekick torso piece."), FVector2D(1012.0f, 124.0f), FVector2D(360.0f, 20.0f), 10, Palette.Muted);
    AddCommandButton(Canvas, TEXT("KNIGHT TORSO"), FName(TEXT("outfit_knight")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
    AddCommandButton(Canvas, TEXT("SCI-FI TORSO"), FName(TEXT("outfit_scifi")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
    AddCommandButton(Canvas, TEXT("REMOVE OUTFIT"), FName(TEXT("outfit_none")), FVector2D(1012.0f, 286.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Ghost);
    AddCommandButton(Canvas, TEXT("EQUIP LEGS + FEET"), FName(TEXT("clothing_lower_body")), FVector2D(1012.0f, 338.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
    AddLabel(WidgetTree, Canvas, TEXT("SelectionLabel"), TEXT("CURRENT SELECTION"), FVector2D(1012.0f, 360.0f), FVector2D(220.0f, 16.0f), 9, Palette.Muted);
    SelectionSummaryText = Label(WidgetTree, TEXT("None"), 11, Palette.Text);
    Place(Canvas, SelectionSummaryText, FVector2D(1012.0f, 382.0f), FVector2D(352.0f, 54.0f));
}

void UCharacterCreatorWorkflowScreenWidget::BuildHairInspector(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorWorkflowUI;
    AddLabel(WidgetTree, Canvas, TEXT("InspectorHeading"), TEXT("HAIR + GROOMING"), FVector2D(1012.0f, 96.0f), FVector2D(260.0f, 20.0f), 10, Palette.Muted);
    AddLabel(WidgetTree, Canvas, TEXT("InspectorHint"), TEXT("Choose a Sidekick hair mesh."), FVector2D(1012.0f, 124.0f), FVector2D(360.0f, 20.0f), 10, Palette.Muted);
    AddCommandButton(Canvas, TEXT("BASE HAIR 01"), FName(TEXT("hair_base_01")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
    AddCommandButton(Canvas, TEXT("BASE HAIR 02"), FName(TEXT("hair_base_02")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
    AddCommandButton(Canvas, TEXT("REMOVE HAIR"), FName(TEXT("hair_none")), FVector2D(1012.0f, 286.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Ghost);
    AddCommandButton(Canvas, TEXT("SHORT GROOMING PROFILE"), FName(TEXT("grooming_short")), FVector2D(1012.0f, 338.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
    AddLabel(WidgetTree, Canvas, TEXT("SelectionLabel"), TEXT("CURRENT SELECTION"), FVector2D(1012.0f, 360.0f), FVector2D(220.0f, 16.0f), 9, Palette.Muted);
    SelectionSummaryText = Label(WidgetTree, TEXT("None"), 11, Palette.Text);
    Place(Canvas, SelectionSummaryText, FVector2D(1012.0f, 382.0f), FVector2D(352.0f, 54.0f));
}

void UCharacterCreatorWorkflowScreenWidget::BuildMaterialsInspector(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorWorkflowUI;
    AddLabel(WidgetTree, Canvas, TEXT("InspectorHeading"), TEXT("MATERIALS + COLOR"), FVector2D(1012.0f, 96.0f), FVector2D(260.0f, 20.0f), 10, Palette.Muted);
    AddLabel(WidgetTree, Canvas, TEXT("InspectorHint"), TEXT("Apply a coordinated color palette."), FVector2D(1012.0f, 124.0f), FVector2D(360.0f, 20.0f), 10, Palette.Muted);
    AddCommandButton(Canvas, TEXT("EMBER / GOLD"), FName(TEXT("palette_ember")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Accent);
    AddCommandButton(Canvas, TEXT("OCEAN / STEEL"), FName(TEXT("palette_ocean")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
    AddCommandButton(Canvas, TEXT("FOREST / BRASS"), FName(TEXT("palette_forest")), FVector2D(1012.0f, 286.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
    AddLabel(WidgetTree, Canvas, TEXT("SelectionLabel"), TEXT("COLOR TARGETS"), FVector2D(1012.0f, 360.0f), FVector2D(220.0f, 16.0f), 9, Palette.Muted);
    SelectionSummaryText = Label(WidgetTree, TEXT("Skin / hair / primary / secondary"), 11, Palette.Text);
    Place(Canvas, SelectionSummaryText, FVector2D(1012.0f, 382.0f), FVector2D(352.0f, 54.0f));
}

void UCharacterCreatorWorkflowScreenWidget::BuildWeaponsInspector(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorWorkflowUI;
    AddLabel(WidgetTree, Canvas, TEXT("InspectorHeading"), TEXT("WEAPONS + IK"), FVector2D(1012.0f, 96.0f), FVector2D(260.0f, 20.0f), 10, Palette.Muted);
    AddLabel(WidgetTree, Canvas, TEXT("InspectorHint"), TEXT("Prepare the hand socket and IK pass."), FVector2D(1012.0f, 124.0f), FVector2D(360.0f, 20.0f), 10, Palette.Muted);
    AddCommandButton(Canvas, TEXT("EMPTY HAND SOCKET"), FName(TEXT("weapon_none")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
    AddCommandButton(Canvas, TEXT("ENABLE RIGHT-HAND IK"), FName(TEXT("ik_right")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
    AddCommandButton(Canvas, TEXT("ENABLE FOOT IK"), FName(TEXT("ik_foot")), FVector2D(1012.0f, 286.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
    AddLabel(WidgetTree, Canvas, TEXT("SelectionLabel"), TEXT("SOCKET STATUS"), FVector2D(1012.0f, 360.0f), FVector2D(220.0f, 16.0f), 9, Palette.Muted);
    SelectionSummaryText = Label(WidgetTree, TEXT("No weapon asset assigned"), 11, Palette.Text);
    Place(Canvas, SelectionSummaryText, FVector2D(1012.0f, 382.0f), FVector2D(352.0f, 54.0f));
}

UCharacterCreatorCommandButtonWidget* UCharacterCreatorWorkflowScreenWidget::AddCommandButton(UCanvasPanel* Canvas, const FString& LabelText, FName CommandId, const FVector2D& Position, const FVector2D& Size, ECharacterCreatorButtonStyle Style)
{
    if (!Canvas)
    {
        return nullptr;
    }

    UCharacterCreatorCommandButtonWidget* Button = FCharacterCreatorUIFactory::MakeCommandButton(WidgetTree, LabelText, CommandId, Style, 10);
    Button->OnCommand.AddUObject(this, &UCharacterCreatorWorkflowScreenWidget::HandleCommand);
    CommandButtons.Add(Button);
    CharacterCreatorWorkflowUI::Place(Canvas, Button, Position, Size);
    return Button;
}

void UCharacterCreatorWorkflowScreenWidget::ApplyAppearance(const FCharacterAppearanceState& NewAppearance)
{
    if (EditStatusText)
    {
        EditStatusText->SetText(NewAppearance.bHasUnsavedChanges
            ? FText::FromString(TEXT("LIVE EDIT • NOT APPLIED"))
            : FText::FromString(TEXT("APPLIED TO SESSION")));
        EditStatusText->SetColorAndOpacity(FSlateColor(NewAppearance.bHasUnsavedChanges ? CharacterCreatorWorkflowUI::Palette.Gold : CharacterCreatorWorkflowUI::Palette.Success));
    }

    if (SelectionSummaryText)
    {
        SelectionSummaryText->SetText(GetSelectionSummary(NewAppearance));
    }
}

void UCharacterCreatorWorkflowScreenWidget::ApplyPreviewRenderTarget()
{
    if (!PreviewActor || !PreviewImage)
    {
        return;
    }

    if (UTextureRenderTarget2D* RenderTarget = PreviewActor->GetPreviewRenderTarget())
    {
        FSlateBrush PreviewBrush;
        PreviewBrush.DrawAs = ESlateBrushDrawType::Image;
        PreviewBrush.SetResourceObject(RenderTarget);
        PreviewBrush.ImageSize = FVector2D(512.0f, 768.0f);
        PreviewBrush.TintColor = FSlateColor(FLinearColor::White);
        PreviewImage->SetBrush(PreviewBrush);
    }
}

void UCharacterCreatorWorkflowScreenWidget::ApplyPreviewState(ECharacterCreatorPreviewState NewState, const FText& Message)
{
    if (!PreviewStatusText)
    {
        return;
    }

    const FCharacterCreatorStylePalette& Palette = CharacterCreatorWorkflowUI::Palette;
    const FLinearColor StateColor = NewState == ECharacterCreatorPreviewState::Ready
        ? Palette.Success
        : NewState == ECharacterCreatorPreviewState::Failed
            ? Palette.Danger
            : Palette.Gold;
    PreviewStatusText->SetText(Message);
    PreviewStatusText->SetColorAndOpacity(FSlateColor(StateColor));
    ApplyPreviewRenderTarget();
}

void UCharacterCreatorWorkflowScreenWidget::ApplyMaterialPreset(const TArray<FLinearColor>& Colors)
{
    if (!Session || Colors.Num() < 4)
    {
        return;
    }

    Session->SetColorTarget(ECharacterCreatorColorTarget::Skin, Colors[0]);
    Session->SetColorTarget(ECharacterCreatorColorTarget::Hair, Colors[1]);
    Session->SetColorTarget(ECharacterCreatorColorTarget::PrimaryOutfit, Colors[2]);
    Session->SetColorTarget(ECharacterCreatorColorTarget::SecondaryOutfit, Colors[3]);
}

FText UCharacterCreatorWorkflowScreenWidget::GetWorkflowTitle() const
{
    switch (WorkflowScreen)
    {
    case ECharacterCreatorScreen::OutfitAndArmor:
        return FText::FromString(TEXT("OUTFIT + ARMOR"));
    case ECharacterCreatorScreen::HairAndGrooming:
        return FText::FromString(TEXT("HAIR + GROOMING"));
    case ECharacterCreatorScreen::MaterialsAndColor:
        return FText::FromString(TEXT("MATERIALS + COLOR"));
    case ECharacterCreatorScreen::WeaponsAndIK:
        return FText::FromString(TEXT("WEAPONS + IK"));
    default:
        return FText::FromString(TEXT("CHARACTER WORKSPACE"));
    }
}

FText UCharacterCreatorWorkflowScreenWidget::GetWorkflowSubtitle() const
{
    switch (WorkflowScreen)
    {
    case ECharacterCreatorScreen::OutfitAndArmor:
        return FText::FromString(TEXT("Layer modular Sidekick pieces over the base character."));
    case ECharacterCreatorScreen::HairAndGrooming:
        return FText::FromString(TEXT("Choose a hair asset while keeping the preview live."));
    case ECharacterCreatorScreen::MaterialsAndColor:
        return FText::FromString(TEXT("Tune a coordinated material palette before applying."));
    case ECharacterCreatorScreen::WeaponsAndIK:
        return FText::FromString(TEXT("Prepare socket and IK settings for the animation pass."));
    default:
        return FText::FromString(TEXT("Edit the active character state."));
    }
}

FText UCharacterCreatorWorkflowScreenWidget::GetSelectionSummary(const FCharacterAppearanceState& Appearance) const
{
    switch (WorkflowScreen)
    {
    case ECharacterCreatorScreen::OutfitAndArmor:
        return FText::FromString(GetAssetLabel(Appearance.Loadout.OutfitMesh));
    case ECharacterCreatorScreen::HairAndGrooming:
        return FText::FromString(GetAssetLabel(Appearance.Loadout.HairMesh));
    case ECharacterCreatorScreen::MaterialsAndColor:
        return FText::FromString(FString::Printf(TEXT("Skin %s  •  Hair %s"), *Appearance.SkinColor.ToFColor(true).ToHex(), *Appearance.HairColor.ToFColor(true).ToHex()));
    case ECharacterCreatorScreen::WeaponsAndIK:
        return FText::FromString(FString::Printf(TEXT("%s  •  Hand IK %s  •  Feet IK %s"),
            Appearance.Loadout.WeaponMesh.IsNull() ? TEXT("No weapon asset") : *GetAssetLabel(Appearance.Loadout.WeaponMesh),
            Appearance.IK.bRightHandIKEnabled ? TEXT("ON") : TEXT("OFF"),
            Appearance.IK.bFeetIKEnabled ? TEXT("ON") : TEXT("OFF")));
    default:
        return FText::FromString(TEXT("Ready"));
    }
}

FString UCharacterCreatorWorkflowScreenWidget::GetAssetLabel(const FSoftObjectPath& AssetPath) const
{
    return AssetPath.IsNull() ? TEXT("None selected") : AssetPath.GetAssetName();
}

void UCharacterCreatorWorkflowScreenWidget::HandleCommand(FName CommandId)
{
    if (!Session)
    {
        return;
    }

    if (IsCommand(CommandId, TEXT("body")))
    {
        Session->SetScreen(ECharacterCreatorScreen::CharacterCreator);
        return;
    }
    if (IsCommand(CommandId, TEXT("outfit")))
    {
        Session->SetScreen(ECharacterCreatorScreen::OutfitAndArmor);
        return;
    }
    if (IsCommand(CommandId, TEXT("hair")))
    {
        Session->SetScreen(ECharacterCreatorScreen::HairAndGrooming);
        return;
    }
    if (IsCommand(CommandId, TEXT("materials")))
    {
        Session->SetScreen(ECharacterCreatorScreen::MaterialsAndColor);
        return;
    }
    if (IsCommand(CommandId, TEXT("weapons")))
    {
        Session->SetScreen(ECharacterCreatorScreen::WeaponsAndIK);
        return;
    }
    if (IsCommand(CommandId, TEXT("animation_overview")))
    {
        Session->SetScreen(ECharacterCreatorScreen::AnimationOverview);
        return;
    }
    if (IsCommand(CommandId, TEXT("apply")))
    {
        Session->ApplyAppearanceChanges();
        return;
    }
    if (IsCommand(CommandId, TEXT("revert")))
    {
        Session->RevertAppearanceChanges();
        return;
    }

    if (IsCommand(CommandId, TEXT("outfit_knight")))
    {
        Session->SetLoadoutAsset(ECharacterCreatorLoadoutSlot::Outfit, FSoftObjectPath(TEXT("/Game/Synty/SidekickCharacters/Resources/Meshes/Outfits/Starter/SK_FANT_KNGT_17_10TORS_HU01.SK_FANT_KNGT_17_10TORS_HU01")));
        return;
    }
    if (IsCommand(CommandId, TEXT("outfit_scifi")))
    {
        Session->SetLoadoutAsset(ECharacterCreatorLoadoutSlot::Outfit, FSoftObjectPath(TEXT("/Game/Synty/SidekickCharacters/Resources/Meshes/Outfits/Starter/SK_SCFI_CIVL_09_10TORS_HU01.SK_SCFI_CIVL_09_10TORS_HU01")));
        return;
    }
    if (IsCommand(CommandId, TEXT("outfit_none")))
    {
        Session->SetLoadoutAsset(ECharacterCreatorLoadoutSlot::Outfit, FSoftObjectPath());
        return;
    }
    if (IsCommand(CommandId, TEXT("clothing_lower_body")))
    {
        const FSoftObjectPath LowerBody(TEXT("/Game/Synty/SidekickCharacters/Resources/Meshes/Outfits/Starter/SK_FANT_KNGT_17_10LEGS_HU01.SK_FANT_KNGT_17_10LEGS_HU01"));
        Session->SetClothingAsset(ECharacterCreatorClothingSlot::Legs, LowerBody);
        Session->SetClothingAsset(ECharacterCreatorClothingSlot::Feet, LowerBody);
        Session->SetStatusMessage(FText::FromString(TEXT("Lower-body clothing slots configured")));
        return;
    }
    if (IsCommand(CommandId, TEXT("hair_base_01")))
    {
        Session->SetLoadoutAsset(ECharacterCreatorLoadoutSlot::Hair, FSoftObjectPath(TEXT("/Game/Synty/SidekickCharacters/Resources/Meshes/Species/Humans/SK_HUMN_BASE_01_02HAIR_HU01.SK_HUMN_BASE_01_02HAIR_HU01")));
        return;
    }
    if (IsCommand(CommandId, TEXT("hair_base_02")))
    {
        Session->SetLoadoutAsset(ECharacterCreatorLoadoutSlot::Hair, FSoftObjectPath(TEXT("/Game/Synty/SidekickCharacters/Resources/Meshes/Species/Humans/SK_HUMN_BASE_02_02HAIR_HU01.SK_HUMN_BASE_02_02HAIR_HU01")));
        return;
    }
    if (IsCommand(CommandId, TEXT("hair_none")))
    {
        Session->SetLoadoutAsset(ECharacterCreatorLoadoutSlot::Hair, FSoftObjectPath());
        return;
    }
    if (IsCommand(CommandId, TEXT("grooming_short")))
    {
        Session->SetHairStyle(FName(TEXT("ShortClean")));
        Session->SetGroomingValue(ECharacterCreatorGroomingParameter::HairLength, 0.20f);
        Session->SetGroomingValue(ECharacterCreatorGroomingParameter::HairDensity, 0.78f);
        Session->SetSkinProfile(FName(TEXT("Natural")));
        Session->SetStatusMessage(FText::FromString(TEXT("Short grooming profile applied")));
        return;
    }
    if (IsCommand(CommandId, TEXT("weapon_none")))
    {
        Session->SetLoadoutAsset(ECharacterCreatorLoadoutSlot::Weapon, FSoftObjectPath());
        return;
    }
    if (IsCommand(CommandId, TEXT("palette_ember")))
    {
        ApplyMaterialPreset({
            FLinearColor(0.42f, 0.22f, 0.12f, 1.0f),
            FLinearColor(0.06f, 0.025f, 0.012f, 1.0f),
            FLinearColor(0.38f, 0.08f, 0.035f, 1.0f),
            FLinearColor(0.78f, 0.48f, 0.12f, 1.0f)
        });
        return;
    }
    if (IsCommand(CommandId, TEXT("palette_ocean")))
    {
        ApplyMaterialPreset({
            FLinearColor(0.28f, 0.20f, 0.15f, 1.0f),
            FLinearColor(0.015f, 0.03f, 0.05f, 1.0f),
            FLinearColor(0.04f, 0.18f, 0.36f, 1.0f),
            FLinearColor(0.22f, 0.55f, 0.75f, 1.0f)
        });
        return;
    }
    if (IsCommand(CommandId, TEXT("palette_forest")))
    {
        ApplyMaterialPreset({
            FLinearColor(0.30f, 0.20f, 0.12f, 1.0f),
            FLinearColor(0.035f, 0.045f, 0.025f, 1.0f),
            FLinearColor(0.08f, 0.25f, 0.15f, 1.0f),
            FLinearColor(0.55f, 0.42f, 0.12f, 1.0f)
        });
        return;
    }

    if (IsCommand(CommandId, TEXT("ik_right")))
    {
        Session->SetIKEnabled(ECharacterCreatorIKTarget::RightHand, true);
        Session->SetStatusMessage(FText::FromString(TEXT("Right-hand IK enabled for the animation workspace")));
        return;
    }
    if (IsCommand(CommandId, TEXT("ik_foot")))
    {
        Session->SetIKEnabled(ECharacterCreatorIKTarget::Feet, true);
        Session->SetStatusMessage(FText::FromString(TEXT("Feet IK enabled for the animation workspace")));
    }
}
