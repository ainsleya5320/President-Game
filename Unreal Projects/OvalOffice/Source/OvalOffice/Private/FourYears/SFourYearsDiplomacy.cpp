#include "SFourYearsDiplomacy.h"
#include "SWorldAtlas.h"
#include "Widgets/Input/SSearchBox.h"
#include "Framework/Application/SlateApplication.h"
#include "FourYears/Core/FourYearsDiplomacy.h"
#include "Brushes/SlateColorBrush.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
namespace {
using V=FourYears::JsonValue;
namespace D=FourYears::Diplomacy;
const FLinearColor Navy(.012f,.025f,.045f),Card(.032f,.062f,.09f),Ink(.9f,.94f,.96f),Muted(.48f,.63f,.70f),Gold(.92f,.71f,.35f),Teal(.24f,.82f,.72f),Red(.94f,.37f,.33f);
FString F(const std::string& S){return FString(UTF8_TO_TCHAR(S.c_str()));}
double Num(const V& S,const char* K){return S.Get(K).NumberOr(0);}
FString Number(double N){return F(FourYears::FormatNumber(FourYears::JsRound(N)));}
TSharedRef<STextBlock> Text(const FString& S,int Size=16,FLinearColor Color=Ink,bool Bold=false,bool Wrap=true){
 return SNew(STextBlock).Text(FText::FromString(S)).Font(FCoreStyle::GetDefaultFontStyle(Bold?"Bold":"Regular",Size)).ColorAndOpacity(Color).AutoWrapText(Wrap);
}
const FButtonStyle& ButtonStyle(){
 static FButtonStyle Style=FButtonStyle().SetNormal(FSlateColorBrush(FLinearColor(.06f,.12f,.16f))).SetHovered(FSlateColorBrush(FLinearColor(.12f,.25f,.29f))).SetPressed(FSlateColorBrush(FLinearColor(.2f,.27f,.28f))).SetDisabled(FSlateColorBrush(FLinearColor(.035f,.05f,.07f)));return Style;
}
TSharedRef<SButton> Button(const FString& S,FOnClicked Click,bool Enabled=true){
 return SNew(SButton).ButtonStyle(&ButtonStyle()).IsFocusable(false).IsEnabled(Enabled).ContentPadding(FMargin(14,10)).OnClicked(Click)[Text(S,15,Enabled?Ink:Muted,true,false)];
}
void Line(TSharedRef<SVerticalBox> Box,const FString& S,int Size=16,FLinearColor Color=Ink,bool Bold=false){
 Box->AddSlot().AutoHeight().Padding(0,0,0,8)[Text(S,Size,Color,Bold)];
}
TSharedRef<SWidget> Panel(TSharedRef<SWidget> W){return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Card).Padding(18)[W];}

}
void SFourYearsDiplomacy::Construct(const FArguments& Args){
 Game=Args._Subsystem;OnClose=Args._OnClose;
 Atlas=SNew(SWorldAtlas).OnSelect(TDelegate<void(FString)>::CreateLambda([this](FString Id){SelectRegion(Id);}));
 ChildSlot[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.005f,.012f,.023f,.98f)).Padding(0)
 [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
 [SNew(SBox).WidthOverride(1280).HeightOverride(800)
 [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Navy).Padding(20)
 [SAssignNew(Body,SVerticalBox)]]]]];
}
void SFourYearsDiplomacy::Tick(const FGeometry& G,double T,float Dt){
 SCompoundWidget::Tick(G,T,Dt);
 if(Closing){Closing=false;auto KeepAlive=SharedThis(this);OnClose.ExecuteIfBound();return;}
 if(Dirty){Dirty=false;Rebuild();SlatePrepass(GetPrepassLayoutScaleMultiplier());}
}
FReply SFourYearsDiplomacy::OnKeyDown(const FGeometry&,const FKeyEvent& E){
 if(AtlasSearch.IsValid()&&(AtlasSearch->HasKeyboardFocus()||AtlasSearch->HasFocusedDescendants()))return FReply::Unhandled();
 if(!E.IsRepeat()&&(E.GetKey()==EKeys::Escape||E.GetKey()==EKeys::M||E.GetKey()==EKeys::E))return Close();
 return FReply::Handled();
}
FReply SFourYearsDiplomacy::Close(){Closing=true;return FReply::Handled();}
FReply SFourYearsDiplomacy::SelectRegion(FString Id){FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));SelectedCountry=Id;const auto* C=Atlas->Country(Id);Selected=C?C->Region:TEXT("");Atlas->Select(Id);Dirty=true;return FReply::Handled();}
FReply SFourYearsDiplomacy::SelectPage(int32 Index){FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));Page=Index;Dirty=true;return FReply::Handled();}
void SFourYearsDiplomacy::MessageResult(const FFourYearsActionResult& R){Message=R.Message;Success=R.bOk;Dirty=true;}
FReply SFourYearsDiplomacy::Action(FString Id){if(Game.IsValid())MessageResult(Game->DiplomacyAction(Selected,Id));return FReply::Handled();}
FReply SFourYearsDiplomacy::CrisisAction(int32 Index,FString Id){if(Game.IsValid())MessageResult(Game->RespondDiplomacyCrisis(Index,Id));return FReply::Handled();}
FReply SFourYearsDiplomacy::Doctrine(FString Id){if(Game.IsValid())MessageResult(Game->ChooseDiplomacyDoctrine(Id));return FReply::Handled();}
void SFourYearsDiplomacy::Rebuild(){
 Body->ClearChildren();if(!Game.IsValid())return;
 const V& State=Game->GetState();const V World=D::View(State);const bool Ended=State.Get("ended").Truthy();
 const FString Goal=F(World.Get("goal").AsString());int Open=0;for(const auto& C:World.Get("crises").Items())if(C.Get("status").AsString()=="open")++Open;
 auto Header=SNew(SHorizontalBox);
 auto Title=SNew(SVerticalBox);Line(Title,TEXT("THE PRESIDENT'S WORLD ATLAS"),25,Ink,true);
 Header->AddSlot().FillWidth(1)[Title];Header->AddSlot().AutoWidth().VAlign(VAlign_Center)[Button(TEXT("Return to office  [M]"),FOnClicked::CreateSP(this,&SFourYearsDiplomacy::Close))];
 Body->AddSlot().AutoHeight()[Header];
 auto Stats=SNew(SHorizontalBox);
 const TArray<TPair<FString,FString>> Values={
 {TEXT("QUARTER"),Number(FMath::Min(16.0,Num(State,"quarter")+1))+TEXT(" / 16")},
 {TEXT("INFLUENCE  /  RIVAL"),Number(Num(World,"influence"))+TEXT(" / ")+Number(Num(World,"rival"))},
 {TEXT("PRESTIGE"),Number(Num(World,"prestige"))},
 {TEXT("WORLD TENSION"),Number(Num(World,"tension"))+TEXT(" / 100")},
 {TEXT("DIPLOMATIC ACTIONS"),Number(Num(World,"actions"))+TEXT(" / 3")},
 {TEXT("POLITICAL CAPITAL"),Number(Num(State,"capital"))}
 };
 for(const auto& Pair:Values){auto Cell=SNew(SVerticalBox);Line(Cell,Pair.Key,11,Muted,true);Line(Cell,Pair.Value,18,Pair.Key==TEXT("WORLD TENSION")&&Num(World,"tension")>=60?Red:Teal,true);Stats->AddSlot().FillWidth(1).Padding(0,0,8,0)[Cell];}
 Body->AddSlot().AutoHeight().Padding(0,4,0,8)[Stats];
 auto Nav=SNew(SHorizontalBox);
 const TCHAR* Pages[]={TEXT("WORLD MAP"),TEXT("CRISIS DESK"),TEXT("STRATEGY & RECORD")};
 for(int I=0;I<3;++I)Nav->AddSlot().AutoWidth().Padding(0,0,10,0)[Button(FString(Pages[I])+(I==1?FString::Printf(TEXT("  (%d)"),Open):TEXT("")),FOnClicked::CreateSP(this,&SFourYearsDiplomacy::SelectPage,I))];
 Nav->AddSlot().FillWidth(1).VAlign(VAlign_Center).HAlign(HAlign_Right)[Text(Ended?F(World.Get("outcome").AsString()):FString::Printf(TEXT("VICTORY PROGRESS  %d / 100"),int(Num(World,"score"))),16,Gold,true,false)];
 Body->AddSlot().AutoHeight().Padding(0,0,0,10)[Nav];

 if(!Message.IsEmpty())Body->AddSlot().AutoHeight().Padding(0,0,0,8)[Text(Message,14,Success?Teal:Red)];
 if(!State.Get("agendaChoice").IsNull()&&!Ended)Body->AddSlot().AutoHeight().Padding(0,0,0,8)[Text(TEXT("Quarter closed: advance from your domestic briefing to resume diplomacy."),14,Gold)];
 if(Page==0){
  auto Main=SNew(SHorizontalBox);auto Left=SNew(SVerticalBox);
  Atlas->SetWorld(World);
  auto Small=[&](const FString& Label,TFunction<void()> Fn){return SNew(SButton).ButtonStyle(&ButtonStyle()).IsFocusable(false).ContentPadding(FMargin(9,6)).OnClicked_Lambda([this,Fn](){FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));Fn();return FReply::Handled();})[Text(Label,12,Ink,true,false)];};
  auto Tools=SNew(SHorizontalBox);
  const TCHAR* Modes[]={TEXT("POLITICAL"),TEXT("TERRAIN"),TEXT("INFLUENCE")};
  for(int I=0;I<3;++I)Tools->AddSlot().AutoWidth().Padding(0,0,4,0)[Small(FString(Atlas->Mode==I?TEXT("> "):TEXT(""))+Modes[I],[this,I](){Atlas->Mode=I;Dirty=true;})];
  Tools->AddSlot().FillWidth(1).Padding(6,0)[SAssignNew(AtlasSearch,SSearchBox).HintText(FText::FromString(TEXT("Find country or city; Enter"))).OnTextCommitted_Lambda([this](const FText& T,ETextCommit::Type How){if(How==ETextCommit::OnEnter){if(Atlas->Search(T.ToString())){Message=TEXT("");Success=true;}else{Message=TEXT("No matching country or city. Try its full name.");Success=false;}Dirty=true;FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));}})];
  Tools->AddSlot().AutoWidth()[Small(FullAtlas?TEXT("SHOW DETAILS"):TEXT("EXPAND MAP"),[this](){FullAtlas=!FullAtlas;Dirty=true;})];
  Left->AddSlot().AutoHeight().Padding(0,0,0,6)[Tools];
  auto Navigation=SNew(SHorizontalBox);
  Navigation->AddSlot().AutoWidth().Padding(0,0,4,0)[Small(TEXT("+"),[this](){Atlas->ZoomBy(1.4);})];
  Navigation->AddSlot().AutoWidth().Padding(0,0,4,0)[Small(TEXT("-"),[this](){Atlas->ZoomBy(1/1.4);})];
  Navigation->AddSlot().AutoWidth().Padding(0,0,8,0)[Small(TEXT("WHOLE WORLD"),[this](){Atlas->Reset();})];
  Navigation->AddSlot().AutoWidth().Padding(0,0,4,0)[Small(Atlas->ShowCities?TEXT("CITIES ON"):TEXT("CITIES OFF"),[this](){Atlas->ShowCities=!Atlas->ShowCities;Dirty=true;})];
  Navigation->AddSlot().AutoWidth().Padding(0,0,4,0)[Small(Atlas->ShowGrid?TEXT("GRID ON"):TEXT("GRID OFF"),[this](){Atlas->ShowGrid=!Atlas->ShowGrid;Dirty=true;})];
  Navigation->AddSlot().AutoWidth()[Small(Atlas->ShowRoutes?TEXT("TRADE LINKS ON"):TEXT("TRADE LINKS OFF"),[this](){Atlas->ShowRoutes=!Atlas->ShowRoutes;Dirty=true;})];
  Navigation->AddSlot().FillWidth(1).VAlign(VAlign_Center).HAlign(HAlign_Right)[SNew(STextBlock).Text_Lambda([this](){return FText::FromString(FString::Printf(TEXT("ZOOM %.0f%%"),Atlas->GetZoom()*100));}).Font(FCoreStyle::GetDefaultFontStyle("Regular",11)).ColorAndOpacity(Muted)];
  Left->AddSlot().AutoHeight().Padding(0,0,0,6)[Navigation];
  Left->AddSlot().FillHeight(1)[Atlas.ToSharedRef()];
  Left->AddSlot().AutoHeight().Padding(0,6,0,2)[Text(TEXT("SCROLL TO ZOOM  /  DRAG TO PAN  /  CLICK TERRITORY  /  DOUBLE-CLICK TO FOCUS"),11,Muted,true,false)];
  Left->AddSlot().AutoHeight()[Text(Atlas->Mode==2?TEXT("TEAL: U.S. influence leads  /  CORAL: rival leads  /  Gold: selected territory"):TEXT("POINT SYMBOLS: named cities only  /  Gold ring + center: capital  /  Gold outline: selection"),11,Muted,false,false)];
  Main->AddSlot().FillWidth(1).Padding(0,0,FullAtlas?0:14,0)[Left];
  auto Right=SNew(SVerticalBox);const D::Region* Definition=nullptr;for(const auto& R:D::Regions())if(F(R.Id)==Selected)Definition=&R;
  const auto* Country=Atlas->Country(SelectedCountry);
  if(Country){
   Line(Right,Country->Name,23,Ink,true);
   Line(Right,Country->Continent.ToUpper(),11,Gold,true);
   if(!Country->Capital.IsEmpty())Line(Right,TEXT("Capital: ")+Country->Capital,14,Muted);
   Right->AddSlot().AutoHeight().Padding(0,0,0,12)[Small(TEXT("FOCUS ON TERRITORY"),[this](){Atlas->FocusCountry(SelectedCountry);})];
  }
  if(!Definition){
   Line(Right,SelectedCountry==TEXT("USA")?TEXT("YOUR ADMINISTRATION"):TEXT("GEOGRAPHIC REFERENCE"),14,Gold,true);
   Line(Right,SelectedCountry==TEXT("USA")?TEXT("Your diplomacy begins in Washington, D.C. Select a foreign partner to negotiate."):TEXT("This territory is part of the atlas. Bilateral actions are not yet available for this country."),15,Muted);
   Line(Right,TEXT("Each national border remains visible. European Union and Gulf negotiations cover several countries without merging their geography."),14,Muted);
  }
  if(Definition){
   const V& R=World.Get("regions").Get(Definition->Id);
   Line(Right,TEXT("DIPLOMATIC PARTNER: ")+F(Definition->Name),13,Gold,true);Line(Right,F(Definition->Interest),14,Muted);
   Line(Right,TEXT("Relations ")+Number(Num(R,"relation"))+TEXT("   Stability ")+Number(Num(R,"stability")),15,Teal,true);
   Line(Right,TEXT("Your influence ")+Number(Num(R,"influence"))+TEXT("   Rival ")+Number(Num(R,"rival")),15,Gold);
   Line(Right,R.Get("trade").Truthy()?(Num(R,"stability")<35?TEXT("TRADE PACT SUSPENDED: stability below 35"):TEXT("ACTIVE TRADE PACT")):TEXT("No trade agreement"),12,Muted,true);
   const V Choices=D::Options(State,Definition->Id);
   for(const auto& O:Choices.Items()){
    if(O.Get("id").AsString()=="lift"&&!R.Get("sanction").Truthy())continue;
    auto Box=SNew(SVerticalBox);Line(Box,F(O.Get("detail").AsString()),13,Muted);
    const bool Available=O.Get("available").Truthy();if(!Available)Line(Box,F(O.Get("reason").AsString()),12,Gold);
    Right->AddSlot().AutoHeight().Padding(0,4,0,4)[Button(F(O.Get("label").AsString())+TEXT("\n")+Number(Num(O,"cost"))+TEXT(" capital")+ (Num(O,"debt")>0?TEXT(" + ")+Number(Num(O,"debt"))+TEXT(" debt"):TEXT("")),FOnClicked::CreateSP(this,&SFourYearsDiplomacy::Action,F(O.Get("id").AsString())),Available)];
    Right->AddSlot().AutoHeight()[Box];
   }
  }
  if(!FullAtlas)Main->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(300)[SNew(SScrollBox)+SScrollBox::Slot()[Panel(Right)]]];
  Body->AddSlot().FillHeight(1)[Main];
 }else{
  auto List=SNew(SVerticalBox);
  if(Page==1){
   Line(List,TEXT("Resolve crises before their deadlines. Mediation takes two rounds in different quarters; funded settlements and strong coalitions act immediately."),17,Muted);
   for(int I=0;I<int(World.Get("crises").Size());++I){
    const V& C=World.Get("crises")[I];if(C.Get("status").AsString()!="open")continue;
    auto Box=SNew(SVerticalBox);Line(Box,F(C.Get("title").AsString()),23,Ink,true);
    Line(Box,TEXT("DEADLINE: END OF QUARTER ")+Number(Num(C,"deadline"))+TEXT("  /  TALKS ")+Number(Num(C,"progress"))+TEXT(" OF 2"),13,Gold,true);
    Line(Box,F(C.Get("body").AsString()),17,Muted);
    const V Choices=D::CrisisOptions(State,I);for(const V& O:Choices.Items()){
     Box->AddSlot().AutoHeight().Padding(0,4)[Button(F(O.Get("label").AsString())+TEXT("  /  ")+Number(Num(O,"cost"))+TEXT(" capital")+(Num(O,"debt")>0?TEXT(" + ")+Number(Num(O,"debt"))+TEXT(" debt"):TEXT("")),FOnClicked::CreateSP(this,&SFourYearsDiplomacy::CrisisAction,I,F(O.Get("id").AsString())),O.Get("available").Truthy())];
     Line(Box,F(O.Get(O.Get("available").Truthy()?"detail":"reason").AsString()),14,Muted);
    }
    List->AddSlot().AutoHeight().Padding(0,0,0,16)[Panel(Box)];
   }
   if(Open==0)Line(List,TEXT("No open crises. Use this breathing room to strengthen your partnerships."),22,Teal,true);
  }else{
   Line(List,F(World.Get("outcome").AsString()),24,Gold,true);
   Line(List,TEXT("Victory requires all three objectives at the end of your presidency. Domestic elections still matter: a foreign-policy victory does not guarantee re-election."),17,Muted);
   const TCHAR* Ids[]={TEXT("commerce"),TEXT("coalition"),TEXT("peace")};
   const TCHAR* Labels[]={TEXT("COMMERCIAL POWER  /  4 trade pacts, 55 influence, tension <= 45"),TEXT("COALITION BUILDER  /  4 security partners, 60 influence, tension <= 55"),TEXT("PEACEMAKER  /  4 resolved crises, 70 prestige, tension <= 30")};
   for(int I=0;I<3;++I)List->AddSlot().AutoHeight().Padding(0,4)[Button(FString(World.Get("doctrine").AsString()==std::string(TCHAR_TO_UTF8(Ids[I]))?TEXT("SELECTED: "):TEXT(""))+Labels[I],FOnClicked::CreateSP(this,&SFourYearsDiplomacy::Doctrine,FString(Ids[I])),!World.Get("committed").Truthy()&&!Ended&&State.Get("agendaChoice").IsNull())];
   Line(List,World.Get("committed").Truthy()?TEXT("Your strategy is committed for this presidency."):TEXT("Choose freely until your first diplomatic action, then your victory goal is locked."),14,Gold);
   Line(List,TEXT("HOW THE WORLD MOVES"),17,Ink,true);
   Line(List,TEXT("Three actions each quarter; one bilateral action per partner. Actions spend the same political capital as domestic policy. Rival influence grows each quarter, with one focused offensive. Security partners resist pressure. Sanctions raise tension and end trade pacts. Trade pauses below 35 stability; partners leave below 50 relations."),16,Muted);
   Line(List,TEXT("Active trade dividend next quarter: ")+F(FourYears::FormatNumber(Num(World,"dividend")))+TEXT(" debt reduction. High tension (70+) hurts prices and growth. Advance time from the domestic briefing."),16,Teal);
   Line(List,TEXT("DIPLOMATIC RECORD"),17,Ink,true);
   for(const auto& N:World.Get("news").Items())Line(List,TEXT("Q")+Number(Num(N,"quarter")+1)+TEXT("  ")+F(N.Get("text").AsString()),15,Muted);
  }
  Body->AddSlot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[List]];
 }
 Body->AddSlot().AutoHeight().Padding(0,12,0,0)[Text(TEXT("NATURAL EARTH GEOGRAPHY  /  Robinson projection; de facto borders, v5.1.1  /  Fictional diplomacy  /  M or Esc: office"),11,Muted,false,false)];
}
