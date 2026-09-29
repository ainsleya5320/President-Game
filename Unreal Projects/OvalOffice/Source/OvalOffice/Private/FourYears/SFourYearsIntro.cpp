#include "SFourYearsIntro.h"

#include "FourYears/FourYearsSubsystem.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
const FLinearColor Cream(.95f, .93f, .87f);
const FLinearColor Gold(.83f, .67f, .37f);
const FLinearColor Navy(.018f, .030f, .053f);
TSharedRef<STextBlock> Text(const TCHAR* Words, int32 Size, FLinearColor Color = Cream, bool Bold = false, bool Wrap = true)
{
	return SNew(STextBlock).Text(FText::FromString(Words))
		.Font(FCoreStyle::GetDefaultFontStyle(Bold ? "Bold" : "Regular", Size))
		.ColorAndOpacity(Color).AutoWrapText(Wrap);
}
const FButtonStyle& IntroButton()
{
	static const FButtonStyle Style = FButtonStyle()
		.SetNormal(FSlateColorBrush(FLinearColor(.09f,.13f,.19f)))
		.SetHovered(FSlateColorBrush(FLinearColor(.21f,.27f,.33f)))
		.SetPressed(FSlateColorBrush(FLinearColor(.29f,.24f,.15f)));
	return Style;
}

class SInaugurationImage : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SInaugurationImage) {} SLATE_ARGUMENT(SFourYearsIntro*, Intro) SLATE_END_ARGS()
	void Construct(const FArguments& Args) { Intro = Args._Intro; SetClipping(EWidgetClipping::ClipToBounds); }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1280,720); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out,
		int32 Layer, const FWidgetStyle&, bool) const override
	{
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor::Black);
		const FSlateBrush* Brush = Intro->GetShotBrush();
		if (!Brush->GetResourceObject()) return Layer;
		const FVector2D View = G.GetLocalSize();
		const float Scale = FMath::Max(View.X / Brush->ImageSize.X, View.Y / Brush->ImageSize.Y) * (1.035f + .045f * Intro->GetShotProgress());
		const FVector2D Size = FVector2D(Brush->ImageSize) * Scale;
		FVector2D Offset = (View - Size) * .5f;
		Offset.X += (Intro->GetShotProgress() - .5f) * 12.f;
		FSlateDrawElement::MakeBox(Out, Layer+1, G.ToPaintGeometry(Size, FSlateLayoutTransform(Offset)), Brush,
			ESlateDrawEffect::None, FLinearColor(1,1,1,Intro->GetShotOpacity()));
		return Layer+1;
	}
private:
	SFourYearsIntro* Intro = nullptr;
};
}

void SFourYearsIntro::Construct(const FArguments& Args)
{
	OnFinished = Args._OnFinished;
	for (int32 I=0; I<3; ++I)
	{
		if (UTexture2D* Texture = Args._Subsystem ? Args._Subsystem->GetInaugurationImage(I) : nullptr)
		{
			Images[I].SetResourceObject(Texture);
			Images[I].ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
			Images[I].DrawAs = ESlateBrushDrawType::Image;
		}
	}
	BuildContent();
}

float SFourYearsIntro::GetShotOpacity() const
{
	if (bBriefing) return .38f;
	const float Time = FMath::Fmod(Elapsed,6.f);
	return FMath::Clamp(FMath::Min(Time/.65f,(6.f-Time)/.65f),0.f,1.f);
}

void SFourYearsIntro::BuildContent()
{
	TSharedRef<SOverlay> Layers = SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SInaugurationImage).Intro(this)];
	if (!bBriefing)
	{
		Layers->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(44,32)
		[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(.008f,.015f,.028f,.85f)).Padding(FMargin(12,8))
			[Text(TEXT("FOUR YEARS  /  A NEW PRESIDENCY"),16,Gold,true,false)]];
		Layers->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(32)
		[SNew(SButton).IsFocusable(false).ButtonStyle(&IntroButton()).ContentPadding(FMargin(20,12)).OnClicked(this,&SFourYearsIntro::Skip)
			[Text(TEXT("Skip to briefing  [Enter]"),16,Cream,false,false)]];
		Layers->AddSlot().VAlign(VAlign_Bottom)
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(.008f,.015f,.028f,.91f)).Padding(FMargin(44,24))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
				[ SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",15)).ColorAndOpacity(Gold)
					.Text_Lambda([this]{return FText::FromString(FString::Printf(TEXT("INAUGURATION DAY     /     0%d"),GetShotIndex()+1));}) ]
				+ SVerticalBox::Slot().AutoHeight()
				[ SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",38)).ColorAndOpacity(Cream)
					.Text_Lambda([this]{static const TCHAR* Titles[]={TEXT("The country has chosen."),TEXT("An oath. A promise."),TEXT("Now comes the hard part.")};return FText::FromString(Titles[GetShotIndex()]);}) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,10,0,0)
				[ SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",19)).ColorAndOpacity(Cream)
					.Text_Lambda([this]{static const TCHAR* Lines[]={TEXT("Millions of voices. One office. Four years to earn their trust."),TEXT("Preserve, protect and defend the Constitution of the United States."),TEXT("The celebrations fade. The decisions are yours.")};return FText::FromString(Lines[GetShotIndex()]);}) ]
			]
		];
	}
	else
	{
		Layers->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(900)
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(Gold).Padding(2)
				[
					SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(Navy).Padding(FMargin(38,24))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[Text(TEXT("JANUARY 20  /  YOUR FIRST DAY"),15,Gold,true)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)[Text(TEXT("Welcome, President."),34,Cream,true)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[Text(TEXT("You have been elected President of the United States. The oath is taken. The Oval Office is yours. Now you must turn your promises into a presidency."),18)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[Text(TEXT("Your citizens want prosperity and security. Your allies need leadership. Congress has its own ambitions, and every adviser sees a different path forward."),18)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,18)[Text(TEXT("The fate of the free world rests on your shoulders. You cannot please everyone. Choose whom to trust, decide what is worth fighting for, and live with the consequences."),18)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[Text(TEXT("YOUR FIRST STEPS"),15,Gold,true)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,6)[Text(TEXT("Walk with WASD and look with the mouse. Press E by the sofas to meet your advisers, or at the Resolute Desk for your first briefing."),17)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,24)[Text(TEXT("P: policies    C: Congress    I: replay this introduction"),16,Gold)]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
						[SNew(SButton).IsFocusable(false).ButtonStyle(&IntroButton()).ContentPadding(FMargin(24,14)).OnClicked(this,&SFourYearsIntro::Continue)
							[Text(TEXT("Continue to the game  [Enter]"),18,Cream,true,false)]]
					]
				]
			]
		];
	}
	ChildSlot[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor::Black).Padding(0)
		[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
			[SNew(SBox).WidthOverride(1280).HeightOverride(720)[Layers]]]];
}

void SFourYearsIntro::Tick(const FGeometry& G,double Time,float Delta)
{
	SCompoundWidget::Tick(G,Time,Delta);
	if (bFinishRequested)
	{
		bFinishRequested=false;
		const TSharedRef<SFourYearsIntro> KeepAlive=SharedThis(this);
		OnFinished.ExecuteIfBound();
		return;
	}
	if (!bBriefing)
	{
		Elapsed += FMath::Min(Delta,.25f); // Loading hitches must not swallow an entire shot.
		if (Elapsed>=18.f) SkipToBriefing();
	}
	if (bRebuild) {bRebuild=false;BuildContent();}
	Invalidate(EInvalidateWidgetReason::Paint);
}
void SFourYearsIntro::SkipToBriefing() {bBriefing=true;bRebuild=true;}
FReply SFourYearsIntro::Skip() {SkipToBriefing();return FReply::Handled();}
FReply SFourYearsIntro::Continue() {bFinishRequested=true;return FReply::Handled();}
FReply SFourYearsIntro::OnKeyDown(const FGeometry&,const FKeyEvent& Event)
{
	if (!Event.IsRepeat() && (Event.GetKey()==EKeys::Enter || Event.GetKey()==EKeys::SpaceBar || Event.GetKey()==EKeys::Escape))
		return bBriefing ? Continue() : Skip();
	return FReply::Handled();
}
