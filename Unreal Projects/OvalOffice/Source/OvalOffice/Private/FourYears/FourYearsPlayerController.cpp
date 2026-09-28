#include "FourYears/FourYearsPlayerController.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "FourYears/FourYearsSubsystem.h"
#include "FourYears/FourYearsWalker.h"
#include "InputCoreTypes.h"
#include "SFourYearsBriefing.h"
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
		if (Briefing.IsValid()) Viewport->RemoveViewportWidgetContent(Briefing.ToSharedRef());
	}
	Prompt.Reset();
	Briefing.Reset();
	Super::EndPlay(EndPlayReason);
}

void AFourYearsPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!IsLocalController() || IsBriefingOpen())
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
	if (PromptText.IsValid())
	{
		PromptText->SetText(FText::FromString(bAtDesk ? TEXT("Press E at the Resolute Desk to read your quarterly briefing") : TEXT("W/A/S/D to walk · mouse to look · walk to the Resolute Desk")));
	}
	if (bAtDesk && WasInputKeyJustPressed(EKeys::E))
	{
		OpenBriefing();
	}
}

void AFourYearsPlayerController::OpenBriefing()
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	UGameInstance* Instance = GetGameInstance();
	if (IsBriefingOpen() || !Viewport || !Instance)
	{
		return;
	}
	Briefing = SNew(SFourYearsBriefing)
		.Subsystem(Instance->GetSubsystem<UFourYearsSubsystem>())
		.OnClose(FSimpleDelegate::CreateUObject(this, &AFourYearsPlayerController::CloseBriefing));
	Viewport->AddViewportWidgetContent(Briefing.ToSharedRef(), 10);
	if (Prompt.IsValid()) Prompt->SetVisibility(EVisibility::Collapsed);
	if (AFourYearsWalker* Walker = Cast<AFourYearsWalker>(GetPawn())) Walker->SetMovementEnabled(false);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Briefing);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
}

void AFourYearsPlayerController::CloseBriefing()
{
	if (!IsBriefingOpen())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(Briefing.ToSharedRef());
	}
	Briefing.Reset();
	if (Prompt.IsValid()) Prompt->SetVisibility(EVisibility::HitTestInvisible);
	if (AFourYearsWalker* Walker = Cast<AFourYearsWalker>(GetPawn())) Walker->SetMovementEnabled(true);
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
}
