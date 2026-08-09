#pragma once

#include "CoreMinimal.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/CharacterCreatorSession.h"
#include "CharacterCreatorUIFramework.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UTextBlock;
class UUserWidget;
class UWidget;
class UWidgetTree;
class UCharacterCreatorModalManager;

UENUM(BlueprintType)
enum class ECharacterCreatorPanelStyle : uint8
{
    Base,
    Raised,
    Muted,
    Overlay
};

UENUM(BlueprintType)
enum class ECharacterCreatorButtonStyle : uint8
{
    Primary,
    Secondary,
    Ghost,
    Accent
};

USTRUCT(BlueprintType)
struct THREEDCHARACTER_API FCharacterCreatorStylePalette
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Style")
    FLinearColor Ink = FLinearColor(0.027f, 0.063f, 0.086f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Style")
    FLinearColor Surface = FLinearColor(0.055f, 0.098f, 0.129f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Style")
    FLinearColor SurfaceRaised = FLinearColor(0.082f, 0.145f, 0.188f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Style")
    FLinearColor SurfaceMuted = FLinearColor(0.043f, 0.086f, 0.118f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Style")
    FLinearColor Blue = FLinearColor(0.090f, 0.470f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Style")
    FLinearColor Gold = FLinearColor(0.790f, 0.655f, 0.365f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Style")
    FLinearColor Text = FLinearColor(0.910f, 0.940f, 0.950f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Style")
    FLinearColor Muted = FLinearColor(0.550f, 0.650f, 0.710f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Style")
    FLinearColor Success = FLinearColor(0.300f, 0.750f, 0.560f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Style")
    FLinearColor Danger = FLinearColor(0.820f, 0.360f, 0.390f, 1.0f);
};

UCLASS(BlueprintType)
class THREEDCHARACTER_API UCharacterCreatorPanelWidget : public UBorder
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Style")
    void SetPanelStyle(ECharacterCreatorPanelStyle Style);

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Style")
    void SetPanelColor(const FLinearColor& Color);
};

UCLASS(BlueprintType)
class THREEDCHARACTER_API UCharacterCreatorButtonWidget : public UButton
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Style")
    void SetButtonStyle(ECharacterCreatorButtonStyle Style);
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterCreatorCommand, FName);

UCLASS(BlueprintType)
class THREEDCHARACTER_API UCharacterCreatorCommandButtonWidget : public UCharacterCreatorButtonWidget
{
    GENERATED_BODY()

public:
    UCharacterCreatorCommandButtonWidget(const FObjectInitializer& ObjectInitializer);

    void SetCommandId(FName InCommandId) { CommandId = InCommandId; }

    FName GetCommandId() const { return CommandId; }

    FOnCharacterCreatorCommand OnCommand;

private:
    UFUNCTION()
    void HandleClicked();

    FName CommandId;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCharacterCreatorParameterValueChanged, ECharacterCreatorParameter, float);

UCLASS(BlueprintType)
class THREEDCHARACTER_API UCharacterCreatorSliderWidget : public USlider
{
    GENERATED_BODY()

public:
    UCharacterCreatorSliderWidget(const FObjectInitializer& ObjectInitializer);

    void SetParameter(ECharacterCreatorParameter InParameter) { Parameter = InParameter; }
    ECharacterCreatorParameter GetParameter() const { return Parameter; }

    FOnCharacterCreatorParameterValueChanged OnParameterValueChanged;

private:
    UFUNCTION()
    void HandleValueChanged(float NewValue);

    ECharacterCreatorParameter Parameter = ECharacterCreatorParameter::Height;
};

UCLASS(BlueprintType)
class THREEDCHARACTER_API UCharacterCreatorTabButtonWidget : public UCharacterCreatorButtonWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Navigation")
    void SetTabId(FName InTabId) { TabId = InTabId; }

    UFUNCTION(BlueprintPure, Category = "Character Creator|Navigation")
    FName GetTabId() const { return TabId; }

    UFUNCTION(BlueprintCallable, Category = "Character Creator|Navigation")
    void SetSelected(bool bInSelected);

private:
    FName TabId;
    bool bSelected = false;
};

UCLASS(BlueprintType)
class THREEDCHARACTER_API UCharacterCreatorModalWidget : public UBorder
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Modal")
    void SetModalOpen(bool bOpen);
};

UCLASS(BlueprintType)
class THREEDCHARACTER_API UCharacterCreatorModalManager : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UUserWidget* InHostWidget);
    bool OpenModal(UCharacterCreatorModalWidget* Modal, UWidget* ReturnFocusWidget);
    bool CloseModal(UCharacterCreatorModalWidget* Modal);
    bool CloseTopModal();
    void CloseAllModals();

    UFUNCTION(BlueprintPure, Category = "Character Creator|Modal")
    bool HasOpenModal() const { return ModalStack.Num() > 0; }

    UFUNCTION(BlueprintPure, Category = "Character Creator|Modal")
    int32 GetModalDepth() const { return ModalStack.Num(); }

private:
    void RestoreFocusForIndex(int32 Index);

    UPROPERTY()
    TObjectPtr<UUserWidget> HostWidget;

    UPROPERTY()
    TArray<TObjectPtr<UCharacterCreatorModalWidget>> ModalStack;

    TArray<TWeakObjectPtr<UWidget>> FocusStack;
};

struct THREEDCHARACTER_API FCharacterCreatorUIStyle
{
    static const FCharacterCreatorStylePalette& GetPalette();
};

struct THREEDCHARACTER_API FCharacterCreatorUIFactory
{
    static UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, const FVector2D& Size);
    static UTextBlock* MakeLabel(UWidgetTree* Tree, const FString& Value, int32 FontSize, const FLinearColor& Color);
    static UCharacterCreatorPanelWidget* MakePanel(UWidgetTree* Tree, const FLinearColor& Color);
    static UCharacterCreatorButtonWidget* MakeButton(UWidgetTree* Tree, const FString& Value, ECharacterCreatorButtonStyle Style, int32 FontSize = 12);
    static UCharacterCreatorCommandButtonWidget* MakeCommandButton(UWidgetTree* Tree, const FString& Value, FName CommandId, ECharacterCreatorButtonStyle Style, int32 FontSize = 12);
    static UCharacterCreatorSliderWidget* MakeSlider(UWidgetTree* Tree, ECharacterCreatorParameter Parameter, float Value);
    static void AddLabel(UWidgetTree* Tree, UCanvasPanel* Canvas, const FString& Name, const FString& Value, const FVector2D& Position, const FVector2D& Size, int32 FontSize, const FLinearColor& Color);
    static void AddPanel(UWidgetTree* Tree, UCanvasPanel* Canvas, const FString& Name, const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color);
};

UCLASS()
class THREEDCHARACTER_API UCharacterCreatorUIHelpers : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Character Creator|Focus")
    static void FocusWidget(UWidget* Widget);

    UFUNCTION(BlueprintPure, Category = "Character Creator|Layout")
    static FVector2D ClampPopupPosition(const FVector2D& DesiredPosition, const FVector2D& PopupSize, const FVector2D& ViewportSize);
};
