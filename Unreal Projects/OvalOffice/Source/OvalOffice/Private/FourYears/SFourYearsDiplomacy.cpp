#include "SFourYearsDiplomacy.h"
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
FVector2D Project(double Lon,double Lat,FVector2D Size){return FVector2D((Lon+180)/360*Size.X,(83-Lat)/155*Size.Y);}
class SStrategicMap:public SLeafWidget {
public:
 SLATE_BEGIN_ARGS(SStrategicMap){} SLATE_ARGUMENT(V,World) SLATE_ARGUMENT(FString,Selected) SLATE_EVENT(TDelegate<void(FString)>,OnSelect) SLATE_END_ARGS()
 void Construct(const FArguments& A){World=A._World;Selected=A._Selected;OnSelect=A._OnSelect;SetClipping(EWidgetClipping::ClipToBounds);SetCursor(EMouseCursor::Hand);}
 FVector2D ComputeDesiredSize(float) const override{return FVector2D(780,390);}
 FReply OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override{
  if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();
  FVector2D P=G.AbsoluteToLocal(E.GetScreenSpacePosition());double Best=28;const D::Region* Hit=nullptr;
  for(const auto& R:D::Regions()){double Dist=FVector2D::Distance(P,Project(R.Lon,R.Lat,G.GetLocalSize()));if(Dist<Best){Best=Dist;Hit=&R;}}
  if(Hit){OnSelect.ExecuteIfBound(F(Hit->Id));return FReply::Handled();}return FReply::Unhandled();
 }
 int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override{
  auto Size=G.GetLocalSize();const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
  FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(),Brush,ESlateDrawEffect::None,Navy);
  auto Lines=[&](const TArray<FVector2D>& P,FLinearColor Color,float Width=1.f){FSlateDrawElement::MakeLines(Out,Layer+1,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Color,true,Width);};
  for(int Lon=-150;Lon<=150;Lon+=30)Lines({Project(Lon,80,Size),Project(Lon,-65,Size)},FLinearColor(.055f,.11f,.15f));
  for(int Lat=-60;Lat<=60;Lat+=30)Lines({Project(-180,Lat,Size),Project(180,Lat,Size)},FLinearColor(.055f,.11f,.15f));
  // Original schematic coastlines: gameplay anchors represent partners, not territorial claims.
  static const TArray<TArray<FVector2D>> Land={
   {{-168,66},{-145,70},{-125,72},{-110,70},{-92,74},{-70,62},{-58,52},{-65,44},{-78,32},{-81,24},{-88,30},{-98,23},{-88,16},{-79,8},{-85,9},{-103,20},{-117,32},{-125,48},{-137,58},{-160,60},{-168,66}},
   {{-73,82},{-25,82},{-20,67},{-43,59},{-60,69},{-73,82}},
   {{-81,10},{-68,11},{-51,3},{-35,-7},{-40,-20},{-53,-35},{-66,-55},{-73,-51},{-76,-30},{-81,-5},{-81,10}},
   {{-10,36},{-10,44},{-1,49},{8,54},{5,60},{20,72},{33,68},{33,58},{50,55},{62,68},{95,78},{140,71},{178,65},{175,52},{153,58},{143,47},{131,42},{122,30},{121,20},{108,20},{110,5},{103,1},{98,16},{90,22},{79,7},{70,24},{56,25},{50,13},{43,12},{35,30},{22,37},{14,41},{4,43},{-10,36}},
   {{-17,34},{10,37},{32,31},{43,12},{51,12},{42,-4},{35,-20},{19,-35},{12,-30},{8,-12},{-2,5},{-16,14},{-17,34}},
   {{112,-12},{132,-11},{141,-17},{146,-11},{154,-26},{148,-38},{132,-35},{114,-34},{112,-12}},
   {{130,32},{139,36},{145,44},{142,46},{136,37},{130,32}},
   {{-8,50},{-6,58},{-3,59},{1,52},{-8,50}},
   {{96,5},{106,-5},{117,-8},{129,-3},{138,-5},{149,-9},{138,-11},{125,-8},{113,-8},{104,-6},{96,5}}
  };
  for(const auto& Shape:Land){TArray<FVector2D> P;for(auto Point:Shape)P.Add(Project(Point.X,Point.Y,Size));Lines(P,FLinearColor(.22f,.39f,.44f),1.5f);}
  const FVector2D Home=Project(-98,38,Size);
  auto Caption=[&](FVector2D P,const FString& S,FLinearColor Color,int Font=12){
   FSlateDrawElement::MakeText(Out,Layer+3,G.ToPaintGeometry(FVector2D(160,26),FSlateLayoutTransform(P)),S,FCoreStyle::GetDefaultFontStyle("Bold",Font),ESlateDrawEffect::None,Color);
  };
  Caption(Home+FVector2D(-22,12),TEXT("USA"),Gold,15);
  FSlateDrawElement::MakeBox(Out,Layer+3,G.ToPaintGeometry(FVector2D(10,10),FSlateLayoutTransform(Home-FVector2D(5,5))),Brush,ESlateDrawEffect::None,Gold);
  for(const auto& R:D::Regions()){
   const V& Data=World.Get("regions").Get(R.Id);FVector2D P=Project(R.Lon,R.Lat,Size);
   FLinearColor Color=Num(Data,"influence")>=Num(Data,"rival")?Teal:Red;
   if(Data.Get("trade").Truthy()){TArray<FVector2D> Route;for(int I=0;I<=24;++I){float T=I/24.f;FVector2D Point=Home*(1-T)+P*T;Point.Y-=FMath::Sin(T*PI)*38;Route.Add(Point);}Lines(Route,FLinearColor(.12f,.45f,.39f),1.4f);}
   TArray<FVector2D> Circle;bool Active=Selected==F(R.Id);for(int I=0;I<=24;++I){float A=I*2*PI/24;Circle.Add(P+FVector2D(FMath::Cos(A),FMath::Sin(A))*(Active?12:7));}Lines(Circle,Active?Gold:Color,Active?2.8f:2.f);
   FSlateDrawElement::MakeBox(Out,Layer+2,G.ToPaintGeometry(FVector2D(4,4),FSlateLayoutTransform(P-FVector2D(2,2))),Brush,ESlateDrawEffect::None,Color);
   FVector2D Offset(10,-8);if(F(R.Id)==TEXT("uk"))Offset=FVector2D(-42,-24);if(F(R.Id)==TEXT("europe"))Offset=FVector2D(10,8);if(F(R.Id)==TEXT("japan"))Offset=FVector2D(-4,-28);if(F(R.Id)==TEXT("australia"))Offset=FVector2D(-35,16);
   FString Name=F(R.Name);if(F(R.Id)==TEXT("europe"))Name=TEXT("EU");if(F(R.Id)==TEXT("uk"))Name=TEXT("UK");if(F(R.Id)==TEXT("gulf"))Name=TEXT("Gulf");
   Caption(P+Offset,Name,Active?Gold:Ink);
  }
  return Layer+3;
 }
private: V World;FString Selected;TDelegate<void(FString)> OnSelect;
};
}
void SFourYearsDiplomacy::Construct(const FArguments& Args){
 Game=Args._Subsystem;OnClose=Args._OnClose;
 ChildSlot[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.005f,.012f,.023f,.98f)).Padding(0)
 [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
 [SNew(SBox).WidthOverride(1280).HeightOverride(800)
 [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Navy).Padding(30)
 [SAssignNew(Body,SVerticalBox)]]]]];
}
void SFourYearsDiplomacy::Tick(const FGeometry& G,double T,float Dt){
 SCompoundWidget::Tick(G,T,Dt);
 if(Closing){Closing=false;auto KeepAlive=SharedThis(this);OnClose.ExecuteIfBound();return;}
 if(Dirty){Dirty=false;Rebuild();SlatePrepass(GetPrepassLayoutScaleMultiplier());}
}
FReply SFourYearsDiplomacy::OnKeyDown(const FGeometry&,const FKeyEvent& E){
 if(!E.IsRepeat()&&(E.GetKey()==EKeys::Escape||E.GetKey()==EKeys::M||E.GetKey()==EKeys::E))return Close();
 return FReply::Handled();
}
FReply SFourYearsDiplomacy::Close(){Closing=true;return FReply::Handled();}
FReply SFourYearsDiplomacy::SelectRegion(FString Id){Selected=Id;Dirty=true;return FReply::Handled();}
FReply SFourYearsDiplomacy::SelectPage(int32 Index){Page=Index;Dirty=true;return FReply::Handled();}
void SFourYearsDiplomacy::MessageResult(const FFourYearsActionResult& R){Message=R.Message;Success=R.bOk;Dirty=true;}
FReply SFourYearsDiplomacy::Action(FString Id){if(Game.IsValid())MessageResult(Game->DiplomacyAction(Selected,Id));return FReply::Handled();}
FReply SFourYearsDiplomacy::CrisisAction(int32 Index,FString Id){if(Game.IsValid())MessageResult(Game->RespondDiplomacyCrisis(Index,Id));return FReply::Handled();}
FReply SFourYearsDiplomacy::Doctrine(FString Id){if(Game.IsValid())MessageResult(Game->ChooseDiplomacyDoctrine(Id));return FReply::Handled();}
void SFourYearsDiplomacy::Rebuild(){
 Body->ClearChildren();if(!Game.IsValid())return;
 const V& State=Game->GetState();const V World=D::View(State);const bool Ended=State.Get("ended").Truthy();
 const FString Goal=F(World.Get("goal").AsString());int Open=0;for(const auto& C:World.Get("crises").Items())if(C.Get("status").AsString()=="open")++Open;
 auto Header=SNew(SHorizontalBox);
 auto Title=SNew(SVerticalBox);Line(Title,TEXT("THE PRESIDENT'S WORLD ATLAS"),13,Gold,true);Line(Title,TEXT("A world of consequences"),30,Ink,true);
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
 for(const auto& Pair:Values){auto Cell=SNew(SVerticalBox);Line(Cell,Pair.Key,11,Muted,true);Line(Cell,Pair.Value,22,Pair.Key==TEXT("WORLD TENSION")&&Num(World,"tension")>=60?Red:Teal,true);Stats->AddSlot().FillWidth(1).Padding(0,0,8,0)[Panel(Cell)];}
 Body->AddSlot().AutoHeight().Padding(0,8,0,12)[Stats];
 auto Nav=SNew(SHorizontalBox);
 const TCHAR* Pages[]={TEXT("WORLD MAP"),TEXT("CRISIS DESK"),TEXT("STRATEGY & RECORD")};
 for(int I=0;I<3;++I)Nav->AddSlot().AutoWidth().Padding(0,0,10,0)[Button(FString(Pages[I])+(I==1?FString::Printf(TEXT("  (%d)"),Open):TEXT("")),FOnClicked::CreateSP(this,&SFourYearsDiplomacy::SelectPage,I))];
 Nav->AddSlot().FillWidth(1).VAlign(VAlign_Center).HAlign(HAlign_Right)[Text(Ended?F(World.Get("outcome").AsString()):FString::Printf(TEXT("VICTORY PROGRESS  %d / 100"),int(Num(World,"score"))),16,Gold,true,false)];
 Body->AddSlot().AutoHeight().Padding(0,0,0,10)[Nav];
 Body->AddSlot().AutoHeight().Padding(0,0,0,10)[SNew(SProgressBar).Percent(float(Num(World,"score")/100)).FillColorAndOpacity(Gold)];
 if(!Message.IsEmpty())Body->AddSlot().AutoHeight().Padding(0,0,0,8)[Text(Message,14,Success?Teal:Red)];
 if(!State.Get("agendaChoice").IsNull()&&!Ended)Body->AddSlot().AutoHeight().Padding(0,0,0,8)[Text(TEXT("Quarter closed: advance from your domestic briefing to resume diplomacy."),14,Gold)];
 if(Page==0){
  auto Main=SNew(SHorizontalBox);auto Left=SNew(SVerticalBox);
  Left->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(320)[SNew(SStrategicMap).World(World).Selected(Selected).OnSelect(TDelegate<void(FString)>::CreateLambda([this](FString Id){SelectRegion(Id);}))]];
  Line(Left,TEXT("Click a partner  |  TEAL: U.S. influence leads  |  CORAL: rival leads  |  LINES: trade pacts"),12,Muted);
  Line(Left,TEXT("Your mission: ")+Goal,15,Gold);
  Line(Left,TEXT("Trade pacts ")+Number(Num(World,"tradeCount"))+TEXT("  /  Security partners ")+Number(Num(World,"allies"))+TEXT("  /  Crises resolved ")+Number(Num(World,"resolved")),16,Teal,true);
  Main->AddSlot().FillWidth(.66f).Padding(0,0,18,0)[Left];
  auto Right=SNew(SVerticalBox);const D::Region* Definition=nullptr;for(const auto& R:D::Regions())if(F(R.Id)==Selected)Definition=&R;
  if(Definition){
   const V& R=World.Get("regions").Get(Definition->Id);
   Line(Right,F(Definition->Name),24,Ink,true);Line(Right,F(Definition->Interest),14,Muted);
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
  Main->AddSlot().FillWidth(.34f)[SNew(SScrollBox)+SScrollBox::Slot()[Panel(Right)]];
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
 Body->AddSlot().AutoHeight().Padding(0,12,0,0)[Text(TEXT("FICTIONAL STRATEGIC SCENARIO  /  Schematically placed partners; no territorial conquest.  /  M or Esc returns to the office."),11,Muted,false,false)];
}
