#include "UI/CharacterCreatorUtilityWorkspaceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

#include "CharacterCreatorPreviewActor.h"
#include "UI/CharacterCreatorSubsystem.h"
#include "UI/CharacterCreatorExportService.h"

#define IsCommand IsUtilityCommand
#define PreviewMessageForState UtilityPreviewMessageForState

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
    if (WorkspaceScreen == ECharacterCreatorScreen::AssetBrowser && Session && Session->GetAssetBrowserState().Entries.Num() == 0)
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            FCharacterCreatorImportProgress Progress;
            Subsystem->ScanAssetDirectory(TEXT("/Game/Synty/SidekickCharacters"), FString(), FString(), Progress);
        }
    }
    if (WidgetTree && !bLayoutBuilt)
    {
        BuildLayout();
        bLayoutBuilt = true;
    }
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

#if WITH_DEV_AUTOMATION_TESTS
bool UCharacterCreatorUtilityWorkspaceWidget::BuildForAutomation()
{
    if (!WidgetTree) return false;
    if (!bLayoutBuilt)
    {
        BuildLayout();
        bLayoutBuilt = true;
    }
    return RootCanvas != nullptr && CommandButtons.Num() > 0;
}

bool UCharacterCreatorUtilityWorkspaceWidget::ExecuteCommandForAutomation(FName CommandId)
{
    for (UCharacterCreatorCommandButtonWidget* Button : CommandButtons)
    {
        if (Button && Button->GetCommandId() == CommandId)
        {
            Button->OnCommand.Broadcast(CommandId);
            return true;
        }
    }
    return false;
}

bool UCharacterCreatorUtilityWorkspaceWidget::HasCommandForAutomation(FName CommandId) const
{
    for (const UCharacterCreatorCommandButtonWidget* Button : CommandButtons)
    {
        if (Button && Button->GetCommandId() == CommandId) return true;
    }
    return false;
}
#endif

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
        Session->OnStatusChanged.AddUObject(this, &UCharacterCreatorUtilityWorkspaceWidget::ApplyStatus);
        Session->OnProjectBrowserChanged.AddUObject(this, &UCharacterCreatorUtilityWorkspaceWidget::ApplyProjectBrowser);
        ApplyAppearance(Session->GetAppearanceStateNative());
        ApplyStatus(Session->GetStatusMessage());
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
        Session->OnStatusChanged.RemoveAll(this);
        Session->OnProjectBrowserChanged.RemoveAll(this);
    }
    if (PreviewActor)
    {
        PreviewActor->OnPreviewStateChanged.RemoveAll(this);
    }
    OnModalRequested.Clear();
    Super::NativeDestruct();
}

void UCharacterCreatorUtilityWorkspaceWidget::BuildLayout()
{
    using namespace CharacterCreatorUtilityUI;
    if (!RootCanvas)
    {
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
        WidgetTree->RootWidget = RootCanvas;
    }
    else
    {
        RootCanvas->ClearChildren();
    }
    CommandButtons.Reset();
    PreviewImage = nullptr;
    PreviewStatusText = nullptr;
    UtilitySummaryText = nullptr;
    EditStatusText = nullptr;
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
    if (WorkspaceScreen == ECharacterCreatorScreen::AssetBrowser && Session)
    {
        const FCharacterCreatorAssetBrowserState Browser = Session->GetAssetBrowserState();
        AssetLayoutSignature = Browser.CategoryFilter + (Browser.bFavoritesOnly ? TEXT("|favorites") : TEXT("|all"));
        for (const FCharacterCreatorAssetCatalogEntry& Entry : Browser.Entries)
        {
            AssetLayoutSignature += FString::Printf(TEXT("|%s:%d:%d"), *Entry.SourceFile, Entry.bSelected ? 1 : 0, Entry.bFavorite ? 1 : 0);
        }
    }
}

void UCharacterCreatorUtilityWorkspaceWidget::BuildNavigation(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorUtilityUI;
    AddLabel(WidgetTree, Canvas, TEXT("UtilityHeading"), TEXT("PRODUCTION"), FVector2D(28.0f, 94.0f), FVector2D(160.0f, 18.0f), 10, Palette.Muted);
    const TArray<TTuple<const TCHAR*, const TCHAR*>> Entries = {
        {TEXT("PROJECT BROWSER"), TEXT("projects")},
        {TEXT("BODY / FACE"), TEXT("body")},
        {TEXT("ANIMATION"), TEXT("animation")},
        {TEXT("PHYSICS"), TEXT("physics")},
        {TEXT("GAMEPLAY TEST"), TEXT("gameplay")},
        {TEXT("PREVIEW STUDIO"), TEXT("preview")},
        {TEXT("PORTRAIT STUDIO"), TEXT("portrait")},
        {TEXT("LOD / PERFORMANCE"), TEXT("lod")},
        {TEXT("ASSET BROWSER"), TEXT("assets")},
        {TEXT("IMPORT WIZARD"), TEXT("import")},
        {TEXT("SETTINGS"), TEXT("settings")},
        {TEXT("VALIDATION / EXPORT"), TEXT("validation")}
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

    if (WorkspaceScreen == ECharacterCreatorScreen::ProjectBrowser || WorkspaceScreen == ECharacterCreatorScreen::AssetBrowser || WorkspaceScreen == ECharacterCreatorScreen::ValidationExport)
    {
        PreviewImage->SetVisibility(ESlateVisibility::Collapsed);
        if (PreviewStatusText) PreviewStatusText->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (WorkspaceScreen == ECharacterCreatorScreen::ProjectBrowser)
    {
        AddLabel(WidgetTree, Canvas, TEXT("ProjectBrowserHeading"), TEXT("PROJECTS"), FVector2D(278.0f, 264.0f), FVector2D(180.0f, 18.0f), 10, Palette.Muted);
        const FCharacterCreatorProjectBrowserState Browser = Session ? Session->GetProjectBrowserState() : FCharacterCreatorProjectBrowserState();
        int32 Index = 0;
        for (const FCharacterCreatorProjectRecord& Project : Browser.Projects)
        {
            if (Index >= 5) break;
            const float RowY = 294.0f + static_cast<float>(Index) * 58.0f;
            const FString StateBadge = Project.SlotName == Browser.ActiveSlotName ? TEXT("ACTIVE") : Project.bRecoveryAvailable ? TEXT("RECOVERY") : TEXT("SAVED");
            AddLabel(WidgetTree, Canvas, FString::Printf(TEXT("Project_%d"), Index), Project.DisplayName.ToString().ToUpper(), FVector2D(278.0f, RowY), FVector2D(300.0f, 18.0f), 11, Palette.Text);
            AddLabel(WidgetTree, Canvas, FString::Printf(TEXT("ProjectMeta_%d"), Index), FString::Printf(TEXT("%s  •  %d assets  •  %d backups  •  %s"), *Project.LastModifiedUtc.ToString(), Project.AssetCount, Project.BackupCount, *StateBadge), FVector2D(278.0f, RowY + 20.0f), FVector2D(425.0f, 14.0f), 9, Project.bRecoveryAvailable ? Palette.Gold : Palette.Muted);
            AddCommandButton(Canvas, TEXT("OPEN"), FName(*FString::Printf(TEXT("project_open_%s"), *Project.SlotName)), FVector2D(710.0f, RowY - 4.0f), FVector2D(100.0f, 30.0f), ECharacterCreatorButtonStyle::Secondary);
            ++Index;
        }
        if (Browser.Projects.Num() == 0)
        {
            AddLabel(WidgetTree, Canvas, TEXT("ProjectEmpty"), TEXT("NO PROJECTS YET  •  CREATE A CHARACTER TO BEGIN"), FVector2D(278.0f, 330.0f), FVector2D(500.0f, 20.0f), 10, Palette.Gold);
        }
    }
    else if (WorkspaceScreen == ECharacterCreatorScreen::AssetBrowser)
    {
        const FCharacterCreatorAssetBrowserState Browser = Session ? Session->GetAssetBrowserState() : FCharacterCreatorAssetBrowserState();
        AddLabel(WidgetTree, Canvas, TEXT("AssetBrowserHeading"), FString::Printf(TEXT("ASSET LIBRARY  •  %s"), Browser.CategoryFilter.IsEmpty() ? TEXT("ALL TYPES") : *Browser.CategoryFilter.ToUpper()), FVector2D(278.0f, 250.0f), FVector2D(540.0f, 18.0f), 10, Palette.Muted);
        int32 VisibleIndex = 0;
        for (int32 EntryIndex = 0; EntryIndex < Browser.Entries.Num() && VisibleIndex < 5; ++EntryIndex)
        {
            const FCharacterCreatorAssetCatalogEntry& Entry = Browser.Entries[EntryIndex];
            if (Browser.bFavoritesOnly && !Entry.bFavorite) continue;
            const float RowY = 280.0f + static_cast<float>(VisibleIndex) * 66.0f;
            FCharacterCreatorUIFactory::AddPanel(WidgetTree, Canvas, FString::Printf(TEXT("AssetRow_%d"), VisibleIndex), FVector2D(278.0f, RowY), FVector2D(654.0f, 58.0f), Entry.SourceFile == Browser.SelectedAsset ? Palette.SurfaceRaised : Palette.Surface);
            if (Entry.AssetClass.Contains(TEXT("Texture")) && !Entry.ObjectPath.IsEmpty())
            {
                if (UTexture2D* Texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *Entry.ObjectPath)))
                {
                    UImage* Thumbnail = WidgetTree->ConstructWidget<UImage>();
                    FSlateBrush Brush;
                    Brush.SetResourceObject(Texture);
                    Brush.ImageSize = FVector2D(48.0f, 48.0f);
                    Thumbnail->SetBrush(Brush);
                    Place(Canvas, Thumbnail, FVector2D(284.0f, RowY + 5.0f), FVector2D(48.0f, 48.0f));
                }
            }
            const FString Compatibility = Entry.Compatibility == ECharacterCreatorAssetCompatibility::Compatible ? TEXT("COMPATIBLE")
                : Entry.Compatibility == ECharacterCreatorAssetCompatibility::Warning ? TEXT("WARNING")
                : Entry.Compatibility == ECharacterCreatorAssetCompatibility::Incompatible ? TEXT("INCOMPATIBLE") : TEXT("UNKNOWN");
            const FLinearColor CompatibilityColor = Entry.Compatibility == ECharacterCreatorAssetCompatibility::Compatible ? Palette.Success : Entry.Compatibility == ECharacterCreatorAssetCompatibility::Incompatible ? Palette.Danger : Palette.Gold;
            AddLabel(WidgetTree, Canvas, FString::Printf(TEXT("AssetName_%d"), VisibleIndex), FString::Printf(TEXT("%s%s"), Entry.bFavorite ? TEXT("★  ") : TEXT(""), *Entry.AssetName.ToUpper()), FVector2D(340.0f, RowY + 8.0f), FVector2D(300.0f, 16.0f), 10, Palette.Text);
            AddLabel(WidgetTree, Canvas, FString::Printf(TEXT("AssetMeta_%d"), VisibleIndex), FString::Printf(TEXT("%s  •  %s  •  %d deps"), Entry.AssetClass.IsEmpty() ? *Entry.Category : *Entry.AssetClass, *Compatibility, Entry.Dependencies.Num()), FVector2D(340.0f, RowY + 30.0f), FVector2D(360.0f, 14.0f), 8, CompatibilityColor);
            AddCommandButton(Canvas, Entry.bSelected ? TEXT("SELECTED") : TEXT("SELECT"), FName(*FString::Printf(TEXT("asset_select_%d"), EntryIndex)), FVector2D(718.0f, RowY + 12.0f), FVector2D(92.0f, 30.0f), Entry.bSelected ? ECharacterCreatorButtonStyle::Accent : ECharacterCreatorButtonStyle::Secondary);
            AddCommandButton(Canvas, Entry.bFavorite ? TEXT("UNFAVORITE") : TEXT("FAVORITE"), FName(*FString::Printf(TEXT("asset_favorite_%d"), EntryIndex)), FVector2D(818.0f, RowY + 12.0f), FVector2D(104.0f, 30.0f), ECharacterCreatorButtonStyle::Secondary);
            ++VisibleIndex;
        }
        if (VisibleIndex == 0)
        {
            AddLabel(WidgetTree, Canvas, TEXT("AssetEmpty"), TEXT("NO ASSETS MATCH THIS FILTER  •  REFRESH OR CHANGE TYPE"), FVector2D(278.0f, 330.0f), FVector2D(560.0f, 20.0f), 10, Palette.Gold);
        }
    }
    else if (WorkspaceScreen == ECharacterCreatorScreen::ValidationExport)
    {
        AddLabel(WidgetTree, Canvas, TEXT("ValidationDashboardHeading"), TEXT("HEALTH DASHBOARD"), FVector2D(278.0f, 264.0f), FVector2D(260.0f, 18.0f), 10, Palette.Muted);
        const TArray<FCharacterCreatorValidationIssue> Issues = Session && Session->GetTypedOuter<UCharacterCreatorSubsystem>()
            ? Session->GetTypedOuter<UCharacterCreatorSubsystem>()->GetLastValidationIssues()
            : TArray<FCharacterCreatorValidationIssue>();
        if (Issues.Num() == 0)
        {
            AddLabel(WidgetTree, Canvas, TEXT("ValidationEmpty"), TEXT("RUN VALIDATION TO CHECK MESH, MATERIAL, ANIMATION, AND LOADOUT HEALTH"), FVector2D(278.0f, 322.0f), FVector2D(570.0f, 40.0f), 10, Palette.Gold);
        }
        else
        {
            int32 Index = 0;
            for (const FCharacterCreatorValidationIssue& Issue : Issues)
            {
                if (Index >= 6) break;
                const FLinearColor IssueColor = Issue.Severity == ECharacterCreatorValidationSeverity::Error ? Palette.Danger : Issue.Severity == ECharacterCreatorValidationSeverity::Warning ? Palette.Gold : Palette.Success;
                AddLabel(WidgetTree, Canvas, FString::Printf(TEXT("ValidationIssue_%d"), Index), FString::Printf(TEXT("[%s] %s"), *Issue.Code.ToString().ToUpper(), *Issue.Message.ToString()), FVector2D(278.0f, 310.0f + static_cast<float>(Index) * 38.0f), FVector2D(580.0f, 30.0f), 9, IssueColor);
                ++Index;
            }
        }
    }
}

void UCharacterCreatorUtilityWorkspaceWidget::BuildInspector(UCanvasPanel* Canvas)
{
    using namespace CharacterCreatorUtilityUI;
    AddLabel(WidgetTree, Canvas, TEXT("UtilityInspectorHeading"), GetWorkspaceTitle().ToString().ToUpper(), FVector2D(1012.0f, 96.0f), FVector2D(350.0f, 20.0f), 10, Palette.Muted);
    AddLabel(WidgetTree, Canvas, TEXT("UtilityInspectorHint"), TEXT("Run a scoped production action."), FVector2D(1012.0f, 124.0f), FVector2D(360.0f, 20.0f), 10, Palette.Muted);

    switch (WorkspaceScreen)
    {
    case ECharacterCreatorScreen::ProjectBrowser:
        AddCommandButton(Canvas, TEXT("SAVE"), FName(TEXT("project_save")), FVector2D(1012.0f, 182.0f), FVector2D(170.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("SAVE AS"), FName(TEXT("project_save_as")), FVector2D(1194.0f, 182.0f), FVector2D(170.0f, 40.0f), ECharacterCreatorButtonStyle::Accent);
        AddCommandButton(Canvas, TEXT("NEW PROJECT"), FName(TEXT("project_new")), FVector2D(1012.0f, 234.0f), FVector2D(170.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("REFRESH"), FName(TEXT("project_refresh")), FVector2D(1194.0f, 234.0f), FVector2D(170.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("RENAME"), FName(TEXT("project_rename")), FVector2D(1012.0f, 286.0f), FVector2D(108.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("DUPLICATE"), FName(TEXT("project_duplicate")), FVector2D(1134.0f, 286.0f), FVector2D(108.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("DELETE"), FName(TEXT("project_delete")), FVector2D(1256.0f, 286.0f), FVector2D(108.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        if (Session && Session->GetProjectBrowserState().bAutosaveRecoveryAvailable)
        {
            AddCommandButton(Canvas, TEXT("RECOVER AUTOSAVE"), FName(TEXT("project_recover")), FVector2D(1012.0f, 338.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Accent);
        }
        break;
    case ECharacterCreatorScreen::PhysicsSetup:
        AddCommandButton(Canvas, TEXT("VALIDATE PHYSICS ASSET"), FName(TEXT("physics_validate")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("INSPECT COLLISION BODY"), FName(TEXT("physics_inspect")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::GameplayTest:
        AddCommandButton(Canvas, TEXT("START SESSION INPUT CHECK"), FName(TEXT("gameplay_start")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("COMPLETE INPUT CHECK"), FName(TEXT("gameplay_stop")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
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
        AddCommandButton(Canvas, TEXT("ESTIMATE LOD MEMORY"), FName(TEXT("lod_profile")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("VALIDATE SCREEN SIZE"), FName(TEXT("lod_validate")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::AssetBrowser:
        AddCommandButton(Canvas, TEXT("REFRESH SIDEKICK ASSETS"), FName(TEXT("assets_refresh_sidekick")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("REFRESH FAB ASSETS"), FName(TEXT("assets_refresh_fab")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("CYCLE TYPE FILTER"), FName(TEXT("assets_filter")), FVector2D(1012.0f, 286.0f), FVector2D(170.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("FAVORITES ONLY"), FName(TEXT("assets_favorites")), FVector2D(1194.0f, 286.0f), FVector2D(170.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::ImportWizard:
        AddCommandButton(Canvas, TEXT("VALIDATE CONTENT FOLDER"), FName(TEXT("import_validate")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("CHECK FAB ANIMATION PACK"), FName(TEXT("import_check_fab")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("IMPORT SELECTED ASSETS"), FName(TEXT("import_execute")), FVector2D(1012.0f, 286.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Accent);
        AddCommandButton(Canvas, TEXT("KEEP BOTH ON CONFLICT"), FName(TEXT("import_keep_both")), FVector2D(1012.0f, 338.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("OVERWRITE ON CONFLICT"), FName(TEXT("import_overwrite")), FVector2D(1012.0f, 390.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("REVIEW DEPENDENCIES"), FName(TEXT("import_dependencies")), FVector2D(1012.0f, 442.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("CANCEL ACTIVE IMPORT"), FName(TEXT("import_cancel")), FVector2D(1012.0f, 494.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("SET SOURCE FOLDER"), FName(TEXT("import_source")), FVector2D(1012.0f, 546.0f), FVector2D(170.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("SET DESTINATION"), FName(TEXT("import_destination")), FVector2D(1194.0f, 546.0f), FVector2D(170.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        FCharacterCreatorUIFactory::AddPanel(WidgetTree, Canvas, TEXT("ImportProgressPanel"), FVector2D(1012.0f, 594.0f), FVector2D(352.0f, 62.0f), Palette.Surface);
        AddLabel(WidgetTree, Canvas, TEXT("ImportProgressLabel"), TEXT("IMPORT PROGRESS / RESULT"), FVector2D(1030.0f, 604.0f), FVector2D(250.0f, 14.0f), 9, Palette.Muted);
        UtilitySummaryText = FCharacterCreatorUIFactory::MakeLabel(WidgetTree, TEXT("Ready to scan a source folder"), 10, Palette.Text);
        Place(Canvas, UtilitySummaryText, FVector2D(1030.0f, 624.0f), FVector2D(318.0f, 24.0f));
        break;
    case ECharacterCreatorScreen::Settings:
        AddCommandButton(Canvas, TEXT("TOGGLE GAMEPAD NAVIGATION"), FName(TEXT("settings_gamepad")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 36.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("TOGGLE HIGH CONTRAST"), FName(TEXT("settings_contrast")), FVector2D(1012.0f, 226.0f), FVector2D(352.0f, 36.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("TOGGLE REDUCED MOTION"), FName(TEXT("settings_motion")), FVector2D(1012.0f, 270.0f), FVector2D(352.0f, 36.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("CYCLE UI SCALE"), FName(TEXT("settings_scale")), FVector2D(1012.0f, 314.0f), FVector2D(352.0f, 36.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("SAVE PREFERENCES"), FName(TEXT("settings_save")), FVector2D(1012.0f, 358.0f), FVector2D(352.0f, 36.0f), ECharacterCreatorButtonStyle::Accent);
        AddCommandButton(Canvas, TEXT("RESTART ONBOARDING"), FName(TEXT("settings_onboarding")), FVector2D(1012.0f, 402.0f), FVector2D(352.0f, 36.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("TOGGLE AUTOSAVE / BACKUPS"), FName(TEXT("settings_backup")), FVector2D(1012.0f, 446.0f), FVector2D(352.0f, 36.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("TOGGLE PACKAGE EXPORT DEFAULT"), FName(TEXT("settings_export")), FVector2D(1012.0f, 490.0f), FVector2D(352.0f, 36.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("TOGGLE IMPORT OVERWRITE DEFAULT"), FName(TEXT("settings_import")), FVector2D(1012.0f, 534.0f), FVector2D(352.0f, 36.0f), ECharacterCreatorButtonStyle::Secondary);
        break;
    case ECharacterCreatorScreen::ValidationExport:
        AddCommandButton(Canvas, TEXT("RUN VALIDATION"), FName(TEXT("validation_run")), FVector2D(1012.0f, 182.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Primary);
        AddCommandButton(Canvas, TEXT("APPLY SAFE FIXES"), FName(TEXT("validation_fix_all")), FVector2D(1012.0f, 234.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("EXPORT FULL PACKAGE"), FName(TEXT("export_full_package")), FVector2D(1012.0f, 286.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Accent);
        AddCommandButton(Canvas, TEXT("EXPORT BLUEPRINT + DATA ASSET"), FName(TEXT("export_authoring")), FVector2D(1012.0f, 338.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("SHOW EXPORT HISTORY"), FName(TEXT("export_history")), FVector2D(1012.0f, 390.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("SET EXPORT DESTINATION"), FName(TEXT("export_destination")), FVector2D(1012.0f, 442.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("OPEN OUTPUT FOLDER"), FName(TEXT("export_open_folder")), FVector2D(1012.0f, 494.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        AddCommandButton(Canvas, TEXT("CANCEL QUEUED EXPORT"), FName(TEXT("export_cancel")), FVector2D(1012.0f, 546.0f), FVector2D(352.0f, 40.0f), ECharacterCreatorButtonStyle::Secondary);
        FCharacterCreatorUIFactory::AddPanel(WidgetTree, Canvas, TEXT("ExportProgressPanel"), FVector2D(1012.0f, 594.0f), FVector2D(352.0f, 62.0f), Palette.Surface);
        AddLabel(WidgetTree, Canvas, TEXT("ExportProgressLabel"), TEXT("EXPORT PROGRESS / RESULT"), FVector2D(1030.0f, 604.0f), FVector2D(250.0f, 14.0f), 9, Palette.Muted);
        UtilitySummaryText = FCharacterCreatorUIFactory::MakeLabel(WidgetTree, TEXT("Ready to validate and export"), 10, Palette.Text);
        Place(Canvas, UtilitySummaryText, FVector2D(1030.0f, 624.0f), FVector2D(318.0f, 24.0f));
        break;
    default:
        break;
    }

    if (WorkspaceScreen != ECharacterCreatorScreen::Settings && WorkspaceScreen != ECharacterCreatorScreen::ImportWizard && WorkspaceScreen != ECharacterCreatorScreen::ValidationExport)
    {
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
    if (WorkspaceScreen == ECharacterCreatorScreen::AssetBrowser)
    {
        FString NewSignature = NewAppearance.AssetBrowser.CategoryFilter + (NewAppearance.AssetBrowser.bFavoritesOnly ? TEXT("|favorites") : TEXT("|all"));
        for (const FCharacterCreatorAssetCatalogEntry& Entry : NewAppearance.AssetBrowser.Entries)
        {
            NewSignature += FString::Printf(TEXT("|%s:%d:%d"), *Entry.SourceFile, Entry.bSelected ? 1 : 0, Entry.bFavorite ? 1 : 0);
        }
        if (NewSignature != AssetLayoutSignature)
        {
            BuildLayout();
        }
    }
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

void UCharacterCreatorUtilityWorkspaceWidget::ApplyProjectBrowser(const FCharacterCreatorProjectBrowserState& NewState)
{
    if (WorkspaceScreen == ECharacterCreatorScreen::ProjectBrowser && RootCanvas)
    {
        BuildLayout();
    }
}

void UCharacterCreatorUtilityWorkspaceWidget::ApplyStatus(const FText& NewStatus)
{
    if (UtilitySummaryText && !NewStatus.IsEmpty())
    {
        UtilitySummaryText->SetText(NewStatus);
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
    case ECharacterCreatorScreen::ProjectBrowser: return FText::FromString(TEXT("PROJECT / CHARACTER BROWSER"));
    case ECharacterCreatorScreen::PhysicsSetup: return FText::FromString(TEXT("PHYSICS SETUP"));
    case ECharacterCreatorScreen::GameplayTest: return FText::FromString(TEXT("GAMEPLAY TEST"));
    case ECharacterCreatorScreen::PreviewStudio: return FText::FromString(TEXT("PREVIEW STUDIO"));
    case ECharacterCreatorScreen::PortraitStudio: return FText::FromString(TEXT("PORTRAIT STUDIO"));
    case ECharacterCreatorScreen::LODPerformance: return FText::FromString(TEXT("LOD + PERFORMANCE"));
    case ECharacterCreatorScreen::AssetBrowser: return FText::FromString(TEXT("ASSET BROWSER"));
    case ECharacterCreatorScreen::ImportWizard: return FText::FromString(TEXT("IMPORT WIZARD"));
    case ECharacterCreatorScreen::Settings: return FText::FromString(TEXT("SETTINGS"));
    case ECharacterCreatorScreen::ValidationExport: return FText::FromString(TEXT("VALIDATION + EXPORT"));
    default: return FText::FromString(TEXT("PRODUCTION WORKSPACE"));
    }
}

FText UCharacterCreatorUtilityWorkspaceWidget::GetWorkspaceSubtitle() const
{
    switch (WorkspaceScreen)
    {
    case ECharacterCreatorScreen::ProjectBrowser: return FText::FromString(TEXT("Open, restore, and organize character projects and autosaves."));
    case ECharacterCreatorScreen::ImportWizard: return FText::FromString(TEXT("Validate extracted Content assets before adding them to the project."));
    case ECharacterCreatorScreen::GameplayTest: return FText::FromString(TEXT("Records creator-session input checks; it does not launch a separate playable level."));
    case ECharacterCreatorScreen::LODPerformance: return FText::FromString(TEXT("Estimates asset memory and validates configured LOD thresholds; use packaged profiling for frame timing."));
    case ECharacterCreatorScreen::ValidationExport: return FText::FromString(TEXT("Resolve blockers and generate a complete Unreal-ready delivery set."));
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
    case ECharacterCreatorScreen::GameplayTest:
        return Appearance.PreviewTesting.Gameplay.LastResult.IsEmpty()
            ? FText::FromString(TEXT("Gameplay test idle • movement and combat actions ready"))
            : Appearance.PreviewTesting.Gameplay.LastResult;
    case ECharacterCreatorScreen::PortraitStudio:
        return Appearance.PreviewTesting.Portrait.bCaptureReady
            ? FText::FromString(FString::Printf(TEXT("Portrait ready • %dx%d %s"), Appearance.PreviewTesting.Portrait.Width, Appearance.PreviewTesting.Portrait.Height, *Appearance.PreviewTesting.Portrait.Format.ToString()))
            : FText::FromString(TEXT("Portrait capture not prepared"));
    case ECharacterCreatorScreen::AssetBrowser:
        return FText::FromString(FString::Printf(TEXT("%d assets visible • %s"), Appearance.AssetBrowser.FilteredCount, Appearance.AssetBrowser.bCanImport ? TEXT("import ready") : TEXT("no compatible selection")));
    case ECharacterCreatorScreen::ImportWizard:
        return FText::FromString(TEXT("FAB pack available under /Game/FreeAnimationsPack"));
    case ECharacterCreatorScreen::ProjectBrowser:
        return FText::FromString(FString::Printf(TEXT("%d projects available • selected project: %s"), Session ? Session->GetProjectBrowserState().Projects.Num() : 0, Session ? *Session->GetProjectBrowserState().SelectedSlotName : TEXT("none")));
    case ECharacterCreatorScreen::ValidationExport:
        return FText::FromString(TEXT("Run validation to see blockers, warnings, safe fixes, and export history."));
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
    if (IsCommand(CommandId, TEXT("projects"))) { Session->SetScreen(ECharacterCreatorScreen::ProjectBrowser); return; }
    if (IsCommand(CommandId, TEXT("animation"))) { Session->SetScreen(ECharacterCreatorScreen::AnimationOverview); return; }
    if (IsCommand(CommandId, TEXT("physics"))) { Session->SetScreen(ECharacterCreatorScreen::PhysicsSetup); return; }
    if (IsCommand(CommandId, TEXT("gameplay"))) { Session->SetScreen(ECharacterCreatorScreen::GameplayTest); return; }
    if (IsCommand(CommandId, TEXT("preview"))) { Session->SetScreen(ECharacterCreatorScreen::PreviewStudio); return; }
    if (IsCommand(CommandId, TEXT("portrait"))) { Session->SetScreen(ECharacterCreatorScreen::PortraitStudio); return; }
    if (IsCommand(CommandId, TEXT("lod"))) { Session->SetScreen(ECharacterCreatorScreen::LODPerformance); return; }
    if (IsCommand(CommandId, TEXT("assets"))) { Session->SetScreen(ECharacterCreatorScreen::AssetBrowser); return; }
    if (IsCommand(CommandId, TEXT("import"))) { Session->SetScreen(ECharacterCreatorScreen::ImportWizard); return; }
    if (IsCommand(CommandId, TEXT("settings"))) { Session->SetScreen(ECharacterCreatorScreen::Settings); return; }
    if (IsCommand(CommandId, TEXT("validation"))) { Session->SetScreen(ECharacterCreatorScreen::ValidationExport); return; }
    if (IsCommand(CommandId, TEXT("apply"))) { Session->ApplyAppearanceChanges(); return; }
    if (IsCommand(CommandId, TEXT("revert"))) { Session->RevertAppearanceChanges(); return; }

    if (CommandId.ToString().StartsWith(TEXT("project_open_")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            if (!Subsystem->SelectProject(CommandId.ToString().RightChop(13)) && Session->GetProjectBrowserState().bUnsavedConfirmationRequired)
            {
                OnModalRequested.Broadcast(FName(TEXT("unsaved_project")));
            }
        }
        return;
    }
    if (IsCommand(CommandId, TEXT("project_refresh")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>()) Subsystem->RefreshProjectBrowser();
        Session->SetStatusMessage(FText::FromString(TEXT("Project browser refreshed")));
        return;
    }
    if (IsCommand(CommandId, TEXT("project_new")))
    {
        OnModalRequested.Broadcast(FName(TEXT("project_new")));
        return;
    }
    if (IsCommand(CommandId, TEXT("project_save")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>()) Subsystem->SaveCurrentProject();
        return;
    }
    if (IsCommand(CommandId, TEXT("project_save_as")) || IsCommand(CommandId, TEXT("project_rename")) || IsCommand(CommandId, TEXT("project_duplicate")) || IsCommand(CommandId, TEXT("project_delete")) || IsCommand(CommandId, TEXT("project_recover")))
    {
        OnModalRequested.Broadcast(CommandId);
        return;
    }

    if (IsCommand(CommandId, TEXT("camera_front")) && PreviewActor)
    {
        FCharacterCreatorPreviewStudioState StudioState = Session->GetPreviewStudioState();
        StudioState.CameraMode = FName(TEXT("Front"));
        Session->SetPreviewStudioState(StudioState);
        PreviewActor->SetCameraMode(ECharacterCreatorPreviewCameraMode::Front);
        Session->SetStatusMessage(FText::FromString(TEXT("Preview camera set to front")));
        return;
    }
    if (IsCommand(CommandId, TEXT("camera_three_quarter")) && PreviewActor)
    {
        FCharacterCreatorPreviewStudioState StudioState = Session->GetPreviewStudioState();
        StudioState.CameraMode = FName(TEXT("ThreeQuarter"));
        Session->SetPreviewStudioState(StudioState);
        PreviewActor->SetCameraMode(ECharacterCreatorPreviewCameraMode::ThreeQuarter);
        Session->SetStatusMessage(FText::FromString(TEXT("Preview camera set to three-quarter")));
        return;
    }
    if (IsCommand(CommandId, TEXT("portrait_capture")) && PreviewActor)
    {
        const FString OutputPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CharacterCreator"), TEXT("Portraits"), TEXT("ActiveCharacter.png"));
        FCharacterCreatorPreviewStudioState StudioState = Session->GetPreviewStudioState();
        StudioState.CameraMode = FName(TEXT("Portrait"));
        Session->SetPreviewStudioState(StudioState);
        PreviewActor->SetCameraMode(ECharacterCreatorPreviewCameraMode::Portrait);
        FString CaptureError;
        if (!PreviewActor->CapturePortrait(OutputPath, 1024, 1024, CaptureError))
        {
            Session->SetStatusMessage(FText::FromString(FString::Printf(TEXT("Portrait capture failed: %s"), *CaptureError)));
        }
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
        const bool bAssetLoaded = PhysicsState.PhysicsAsset.TryLoad() != nullptr;
        PhysicsState.bValidated = bAssetLoaded;
        Session->SetPhysicsSetup(PhysicsState);
        Session->SetStatusMessage(FText::FromString(bAssetLoaded ? TEXT("Physics asset loaded; collision profile is ready for inspection") : TEXT("Physics validation failed: the configured asset could not be loaded")));
        return;
    }
    if (IsCommand(CommandId, TEXT("portrait_lighting")))
    {
        FCharacterCreatorPreviewStudioState StudioState = Session->GetPreviewStudioState();
        StudioState.LightingProfile = FName(TEXT("Dramatic"));
        Session->SetPreviewStudioState(StudioState);
        Session->SetStatusMessage(FText::FromString(TEXT("Dramatic portrait lighting enabled")));
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
            Subsystem->ScanAssetDirectory(Session->GetSettings().ImportSourceDirectory, FString(), FString(), Progress);
            return;
        }
    }

    if (IsCommand(CommandId, TEXT("import_keep_both")) || IsCommand(CommandId, TEXT("import_overwrite")) || IsCommand(CommandId, TEXT("import_execute")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            FCharacterCreatorImportOptions Options;
            FCharacterCreatorAssetBrowserState Browser = Session->GetAssetBrowserState();
            Options.DestinationContentDirectory = Session->GetSettings().ImportDestinationDirectory;
            Options.bCopyDependencies = true;
            Options.bOverwriteExisting = Session->GetSettings().bImportOverwrite || IsCommand(CommandId, TEXT("import_overwrite"));
            Options.ConflictResolution = IsCommand(CommandId, TEXT("import_keep_both")) ? ECharacterCreatorImportConflictResolution::KeepBoth : IsCommand(CommandId, TEXT("import_overwrite")) ? ECharacterCreatorImportConflictResolution::Overwrite : Browser.ConflictResolution;
            if (IsCommand(CommandId, TEXT("import_execute")))
            {
                Subsystem->StartImportSelectedAssets(Options);
            }
            else
            {
                Browser.ConflictResolution = Options.ConflictResolution;
                Session->SetAssetBrowserState(Browser);
                Session->SetStatusMessage(Options.ConflictResolution == ECharacterCreatorImportConflictResolution::KeepBoth ? FText::FromString(TEXT("Conflict policy set to keep both")) : FText::FromString(TEXT("Conflict policy set to overwrite")));
            }
        }
        return;
    }
    if (IsCommand(CommandId, TEXT("import_cancel")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            if (!Subsystem->CancelImport()) Session->SetStatusMessage(FText::FromString(TEXT("No import operation is running.")));
        }
        return;
    }
    if (IsCommand(CommandId, TEXT("import_source")) || IsCommand(CommandId, TEXT("import_destination")) || IsCommand(CommandId, TEXT("export_destination")))
    {
        OnModalRequested.Broadcast(CommandId);
        return;
    }
    if (IsCommand(CommandId, TEXT("import_dependencies")))
    {
        int32 WarningCount = 0;
        for (const FCharacterCreatorAssetCatalogEntry& Entry : Session->GetAssetBrowserState().Entries) WarningCount += Entry.DependencyWarnings.Num();
        Session->SetStatusMessage(FText::FromString(FString::Printf(TEXT("Dependency review: %d warnings across selected assets"), WarningCount)));
        return;
    }

    if (IsCommand(CommandId, TEXT("assets_refresh_sidekick")) || IsCommand(CommandId, TEXT("assets_refresh_fab")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            const FString Directory = IsCommand(CommandId, TEXT("assets_refresh_fab"))
                ? TEXT("/Game/FreeAnimationsPack")
                : TEXT("/Game/Synty/SidekickCharacters");
            FCharacterCreatorImportProgress Progress;
            Subsystem->ScanAssetDirectory(Directory, FString(), FString(), Progress);
            return;
        }
    }
    if (CommandId.ToString().StartsWith(TEXT("asset_select_")) || CommandId.ToString().StartsWith(TEXT("asset_favorite_")))
    {
        const bool bFavorite = CommandId.ToString().StartsWith(TEXT("asset_favorite_"));
        const int32 Index = FCString::Atoi(*CommandId.ToString().RightChop(bFavorite ? 15 : 13));
        const FCharacterCreatorAssetBrowserState Browser = Session->GetAssetBrowserState();
        if (Browser.Entries.IsValidIndex(Index))
        {
            if (bFavorite) Session->ToggleBrowserAssetFavorite(Browser.Entries[Index].SourceFile);
            else Session->ToggleBrowserAssetSelection(Browser.Entries[Index].SourceFile);
        }
        return;
    }
    if (IsCommand(CommandId, TEXT("assets_favorites")))
    {
        FCharacterCreatorAssetBrowserState Browser = Session->GetAssetBrowserState();
        Browser.bFavoritesOnly = !Browser.bFavoritesOnly;
        Session->SetAssetBrowserState(Browser);
        return;
    }
    if (IsCommand(CommandId, TEXT("assets_filter")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            const FCharacterCreatorAssetBrowserState Current = Session->GetAssetBrowserState();
            const TArray<FString> Filters = { FString(), TEXT("SkeletalMesh"), TEXT("AnimSequence"), TEXT("Texture2D") };
            int32 FilterIndex = Filters.IndexOfByKey(Current.CategoryFilter);
            FilterIndex = (FilterIndex + 1) % Filters.Num();
            const FString Directory = Session->GetImportProgress().SourceDirectory.IsEmpty() ? Session->GetSettings().ImportSourceDirectory : Session->GetImportProgress().SourceDirectory;
            FCharacterCreatorImportProgress Progress;
            Subsystem->ScanAssetDirectory(Directory, Current.SearchQuery, Filters[FilterIndex], Progress);
        }
        return;
    }

    if (IsCommand(CommandId, TEXT("validation_run")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>()) Subsystem->RunValidation();
        return;
    }
    if (IsCommand(CommandId, TEXT("validation_fix_all")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            const int32 FixedCount = Subsystem->ApplyAllValidationFixes();
            Session->SetStatusMessage(FText::FromString(FString::Printf(TEXT("Applied %d safe validation fixes"), FixedCount)));
        }
        return;
    }
    if (IsCommand(CommandId, TEXT("export_full_package")) || IsCommand(CommandId, TEXT("export_authoring")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            FCharacterCreatorExportProfile Profile;
            Profile.bIncludeMetadata = Session->GetSettings().bExportManifest;
            Profile.bGenerateBlueprint = Session->GetSettings().bExportBlueprint;
            Profile.bGenerateDataAsset = Session->GetSettings().bExportDataAsset;
            Profile.bGeneratePackage = IsCommand(CommandId, TEXT("export_full_package")) && Session->GetSettings().bExportPackage;
            Subsystem->StartExportCurrentDeliverables(Profile, FString());
        }
        return;
    }
    if (IsCommand(CommandId, TEXT("export_cancel")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>())
        {
            if (!Subsystem->CancelExport())
            {
                Session->SetStatusMessage(FText::FromString(TEXT("No queued export is available to cancel.")));
            }
        }
        return;
    }
    if (IsCommand(CommandId, TEXT("export_history")))
    {
        Session->SetStatusMessage(FText::FromString(FString::Printf(TEXT("Export history contains %d entries"), Session->GetExportHistory().Num())));
        return;
    }
    if (IsCommand(CommandId, TEXT("export_open_folder")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>()) Subsystem->OpenExportOutputFolder();
        return;
    }

    const TCHAR* Message = TEXT("Production action ready");
    if (IsCommand(CommandId, TEXT("gameplay_start")))
    {
        Session->StartGameplayTest();
        Session->RecordGameplayAction(FName(TEXT("Start")));
        return;
    }
    if (IsCommand(CommandId, TEXT("gameplay_stop")))
    {
        Session->RecordGameplayAction(FName(TEXT("Stop")));
        Session->StopGameplayTest(true);
        return;
    }
    if (IsCommand(CommandId, TEXT("settings_gamepad")) || IsCommand(CommandId, TEXT("settings_contrast")) || IsCommand(CommandId, TEXT("settings_motion")) || IsCommand(CommandId, TEXT("settings_scale")) || IsCommand(CommandId, TEXT("settings_backup")) || IsCommand(CommandId, TEXT("settings_export")) || IsCommand(CommandId, TEXT("settings_import")))
    {
        FCharacterCreatorSettings Settings = Session->GetSettings();
        if (IsCommand(CommandId, TEXT("settings_gamepad"))) Settings.bGamepadEnabled = !Settings.bGamepadEnabled;
        if (IsCommand(CommandId, TEXT("settings_contrast"))) Settings.bHighContrast = !Settings.bHighContrast;
        if (IsCommand(CommandId, TEXT("settings_motion"))) Settings.bReducedMotion = !Settings.bReducedMotion;
        if (IsCommand(CommandId, TEXT("settings_scale"))) Settings.UIScale = Settings.UIScale >= 1.25f ? 0.85f : Settings.UIScale + 0.10f;
        if (IsCommand(CommandId, TEXT("settings_backup"))) Settings.bAutosaveEnabled = !Settings.bAutosaveEnabled;
        if (IsCommand(CommandId, TEXT("settings_export"))) Settings.bExportPackage = !Settings.bExportPackage;
        if (IsCommand(CommandId, TEXT("settings_import"))) Settings.bImportOverwrite = !Settings.bImportOverwrite;
        Session->SetSettings(Settings);
        Message = TEXT("Preferences updated");
    }
    if (IsCommand(CommandId, TEXT("settings_save")))
    {
        if (UCharacterCreatorSubsystem* Subsystem = Session->GetTypedOuter<UCharacterCreatorSubsystem>()) Subsystem->SavePreferences();
        return;
    }
    if (IsCommand(CommandId, TEXT("settings_onboarding")))
    {
        Session->ResetOnboarding();
        Session->SetScreen(ECharacterCreatorScreen::Dashboard);
        return;
    }
    Session->SetStatusMessage(FText::FromString(Message));
}

#undef IsCommand
#undef PreviewMessageForState
