#include "SFourYearsPresidency.h"
#include "FourYears/Core/FourYearsPresidency.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "InputCoreTypes.h"
#include "Widgets/Images/SImage.h"
namespace {
using V=FourYears::JsonValue;
const FLinearColor Ink(.94f,.93f,.87f),Gold(.87f,.69f,.37f),Muted(.57f,.68f,.73f),Bg(.022f,.041f,.064f),Card(.07f,.115f,.16f);
FString F(const std::string& S){return UTF8_TO_TCHAR(S.c_str());}
FString Get(const V& X,const char* Key){return F(X.Get(Key).AsString());}
TSharedRef<STextBlock> Label(FString T,int Size,FLinearColor C=Ink){return SNew(STextBlock).Text(FText::FromString(T)).Font(FCoreStyle::GetDefaultFontStyle(Size>=22?"Bold":"Regular",Size)).ColorAndOpacity(C).AutoWrapText(true);}
const FButtonStyle& Style(){static FButtonStyle S=FButtonStyle().SetNormal(FSlateColorBrush(Card)).SetHovered(FSlateColorBrush(FLinearColor(.13f,.22f,.28f))).SetPressed(FSlateColorBrush(FLinearColor(.26f,.22f,.12f))).SetDisabled(FSlateColorBrush(FLinearColor(.045f,.06f,.07f)));return S;}
FString Place(FString Id){return Id==TEXT("oval")?TEXT("Oval Office"):Id==TEXT("cabinet")?TEXT("Cabinet Room"):Id==TEXT("residence")?TEXT("Executive Residence"):Id==TEXT("limo")?TEXT("Presidential limo"):TEXT("Air Force One");}
}
void SFourYearsPresidency::Construct(const FArguments& A){
 Game=A._Subsystem;Navigate=A._OnNavigate;Page=A._Page;
 TSharedRef<SVerticalBox> Side=SNew(SVerticalBox);
 Side->AddSlot().AutoHeight().Padding(14,22)[Label(TEXT("FOUR\nYEARS"),34,Gold)];
 Side->AddSlot().AutoHeight().Padding(14,0,14,24)[Label(TEXT("THE PRESIDENCY\nA country. A term. A legacy."),12,Muted)];
 const TArray<TPair<FString,FString>> Tabs={{TEXT("overview"),TEXT("01  YOUR MANDATE")},{TEXT("story"),TEXT("02  THIS CHAPTER")},{TEXT("travel"),TEXT("03  PLACES")},{TEXT("relations"),TEXT("04  GOVERNMENT")},{TEXT("record"),TEXT("05  THE RECORD")}};
 for(const auto& T:Tabs){const FString Id=T.Key;Side->AddSlot().AutoHeight().Padding(6,5)[SNew(SButton).ButtonStyle(&Style()).IsFocusable(false).ContentPadding(FMargin(15,16)).OnClicked_Lambda([this,Id]{SetPage(Id);return FReply::Handled();})[Label(T.Value,13,Gold)]];}
 Side->AddSlot().FillHeight(1).VAlign(VAlign_Bottom).Padding(14)[Label(TEXT("WASD walk / mouse look\nE interact / J journal\nP policies / C Congress\nM world / V voters\n\nChoices autosave.\nTime advances at the desk."),12,Muted)];
 Side->AddSlot().AutoHeight().Padding(6,5)[SNew(SButton).ButtonStyle(&Style()).IsFocusable(false).ContentPadding(15).OnClicked_Lambda([this]{Navigate.ExecuteIfBound(TEXT("close"));return FReply::Handled();})[Label(TEXT("RETURN TO THE ROOM  [ESC]"),11,Gold)]];
 ChildSlot[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0,0,0,.8f)).Padding(18)
 [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1180).HeightOverride(760)
 [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Bg).Padding(0)
 [SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(235)[Side]]
 +SHorizontalBox::Slot().FillWidth(1).Padding(28,25)[SAssignNew(Scroll,SScrollBox)+SScrollBox::Slot()[SAssignNew(Content,SVerticalBox)]]]]]]];
}
void SFourYearsPresidency::SetPage(const FString& P){Page=P;Message.Reset();Dirty=true;Scroll->ScrollToStart();}
void SFourYearsPresidency::Tick(const FGeometry& G,double T,float D){SCompoundWidget::Tick(G,T,D);if(Dirty){Dirty=false;Rebuild();}}
FReply SFourYearsPresidency::OnKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::Escape||E.GetKey()==EKeys::J){Navigate.ExecuteIfBound(TEXT("close"));return FReply::Handled();}return SCompoundWidget::OnKeyDown(G,E);}
void SFourYearsPresidency::Text(const FString& T,int32 S,bool GoldText){Content->AddSlot().AutoHeight().Padding(0,5)[Label(T,S,GoldText?Gold:Ink)];}
void SFourYearsPresidency::Button(const FString& Title,const FString& Detail,TFunction<void()> Action,bool Enabled){Content->AddSlot().AutoHeight().Padding(0,5)[SNew(SButton).ButtonStyle(&Style()).IsFocusable(false).IsEnabled(Enabled).ContentPadding(FMargin(17,13)).OnClicked_Lambda([Action]{Action();return FReply::Handled();})[SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight()[Label(Title,17,Gold)]+SVerticalBox::Slot().AutoHeight().Padding(0,5)[Label(Detail,13,Muted)]]];}
void SFourYearsPresidency::Nav(const FString& T,const FString& D,const FString& N){Button(T,D,[this,N]{Navigate.ExecuteIfBound(N);});}
void SFourYearsPresidency::Act(const FString& A,const FString& T){if(auto* G=Game.Get()){Message=G->PresidencyAction(A,T).Message;Dirty=true;}}
void SFourYearsPresidency::Executive(const FString& A,const FString& T,const FString& C){if(auto* G=Game.Get()){Message=G->ExecutiveAction(A,T,C).Message;Dirty=true;}}
void SFourYearsPresidency::Rebuild(){
 Content->ClearChildren();auto* G=Game.Get();if(!G||!G->IsReady()){Text(TEXT("Unable to load the presidency."));return;}
 const auto& S=G->GetState();const auto& Sim=G->GetSimulation();const auto Vw=FourYears::Presidency::View(Sim,S);const auto& Ch=Vw.Get("chapter");const int Q=FMath::Min(15,int(S.Get("quarter").AsNumber()));const bool End=S.Get("ended").Truthy(),Open=!End&&S.Get("agendaChoice").IsNull();
 Text(End?TEXT("THE TERM IS COMPLETE"):FString::Printf(TEXT("YEAR %d / QUARTER %d     %s"),Q/4+1,Q%4+1,*Place(Get(Vw,"place")).ToUpper()),12,true);
 Content->AddSlot().AutoHeight().Padding(0,10)[SNew(SProgressBar).Percent(End?1.f:float(Q)/16).FillColorAndOpacity(Gold).BorderPadding(FVector2D(0,0))];
 if(!Message.IsEmpty())Text(Message,15,true);
 if(Page==TEXT("overview")){
  Text(End?Get(Vw,"endingTitle"):Get(Vw.Get("act"),"name"),34,true);Text(End?TEXT("You can keep exploring the rooms, read your legacy, or begin another presidency."):Get(Vw.Get("act"),"subtitle"),17);
  const auto Status=G->GetStatus();Text(FString::Printf(TEXT("APPROVAL %d%%     CAPITAL %d     FATIGUE %d/100"),Status.Approval,Status.Capital,int(Vw.Get("fatigue").AsNumber())),16,true);
  if(End){Button(TEXT("Read your epilogue"),TEXT("The election, your promises and the country you leave behind."),[this]{SetPage(TEXT("record"));});return;}
  Text(TEXT("Your term lasts 16 quarters. Build support, improve the country, deliver your platform and face reelection. Each quarter gives you a personal chapter plus a decision at the desk. Meetings and policy changes must happen before you file that decision."),16);
  if(S.Get("executive").Get("platform").IsNull()){
   Text(TEXT("First, choose the promise your presidency will be judged by."),21,true);
   for(const auto& M:Vw.Get("mandates").Items()){const FString Id=Get(M,"id");Button(Get(M,"name"),Get(M,"body"),[this,Id]{Act(TEXT("mandate"),Id);},Open);}
  }
  Button(Vw.Get("done").Truthy()?TEXT("PERSONAL CHAPTER / on the record"):TEXT("NEXT / ")+Get(Ch,"title"),TEXT("Meet ")+Get(Ch,"speaker")+TEXT(" in the ")+Place(Get(Ch,"place"))+TEXT(". Or delegate for trust -1."),[this]{SetPage(TEXT("story"));});
  Text(FString::Printf(TEXT("%d appointments left  /  %s  /  personal time %s"),Status.AppointmentsLeft,Status.bDecisionFiled?TEXT("quarterly decision filed"):TEXT("decision still open"),Vw.Get("activityAvailable").Truthy()?TEXT("available"):TEXT("used")),14);
  Nav(TEXT("GOVERN / policies and their consequences"),TEXT("Changes take time. Level 4 policies require legislation. Watch the quarterly report for effects."),TEXT("policies"));
  Nav(Status.bDecisionFiled?TEXT("FINISH QUARTER / return to the desk"):TEXT("WHEN READY / read your quarterly briefing"),TEXT("File one response, then advance. Any unfinished personal chapter is delegated automatically for trust -1."),TEXT("briefing"));
  Text(TEXT("YOUR PLATFORM"),20,true);
  for(const auto& Goal:Vw.Get("legacy").Get("goals").Items())Text(FString::Printf(TEXT("%s  %s: %.0f / target %.0f"),Goal.Get("met").Truthy()?TEXT("ACHIEVED"):TEXT("IN PROGRESS"),*Get(Goal,"name"),Goal.Get("value").AsNumber(),Goal.Get("target").AsNumber()),15);
  Nav(TEXT("The world beyond Washington"),TEXT("Diplomacy, trade and international crises."),TEXT("world"));Nav(TEXT("The electoral map"),TEXT("County coalitions, organizing and the road to 270."),TEXT("voters"));
 }else if(Page==TEXT("story")){
  Text(Get(Ch,"title"),32,true);
  if(auto* Texture=G->GetPortrait(Get(Ch,"speaker"))){PortraitBrush=MakeShared<FSlateBrush>();PortraitBrush->SetResourceObject(Texture);PortraitBrush->ImageSize=FVector2D(112,112);Content->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0,8)[SNew(SBox).WidthOverride(112).HeightOverride(112)[SNew(SImage).Image(PortraitBrush.Get())]];}
  Text(Get(Ch,"speaker")+TEXT(" / ")+Place(Get(Ch,"place")),17,true);Text(Get(Ch,"body"),20);if(!Get(Vw,"context").IsEmpty())Text(Get(Vw,"context"),15,true);
  if(Vw.Get("done").Truthy()||End){Text(TEXT("ON THE RECORD"),21,true);for(const auto& D:S.Get("presidency").Get("decisions").Items())if(int(D.Get("quarter").AsNumber())==Q){Text(Get(D,"choice"),20);Text(Get(D,"detail"),14);}Nav(TEXT("Return to the quarterly briefing"),TEXT("Continue governing or finish this quarter."),TEXT("briefing"));}
  else if(Get(Vw,"place")!=Get(Ch,"place")){Nav(TEXT("Travel to the ")+Place(Get(Ch,"place")),TEXT("Have this conversation in person. Press J after arriving to open the chapter."),TEXT("travel:")+Get(Ch,"place"));Button(TEXT("Delegate to staff"),TEXT("Trust -1. Skip this personal choice and continue governing."),[this]{Act(TEXT("delegate"));},Open);}
  else{int I=0;for(const auto& C:Ch.Get("choices").Items()){FString Id=FString::FromInt(I++);Button(Get(C,"label"),Get(C,"summary"),[this,Id]{Act(TEXT("story"),Id);},Open);}if(!Open)Text(TEXT("The desk decision is already filed. Advance the quarter to reopen your schedule."),15,true);}
 }else if(Page==TEXT("travel")){
  Text(TEXT("A life inside the presidency"),32,true);Text(TEXT("Move between familiar working spaces and the quieter places behind the office. Travel does not advance the quarter."),17);
  Nav(TEXT("Oval Office"),TEXT("The Resolute Desk, Maya Chen, Daniel Brooks and your protective detail."),TEXT("travel:oval"));
  Nav(TEXT("Cabinet Room"),TEXT("Meet the cabinet and build lasting support with congressional leaders."),TEXT("travel:cabinet"));
  Nav(TEXT("Executive Residence"),TEXT("A private sitting room, conversations after work and a chance to recover."),TEXT("travel:residence"));
  Nav(TEXT("Presidential limo"),TEXT("Explore the secure cabin, review a briefing or call a congressional leader."),TEXT("travel:limo"));
  Nav(TEXT("Air Force One"),TEXT("The presidential office, aircraft hallway and conference room. M opens diplomacy."),TEXT("travel:aircraft"));
  Text(TEXT("PERSONAL TIME / once each quarter"),21,true);Text(TEXT("Fatigue rises as the term goes on. At 70 or higher, advancing costs an extra capital and trust point. Choose quiet recovery or more political preparation."),15);
  Button(TEXT("A quiet evening / fatigue -20"),TEXT("Available in the residence. Shares this quarter's personal time with the limo briefing."),[this]{Act(TEXT("rest"));},Open&&Vw.Get("activityAvailable").Truthy()&&Get(Vw,"place")==TEXT("residence"));
  Button(TEXT("Review the route briefing / capital +2, fatigue +4"),TEXT("Available in the limo. This uses your personal time for the quarter."),[this]{Act(TEXT("prepare"));},Open&&Vw.Get("activityAvailable").Truthy()&&Get(Vw,"place")==TEXT("limo"));
 }else if(Page==TEXT("relations")){
  Text(TEXT("The people who make it possible"),31,true);Text(TEXT("Relationships influence every bill. A meeting costs 2 capital, adds 9 relationship points, and is available once per quarter in the Cabinet Room or limo. Relationships slowly drift toward neutral if you stop investing in them."),16);
  for(const auto& L:Vw.Get("leaders").Items()){const FString Id=Get(L,"id");Text(Get(L,"name")+TEXT(" / ")+Get(L,"role"),20,true);Text(Get(L,"want"),14);Text(FString::Printf(TEXT("Relationship %.0f/100   /   underlying bill support %+.1f points"),L.Get("relationship").AsNumber(),L.Get("bonus").AsNumber()),14);Button(TEXT("Make time for a private conversation"),TEXT("2 capital / relationship +9"),[this,Id]{Act(TEXT("contact"),Id);},Open&&Vw.Get("contactAvailable").Truthy()&&(Get(Vw,"place")==TEXT("cabinet")||Get(Vw,"place")==TEXT("limo")));}
  Nav(TEXT("Negotiate legislation"),TEXT("Introduce bills, amend them, lobby caucuses and call a floor vote."),TEXT("congress"));Nav(TEXT("Meet your advisers"),TEXT("Listen, make promises, compromise and track relationships."),TEXT("advisers"));
  Text(TEXT("PRESS ROOM"),23,true);
  bool Active=false;for(const auto& E:S.Get("executive").Get("encounters").Items())if(E.Get("quarter").AsNumber()==Q&&!E.Get("done").Truthy()){Active=true;const auto Opt=Sim.EncounterOptions(S,E);Text(Get(Opt,"speaker"),16,true);Text(Get(Opt,"question"),20);for(const auto& C:Opt.Get("choices").Items()){const auto Id=Get(C,"id");Button(Get(C,"title"),Get(C,"detail"),[this,Id]{Executive(TEXT("answer"),Id);},Open);}}
  if(!Active){Button(TEXT("Hold a press conference"),TEXT("One media appearance per quarter. Uses an appointment; answer the opening question and follow-up."),[this]{Executive(TEXT("media"),TEXT("press"));},Open);if(Q==7||Q==15)Button(TEXT("Face your opponent in a debate"),TEXT("Election quarter only. Uses this quarter's media appearance."),[this]{Executive(TEXT("media"),TEXT("debate"));},Open);}
  Text(TEXT("ACTIVE DOMESTIC CRISES"),23,true);bool Crises=false;
  for(const auto& C:S.Get("executive").Get("crises").Items())if(Get(C,"stage")!=TEXT("resolved")){Crises=true;const auto Opt=Sim.CrisisOptions(C);const FString Id=Get(C,"id");Text(Get(Opt,"title"),20,true);Text(Get(Opt,"body"),16);for(const auto& O:Opt.Get("choices").Items()){FString Choice=Get(O,"id");Button(Get(O,"title"),Get(O,"detail"),[this,Id,Choice]{Executive(TEXT("crisis"),Id,Choice);},Open&&!C.Get("response").Truthy());}}
  if(!Crises)Text(TEXT("No active domestic crisis. Use this time to prepare."),15);
  Text(TEXT("CABINET MANAGEMENT"),23,true);
  for(const auto& A:G->GetAdvisers()){Text(A.Name+TEXT(" / ")+A.Role,18,true);const FString Id=A.Id;Button(A.bResigned?TEXT("Appoint an acting replacement"):TEXT("Support this department"),A.bResigned?TEXT("Restore leadership after a resignation."):TEXT("Spend political capital to improve morale and delivery."),[this,Id,A]{Executive(TEXT("team"),Id,A.bResigned?TEXT("replace"):TEXT("support"));},Open);}
 }else{
  Text(End?Get(Vw,"endingTitle"):TEXT("The presidential record"),32,true);
  if(End){Text(FString::Printf(TEXT("LEGACY SCORE  %.0f / 100"),Vw.Get("score").AsNumber()),22,true);Text(Get(Vw,"epilogue"),19);Text(TEXT("The score weighs public approval (40%), three platform goals (30%), delivery of up to three laws (21%) and institutional trust (9%). Election victory is reported separately."),13);}
  for(const auto& Goal:Vw.Get("legacy").Get("goals").Items())Text(FString::Printf(TEXT("%s  %s: %.0f / target %.0f"),Goal.Get("met").Truthy()?TEXT("ACHIEVED"):TEXT("UNFINISHED"),*Get(Goal,"name"),Goal.Get("value").AsNumber(),Goal.Get("target").AsNumber()),17,true);
  for(const auto& D:S.Get("presidency").Get("decisions").Items()){Text(FString::Printf(TEXT("QUARTER %d / %s"),int(D.Get("quarter").AsNumber())+1,*Get(D,"title")),18,true);Text(Get(D,"choice"),16);Text(Get(D,"detail"),13);}
  Text(TEXT("RECENT ACTIONS"),21,true);const auto& H=S.Get("history").Items();for(int I=int(H.size())-1;I>=FMath::Max(0,int(H.size())-24);--I){Text(Get(H[I],"title"),17,true);Text(Get(H[I],"detail"),13);}
  Button(ConfirmRestart?TEXT("Confirm / start a new presidency"):TEXT("Start a new presidency"),ConfirmRestart?TEXT("Your current term will be archived as PreviousTerm.json before a fresh inauguration."):TEXT("Requires a second click. Choose a new party, mandate and path through the term."),[this]{if(!ConfirmRestart){ConfirmRestart=true;Dirty=true;return;}Navigate.ExecuteIfBound(TEXT("newterm"));});
 }
}
