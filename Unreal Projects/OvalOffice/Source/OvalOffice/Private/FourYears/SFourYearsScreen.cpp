#include "SFourYearsScreen.h"

#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
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
const FLinearColor Good(0.58f, 0.82f, 0.62f);
const FLinearColor Card(0.10f, 0.15f, 0.21f, 1.f);

FSlateFontInfo Font(int32 Size, bool bBold = false)
{
	return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
}

TSharedRef<STextBlock> Label(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold = false)
{
	return SNew(STextBlock).Text(FText::FromString(Text)).Font(Font(Size, bBold)).ColorAndOpacity(FSlateColor(Color)).AutoWrapText(true);
}

FString Dots(int32 Level)
{
	FString Out;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Out += Index <= Level ? TEXT("●") : TEXT("○");
	}
	return Out;
}
} // namespace

void SFourYearsScreen::Construct(const FArguments& InArgs)
{
	Subsystem = InArgs._Subsystem;
	OnClose = InArgs._OnClose;
	Page = InArgs._Page;
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.01f, 0.02f, 0.04f, 0.72f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(940.f)
			.MaxDesiredHeight(820.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.07f, 0.11f, 0.16f, 0.97f))
				.Padding(FMargin(36.f, 24.f))
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

void SFourYearsScreen::ShowPage(EFourYearsPage NewPage)
{
	Page = NewPage;
	Message.Reset();
	bNeedsRebuild = true;
}

void SFourYearsScreen::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// Rebuild outside of click handling so buttons are never destroyed while they process a click.
	if (bNeedsRebuild)
	{
		bNeedsRebuild = false;
		Rebuild();
	}
}

FReply SFourYearsScreen::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::E)
	{
		return Close();
	}
	if (Key == EKeys::P)
	{
		return SelectPage(EFourYearsPage::Policies);
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SFourYearsScreen::AddText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 4.f))[Label(Text, Size, Color, bBold)];
}

void SFourYearsScreen::AddButton(const FString& Text, const FString& Detail, FOnClicked OnClicked)
{
	TSharedRef<SVerticalBox> Face = SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[Label(Text, 13, FLinearColor(0.08f, 0.08f, 0.08f), true)];
	if (!Detail.IsEmpty())
	{
		Face->AddSlot().AutoHeight().Padding(FMargin(0.f, 3.f, 0.f, 0.f))[Label(Detail, 10, FLinearColor(0.2f, 0.2f, 0.2f))];
	}
	Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 4.f))
	[
		SNew(SButton).ContentPadding(FMargin(14.f, 9.f)).OnClicked(OnClicked)[Face]
	];
}

void SFourYearsScreen::BuildTabs()
{
	TSharedRef<SHorizontalBox> Tabs = SNew(SHorizontalBox);
	const auto Tab = [this, &Tabs](const TCHAR* Text, EFourYearsPage Target) {
		Tabs->AddSlot().AutoWidth().Padding(FMargin(0.f, 0.f, 8.f, 0.f))
		[
			SNew(SButton)
			.ContentPadding(FMargin(14.f, 6.f))
			.OnClicked(FOnClicked::CreateSP(this, &SFourYearsScreen::SelectPage, Target))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Text))
				.Font(Font(12, Page == Target))
				.ColorAndOpacity(FSlateColor(Page == Target ? FLinearColor(0.45f, 0.3f, 0.05f) : FLinearColor(0.1f, 0.1f, 0.1f)))
			]
		];
	};
	Tab(TEXT("Briefing"), EFourYearsPage::Briefing);
	Tab(TEXT("Policies"), EFourYearsPage::Policies);
	Tab(TEXT("Advisers"), EFourYearsPage::Advisers);
	Tabs->AddSlot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center)[Label(TEXT("Esc or E returns to the office"), 10, Muted)];
	Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 12.f))[Tabs];
}

void SFourYearsScreen::BuildMessage()
{
	if (!Message.IsEmpty())
	{
		AddText(Message, 12, bMessageOk ? Good : Alert, true);
	}
}

void SFourYearsScreen::Rebuild()
{
	Content->ClearChildren();
	UFourYearsSubsystem* Game = Subsystem.Get();
	if (!Game)
	{
		AddText(TEXT("The game is not running."), 14, Alert);
		return;
	}
	BuildTabs();
	switch (Page)
	{
	case EFourYearsPage::Policies: BuildPolicies(*Game); break;
	case EFourYearsPage::Advisers: BuildAdvisers(*Game); break;
	default: BuildBriefing(*Game); break;
	}
}

void SFourYearsScreen::BuildBriefing(UFourYearsSubsystem& Game)
{
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
		AddButton(TEXT("Continue"), TEXT("Read the next briefing"), FOnClicked::CreateSP(this, &SFourYearsScreen::Continue));
		return;
	}
	const FFourYearsBriefing Briefing = Game.GetBriefing();
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
		AddButton(TEXT("Start a new term"), TEXT("Begin again with a new country and a new event deck"), FOnClicked::CreateSP(this, &SFourYearsScreen::NewTerm));
	}
	else if (Briefing.ChosenIndex >= 0 && Briefing.Choices.IsValidIndex(Briefing.ChosenIndex))
	{
		AddText(TEXT("DECISION FILED"), 11, Gold, true);
		AddText(Briefing.Choices[Briefing.ChosenIndex].Label + TEXT(". Effects arrive in next quarter's report."), 14, Ink);
		AddButton(TEXT("Advance to the next quarter"), TEXT("Resolve the quarter and read the report"), FOnClicked::CreateSP(this, &SFourYearsScreen::Advance));
	}
	else
	{
		AddText(TEXT("DECISION ON YOUR DESK"), 11, Gold, true);
		AddText(TEXT("Adjust policies and meet your advisers first: both close once you file this decision."), 11, Muted);
		for (int32 Index = 0; Index < Briefing.Choices.Num(); ++Index)
		{
			AddButton(Briefing.Choices[Index].Label, Briefing.Choices[Index].Summary, FOnClicked::CreateSP(this, &SFourYearsScreen::Choose, Index));
		}
	}
}

void SFourYearsScreen::BuildPolicies(UFourYearsSubsystem& Game)
{
	const FFourYearsStatus Status = Game.GetStatus();
	AddText(TEXT("POLICY ATLAS"), 11, Gold, true);
	AddText(TEXT("Set the course"), 26, Ink, true);
	AddText(FString::Printf(TEXT("Political capital %d/20   ·   Approval %d%%   ·   Budget %s%d per quarter"),
		Status.Capital, Status.Approval, Status.BudgetBalance > 0 ? TEXT("+") : TEXT(""), Status.BudgetBalance), 12, Muted);
	AddText(TEXT("Each step costs political capital. Changes take one to three quarters to take full effect, and every lever has trade-offs."), 11, Muted);
	if (Status.bDecisionFiled || Status.bTermOver)
	{
		AddText(Status.bTermOver ? TEXT("The term is over.") : TEXT("This quarter's decision is filed. Policy changes open again next quarter."), 12, Alert, true);
	}
	BuildMessage();
	FString Department;
	for (const FFourYearsPolicy& Policy : Game.GetPolicies())
	{
		if (Policy.Department != Department)
		{
			Department = Policy.Department;
			Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 14.f, 0.f, 2.f))[Label(Department.ToUpper(), 11, Gold, true)];
		}
		const FString Budget = Policy.BudgetPerLevel < 0
			? FString::Printf(TEXT("Raises %d revenue per level"), -Policy.BudgetPerLevel)
			: FString::Printf(TEXT("Costs %d per level"), Policy.BudgetPerLevel);
		FString Detail = FString::Printf(TEXT("%s · %d capital per step · %d %s to take effect · now at %.1f"),
			*Budget, Policy.CapitalPerStep, Policy.Lag, Policy.Lag == 1 ? TEXT("quarter") : TEXT("quarters"), Policy.Implemented);
		if (Policy.bNeedsAct)
		{
			Detail += TEXT(" · full strength needs an act of Congress");
		}
		Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 3.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(Card)
			.Padding(FMargin(12.f, 8.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Label(Policy.Name, 13, Ink, true)]
					+ SVerticalBox::Slot().AutoHeight()[Label(Policy.Effects, 10, Muted)]
					+ SVerticalBox::Slot().AutoHeight()[Label(Detail, 10, Muted)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(8.f, 0.f))
				[
					SNew(SButton).ContentPadding(FMargin(10.f, 4.f)).IsEnabled(Policy.Level > 0)
					.OnClicked(FOnClicked::CreateSP(this, &SFourYearsScreen::ChangePolicy, Policy.Id, Policy.Level - 1))
					[Label(TEXT("−"), 14, FLinearColor(0.08f, 0.08f, 0.08f), true)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(92.f).HAlign(HAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Label(Dots(Policy.Level), 12, Gold)]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Label(FString::Printf(TEXT("%d / 4"), Policy.Level), 10, Muted)]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(8.f, 0.f, 0.f, 0.f))
				[
					SNew(SButton).ContentPadding(FMargin(10.f, 4.f)).IsEnabled(Policy.Level < 4)
					.OnClicked(FOnClicked::CreateSP(this, &SFourYearsScreen::ChangePolicy, Policy.Id, Policy.Level + 1))
					[Label(TEXT("+"), 14, FLinearColor(0.08f, 0.08f, 0.08f), true)]
				]
			]
		];
	}
}

TSharedRef<SWidget> SFourYearsScreen::Portrait(UFourYearsSubsystem& Game, const FFourYearsAdviser& Adviser)
{
	UTexture2D* Texture = Adviser.bActing ? nullptr : Game.GetPortrait(Adviser.Name);
	if (Texture)
	{
		TSharedPtr<FSlateBrush>& Brush = PortraitBrushes.FindOrAdd(Adviser.Name);
		if (!Brush.IsValid())
		{
			Brush = MakeShared<FSlateBrush>();
			Brush->SetResourceObject(Texture);
			Brush->SetImageSize(FVector2D(84.f, 84.f));
			Brush->DrawAs = ESlateBrushDrawType::Image;
		}
		return SNew(SBox).WidthOverride(84.f).HeightOverride(84.f)[SNew(SImage).Image(Brush.Get())];
	}
	// Acting replacements have no portrait yet; show their initials instead.
	return SNew(SBox).WidthOverride(84.f).HeightOverride(84.f)
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.2f, 0.26f, 0.33f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[Label(Adviser.Initials, 22, Ink, true)]
	];
}

void SFourYearsScreen::BuildAdvisers(UFourYearsSubsystem& Game)
{
	const FFourYearsStatus Status = Game.GetStatus();
	AddText(TEXT("THE SITTING AREA"), 11, Gold, true);
	AddText(TEXT("Your advisers"), 26, Ink, true);
	AddText(FString::Printf(TEXT("Appointments left this quarter: %d of 2   ·   Political capital %d/20"), Status.AppointmentsLeft, Status.Capital), 12, Muted);
	AddText(TEXT("Each meeting uses an appointment. Promises earn support now, but a missed deadline costs trust and the relationship. Keep a promise by raising the policy to its target level in time."), 11, Muted);
	if (Status.bDecisionFiled || Status.bTermOver)
	{
		AddText(Status.bTermOver ? TEXT("The term is over.") : TEXT("This quarter's decision is filed. Meetings open again next quarter."), 12, Alert, true);
	}
	BuildMessage();
	for (const FFourYearsAdviser& Adviser : Game.GetAdvisers())
	{
		TSharedRef<SVerticalBox> Body = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Label(Adviser.Name + TEXT("  ·  ") + Adviser.Role, 15, Ink, true)]
			+ SVerticalBox::Slot().AutoHeight()[Label(FString::Printf(TEXT("Relationship %d/100%s"), Adviser.Relationship, Adviser.bActing ? TEXT(" · acting replacement") : TEXT("")), 11, Adviser.Relationship < 30 ? Alert : Muted)]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 4.f))[Label(TEXT("“") + Adviser.Quote + TEXT("”"), 12, Ink)];
		if (Adviser.bResigned)
		{
			Body->AddSlot().AutoHeight()[Label(TEXT("Has resigned. Appoint an acting replacement (a Team action in the browser edition for now)."), 12, Alert, true)];
		}
		else
		{
			Body->AddSlot().AutoHeight()[Label(Adviser.Promise.IsEmpty() ? TEXT("Asks for: ") + Adviser.Request : TEXT("Your promise: ") + Adviser.Promise, 12, Gold, true)];
			if (Adviser.bMetThisQuarter)
			{
				Body->AddSlot().AutoHeight().Padding(FMargin(0.f, 4.f))[Label(TEXT("You have already met this quarter."), 11, Muted)];
			}
			else
			{
				for (const FFourYearsMeetingOption& Option : Adviser.Options)
				{
					Body->AddSlot().AutoHeight().Padding(FMargin(0.f, 4.f, 0.f, 0.f))
					[
						SNew(SButton)
						.ContentPadding(FMargin(12.f, 6.f))
						.OnClicked(FOnClicked::CreateSP(this, &SFourYearsScreen::Meet, Adviser.Id, Option.Response))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[Label(Option.Label, 12, FLinearColor(0.08f, 0.08f, 0.08f), true)]
							+ SVerticalBox::Slot().AutoHeight()[Label(Option.Detail, 10, FLinearColor(0.2f, 0.2f, 0.2f))]
						]
					];
				}
			}
		}
		Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 6.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(Card)
			.Padding(FMargin(14.f, 12.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(FMargin(0.f, 0.f, 14.f, 0.f))[Portrait(Game, Adviser)]
				+ SHorizontalBox::Slot().FillWidth(1.f)[Body]
			]
		];
	}
	const TArray<FString> Record = Game.GetPromiseRecord();
	if (Record.Num() > 0)
	{
		Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 14.f, 0.f, 2.f))[Label(TEXT("PROMISE RECORD"), 11, Gold, true)];
		for (const FString& Line : Record)
		{
			AddText(Line, 11, Line.StartsWith(TEXT("Broken")) ? Alert : Line.StartsWith(TEXT("Kept")) ? Good : Ink);
		}
	}
}

void SFourYearsScreen::SetMessage(const FFourYearsActionResult& Result)
{
	Message = Result.Message;
	bMessageOk = Result.bOk;
}

FReply SFourYearsScreen::Choose(int32 Index)
{
	if (UFourYearsSubsystem* Game = Subsystem.Get())
	{
		Game->ChooseResponse(Index);
	}
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsScreen::Advance()
{
	if (UFourYearsSubsystem* Game = Subsystem.Get())
	{
		Report = Game->AdvanceQuarter();
	}
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsScreen::Continue()
{
	Report.Reset();
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsScreen::NewTerm()
{
	if (UFourYearsSubsystem* Game = Subsystem.Get())
	{
		Game->StartNewTerm();
	}
	Report.Reset();
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsScreen::ChangePolicy(FString PolicyId, int32 Level)
{
	if (UFourYearsSubsystem* Game = Subsystem.Get())
	{
		SetMessage(Game->SetPolicyLevel(PolicyId, Level));
	}
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsScreen::Meet(FString AdviserId, FString Response)
{
	if (UFourYearsSubsystem* Game = Subsystem.Get())
	{
		SetMessage(Game->MeetAdviser(AdviserId, Response));
	}
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsScreen::SelectPage(EFourYearsPage NewPage)
{
	ShowPage(NewPage);
	return FReply::Handled();
}

FReply SFourYearsScreen::Close()
{
	OnClose.ExecuteIfBound();
	return FReply::Handled();
}
