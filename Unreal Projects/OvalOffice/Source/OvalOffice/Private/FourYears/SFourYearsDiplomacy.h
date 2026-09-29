#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "FourYears/FourYearsSubsystem.h"
class SVerticalBox;
class SFourYearsDiplomacy : public SCompoundWidget
{
public:
 SLATE_BEGIN_ARGS(SFourYearsDiplomacy){} SLATE_ARGUMENT(UFourYearsSubsystem*,Subsystem) SLATE_EVENT(FSimpleDelegate,OnClose) SLATE_END_ARGS()
 void Construct(const FArguments& Args);
 bool SupportsKeyboardFocus() const override{return true;}
 void Tick(const FGeometry&,double,float) override;
 FReply OnKeyDown(const FGeometry&,const FKeyEvent&) override;
private:
 void Rebuild();
 FReply SelectRegion(FString Id);
 FReply SelectPage(int32 Index);
 FReply Action(FString Id);
 FReply CrisisAction(int32 Index,FString Id);
 FReply Doctrine(FString Id);
 FReply Close();
 void MessageResult(const FFourYearsActionResult&);
 TWeakObjectPtr<UFourYearsSubsystem> Game;
 TSharedPtr<SVerticalBox> Body;
 FSimpleDelegate OnClose;
 FString Selected=TEXT("gulf");
 FString Message;
 int32 Page=0;
 bool Dirty=true,Closing=false,Success=true;
};
