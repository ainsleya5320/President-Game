#include "SFourYearsBriefing.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
const FLinearColor Ink(0.93f, 0.91f, 0.86f);
const FLinearColor Muted(0.66f, 0.68f, 0.66f);
const FLinearColor Gold(0.83f, 0.68f, 0.38f);
const FLinearColor Alert(0.93f, 0.55f, 0.47f);
} // namespace

void SFourYearsBriefing::Construct(const FArguments& InArgs)
{
	Subsystem = InArgs._Subsystem;
	OnClose = InArgs._OnClose;
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.01f, 0.02f, 0.04f, 0.72f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(880.f)
			.MaxDesiredHeight(760.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.07f, 0.11f, 0.16f, 0.97f))
				.Padding(FMargin(36.f, 28.f))
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(Content, SVerticalBox)
					]
				]
			]
		]
	];
}

void SFourYearsBriefing::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// Rebuild outside of click handling so buttons are never destroyed while they process a click.
	if (bNeedsRebuild)
	{
		bNeedsRebuild = false;
		Rebuild();
	}
}

FReply SFourYearsBriefing::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::E)
	{
		return Close();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SFourYearsBriefing::AddText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	Content->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 4.f))
	[
		SNew(STextBlock)
		.Text(FText::FromString(Text))
		.Font(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size))
		.ColorAndOpacity(FSlateColor(Color))
		.AutoWrapText(true)
	];
}

void SFourYearsBriefing::AddButton(const FString& Label, const FString& Detail, FOnClicked OnClicked)
{
	TSharedRef<SVerticalBox> Face = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
			.AutoWrapText(true)
		];
	if (!Detail.IsEmpty())
	{
		Face->AddSlot().AutoHeight().Padding(FMargin(0.f, 3.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(Detail))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
			.AutoWrapText(true)
		];
	}
	Content->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 5.f))
	[
		SNew(SButton)
		.ContentPadding(FMargin(14.f, 10.f))
		.OnClicked(OnClicked)
		[
			Face
		]
	];
}

void SFourYearsBriefing::Rebuild()
{
	Content->ClearChildren();
	UFourYearsSubsystem* Game = Subsystem.Get();
	if (!Game)
	{
		AddText(TEXT("The game is not running."), 14, Alert);
		return;
	}
	if (Report.IsSet())
	{
		const FFourYearsReport& Shown = Report.GetValue();
		AddText(FString::Printf(TEXT("QUARTER %d REPORT"), Shown.Quarter), 11, Gold, true);
		AddText(Shown.Election.IsEmpty() ? TEXT("What changed") : Shown.Election, 22, Ink, true);
		if (Shown.Changes.Num() == 0)
		{
			AddText(TEXT("No major changes this quarter. Policies already in motion keep building."), 13, Muted);
		}
		for (const FString& Change : Shown.Changes)
		{
			AddText(TEXT("•  ") + Change, 13, Ink);
		}
		AddButton(TEXT("Continue"), TEXT("Read the next briefing"), FOnClicked::CreateSP(this, &SFourYearsBriefing::Continue));
		return;
	}
	const FFourYearsBriefing Briefing = Game->GetBriefing();
	AddText(TEXT("THE SITUATION ROOM  ·  ") + Briefing.QuarterLabel.ToUpper(), 11, Gold, true);
	AddText(Briefing.Title, 26, Ink, true);
	AddText(Briefing.Body, 14, Ink);
	AddText(FString::Printf(TEXT("Approval %d%%   ·   Political capital %d/20   ·   Budget %s%d   ·   Debt %d"),
		Briefing.Approval, Briefing.Capital, Briefing.BudgetBalance > 0 ? TEXT("+") : TEXT(""), Briefing.BudgetBalance, Briefing.Debt), 12, Muted);
	if (Briefing.Alerts.Num() > 0)
	{
		AddText(FString::Join(Briefing.Alerts, TEXT("   ·   ")), 12, Alert, true);
	}
	if (Briefing.bTermOver)
	{
		AddText(Briefing.Outcome, 15, Gold, true);
		AddButton(TEXT("Start a new term"), TEXT("Begin again with a new country and a new event deck"), FOnClicked::CreateSP(this, &SFourYearsBriefing::NewTerm));
	}
	else if (Briefing.ChosenIndex >= 0 && Briefing.Choices.IsValidIndex(Briefing.ChosenIndex))
	{
		AddText(TEXT("DECISION FILED"), 11, Gold, true);
		AddText(Briefing.Choices[Briefing.ChosenIndex].Label + TEXT(". Effects arrive in next quarter's report."), 14, Ink);
		AddButton(TEXT("Advance to the next quarter"), TEXT("Resolve the quarter and read the report"), FOnClicked::CreateSP(this, &SFourYearsBriefing::Advance));
	}
	else
	{
		AddText(TEXT("DECISION ON YOUR DESK"), 11, Gold, true);
		for (int32 Index = 0; Index < Briefing.Choices.Num(); ++Index)
		{
			AddButton(Briefing.Choices[Index].Label, Briefing.Choices[Index].Summary, FOnClicked::CreateSP(this, &SFourYearsBriefing::Choose, Index));
		}
	}
	AddText(TEXT("Esc or E returns to the office. Policies, Congress and the other strategy screens are still in the browser edition."), 10, Muted);
}

FReply SFourYearsBriefing::Choose(int32 Index)
{
	if (UFourYearsSubsystem* Game = Subsystem.Get())
	{
		Game->ChooseResponse(Index);
	}
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsBriefing::Advance()
{
	if (UFourYearsSubsystem* Game = Subsystem.Get())
	{
		Report = Game->AdvanceQuarter();
	}
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsBriefing::Continue()
{
	Report.Reset();
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsBriefing::NewTerm()
{
	if (UFourYearsSubsystem* Game = Subsystem.Get())
	{
		Game->StartNewTerm();
	}
	Report.Reset();
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsBriefing::Close()
{
	OnClose.ExecuteIfBound();
	return FReply::Handled();
}
