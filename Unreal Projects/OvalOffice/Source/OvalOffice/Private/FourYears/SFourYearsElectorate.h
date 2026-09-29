#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "FourYears/FourYearsSubsystem.h"
class SVerticalBox;class SSearchBox;class SUSVoterAtlas;
class SFourYearsElectorate:public SCompoundWidget {
public:
 SLATE_BEGIN_ARGS(SFourYearsElectorate){} SLATE_ARGUMENT(UFourYearsSubsystem*,Subsystem) SLATE_EVENT(FSimpleDelegate,OnClose) SLATE_END_ARGS()
 void Construct(const FArguments& Args);
 bool SupportsKeyboardFocus()const override{return true;}
 void Tick(const FGeometry&,double,float)override;
 FReply OnKeyDown(const FGeometry&,const FKeyEvent&)override;
private:
 void Rebuild();void Select(FString Id);FReply Close();
 TWeakObjectPtr<UFourYearsSubsystem> Game;
 TSharedPtr<SVerticalBox> Body;
 TSharedPtr<SUSVoterAtlas> Atlas;
 TSharedPtr<SSearchBox> Search;
 FSimpleDelegate OnClose;
 FString Selected=TEXT("17031"),Message;
 int Page=0,Layer=1;
 bool Dirty=true,Closing=false,Expanded=false;
};
