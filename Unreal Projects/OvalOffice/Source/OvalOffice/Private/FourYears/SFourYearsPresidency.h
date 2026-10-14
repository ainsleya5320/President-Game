#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "FourYears/FourYearsSubsystem.h"
DECLARE_DELEGATE_OneParam(FPresidencyNavigate,FString);
class SVerticalBox;
class SScrollBox;
class SFourYearsPresidency : public SCompoundWidget {
public:
 SLATE_BEGIN_ARGS(SFourYearsPresidency) : _Page(TEXT("overview")) {}
 SLATE_ARGUMENT(TWeakObjectPtr<UFourYearsSubsystem>,Subsystem)
 SLATE_ARGUMENT(FString,Page)
 SLATE_EVENT(FPresidencyNavigate,OnNavigate)
 SLATE_END_ARGS()
 void Construct(const FArguments& Args);
 virtual bool SupportsKeyboardFocus()const override{return true;}
 virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent&)override;
 virtual void Tick(const FGeometry&,double,float)override;
private:
 void Rebuild();
 void Text(const FString&,int32 Size=16,bool Gold=false);
 void Button(const FString&,const FString&,TFunction<void()> Action,bool Enabled=true);
 void Nav(const FString&,const FString&,const FString&);
 void Act(const FString&,const FString& Target=TEXT(""));
 void Executive(const FString&,const FString& Target,const FString& Choice=TEXT(""));
 void SetPage(const FString&);
 TWeakObjectPtr<UFourYearsSubsystem> Game;
 FPresidencyNavigate Navigate;
 TSharedPtr<SVerticalBox> Content;
 TSharedPtr<SScrollBox> Scroll;
 TSharedPtr<FSlateBrush> PortraitBrush;
 FString Page,Message;
 bool Dirty=true,ConfirmRestart=false;
};
