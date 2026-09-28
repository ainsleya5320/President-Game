#include "SFourYearsScreen.h"

#include "Engine/Texture2D.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Notifications/SProgressBar.h"
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

const FButtonStyle& ConversationButton()
{
	static const FButtonStyle Style = FButtonStyle()
		.SetNormal(FSlateColorBrush(FLinearColor(0.10f, 0.16f, 0.23f)))
		.SetHovered(FSlateColorBrush(FLinearColor(0.19f, 0.28f, 0.36f)))
		.SetPressed(FSlateColorBrush(FLinearColor(0.27f, 0.23f, 0.15f)))
		.SetDisabled(FSlateColorBrush(FLinearColor(0.09f, 0.10f, 0.12f)));
	return Style;
}

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
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Out += Index < Level ? TEXT("●") : TEXT("○");
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
			.WidthOverride(1180.f)
			.MaxDesiredHeight(1000.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(Gold)
				.Padding(2.f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor(0.025f, 0.045f, 0.075f, 1.f))
					.Padding(FMargin(28.f, 22.f))
					[
					SAssignNew(Scroll, SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(Content, SVerticalBox)
					]
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
	bResetScroll = true;
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
	if (Speech.IsValid() && RevealedCharacters < DialogueText.Len())
	{
		RevealedCharacters = FMath::Min(static_cast<float>(DialogueText.Len()), RevealedCharacters + InDeltaTime * 58.f);
		Speech->SetText(FText::FromString(DialogueText.Left(static_cast<int32>(RevealedCharacters))));
	}
}

FReply SFourYearsScreen::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (InKeyEvent.IsRepeat()) return FReply::Handled();
	if (Page == EFourYearsPage::Advisers)
	{
		if (Key == EKeys::SpaceBar || Key == EKeys::Enter) return FinishDialogue();
		if (Key == EKeys::BackSpace) return AdviserRoster();
		const int32 Number = Key == EKeys::One ? 0 : Key == EKeys::Two ? 1 : Key == EKeys::Three ? 2 : Key == EKeys::Four ? 3 : -1;
		if (Number >= 0)
		{
			if (SelectedAdviser.IsEmpty() && RosterIds.IsValidIndex(Number)) return TalkTo(RosterIds[Number]);
			return Respond(Number);
		}
	}
	if (Key == EKeys::Escape || Key == EKeys::E)
	{
		return Close();
	}
	if (Key == EKeys::P)
	{
		return AllPolicies();
	}
	if (Key == EKeys::C)
	{
		return SelectPage(EFourYearsPage::Congress);
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SFourYearsScreen::AddText(const FString& Text, int32 Size, const FLinearColor& Color, bool bBold)
{
	Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 4.f))[Label(Text, Size, Color, bBold)];
}

void SFourYearsScreen::AddButton(const FString& Text, const FString& Detail, FOnClicked OnClicked)
{
	TSharedRef<SVerticalBox> Face = SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[Label(Text, 16, Ink, true)];
	if (!Detail.IsEmpty())
	{
		Face->AddSlot().AutoHeight().Padding(FMargin(0.f, 3.f, 0.f, 0.f))[Label(Detail, 12, Muted)];
	}
	Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 4.f))
	[
		SNew(SButton).ButtonStyle(&ConversationButton()).IsFocusable(false).ContentPadding(FMargin(14.f, 9.f)).OnClicked(OnClicked)[Face]
	];
}

void SFourYearsScreen::BuildTabs()
{
	TSharedRef<SHorizontalBox> Tabs = SNew(SHorizontalBox);
	const auto Tab = [this, &Tabs](const TCHAR* Text, EFourYearsPage Target) {
		Tabs->AddSlot().AutoWidth().Padding(FMargin(0.f, 0.f, 8.f, 0.f))
		[
			SNew(SButton).ButtonStyle(&ConversationButton()).IsFocusable(false)
			.ContentPadding(FMargin(14.f, 6.f))
			.OnClicked(FOnClicked::CreateSP(this, &SFourYearsScreen::SelectPage, Target))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Text))
				.Font(Font(12, Page == Target))
				.ColorAndOpacity(FSlateColor(Page == Target ? Gold : Ink))
			]
		];
	};
	Tab(TEXT("Briefing"), EFourYearsPage::Briefing);
	Tab(TEXT("Policies"), EFourYearsPage::Policies);
	Tab(TEXT("Advisers"), EFourYearsPage::Advisers);
	Tab(TEXT("Congress"), EFourYearsPage::Congress);
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
	Speech.Reset();
	DialogueOptions.Reset();
	RosterIds.Reset();
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
	case EFourYearsPage::Congress: BuildCongress(*Game); break;
	default: BuildBriefing(*Game); break;
	}
	if (bResetScroll && Scroll.IsValid()) Scroll->ScrollToStart();
	bResetScroll = false;
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
	if (!FocusPolicyId.IsEmpty())
	{
		AddText(TEXT("FOLLOWING UP ON YOUR CONVERSATION"), 11, Gold, true);
		AddButton(TEXT("All policies"), TEXT("Return to the full policy atlas"), FOnClicked::CreateSP(this, &SFourYearsScreen::AllPolicies));
		AddButton(TEXT("Return to the conversation"), TEXT("Review your adviser's request and your promise"), FOnClicked::CreateSP(this, &SFourYearsScreen::SelectPage, EFourYearsPage::Advisers));
	}
	FString Department;
	for (const FFourYearsPolicy& Policy : Game.GetPolicies())
	{
		if (!FocusPolicyId.IsEmpty() && Policy.Id != FocusPolicyId) continue;
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
					+ SVerticalBox::Slot().AutoHeight()[Label(Detail, 12, Muted)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(8.f, 0.f))
				[
					SNew(SButton).ButtonStyle(&ConversationButton()).IsFocusable(false).ContentPadding(FMargin(10.f, 4.f)).IsEnabled(Policy.Level > 0 && !Status.bDecisionFiled && !Status.bTermOver)
					.OnClicked(FOnClicked::CreateSP(this, &SFourYearsScreen::ChangePolicy, Policy.Id, Policy.Level - 1))
					[Label(TEXT("−"), 14, Ink, true)]
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
					SNew(SButton).ButtonStyle(&ConversationButton()).IsFocusable(false).ContentPadding(FMargin(10.f, 4.f)).IsEnabled(Policy.Level < 4 && !Status.bDecisionFiled && !Status.bTermOver)
					.OnClicked(FOnClicked::CreateSP(this, &SFourYearsScreen::ChangePolicy, Policy.Id, Policy.Level + 1))
					[Label(TEXT("+"), 14, Ink, true)]
				]
			]
		];
	}
}

TSharedRef<SWidget> SFourYearsScreen::Portrait(UFourYearsSubsystem& Game, const FFourYearsAdviser& Adviser, float Size)
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
		return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Gold).Padding(2.f)
		[SNew(SBox).WidthOverride(Size).HeightOverride(Size)[SNew(SImage).Image(Brush.Get())]];
	}
	// Acting replacements have no portrait yet; show their initials instead.
	return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
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
    const TArray<FFourYearsAdviser> Advisers = Game.GetAdvisers();
    AddText(TEXT("FOUR YEARS  /  THE OVAL OFFICE  /  PRIVATE AUDIENCE"), 11, Gold, true);
    if (!SelectedAdviser.IsEmpty())
    {
        for (const FFourYearsAdviser& Adviser : Advisers)
        {
            if (Adviser.Id == SelectedAdviser) { BuildConversation(Game, Adviser); return; }
        }
        SelectedAdviser.Reset();
    }
    AddText(TEXT("The people in your corner"), 30, Ink, true);
    AddText(FString::Printf(TEXT("%d appointments remaining   /   %d political capital   /   Quarter %d of 16"),
        Status.AppointmentsLeft, Status.Capital, FMath::Min(Game.GetBriefing().Quarter + 1, 16)), 14, Muted);
    AddText(TEXT("Choose a face to begin a conversation. Reading costs nothing. Committing to a response uses one appointment."), 14, Muted);
    TSharedPtr<SHorizontalBox> Row;
    for (int32 Index = 0; Index < Advisers.Num(); ++Index)
    {
        const FFourYearsAdviser& Adviser = Advisers[Index];
        RosterIds.Add(Adviser.Id);
        if (Index % 2 == 0)
        {
            Row = SNew(SHorizontalBox);
            Content->AddSlot().AutoHeight().Padding(0.f, 8.f)[Row.ToSharedRef()];
        }
        FString Availability = Adviser.bResigned ? TEXT("Unavailable / resigned")
            : Status.bTermOver ? TEXT("Term complete") : Status.bDecisionFiled ? TEXT("Decision filed / next quarter")
            : Adviser.bMetThisQuarter ? TEXT("Met this quarter") : Status.AppointmentsLeft == 0 ? TEXT("No appointments left")
            : !Adviser.Promise.IsEmpty() ? TEXT("A promise is waiting") : TEXT("Ready to talk");
        Row->AddSlot().FillWidth(1.f).Padding(Index % 2 == 0 ? FMargin(0.f, 0.f, 8.f, 0.f) : FMargin(8.f, 0.f, 0.f, 0.f))
        [
            SNew(SButton).ButtonStyle(&ConversationButton()).IsFocusable(false).ContentPadding(16.f)
            .OnClicked(FOnClicked::CreateSP(this, &SFourYearsScreen::TalkTo, Adviser.Id))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 16.f, 0.f)[Portrait(Game, Adviser, 118.f)]
                + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()[Label(FString::Printf(TEXT("%d  /  %s"), Index + 1, *Adviser.Name), 18, Ink, true)]
                    + SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)[Label(Adviser.Role, 12, Muted)]
                    + SVerticalBox::Slot().AutoHeight()[Label(Availability, 12, Gold, true)]
                    + SVerticalBox::Slot().AutoHeight().Padding(0.f, 7.f, 0.f, 0.f)[Label(FString::Printf(TEXT("Relationship  %d / 100"), Adviser.Relationship), 12, Muted)]
                ]
            ]
        ];
    }
    AddText(TEXT("1–4 select a person   /   E returns to the room   /   P policies   /   C Congress"), 12, Muted);
    const TArray<FString> Record = Game.GetPromiseRecord();
    if (Record.Num())
    {
        AddText(TEXT("YOUR WORD, ON THE RECORD"), 12, Gold, true);
        for (const FString& Line : Record) AddText(Line, 13, Line.StartsWith(TEXT("Broken")) ? Alert : Line.StartsWith(TEXT("Kept")) ? Good : Ink);
    }
}

void SFourYearsScreen::SetDialogue(const FString& Text)
{
    if (DialogueText != Text) { DialogueText = Text; RevealedCharacters = 0.f; }
}

void SFourYearsScreen::BuildConversation(UFourYearsSubsystem& Game, const FFourYearsAdviser& Adviser)
{
    const FFourYearsStatus Status = Game.GetStatus();
    const bool CanMeet = !Status.bDecisionFiled && !Status.bTermOver && !Adviser.bMetThisQuarter && !Adviser.bResigned && Status.AppointmentsLeft > 0;
    const FString Voice = Adviser.bActing ? TEXT("") : Adviser.Id;
    FString Cue = Adviser.Relationship < 35 ? TEXT("strained") : Adviser.Relationship >= 65 ? TEXT("trusted") : TEXT("greeting");
    if (!Adviser.Promise.IsEmpty()) Cue = TEXT("pending");
    if (Adviser.bMetThisQuarter) Cue = TEXT("met");
    if (Status.AppointmentsLeft == 0 && !Adviser.bMetThisQuarter) Cue = TEXT("busy");
    if (Status.bDecisionFiled) Cue = TEXT("closed");
    if (Status.bTermOver) Cue = TEXT("ended");
    if (Adviser.bResigned) Cue = TEXT("resigned");
    FString Line = Game.GetDialogueLine(Voice, Cue);
    if (const FString* Reply = ConversationReplies.Find(Adviser.Id)) Line = *Reply;
    SetDialogue(Line);
    AddText(Adviser.Name, 32, Ink, true);
    AddText(Adviser.Role + (Adviser.bActing ? TEXT("  /  ACTING") : TEXT("")), 14, Gold, true);
    Content->AddSlot().AutoHeight().Padding(0.f, 14.f)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 0.f, 22.f, 0.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[Portrait(Game, Adviser, 212.f)]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 4.f)
            [Label(FString::Printf(TEXT("RELATIONSHIP  %d / 100"), Adviser.Relationship), 12, Gold, true)]
            + SVerticalBox::Slot().AutoHeight()
            [SNew(SProgressBar).Percent(Adviser.Relationship / 100.f).FillColorAndOpacity(Adviser.Relationship < 35 ? Alert : Good)]
        ]
        + SHorizontalBox::Slot().FillWidth(1.f)
        [
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Gold).Padding(2.f)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor(0.055f, 0.085f, 0.12f)).Padding(22.f)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()[Label(TEXT("PRIVATE CONVERSATION"), 11, Gold, true)]
                    + SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f)
                    [
                        SNew(SBox).MinDesiredHeight(136.f)
                        [SAssignNew(Speech, STextBlock).Text(FText::FromString(DialogueText.Left(static_cast<int32>(RevealedCharacters))))
                            .Font(Font(21)).ColorAndOpacity(Ink).AutoWrapText(true)]
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [SNew(SButton).ButtonStyle(&ConversationButton()).IsFocusable(false).ContentPadding(8.f)
                        .OnClicked(FOnClicked::CreateSP(this, &SFourYearsScreen::FinishDialogue))
                        [Label(TEXT("SPACE / Show the complete line"), 11, Muted)]]
                ]
            ]
        ]
    ];
    if (!Adviser.bResigned)
    {
        AddText(Adviser.Promise.IsEmpty() ? TEXT("ON THE TABLE  /  ") + Adviser.Request : TEXT("YOUR COMMITMENT  /  ") + Adviser.Promise, 15, Gold, true);
    }
    BuildMessage();
    if (CanMeet)
    {
        DialogueOptions = Adviser.Options;
        AddText(FString::Printf(TEXT("YOUR RESPONSE  /  %d appointments left"), Status.AppointmentsLeft), 12, Muted, true);
        for (int32 Index = 0; Index < DialogueOptions.Num(); ++Index)
        {
            const FFourYearsMeetingOption& Option = DialogueOptions[Index];
            AddButton(FString::Printf(TEXT("%d   %s"), Index + 1, *Option.Label), Option.Detail, FOnClicked::CreateSP(this, &SFourYearsScreen::Respond, Index));
        }
    }
    else
    {
        const FString Reason = Adviser.bResigned ? TEXT("This adviser has left the administration.") : Status.bTermOver ? TEXT("Your term has ended.")
            : Status.bDecisionFiled ? TEXT("Meetings reopen next quarter: this quarter's decision is already filed.")
            : Adviser.bMetThisQuarter ? TEXT("Meeting recorded. You can still work on the policy before filing your decision.")
            : TEXT("Both appointments are used. You can meet again next quarter.");
        AddText(Reason, 14, Muted);
    }
    if (!Adviser.RequestedPolicyId.IsEmpty() && !Adviser.bResigned)
        AddButton(TEXT("Open the policy we're discussing"), TEXT("Review the target, cost and implementation delay"), FOnClicked::CreateSP(this, &SFourYearsScreen::OpenRequestedPolicy));
    AddButton(TEXT("Back to the sitting area"), TEXT("Choose another person / Backspace"), FOnClicked::CreateSP(this, &SFourYearsScreen::AdviserRoster));
}

FReply SFourYearsScreen::FinishDialogue()
{
    RevealedCharacters = static_cast<float>(DialogueText.Len());
    if (Speech.IsValid()) Speech->SetText(FText::FromString(DialogueText));
    return FReply::Handled();
}

FReply SFourYearsScreen::TalkTo(FString AdviserId)
{
    SelectedAdviser = AdviserId;
    DialogueText.Reset();
    Message.Reset();
    bResetScroll = true;
    bNeedsRebuild = true;
    return FReply::Handled();
}

FReply SFourYearsScreen::AdviserRoster()
{
    SelectedAdviser.Reset();
    Message.Reset();
    bResetScroll = true;
    bNeedsRebuild = true;
    return FReply::Handled();
}

FReply SFourYearsScreen::OpenRequestedPolicy()
{
    if (UFourYearsSubsystem* Game = Subsystem.Get())
        for (const FFourYearsAdviser& Adviser : Game->GetAdvisers())
            if (Adviser.Id == SelectedAdviser) { FocusPolicyId = Adviser.RequestedPolicyId; break; }
    ShowPage(EFourYearsPage::Policies);
    return FReply::Handled();
}

FReply SFourYearsScreen::AllPolicies()
{
    FocusPolicyId.Reset();
    ShowPage(EFourYearsPage::Policies);
    return FReply::Handled();
}

FReply SFourYearsScreen::Respond(int32 Index)
{
    if (Page != EFourYearsPage::Advisers || !DialogueOptions.IsValidIndex(Index)) return FReply::Handled();
    // Revalidate after deferred UI rebuilds: a double-click must never spend a second appointment.
    if (UFourYearsSubsystem* Game = Subsystem.Get())
    {
        const FFourYearsStatus Status = Game->GetStatus();
        if (Status.bDecisionFiled || Status.bTermOver || Status.AppointmentsLeft <= 0) return FReply::Handled();
        for (const FFourYearsAdviser& Adviser : Game->GetAdvisers())
            if (Adviser.Id == SelectedAdviser && !Adviser.bMetThisQuarter && !Adviser.bResigned)
                return Meet(SelectedAdviser, DialogueOptions[Index].Response);
    }
    return FReply::Handled();
}

void SFourYearsScreen::BuildCongress(UFourYearsSubsystem& Game)
{
	const FFourYearsStatus Status = Game.GetStatus();
	const FFourYearsCongress Congress = Game.GetCongress();
	AddText(TEXT("CAPITOL  ·  LEGISLATIVE AGENDA"), 11, Gold, true);
	AddText(TEXT("Build a majority"), 26, Ink, true);
	AddText(FString::Printf(TEXT("Political capital %d/20   ·   Appointments left %d of 2   ·   Opposition momentum %d"),
		Status.Capital, Status.AppointmentsLeft, Congress.OppositionMomentum), 12, Muted);
	AddText(TEXT("The chamber has 100 seats and a bill needs 51 votes. Introducing a bill costs 2 capital, each amendment 1, lobbying a faction 1 plus an appointment, and a floor vote 2. A failed bill can be amended and voted on again next quarter. Passing a bill takes its program to full strength."), 11, Muted);
	if (Status.bDecisionFiled || Status.bTermOver)
	{
		AddText(Status.bTermOver ? TEXT("The term is over.") : TEXT("This quarter's decision is filed. Congress reconvenes next quarter."), 12, Alert, true);
	}
	BuildMessage();
	if (Congress.bHasActiveBill)
	{
		const FFourYearsBill& Bill = Congress.ActiveBill;
		Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 12.f, 0.f, 2.f))[Label(Bill.Status == TEXT("failed") ? TEXT("ON THE FLOOR  ·  DEFEATED, AWAITING REVISION") : TEXT("ON THE FLOOR"), 11, Gold, true)];
		AddText(Bill.Name, 20, Ink, true);
		AddText(Bill.Story, 12, Ink);
		AddText(FString::Printf(TEXT("Sponsor %s · takes %s to full strength · %d debt when passed%s"), *Bill.Sponsor, *Bill.Policy, Bill.Cost,
			Bill.LastYes >= 0 ? *FString::Printf(TEXT(" · last vote %d–%d"), Bill.LastYes, 100 - Bill.LastYes) : TEXT("")), 11, Muted);
		const bool bMajority = Congress.ProjectedYes >= 51;
		AddText(FString::Printf(TEXT("Whip count: %d of 100 votes, %s"), Congress.ProjectedYes,
			bMajority ? TEXT("enough to pass") : *FString::Printf(TEXT("%d short of a majority"), 51 - Congress.ProjectedYes)), 16, bMajority ? Good : Alert, true);
		for (const FFourYearsBloc& Bloc : Congress.Blocs)
		{
			TSharedRef<SVerticalBox> Body = SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Label(FString::Printf(TEXT("%s  ·  %d seats"), *Bloc.Name, Bloc.Seats), 13, Ink, true)]
				+ SVerticalBox::Slot().AutoHeight()[Label(FString::Printf(TEXT("Support %d%% → %d yes votes · wants: %s"), Bloc.Support, Bloc.YesVotes, *Bloc.Want), 10, Muted)];
			TSharedRef<SWidget> Action = Bloc.bLobbied
				? StaticCastSharedRef<SWidget>(Label(TEXT("Lobbied"), 11, Good, true))
				: StaticCastSharedRef<SWidget>(SNew(SButton).ButtonStyle(&ConversationButton()).IsFocusable(false).ContentPadding(FMargin(10.f, 5.f))
					.OnClicked(FOnClicked::CreateSP(this, &SFourYearsScreen::Lobby, Bloc.Id))
					[Label(TEXT("Lobby (+15 support)"), 11, Ink, true)]);
			Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 3.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(Card)
				.Padding(FMargin(12.f, 8.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[Body]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(10.f, 0.f, 0.f, 0.f))[Action]
				]
			];
		}
		Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 12.f, 0.f, 2.f))[Label(TEXT("AMENDMENTS"), 11, Gold, true)];
		for (const FFourYearsAmendment& Amendment : Congress.Amendments)
		{
			if (Amendment.bAdopted)
			{
				AddText(TEXT("✓  ") + Amendment.Name + TEXT(" — adopted. ") + Amendment.Detail, 11, Good);
			}
			else
			{
				AddButton(TEXT("Add amendment: ") + Amendment.Name, Amendment.Detail + TEXT(" Costs 1 political capital."), FOnClicked::CreateSP(this, &SFourYearsScreen::Amend, Amendment.Id));
			}
		}
		Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 12.f, 0.f, 0.f))[Label(TEXT("FLOOR VOTE"), 11, Gold, true)];
		if (Bill.bVotedThisQuarter)
		{
			AddText(TEXT("You have already called a vote on this bill this quarter. Revise it and try again next quarter."), 12, Muted);
		}
		else
		{
			AddButton(TEXT("Call the vote"), FString::Printf(TEXT("Costs 2 political capital · the whip count projects %d yes votes · a defeat gives the opposition momentum"), Congress.ProjectedYes),
				FOnClicked::CreateSP(this, &SFourYearsScreen::Vote));
		}
	}
	else if (Congress.Available.Num() > 0)
	{
		Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 12.f, 0.f, 2.f))[Label(TEXT("INTRODUCE A BILL"), 11, Gold, true)];
		for (const FFourYearsBill& Bill : Congress.Available)
		{
			AddButton(Bill.Name, FString::Printf(TEXT("%s Sponsor %s · takes %s to full strength · %d debt when passed · costs 2 political capital to introduce"),
				*Bill.Story, *Bill.Sponsor, *Bill.Policy, Bill.Cost), FOnClicked::CreateSP(this, &SFourYearsScreen::Introduce, Bill.Id));
		}
	}
	else
	{
		AddText(TEXT("Every act has been introduced this term."), 12, Muted);
	}
	if (Congress.Passed.Num() > 0)
	{
		Content->AddSlot().AutoHeight().Padding(FMargin(0.f, 14.f, 0.f, 2.f))[Label(TEXT("LAWS IN FORCE"), 11, Gold, true)];
		for (const FFourYearsBill& Bill : Congress.Passed)
		{
			AddText(FString::Printf(TEXT("%s · %d%% delivered%s"), *Bill.Name, Bill.Progress,
				Bill.Amendments.Num() ? *(TEXT(" · with ") + FString::Join(Bill.Amendments, TEXT(", "))) : TEXT("")), 12, Bill.Progress >= 100 ? Good : Ink);
		}
	}
}

void SFourYearsScreen::SetMessage(const FFourYearsActionResult& Result)
{
	Message = Result.Message;
	// A defeated vote is a completed action but a bad outcome.
	bMessageOk = Result.bOk && !Result.Message.StartsWith(TEXT("Defeated"));
}

FReply SFourYearsScreen::Introduce(FString BillId)
{
	if (UFourYearsSubsystem* Game = Subsystem.Get()) SetMessage(Game->IntroduceBill(BillId));
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsScreen::Amend(FString AmendmentId)
{
	if (UFourYearsSubsystem* Game = Subsystem.Get()) SetMessage(Game->AddAmendment(AmendmentId));
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsScreen::Lobby(FString BlocId)
{
	if (UFourYearsSubsystem* Game = Subsystem.Get()) SetMessage(Game->LobbyBloc(BlocId));
	bNeedsRebuild = true;
	return FReply::Handled();
}

FReply SFourYearsScreen::Vote()
{
	if (UFourYearsSubsystem* Game = Subsystem.Get()) SetMessage(Game->CallVote());
	bNeedsRebuild = true;
	return FReply::Handled();
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
        ConversationReplies.Reset();
        SelectedAdviser.Reset();
        FocusPolicyId.Reset();
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
        ConversationReplies.Reset();
        SelectedAdviser.Reset();
        FocusPolicyId.Reset();
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
		const FFourYearsActionResult Result = Game->MeetAdviser(AdviserId, Response);
        SetMessage(Result);
        FString Voice = AdviserId;
        for (const FFourYearsAdviser& Adviser : Game->GetAdvisers())
            if (Adviser.Id == AdviserId && Adviser.bActing) Voice.Reset();
        ConversationReplies.Add(AdviserId, Game->GetDialogueLine(Voice, Result.bOk ? Response : TEXT("refused")));
        DialogueOptions.Reset();
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
