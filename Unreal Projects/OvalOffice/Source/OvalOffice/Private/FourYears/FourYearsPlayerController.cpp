#include "FourYears/FourYearsPlayerController.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "FourYears/FourYearsSubsystem.h"
#include "FourYears/FourYearsWalker.h"
#include "InputCoreTypes.h"
#include "SFourYearsScreen.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

void AFourYearsPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}
	// This map is the Oval Office, where meetings and appointments happen.
	if (UGameInstance* Instance = GetGameInstance())
	{
		if (UFourYearsSubsystem* Game = Instance->GetSubsystem<UFourYearsSubsystem>())
		{
			Game->SetLocation(TEXT("oval"));
		}
	}
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return;
	}
	Prompt = SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.f, 0.f, 0.f, 48.f))
		.Visibility(EVisibility::HitTestInvisible)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.02f, 0.04f, 0.07f, 0.8f))
			.Padding(FMargin(18.f, 9.f))
			[
				SAssignNew(PromptText, STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.91f, 0.86f)))
			]
		];
	Viewport->AddViewportWidgetContent(Prompt.ToSharedRef(), 5);
}

void AFourYearsPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		if (Prompt.IsValid()) Viewport->RemoveViewportWidgetContent(Prompt.ToSharedRef());
		if (Screen.IsValid()) Viewport->RemoveViewportWidgetContent(Screen.ToSharedRef());
	}
	Prompt.Reset();
	Screen.Reset();
	Super::EndPlay(EndPlayReason);
}

void AFourYearsPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!IsLocalController() || IsScreenOpen())
	{
		return;
	}
	float DeltaX = 0.f, DeltaY = 0.f;
	GetInputMouseDelta(DeltaX, DeltaY);
	if (DeltaX != 0.f || DeltaY != 0.f)
	{
		FRotator View = GetControlRotation();
		View.Yaw += DeltaX * LookSensitivity * 10.f;
		View.Pitch = FMath::Clamp<double>(FRotator::NormalizeAxis(View.Pitch) + DeltaY * LookSensitivity * 10.0, -75.0, 75.0);
		SetControlRotation(View);
	}
	const AFourYearsWalker* Walker = Cast<AFourYearsWalker>(GetPawn());
	const bool bAtDesk = Walker && Walker->IsNearDesk();
	const bool bAtSofas = Walker && !bAtDesk && Walker->IsNearSittingArea();
	if (PromptText.IsValid())
	{
		const TCHAR* Text = bAtDesk ? TEXT("Press E at the Resolute Desk to read your quarterly briefing")
			: bAtSofas ? TEXT("Press E by the sofas to meet your advisers")
			: TEXT("W/A/S/D to walk · mouse to look · P for policies · the desk for briefings · the sofas for advisers");
		PromptText->SetText(FText::FromString(Text));
	}
	if (WasInputKeyJustPressed(EKeys::E))
	{
		if (bAtDesk) OpenBriefing();
		else if (bAtSofas) OpenAdvisers();
	}
	if (WasInputKeyJustPressed(EKeys::P))
	{
		OpenPolicies();
	}
}

void AFourYearsPlayerController::OpenBriefing() { OpenScreen(static_cast<uint8>(EFourYearsPage::Briefing)); }
void AFourYearsPlayerController::OpenPolicies() { OpenScreen(static_cast<uint8>(EFourYearsPage::Policies)); }
void AFourYearsPlayerController::OpenAdvisers() { OpenScreen(static_cast<uint8>(EFourYearsPage::Advisers)); }

void AFourYearsPlayerController::OpenScreen(uint8 Page)
{
	const EFourYearsPage Target = static_cast<EFourYearsPage>(Page);
	if (IsScreenOpen())
	{
		Screen->ShowPage(Target);
		return;
	}
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	UGameInstance* Instance = GetGameInstance();
	if (!Viewport || !Instance)
	{
		return;
	}
	Screen = SNew(SFourYearsScreen)
		.Subsystem(Instance->GetSubsystem<UFourYearsSubsystem>())
		.Page(Target)
		.OnClose(FSimpleDelegate::CreateUObject(this, &AFourYearsPlayerController::CloseScreen));
	Viewport->AddViewportWidgetContent(Screen.ToSharedRef(), 10);
	if (Prompt.IsValid()) Prompt->SetVisibility(EVisibility::Collapsed);
	if (AFourYearsWalker* Walker = Cast<AFourYearsWalker>(GetPawn())) Walker->SetMovementEnabled(false);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Screen);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
}

void AFourYearsPlayerController::CloseScreen()
{
	if (!IsScreenOpen())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(Screen.ToSharedRef());
	}
	Screen.Reset();
	if (Prompt.IsValid()) Prompt->SetVisibility(EVisibility::HitTestInvisible);
	if (AFourYearsWalker* Walker = Cast<AFourYearsWalker>(GetPawn())) Walker->SetMovementEnabled(true);
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
}
