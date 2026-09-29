#include "FourYears/FourYearsPlayerController.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "FourYears/FourYearsSubsystem.h"
#include "FourYears/FourYearsWalker.h"
#include "FourYears/FourYearsRoom.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "SFourYearsScreen.h"
#include "SFourYearsIntro.h"
#include "SFourYearsDiplomacy.h"
#include "SFourYearsElectorate.h"
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
		if (Intro.IsValid()) Viewport->RemoveViewportWidgetContent(Intro.ToSharedRef());
		if (WorldMap.IsValid()) Viewport->RemoveViewportWidgetContent(WorldMap.ToSharedRef());
		if (VoterAtlas.IsValid()) Viewport->RemoveViewportWidgetContent(VoterAtlas.ToSharedRef());
	}
	Prompt.Reset();
	Screen.Reset();
	Intro.Reset();
	WorldMap.Reset();
	VoterAtlas.Reset();
	Super::EndPlay(EndPlayReason);
}

void AFourYearsPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (bVoterAtlasRequested) { bVoterAtlasRequested = false; ShowVoterAtlas(); }
	if (bWorldMapRequested) { bWorldMapRequested = false; ShowWorldMap(); }
	if (IsLocalController() && !Intro.IsValid() && GetGameInstance())
	{
		if (const auto* Game = GetGameInstance()->GetSubsystem<UFourYearsSubsystem>(); Game && Game->NeedsInauguration())
		{
			OpenInauguration();
			return;
		}
	}
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
			: TEXT("W/A/S/D to walk · mouse to look · P policies · C Congress · M world · V voters · E at the desk or sofas");
		const FString Nearby = GetInteractionPrompt();
		PromptText->SetText(FText::FromString(Nearby.IsEmpty() ? FString(Text) : Nearby));
	}
	if (WasInputKeyJustPressed(EKeys::E))
	{
		Interact();
		return; // Travel may replace this world; do not open another screen this frame.
	}
	if (WasInputKeyJustPressed(EKeys::P))
	{
		OpenPolicies();
	}
	if (WasInputKeyJustPressed(EKeys::C))
	{
		OpenCongress();
	}
	if (WasInputKeyJustPressed(EKeys::I)) OpenInauguration();
	if (WasInputKeyJustPressed(EKeys::M)) OpenWorldMap();
	if (WasInputKeyJustPressed(EKeys::V)) OpenVoterAtlas();
}

FString AFourYearsPlayerController::GetInteractionPrompt() const
{
	const AFourYearsWalker* Walker = Cast<AFourYearsWalker>(GetPawn());
	if (Walker && Walker->GetRoom())
	{
		if (const auto* Point = Walker->GetRoom()->FindInteraction(Walker->GetActorLocation())) return Point->Prompt;
		if (Walker->GetRoom()->WalkableAreas.Num()) return TEXT("CABINET ROOM  ·  W/A/S/D walk  ·  E at a named seat to meet  ·  P policies  ·  C Congress  ·  M world  ·  V voters  ·  exit behind you");
	}
	return FString();
}

void AFourYearsPlayerController::Interact()
{
	if (IsScreenOpen()) return;
	const AFourYearsWalker* Walker = Cast<AFourYearsWalker>(GetPawn());
	if (!Walker) return;
	if (const AFourYearsRoom* Room = Walker->GetRoom())
	{
		if (const auto* Point = Room->FindInteraction(Walker->GetActorLocation()))
		{
			if (Point->Action == TEXT("travel"))
			{
				UGameplayStatics::OpenLevel(this, FName(*Point->Target), true, TEXT("FromRoom=1"));
			}
			else if (Point->Action == TEXT("briefing")) OpenBriefing();
			else if (Point->Action == TEXT("adviser"))
			{
				OpenAdvisers();
				if (Screen.IsValid()) Screen->ShowAdviser(Point->Target);
			}
			return;
		}
	}
	if (Walker->IsNearDesk()) OpenBriefing();
	else if (Walker->IsNearSittingArea()) OpenAdvisers();
}

void AFourYearsPlayerController::OpenBriefing() { OpenScreen(static_cast<uint8>(EFourYearsPage::Briefing)); }
void AFourYearsPlayerController::OpenPolicies() { OpenScreen(static_cast<uint8>(EFourYearsPage::Policies)); }
void AFourYearsPlayerController::OpenAdvisers() { OpenScreen(static_cast<uint8>(EFourYearsPage::Advisers)); }
void AFourYearsPlayerController::OpenCongress() { OpenScreen(static_cast<uint8>(EFourYearsPage::Congress)); }

void AFourYearsPlayerController::OpenScreen(uint8 Page)
{
	if (Intro.IsValid() || WorldMap.IsValid() || VoterAtlas.IsValid()) return;
	if (auto* Instance = GetGameInstance())
		if (auto* Game = Instance->GetSubsystem<UFourYearsSubsystem>(); Game && !Game->GetState().Get("ended").Truthy() && Game->GetState().Get("electoral").Get("party").AsString().empty()) { OpenVoterAtlas(); return; }
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
		.OnWorldMap(FSimpleDelegate::CreateUObject(this, &AFourYearsPlayerController::OpenWorldMap))
		.OnVoterAtlas(FSimpleDelegate::CreateUObject(this, &AFourYearsPlayerController::OpenVoterAtlas))
		.OnClose(FSimpleDelegate::CreateUObject(this, &AFourYearsPlayerController::CloseScreen));
	if (const AFourYearsWalker* Walker = Cast<AFourYearsWalker>(GetPawn()))
	{
		if (Walker->GetRoom()) Screen->SetRoomLabel(Walker->GetRoom()->RoomLabel);
	}
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
	if (!Screen.IsValid() && !WorldMap.IsValid() && !VoterAtlas.IsValid())
	{
		return;
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		if (Screen.IsValid()) Viewport->RemoveViewportWidgetContent(Screen.ToSharedRef());
		if (WorldMap.IsValid()) Viewport->RemoveViewportWidgetContent(WorldMap.ToSharedRef());
		if (VoterAtlas.IsValid()) Viewport->RemoveViewportWidgetContent(VoterAtlas.ToSharedRef());
	}
	Screen.Reset();
	WorldMap.Reset();
	VoterAtlas.Reset();
	if (Prompt.IsValid()) Prompt->SetVisibility(EVisibility::HitTestInvisible);
	if (AFourYearsWalker* Walker = Cast<AFourYearsWalker>(GetPawn())) Walker->SetMovementEnabled(true);
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

void AFourYearsPlayerController::OpenInauguration()
{
	if (Intro.IsValid() || !IsLocalController()) return;
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	UGameInstance* Instance = GetGameInstance();
	if (!Viewport || !Instance) return;
	CloseScreen();
	Intro = SNew(SFourYearsIntro)
		.Subsystem(Instance->GetSubsystem<UFourYearsSubsystem>())
		.OnFinished(FSimpleDelegate::CreateUObject(this, &AFourYearsPlayerController::FinishInauguration));
	Viewport->AddViewportWidgetContent(Intro.ToSharedRef(), 20);
	if (Prompt.IsValid()) Prompt->SetVisibility(EVisibility::Collapsed);
	if (auto* Walker = Cast<AFourYearsWalker>(GetPawn())) Walker->SetMovementEnabled(false);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Intro);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
}

void AFourYearsPlayerController::OpenWorldMap()
{
	if (!Intro.IsValid() && !WorldMap.IsValid()) bWorldMapRequested = true;
}

void AFourYearsPlayerController::ShowWorldMap()
{
	if (Intro.IsValid() || WorldMap.IsValid() || !IsLocalController()) return;
	auto* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	auto* Instance = GetGameInstance();
	if (!Viewport || !Instance) return;
	CloseScreen();
	WorldMap = SNew(SFourYearsDiplomacy)
		.Subsystem(Instance->GetSubsystem<UFourYearsSubsystem>())
		.OnClose(FSimpleDelegate::CreateUObject(this, &AFourYearsPlayerController::CloseScreen));
	Viewport->AddViewportWidgetContent(WorldMap.ToSharedRef(), 15);
	if (Prompt.IsValid()) Prompt->SetVisibility(EVisibility::Collapsed);
	if (auto* Walker = Cast<AFourYearsWalker>(GetPawn())) Walker->SetMovementEnabled(false);
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(WorldMap);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
}

void AFourYearsPlayerController::SkipInauguration()
{
	if (Intro.IsValid()) Intro->SkipToBriefing();
}

int32 AFourYearsPlayerController::GetInaugurationStage() const
{
	return Intro.IsValid() ? (Intro->IsBriefing() ? 1 : 0) : -1;
}

void AFourYearsPlayerController::FinishInauguration()
{
	if (!Intro.IsValid()) return;
	if (UGameInstance* Instance = GetGameInstance())
		if (auto* Game = Instance->GetSubsystem<UFourYearsSubsystem>()) Game->CompleteInauguration();
	if (auto* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		Viewport->RemoveViewportWidgetContent(Intro.ToSharedRef());
	Intro.Reset();
	if (Prompt.IsValid()) Prompt->SetVisibility(EVisibility::HitTestInvisible);
	if (auto* Walker = Cast<AFourYearsWalker>(GetPawn())) Walker->SetMovementEnabled(true);
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
	if(auto* Game=GetGameInstance()->GetSubsystem<UFourYearsSubsystem>(); Game && Game->GetState().Get("electoral").Get("party").AsString().empty()) OpenVoterAtlas();
}

void AFourYearsPlayerController::OpenVoterAtlas(){if(!Intro.IsValid()&&!VoterAtlas.IsValid())bVoterAtlasRequested=true;}
void AFourYearsPlayerController::ShowVoterAtlas(){
 if(Intro.IsValid()||VoterAtlas.IsValid()||!IsLocalController())return;
 auto* Viewport=GetWorld()?GetWorld()->GetGameViewport():nullptr;auto* Instance=GetGameInstance();if(!Viewport||!Instance)return;
 CloseScreen();VoterAtlas=SNew(SFourYearsElectorate).Subsystem(Instance->GetSubsystem<UFourYearsSubsystem>()).OnClose(FSimpleDelegate::CreateUObject(this,&AFourYearsPlayerController::CloseScreen));
 Viewport->AddViewportWidgetContent(VoterAtlas.ToSharedRef(),15);if(Prompt.IsValid())Prompt->SetVisibility(EVisibility::Collapsed);if(auto* Walker=Cast<AFourYearsWalker>(GetPawn()))Walker->SetMovementEnabled(false);
 FInputModeUIOnly Mode;Mode.SetWidgetToFocus(VoterAtlas);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);SetShowMouseCursor(true);
}
