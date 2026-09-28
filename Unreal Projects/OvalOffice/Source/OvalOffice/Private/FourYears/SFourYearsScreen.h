// The president's screen: the quarterly briefing at the Resolute Desk, the policy levers, meetings with
// advisers by the sofas, and Congress, as tabs of one overlay.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "FourYears/FourYearsSubsystem.h"

class SVerticalBox;
struct FSlateBrush;

enum class EFourYearsPage : uint8
{
	Briefing,
	Policies,
	Advisers,
	Congress,
};

class SFourYearsScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFourYearsScreen) : _Page(EFourYearsPage::Briefing) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UFourYearsSubsystem>, Subsystem)
		SLATE_ARGUMENT(EFourYearsPage, Page)
		SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void ShowPage(EFourYearsPage NewPage);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void Rebuild();
	void BuildTabs();
	void BuildBriefing(UFourYearsSubsystem& Game);
	void BuildPolicies(UFourYearsSubsystem& Game);
	void BuildAdvisers(UFourYearsSubsystem& Game);
	void BuildCongress(UFourYearsSubsystem& Game);
	void BuildMessage();
	void AddText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = false);
	void AddButton(const FString& Label, const FString& Detail, FOnClicked OnClicked);
	TSharedRef<SWidget> Portrait(UFourYearsSubsystem& Game, const FFourYearsAdviser& Adviser);

	FReply Choose(int32 Index);
	FReply Advance();
	FReply Continue();
	FReply NewTerm();
	FReply ChangePolicy(FString PolicyId, int32 Level);
	FReply Meet(FString AdviserId, FString Response);
	FReply Introduce(FString BillId);
	FReply Amend(FString AmendmentId);
	FReply Lobby(FString BlocId);
	FReply Vote();
	FReply SelectPage(EFourYearsPage NewPage);
	FReply Close();
	void SetMessage(const FFourYearsActionResult& Result);

	TWeakObjectPtr<UFourYearsSubsystem> Subsystem;
	FSimpleDelegate OnClose;
	TSharedPtr<SVerticalBox> Content;
	EFourYearsPage Page = EFourYearsPage::Briefing;
	// The report shown after advancing, until the player continues to the next briefing.
	TOptional<FFourYearsReport> Report;
	// Outcome of the last policy change or meeting.
	FString Message;
	bool bMessageOk = true;
	TMap<FString, TSharedPtr<FSlateBrush>> PortraitBrushes;
	bool bNeedsRebuild = true;
};
