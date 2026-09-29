#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"

class UFourYearsSubsystem;

// An animated still-image prologue, followed by a player-paced welcome briefing.
class SFourYearsIntro : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFourYearsIntro) {}
		SLATE_ARGUMENT(UFourYearsSubsystem*, Subsystem)
		SLATE_EVENT(FSimpleDelegate, OnFinished)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent&) override;
	virtual void Tick(const FGeometry&, double, float) override;
	bool IsBriefing() const { return bBriefing; }
	int32 GetShotIndex() const { return FMath::Clamp(int32(Elapsed / 6.f), 0, 2); }
	void SkipToBriefing();
	const FSlateBrush* GetShotBrush() const { return &Images[bBriefing ? 2 : GetShotIndex()]; }
	float GetShotProgress() const { return bBriefing ? 1.f : FMath::Fmod(Elapsed, 6.f) / 6.f; }
	float GetShotOpacity() const;

private:
	void BuildContent();
	FReply Skip();
	FReply Continue();
	FSimpleDelegate OnFinished;
	FSlateBrush Images[3];
	float Elapsed = 0.f;
	bool bBriefing = false;
	bool bRebuild = false;
	bool bFinishRequested = false;
};
