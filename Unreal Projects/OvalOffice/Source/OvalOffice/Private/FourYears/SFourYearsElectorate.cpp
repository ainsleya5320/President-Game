#include "SFourYearsElectorate.h"
#include "SUSVoterAtlas.h"
#include "Brushes/SlateColorBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
namespace {
using V=FourYears::JsonValue;
FString F(const std::string& S){return UTF8_TO_TCHAR(S.c_str());}
FString T(const V& X,const char* K){return F(X.Get(K).AsString());}
double N(const V& X,const char* K){return X.Get(K).NumberOr(0);}
FLinearColor Hex(const FString& S){return FLinearColor::FromSRGBColor(FColor::FromHex(S));}
const FLinearColor Navy=Hex(TEXT("0d1827")),Card=Hex(TEXT("192a3b")),Ink=Hex(TEXT("f0f2ed")),Muted=Hex(TEXT("9dafbf")),Gold=Hex(TEXT("e7c784")),Teal=Hex(TEXT("8ed6bf")),Blue=Hex(TEXT("67a9e3")),Red=Hex(TEXT("e68c92"));
FString Pct(double X){return FString::Printf(TEXT("%.1f%%"),X);}
FString Compact(double X){return X>=1e6?FString::Printf(TEXT("%.2fm"),X/1e6):X>=1000?FString::Printf(TEXT("%.1fk"),X/1000):FString::Printf(TEXT("%.0f"),X);}
const V* Find(const V& List,const FString& Id){for(const auto& A:List.Items())if(T(A,"id")==Id)return &A;return nullptr;}
TSharedRef<STextBlock> Text(const FString& S,int Size=14,FLinearColor C=Ink,bool Bold=false,bool Wrap=true){return SNew(STextBlock).Text(FText::FromString(S)).Font(FCoreStyle::GetDefaultFontStyle(Bold?"Bold":"Regular",Size)).ColorAndOpacity(C).AutoWrapText(Wrap);}
void Line(TSharedRef<SVerticalBox> B,const FString& S,int Size=14,FLinearColor C=Ink,bool Bold=false){B->AddSlot().AutoHeight().Padding(0,0,0,7)[Text(S,Size,C,Bold)];}
const FButtonStyle& Style(){static FButtonStyle B=FButtonStyle().SetNormal(FSlateColorBrush(Hex(TEXT("243b50")))).SetHovered(FSlateColorBrush(Hex(TEXT("395c70")))).SetPressed(FSlateColorBrush(Hex(TEXT("58777c")))).SetDisabled(FSlateColorBrush(Hex(TEXT("182432"))));return B;}
TSharedRef<SButton> Button(const FString& S,FOnClicked Fn,bool Enabled=true,int Size=12){return SNew(SButton).ButtonStyle(&Style()).IsFocusable(false).IsEnabled(Enabled).ContentPadding(FMargin(10,8)).OnClicked(Fn)[Text(S,Size,Enabled?Ink:Muted,true,false)];}
TSharedRef<SWidget> Panel(TSharedRef<SWidget> Child){return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Card).Padding(16)[Child];}
void Meter(TSharedRef<SVerticalBox> B,const FString& Label,double Value,FLinearColor Color=Teal){auto R=SNew(SHorizontalBox);R->AddSlot().FillWidth(1)[Text(Label,12,Muted)];R->AddSlot().AutoWidth()[Text(Pct(Value),12,Ink,true)];B->AddSlot().AutoHeight().Padding(0,4,0,3)[R];B->AddSlot().AutoHeight().Padding(0,0,0,5)[SNew(SBox).HeightOverride(4)[SNew(SProgressBar).Percent(float(FMath::Clamp(Value/100,0.,1.))).FillColorAndOpacity(Color)]];}
}
void SFourYearsElectorate::Construct(const FArguments& Args){
 Game=Args._Subsystem;OnClose=Args._OnClose;Atlas=SNew(SUSVoterAtlas).OnSelect(TDelegate<void(FString)>::CreateSP(this,&SFourYearsElectorate::Select));
 ChildSlot[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Navy).Padding(0)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1360).HeightOverride(860)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Navy).Padding(24)[SAssignNew(Body,SVerticalBox)]]]]];
}
void SFourYearsElectorate::Tick(const FGeometry& G,double Time,float Dt){SCompoundWidget::Tick(G,Time,Dt);if(Closing){Closing=false;auto KeepAlive=SharedThis(this);OnClose.ExecuteIfBound();return;}if(Dirty){Dirty=false;Rebuild();SlatePrepass(GetPrepassLayoutScaleMultiplier());}}
FReply SFourYearsElectorate::OnKeyDown(const FGeometry&,const FKeyEvent& E){if(Search.IsValid()&&(Search->HasKeyboardFocus()||Search->HasFocusedDescendants()))return FReply::Unhandled();if(!E.IsRepeat()&&(E.GetKey()==EKeys::Escape||E.GetKey()==EKeys::V))return Close();return FReply::Handled();}
FReply SFourYearsElectorate::Close(){Closing=true;return FReply::Handled();}
void SFourYearsElectorate::Select(FString Id){Selected=Id;Atlas->Select(Id);FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));Dirty=true;}
void SFourYearsElectorate::Rebuild(){
 Body->ClearChildren();if(!Game.IsValid())return;const auto& S=Game->GetState();const auto& D=Game->GetSimulation().Data().Get("us-electorate");const auto& View=Game->GetSimulation().VoterAtlas(S);const FString Party=T(View,"party");const bool Republican=Party==TEXT("republican");
 auto Small=[&](const FString& Label,TFunction<void()> Fn,bool Enabled=true){return Button(Label,FOnClicked::CreateLambda([this,Fn](){FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));Message.Empty();Fn();return FReply::Handled();}),Enabled);};
 auto Header=SNew(SHorizontalBox);auto Titles=SNew(SVerticalBox);Line(Titles,TEXT("THE AMERICAN ELECTORATE"),29,Ink,true);Line(Titles,TEXT("3,143 COMMUNITIES  /  50 STATES + D.C.  /  ONE PRESIDENCY"),11,Gold,true);Header->AddSlot().FillWidth(1)[Titles];Header->AddSlot().AutoWidth().VAlign(VAlign_Center)[Button(TEXT("Return to office  [V]"),FOnClicked::CreateSP(this,&SFourYearsElectorate::Close))];Body->AddSlot().AutoHeight()[Header];
 if(Party.IsEmpty()&&!S.Get("ended").Truthy()){
  auto Choice=SNew(SVerticalBox);Line(Choice,TEXT("Choose your administration"),26,Gold,true);Line(Choice,TEXT("Your party establishes the historical electoral coalition you inherit. Policy performance, regional cultures and turnout then change that coalition throughout your presidency."),18);
  Line(Choice,TEXT("This choice lasts for the term. Your current policies, relationships and progress will be kept."),15,Muted);auto Buttons=SNew(SHorizontalBox);
  for(const FString Id:{FString(TEXT("democrat")),FString(TEXT("republican"))})Buttons->AddSlot().FillWidth(1).Padding(0,12,12,0)[Small(Id==TEXT("democrat")?TEXT("DEMOCRATIC PRESIDENT"):TEXT("REPUBLICAN PRESIDENT"),[this,Id](){const auto R=Game->ChooseElectoralParty(Id);Message=R.Message;Dirty=true;})];Choice->AddSlot().AutoHeight()[Buttons];
  Body->AddSlot().FillHeight(1).VAlign(VAlign_Center)[Panel(Choice)];return;
 }
 auto Stats=SNew(SHorizontalBox);const TArray<TPair<FString,FString>> Values={{TEXT("ADMINISTRATION"),Republican?TEXT("Republican"):Party.IsEmpty()?TEXT("Archived term"):TEXT("Democratic")},{TEXT("NATIONAL APPROVAL"),Pct(N(View,"approval"))},{TEXT("PROJECTED TWO-PARTY VOTE"),Pct(N(View,"support"))},{TEXT("ELECTORAL VOTES"),FString::Printf(TEXT("%.0f / 538"),N(View,"points"))},{TEXT("VOTES NEEDED"),TEXT("270")},{TEXT("TERM QUARTER"),FString::Printf(TEXT("%.0f / 16"),FMath::Min(16.,N(S,"quarter")+1))}};
 for(const auto& A:Values){auto Cell=SNew(SVerticalBox);Line(Cell,A.Key,10,Muted,true);Line(Cell,A.Value,21,A.Key==TEXT("ELECTORAL VOTES")?Gold:Teal,true);Stats->AddSlot().FillWidth(1).Padding(0,8,10,8)[Cell];}Body->AddSlot().AutoHeight()[Stats];
 auto Bar=SNew(SHorizontalBox);const double Dem=Republican?N(View,"opponent"):N(View,"points"),Rep=Republican?N(View,"points"):N(View,"opponent");
 for(const auto Pair:{TPair<double,FLinearColor>(Dem,Blue),TPair<double,FLinearColor>(N(View,"tossup"),Gold),TPair<double,FLinearColor>(Rep,Red)})if(Pair.Key>0)Bar->AddSlot().FillWidth(float(Pair.Key))[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Pair.Value).Padding(0)[SNew(SBox).HeightOverride(8)]];
 Body->AddSlot().AutoHeight().Padding(0,0,0,6)[Bar];auto BarLabels=SNew(SHorizontalBox);BarLabels->AddSlot().FillWidth(1)[Text(FString::Printf(TEXT("DEMOCRATIC %.0f"),Dem),11,Blue,true,false)];BarLabels->AddSlot().AutoWidth()[Text(TEXT("270 TO WIN  /  CURRENT GAME PROJECTION"),10,Muted)];BarLabels->AddSlot().FillWidth(1).HAlign(HAlign_Right)[Text(FString::Printf(TEXT("REPUBLICAN %.0f"),Rep),11,Red,true,false)];Body->AddSlot().AutoHeight().Padding(0,0,0,10)[BarLabels];
 auto Tabs=SNew(SHorizontalBox);const TCHAR* Pages[]={TEXT("COUNTY ATLAS"),TEXT("ROAD TO 270"),TEXT("CULTURAL COALITIONS"),TEXT("SOURCES & MODEL")};for(int I=0;I<4;++I)Tabs->AddSlot().AutoWidth().Padding(0,0,8,0)[Small(FString(Page==I?TEXT("> "):TEXT(""))+Pages[I],[this,I](){Page=I;Dirty=true;})];Body->AddSlot().AutoHeight().Padding(0,0,0,10)[Tabs];
 if(!Message.IsEmpty())Body->AddSlot().AutoHeight().Padding(0,0,0,8)[Text(Message,12,Gold)];
 if(Page==0){
  Atlas->SetData(D,View,Layer);auto Main=SNew(SHorizontalBox),Tools=SNew(SHorizontalBox);auto Left=SNew(SVerticalBox);
  const TCHAR* Layers[]={TEXT("VOTE"),TEXT("CULTURE"),TEXT("TURNOUT"),TEXT("INCOME"),TEXT("EDUCATION"),TEXT("POVERTY"),TEXT("APPROVAL"),TEXT("2024")};
  for(int I=0;I<8;++I)Tools->AddSlot().AutoWidth().Padding(0,0,3,0)[Small(FString(Layer==I?TEXT("> "):TEXT(""))+Layers[I],[this,I](){Layer=I;Dirty=true;})];Left->AddSlot().AutoHeight().Padding(0,0,0,6)[Tools];
  auto Nav=SNew(SHorizontalBox);Nav->AddSlot().AutoWidth().Padding(0,0,4,0)[Small(TEXT("+"),[this](){Atlas->ZoomBy(1.5);})];Nav->AddSlot().AutoWidth().Padding(0,0,4,0)[Small(TEXT("-"),[this](){Atlas->ZoomBy(1/1.5);})];Nav->AddSlot().AutoWidth().Padding(0,0,6,0)[Small(TEXT("RESET"),[this](){Atlas->Reset();})];
  Nav->AddSlot().FillWidth(1)[SAssignNew(Search,SSearchBox).HintText(FText::FromString(TEXT("State, city, county or FIPS; Enter"))).OnTextCommitted_Lambda([this](const FText& Q,ETextCommit::Type How){if(How==ETextCommit::OnEnter){Message=Atlas->Search(Q.ToString())?TEXT(""):TEXT("No match. Try a full city/state name or County, ST.");Dirty=true;FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));}})];
  Nav->AddSlot().AutoWidth().Padding(6,0)[Small(Atlas->CitiesOn?TEXT("CITIES ON"):TEXT("CITIES OFF"),[this](){Atlas->CitiesOn=!Atlas->CitiesOn;Dirty=true;})];Nav->AddSlot().AutoWidth()[Small(Expanded?TEXT("DETAILS"):TEXT("EXPAND"),[this](){Expanded=!Expanded;Dirty=true;})];Left->AddSlot().AutoHeight().Padding(0,0,0,5)[Nav];
  Left->AddSlot().FillHeight(1)[Atlas.ToSharedRef()];
  const TCHAR* Legends[]={TEXT("BLUE: Democratic vote  /  RED: Republican vote  /  Pale: competitive"),TEXT("Woodard regional cultures  /  Cook and Orleans have shared affiliations"),TEXT("DARK: 35%  /  LIGHT GREEN: 80%+ modeled adult turnout proxy"),TEXT("BLUE: $30k  /  GOLD: $110k+ median household income (2021 dollars)"),TEXT("DARK: 0%  /  TEAL: 65%+ with a bachelor's degree (age 25+)"),TEXT("TEAL: 0%  /  AMBER: 35%+ below the poverty threshold"),TEXT("PLUM: 30%  /  MINT: 70%+ presidential approval"),TEXT("HISTORICAL 2024 TWO-PARTY VOTE  /  Alaska and Kalawao use state estimates")};
  Left->AddSlot().AutoHeight().Padding(0,6,0,4)[Text(Legends[Layer],11,Muted)];
  if(Layer==1){auto Legend=SNew(SUniformGridPanel).SlotPadding(FMargin(0,2,7,2));int I=0;for(const auto& C:D.Get("cultures").Items()){auto Cell=SNew(SHorizontalBox);Cell->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,5,0)[SNew(SBox).WidthOverride(9).HeightOverride(9)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Hex(T(C,"color"))).Padding(0)]];Cell->AddSlot().FillWidth(1)[Text(T(C,"name"),10,Ink,false,false)];Legend->AddSlot(I%4,I/4)[Cell];++I;}Left->AddSlot().AutoHeight()[Legend];}
  Left->AddSlot().AutoHeight().Padding(0,5,0,0)[Text(TEXT("SCROLL: ZOOM  /  DRAG: PAN  /  CLICK: COUNTY  /  DOUBLE-CLICK: STATE  /  DOTS: NAMED CITIES"),10,Muted)];
  Main->AddSlot().FillWidth(1).Padding(0,0,Expanded?0:14,0)[Left];
  if(!Expanded){auto Detail=SNew(SVerticalBox);const auto* C=Find(D.Get("counties"),Selected);const auto* R=Find(View.Get("counties"),Selected);
   if(C&&R){const auto* St=Find(View.Get("states"),T(*C,"state"));Line(Detail,T(*C,"fullName"),23,Ink,true);Line(Detail,T(*C,"state")+TEXT("  /  COUNTY ")+Selected,11,Gold,true);
    Line(Detail,TEXT("Population ")+Compact(N(*C,"population"))+TEXT("  |  Adults ")+Compact(N(*C,"adults")),13,Muted);Meter(Detail,TEXT("Presidential approval"),N(*R,"approval"));Meter(Detail,TEXT("Your projected two-party vote"),N(*R,"support"),Republican?Red:Blue);Meter(Detail,TEXT("Modeled turnout proxy"),N(*R,"turnout"),Gold);
    if(St)Line(Detail,FString::Printf(TEXT("%s: %.1f%% your vote / %.0f of %.0f electors"),*T(*St,"name"),N(*St,"support"),N(*St,"points"),N(*St,"ev")),13,Gold,true);
    Detail->AddSlot().AutoHeight().Padding(0,4,0,12)[Small(TEXT("ORGANIZE HERE  /  2 CAPITAL"),[this](){Message=Game->OrganizeCounty(Selected).Message;Dirty=true;},!S.Get("ended").Truthy()&&S.Get("agendaChoice").IsNull()&&N(S,"capital")>=2)];
    Line(Detail,TEXT("REGIONAL CULTURE"),11,Gold,true);for(std::size_t I=0;I<C->Get("cultures").Size();++I){const auto* Culture=Find(D.Get("cultures"),F(C->Get("cultures").Keys()[I]));if(Culture){Line(Detail,T(*Culture,"name")+TEXT("  ")+Pct(C->Get("cultures").Values()[I].AsNumber()*100),15,Hex(T(*Culture,"color")),true);Line(Detail,T(*Culture,"description"),12,Muted);}}
    Line(Detail,TEXT("LOCAL CONDITIONS"),11,Gold,true);Line(Detail,FString::Printf(TEXT("Median household income  $%.0f\nPoverty  %.1f%%  /  Unemployment  %.1f%%\nCollege graduates  %.1f%%  /  Age 65+  %.1f%% of adults\nOwner households  %.1f%%"),N(*C,"income"),N(*C,"poverty"),N(*C,"unemployment"),N(*C,"college")*100,N(*C,"senior")*100,N(*C,"homeowner")*100),13,Muted);
    Line(Detail,TEXT("OVERLAPPING VOTER MIXES"),11,Gold,true);Line(Detail,TEXT("Estimated joint groups: age / education / housing. Share of local adults, then your projected vote."),11,Muted);
    for(const auto& A:R->Get("cohorts").Items()){auto Row=SNew(SHorizontalBox);Row->AddSlot().FillWidth(1)[Text(T(A,"name"),11,Muted)];Row->AddSlot().AutoWidth()[Text(FString::Printf(TEXT("%.0f%%  /  %.0f%%"),N(A,"share"),N(A,"support")),11,Ink,true)];Detail->AddSlot().AutoHeight().Padding(0,3)[Row];}
    Line(Detail,TEXT("POPULATION BACKGROUND"),11,Gold,true);const auto& Demographic=C->Get("demographics");for(const auto Pair:{TPair<const char*,const TCHAR*>("white",TEXT("White, non-Hispanic")),{"black",TEXT("Black, non-Hispanic")},{"asian",TEXT("Asian, non-Hispanic")},{"native",TEXT("Native American, non-Hispanic")},{"hispanic",TEXT("Hispanic, any race")}})Line(Detail,FString(Pair.Value)+TEXT("  ")+Pct(N(Demographic,Pair.Key)),12,Muted);
    Line(Detail,TEXT("Background shares describe residents, not party allegiance. Other and multiracial residents are not separately listed here."),11,Muted);
    if(T(*C,"electionSource")!=TEXT("county"))Line(Detail,TEXT("Local election baseline estimated from state totals; county results unavailable in this input."),11,Gold);
   }
   Main->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(320)[SNew(SScrollBox)+SScrollBox::Slot()[Panel(Detail)]]];
  }Body->AddSlot().FillHeight(1)[Main];
 }else{
  auto List=SNew(SVerticalBox);
  if(Page==1){
   Line(List,TEXT("Land does not vote. People choose the electors."),24,Gold,true);Line(List,TEXT("Each tile shows your projected vote and electoral votes secured. Maine and Nebraska award two statewide electors plus one per congressional district."),15,Muted);
   auto Grid=SNew(SUniformGridPanel).SlotPadding(FMargin(4));int I=0;
   for(const auto& A:View.Get("states").Items()){const FString Id=T(A,"id");auto Cell=SNew(SVerticalBox);Line(Cell,Id+FString::Printf(TEXT("   %.0f EV"),N(A,"ev")),17,Ink,true);Line(Cell,Pct(N(A,"support"))+TEXT(" your vote"),12,N(A,"support")>=50?Teal:Red);Line(Cell,FString::Printf(TEXT("%.0f secured"),N(A,"points")),11,Muted);Grid->AddSlot(I%9,I/9)[SNew(SButton).ButtonStyle(&Style()).IsFocusable(false).OnClicked_Lambda([this,Id](){Page=0;Atlas->Search(Id);Dirty=true;return FReply::Handled();})[Cell]];++I;}List->AddSlot().AutoHeight()[Grid];
   Line(List,TEXT("DISTRICT ELECTORS"),16,Gold,true);auto DistrictRow=SNew(SHorizontalBox);for(const auto& A:View.Get("districts").Items())DistrictRow->AddSlot().FillWidth(1).Padding(4)[Text(T(A,"id")+TEXT("  ")+Pct(N(A,"support"))+TEXT(" your vote"),14,A.Get("won").Truthy()?Teal:Red,true)];List->AddSlot().AutoHeight()[DistrictRow];Line(List,TEXT("District estimates use their certified 2024 baseline plus the modeled statewide swing. A 269-269 result has no Electoral College winner; congressional resolution is not simulated."),12,Muted);
  }else if(Page==2){
   Line(List,TEXT("A coalition of regional cultures"),24,Gold,true);Line(List,TEXT("Inspired by Colin Woodard's American Nations and Nations Apart. Regional traditions influence issue priorities; local conditions and overlapping voter groups create variation within every region."),15,Muted);
   auto Grid=SNew(SUniformGridPanel).SlotPadding(FMargin(6));int I=0;for(const auto& C:D.Get("cultures").Items()){const auto* A=Find(View.Get("nations"),T(C,"id"));if(!A)continue;auto Cell=SNew(SVerticalBox);Line(Cell,T(C,"name"),19,Hex(T(C,"color")),true);Line(Cell,T(C,"description"),13,Muted);Line(Cell,Compact(N(*A,"adults"))+TEXT(" adults represented"),12,Muted);Meter(Cell,TEXT("Approval"),N(*A,"approval"));Meter(Cell,TEXT("Your two-party vote"),N(*A,"support"),Republican?Red:Blue);Grid->AddSlot(I%3,I/3)[Panel(Cell)];++I;}List->AddSlot().AutoHeight()[Grid];
  }else{
   Line(List,TEXT("Real geography. Measured baselines. A transparent game model."),24,Gold,true);
   const TArray<TPair<FString,FString>> Notes={
    {TEXT("CULTURAL GEOGRAPHY"),TEXT("Colin Woodard / Portland Press Herald, 2017 county classification table. Cook County is split equally between Yankeedom and Midlands; Orleans Parish between New France and Deep South. County-code changes are reconciled. This is the published historical crosswalk, not Nationhood Lab's newer request-only 2025 spreadsheet.")},
    {TEXT("PEOPLE & PLACES"),TEXT("U.S. Census 2021 county boundaries; 2020 demographic shares, 2017-2021 ACS education, income and housing, and 2021 population/employment through USDA ERS. All 50 states and Washington, D.C. are included. These are dated baselines, not live 2026 estimates.")},
    {TEXT("ELECTORAL BASELINE"),TEXT("MIT Election Data and Science Lab 2024 county returns, reconciled to certified statewide totals. County identifiers and town reporting are harmonized. Alaska boroughs and Kalawao use statewide estimates, not invented local returns. Historical 2024 colors show the input two-party share.")},
    {TEXT("WHAT IS SIMULATED"),TEXT("Eight overlapping age/education/housing groups per county use synthetic joint distributions from measured marginals. Issue weights, turnout response, incumbency advantage and cultural policy responses are authored game rules, not Woodard survey coefficients. Race is descriptive and never directly assigns a vote.")},
    {TEXT("APPROVAL IS NOT A VOTE"),TEXT("Approval responds to public services, economic conditions, policy fit, local crises and political relationships. Party support starts from historical vote patterns and moves with performance and organizing. Turnout changes the weight of each community. A popular-vote victory can still lose the Electoral College.")},
    {TEXT("LIMITS & NEXT STEPS"),TEXT("Two-party competition only. Adult population is a turnout proxy, not a count of eligible citizens. Cohorts assume independent demographic marginals; ACS sampling error is not propagated. Maine/Nebraska districts use state-swing estimates. Midterms remain a simplified national congressional mandate. This is a strategy-game simulation, not a predictive election model.")},
    {TEXT("READ MORE"),TEXT("colinwoodard.com/books/american-nations/  |  colinwoodard.com/books/nations-apart/\nnationhoodlab.org  |  ers.usda.gov  |  electionlab.mit.edu  |  archives.gov/electoral-college/allocation")}};
   for(const auto& Note:Notes){Line(List,Note.Key,13,Gold,true);Line(List,Note.Value,15,Muted);}
  }
  Body->AddSlot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[List]];
 }
 Body->AddSlot().AutoHeight().Padding(0,10,0,0)[Text(TEXT("CENSUS + USDA ERS + MIT ELECTION LAB  /  WOODARD CULTURAL GEOGRAPHY  /  Fictional game projection  /  V or Esc: office"),10,Muted,false,false)];
}
