// The quarterly briefing at the Resolute Desk: the event, its responses, the national picture, and the
// quarterly report after advancing.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "FourYears/FourYearsSubsystem.h"

class SVerticalBox;

class SFourYearsBriefing : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFourYearsBriefing) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UFourYearsSubsystem>, Subsystem)
		SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void Rebuild();
	void AddText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = false);
	void AddButton(const FString& Label, const FString& Detail, FOnClicked OnClicked);
	FReply Choose(int32 Index);
	FReply Advance();
	FReply Continue();
	FReply NewTerm();
	FReply Close();

	TWeakObjectPtr<UFourYearsSubsystem> Subsystem;
	FSimpleDelegate OnClose;
	TSharedPtr<SVerticalBox> Content;
	// The report shown after advancing, until the player continues to the next briefing.
	TOptional<FFourYearsReport> Report;
	bool bNeedsRebuild = true;
};
