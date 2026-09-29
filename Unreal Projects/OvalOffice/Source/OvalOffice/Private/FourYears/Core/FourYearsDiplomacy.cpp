#include "FourYears/Core/FourYearsDiplomacy.h"
#include <algorithm>
#include <cmath>

namespace FourYears::Diplomacy
{
using V=JsonValue;
namespace {
V N(double X) {return V::Number(X);}
V B(bool X) {return V::Boolean(X);}
V T(const std::string& X) {return V::String(X);}
double Num(const V& X,const char* Key) {return X.Get(Key).NumberOr(0);}
void Add(V& X,const char* Key,double Delta,double Max=100) {X.Set(Key,N(std::clamp(Num(X,Key)+Delta,0.0,Max)));}
V Result(bool Ok,const std::string& Message) {V R=V::Object();R.Set("ok",B(Ok));R.Set(Ok?"detail":"reason",T(Message));return R;}
std::string Int(double X) {return FormatNumber(JsRound(X));}
const Region* Def(const std::string& Id) {for(const auto& R:Regions())if(Id==R.Id)return &R;return nullptr;}
std::string Gate(const V& S) {
 if(S.Get("ended").Truthy())return "This presidency has ended.";
 if(!S.Get("agendaChoice").IsNull())return "Diplomacy closes when you file the quarterly decision. Advance the quarter first.";
 if(Num(S.Get("diplomacy"),"actions")<1)return "No diplomatic actions left. Advance the quarter to receive three more.";
 return "";
}
void Log(V& S,const std::string& Text) {
 V Item=V::Object();Item.Set("quarter",S.Get("quarter"));Item.Set("text",T(Text));
 auto& News=S["diplomacy"]["news"];News.Insert(0,Item);if(News.Size()>12)News.Resize(12);
 V H=V::Object();H.Set("quarter",S.Get("quarter"));H.Set("type",T("diplomacy"));H.Set("title",T("World affairs"));H.Set("detail",T(Text));S["history"].Push(H);
}
void Spend(V& S,int Capital,int Debt) {Add(S,"capital",-Capital,20);Add(S,"debt",Debt,250);Add(S["diplomacy"],"actions",-1,3);}
void Metric(V& S,const char* Key,double Change) {Add(S["metrics"],Key,Change);}
struct Crisis {const char* Title;const char* Body;const char* RegionId;const char* Metric;};
const Crisis Crises[]={
 {"Shipping lanes under threat","Commercial ships face harassment in a vital sea lane. Insurers are raising prices. Build an agreement, fund a maritime protection mission, or assemble a strong coalition.","gulf","prices"},
 {"A fragile border ceasefire","An incident threatens to unravel a ceasefire. Both delegations demand guarantees. A sustained mediation effort could prevent a wider confrontation.","europe","trust"},
 {"Critical minerals standoff","Export restrictions threaten manufacturers and clean-energy projects. Investors want a credible agreement before they move their supply chains.","africa","energy"},
 {"Regional debt emergency","A financial shock threatens banks and employment across a major trading partner. Emergency lending buys time; negotiation takes political patience.","brazil","growth"},
 {"Disputed cyber incident","A wave of cyberattacks interrupts ports and payment systems. Public accusations could escalate the dispute before investigators establish responsibility.","india","safety"},
 {"Pacific disaster response","A severe storm has cut off island communities and shipping routes. Several powers are offering assistance and competing for regional trust.","indonesia","prices"}
};
void Spawn(V& S,int Quarter) {
 V& D=S["diplomacy"];int Sequence=int(Num(D,"sequence"));const auto& C=Crises[Sequence%6];
 V R=V::Object();R.Set("title",T(C.Title));R.Set("body",T(C.Body));R.Set("region",T(C.RegionId));R.Set("metric",T(C.Metric));R.Set("deadline",N(Quarter+3));R.Set("progress",N(0));R.Set("lastAction",N(-1));R.Set("status",T("open"));
 D["crises"].Push(R);D.Set("sequence",N(Sequence+1));Log(S,std::string("New crisis: ")+C.Title+". Respond before the end of quarter "+Int(Quarter+3)+".");
}
V Option(const std::string& Id,const std::string& Label,const std::string& Detail,int Cost,int Debt,const std::string& Reason) {
 V O=V::Object();O.Set("id",T(Id));O.Set("label",T(Label));O.Set("detail",T(Detail));O.Set("cost",N(Cost));O.Set("debt",N(Debt));O.Set("reason",T(Reason));O.Set("available",B(Reason.empty()));return O;
}
void FinishCrisis(V& S,V& C,const std::string& How) {
 auto& D=S["diplomacy"];auto& R=D["regions"][C.Get("region").AsString()];
 C.Set("status",T("resolved"));Add(D,"resolved",1,1000);Add(D,"prestige",8);Add(D,"tension",-6);Add(R,"stability",10);Add(R,"relation",5);Add(R,"influence",8);Metric(S,"trust",1);
 Log(S,C.Get("title").AsString()+": resolved through "+How+". Prestige +8; tension -6.");
}
}
const std::vector<Region>& Regions() {
 static const std::vector<Region> R={
 {"canada","Canada","Energy security and integrated supply chains.",-108,58,78,65,18,5},
 {"brazil","Brazil","Market access, infrastructure and strategic autonomy.",-52,-12,52,36,42,6},
 {"uk","United Kingdom","Intelligence cooperation and financial stability.",-3,55,76,65,18,5},
 {"europe","European Union","Reliable trade, shared security and climate investment.",17,48,68,55,30,10},
 {"russia","Russia","Regional leverage and relief from economic isolation.",80,61,25,18,78,6},
 {"gulf","Gulf States","Maritime security and energy investment.",48,25,50,40,48,8},
 {"africa","South Africa","Development finance and critical-mineral exports.",25,-29,48,32,48,5},
 {"india","India","Technology access and freedom to choose its partners.",78,22,55,40,46,8},
 {"china","China","Export access and leadership in international institutions.",108,36,32,22,78,10},
 {"indonesia","Indonesia","Maritime trade, disaster resilience and regional autonomy.",118,-5,52,35,45,6},
 {"japan","Japan","Secure sea lanes and advanced industrial cooperation.",140,38,72,60,25,8},
 {"australia","Australia","Pacific partnerships and resilient supply chains.",135,-25,75,62,22,6}
 };return R;
}
void Initialize(V& S) {
 if(S.Get("diplomacy").IsObject())return;
 V D=V::Object();D.Set("version",N(1));D.Set("actions",N(3));D.Set("prestige",N(40));D.Set("tension",N(28));D.Set("resolved",N(0));D.Set("setbacks",N(0));D.Set("doctrine",T("commerce"));D.Set("committed",B(false));D.Set("sequence",N(0));D.Set("lastQuarter",S.Get("quarter"));D.Set("started",S.Get("quarter"));D.Set("news",V::Array());D.Set("crises",V::Array());D.Set("regions",V::Object());
 for(const auto& R:Regions()) {
  V P=V::Object();P.Set("relation",N(R.Relation));P.Set("influence",N(R.Influence));P.Set("rival",N(R.Rival));P.Set("stability",N(65));P.Set("trade",B(false));P.Set("alliance",B(false));P.Set("sanction",B(false));P.Set("lastAction",N(-1));D["regions"].Set(R.Id,P);
 }
 S.Set("diplomacy",D);Spawn(S,int(Num(S,"quarter")));
}
V View(const V& S) {
 const auto& D=S.Get("diplomacy");V R=D;double Influence=0,Rival=0,Weight=0,Exports=0;int Trade=0,Allies=0;
 for(const auto& Defn:Regions()) {
  const auto& P=D.Get("regions").Get(Defn.Id);Influence+=Num(P,"influence")*Defn.TradeValue;Rival+=Num(P,"rival")*Defn.TradeValue;Weight+=Defn.TradeValue;
  if(P.Get("trade").Truthy()){++Trade;if(Num(P,"stability")>=35&&!P.Get("sanction").Truthy())Exports+=Defn.TradeValue;}
  if(P.Get("alliance").Truthy())++Allies;
 }
 Influence=JsRound(Influence/Weight);Rival=JsRound(Rival/Weight);
 R.Set("influence",N(Influence));R.Set("rival",N(Rival));R.Set("tradeCount",N(Trade));R.Set("allies",N(Allies));R.Set("exports",N(Exports));R.Set("dividend",N(std::min(6.0,Exports*.15)));
 const std::string Doctrine=D.Get("doctrine").AsString();double A=0,C=0;int TensionTarget=45;std::string Goal;
 if(Doctrine=="coalition"){A=Allies/4.0;C=Influence/60.0;TensionTarget=55;Goal="4 security partners / 60 influence / tension at or below 55";}
 else if(Doctrine=="peace"){A=Num(D,"resolved")/4.0;C=Num(D,"prestige")/70.0;TensionTarget=30;Goal="4 crises resolved / 70 prestige / tension at or below 30";}
 else {A=Trade/4.0;C=Influence/55.0;Goal="4 trade pacts / 55 influence / tension at or below 45";}
 double Calm=Num(D,"tension")<=TensionTarget?1.0:std::max(0.0,1.0-(Num(D,"tension")-TensionTarget)/(100-TensionTarget));
 const bool Won=A>=1&&C>=1&&Num(D,"tension")<=TensionTarget;
 R.Set("goal",T(Goal));R.Set("score",N(std::min(Won?100.0:99.0,JsRound((std::min(A,1.0)+std::min(C,1.0)+Calm)*100/3))));
 R.Set("onCourse",B(Won));R.Set("outcome",T(S.Get("ended").Truthy()?(Won?"Foreign-policy victory":"Foreign-policy objectives missed"):(Won?"Victory conditions met - hold them to term end":"Victory campaign in progress")));return R;
}
V SetDoctrine(V& S,const std::string& Doctrine) {
 if(Doctrine!="commerce"&&Doctrine!="coalition"&&Doctrine!="peace")return Result(false,"Unknown strategy.");
 const auto& D=S.Get("diplomacy");if(S.Get("ended").Truthy()||D.Get("committed").Truthy()||!S.Get("agendaChoice").IsNull())return Result(false,"Choose your strategy before your first diplomatic action. It stays fixed for this presidency.");
 S["diplomacy"].Set("doctrine",T(Doctrine));return Result(true,"Strategy selected. Your first diplomatic action locks this victory goal.");
}
V Options(const V& S,const std::string& Id) {
 V Out=V::Array();const auto* Definition=Def(Id);if(!Definition)return Out;const auto& R=S.Get("diplomacy").Get("regions").Get(Id);
 auto Make=[&](const char* Key,const char* Label,const char* Detail,int Cost,int Debt,std::string Why){
  std::string Reason=Gate(S);if(Reason.empty()&&Num(R,"lastAction")==Num(S,"quarter"))Reason="One bilateral action per partner each quarter.";
  if(Reason.empty()&&!Why.empty())Reason=Why;if(Reason.empty()&&Num(S,"capital")<Cost)Reason="Insufficient political capital.";
  if(Reason.empty()&&Num(S,"debt")+Debt>250)Reason="This commitment would exceed the debt limit.";
  Out.Push(Option(Key,Label,Detail,Cost,Debt,Reason));
 };
 Make("envoy","Send a presidential envoy","Relations +10; influence +6. Builds the groundwork for agreements.",1,0,"");
 Make("aid","Development partnership","Stability +10; relations +6; influence +4; prestige +2.",2,3,"");
 Make("trade","Sign a trade pact","Influence +7. Each active pact earns a quarterly debt-reducing trade dividend.",3,2,R.Get("trade").Truthy()?"A trade pact is already signed.":R.Get("sanction").Truthy()?"Lift sanctions before negotiating trade.":Num(R,"relation")<55?"Requires relations of 55.":Num(R,"stability")<40?"Requires stability of 40.":"");
 Make("alliance","Security partnership","Influence +8; prestige +3; tension +2. Partners resist rival pressure.",4,2,R.Get("alliance").Truthy()?"Already a security partner.":Num(R,"relation")<70||Num(R,"influence")<55?"Requires relations 70 and influence 55.":"");
 Make("sanction","Impose targeted sanctions","Rival influence -8; your influence +4; relations -12; tension +8. Ends the trade pact.",2,0,R.Get("sanction").Truthy()?"Sanctions are already in force.":Num(R,"rival")<50?"Requires rival influence of 50 or more.":"");
 Make("lift","Lift sanctions","Relations +10; tension -4. Trade must be renegotiated.",1,0,!R.Get("sanction").Truthy()?"No sanctions to lift.":"");
 return Out;
}
V Act(V& S,const std::string& Id,const std::string& Action) {
 const V Choices=Options(S,Id);const V* Choice=nullptr;for(const auto& O:Choices.Items())if(O.Get("id").AsString()==Action)Choice=&O;
 if(!Choice)return Result(false,"Unknown partner or diplomatic action.");if(!Choice->Get("available").Truthy())return Result(false,Choice->Get("reason").AsString());
 Spend(S,int(Num(*Choice,"cost")),int(Num(*Choice,"debt")));auto& D=S["diplomacy"];auto& R=D["regions"][Id];D.Set("committed",B(true));R.Set("lastAction",S.Get("quarter"));
 if(Action=="envoy"){Add(R,"relation",10);Add(R,"influence",6);}
 if(Action=="aid"){Add(R,"stability",10);Add(R,"relation",6);Add(R,"influence",4);Add(D,"prestige",2);}
 if(Action=="trade"){R.Set("trade",B(true));Add(R,"influence",7);}
 if(Action=="alliance"){R.Set("alliance",B(true));Add(R,"influence",8);Add(D,"prestige",3);Add(D,"tension",2);}
 if(Action=="sanction"){R.Set("sanction",B(true));R.Set("trade",B(false));Add(R,"rival",-8);Add(R,"influence",4);Add(R,"relation",-12);Add(D,"tension",8);}
 if(Action=="lift"){R.Set("sanction",B(false));Add(R,"relation",10);Add(D,"tension",-4);}
 std::string Text=std::string(Def(Id)->Name)+": "+Choice->Get("label").AsString()+". "+Choice->Get("detail").AsString();Log(S,Text);return Result(true,Text);
}
V CrisisOptions(const V& S,int Index) {
 V Out=V::Array();const auto& D=S.Get("diplomacy");if(Index<0||Index>=int(D.Get("crises").Size()))return Out;const auto& C=D.Get("crises")[Index];const auto& R=D.Get("regions").Get(C.Get("region").AsString());
 auto Make=[&](const char* Id,const char* Label,const char* Detail,int Cost,int Debt,std::string Why){
  std::string Reason=Gate(S);
  if(Reason.empty()&&(C.Get("status").AsString()!="open"||Num(C,"lastAction")==Num(S,"quarter")))Reason="This crisis has already received a response this quarter or is closed.";
  if(Reason.empty()&&!Why.empty())Reason=Why;if(Reason.empty()&&Num(S,"capital")<Cost)Reason="Insufficient political capital.";
  if(Reason.empty()&&Num(S,"debt")+Debt>250)Reason="This commitment would exceed the debt limit.";
  Out.Push(Option(Id,Label,Detail,Cost,Debt,Reason));
 };
 Make("mediate",Num(C,"progress")>0?"Conclude the talks":"Open mediation talks","Two rounds, in different quarters, resolve the crisis. Each round reduces tension by 3.",2,0,"");
 Make("fund","Fund a negotiated settlement","Resolves immediately. Debt +8; prestige +8; tension -6.",4,8,"");
 Make("coalition","Mobilize a diplomatic coalition","Resolves immediately with existing leverage. Tension +4 before the settlement reduces it by 6.",3,0,Num(R,"influence")<60&&Num(View(S),"allies")<3?"Requires local influence 60 or three security partners.":"");
 return Out;
}
V Respond(V& S,int Index,const std::string& Choice) {
 const V Choices=CrisisOptions(S,Index);const V* Selected=nullptr;for(const auto& O:Choices.Items())if(O.Get("id").AsString()==Choice)Selected=&O;
 if(!Selected)return Result(false,"Unknown crisis response.");if(!Selected->Get("available").Truthy())return Result(false,Selected->Get("reason").AsString());
 Spend(S,int(Num(*Selected,"cost")),int(Num(*Selected,"debt")));auto& D=S["diplomacy"];D.Set("committed",B(true));auto& C=D["crises"][Index];C.Set("lastAction",S.Get("quarter"));
 if(Choice=="mediate"){
  Add(C,"progress",1,2);Add(D,"tension",-3);
  if(Num(C,"progress")>=2)FinishCrisis(S,C,"mediation");else Log(S,C.Get("title").AsString()+": talks opened. Return next quarter to conclude them before the deadline.");
 } else {if(Choice=="coalition")Add(D,"tension",4);FinishCrisis(S,C,Choice=="fund"?"a funded settlement":"coalition diplomacy");}
 return Result(true,Choice=="mediate"&&Num(C,"progress")<2?"Talks opened. A second round next quarter is required.":"Crisis resolved. Your diplomatic record and international standing improve.");
}
V Advance(V& S) {
 V Report=V::Array();auto& D=S["diplomacy"];const int Q=int(Num(S,"quarter"))+1;
 if(S.Get("ended").Truthy()||S.Get("agendaChoice").IsNull()||Num(D,"lastQuarter")>=Q)return Report;
 auto News=[&](const std::string& Text){Report.Push(T(Text));Log(S,Text);};
 D.Set("lastQuarter",N(Q));D.Set("actions",N(3));
 const V Before=View(S);const double Dividend=Num(Before,"dividend");if(Dividend>0){Add(S,"debt",-Dividend,250);Metric(S,"growth",std::min(2.0,Num(Before,"exports")*.04));News("Trade network: debt reduced by "+FormatNumber(Dividend)+"; export demand supports growth.");}
 const int Target=(Q+int(std::fmod(std::abs(Num(S,"seed")),12.0)))%int(Regions().size());
 for(int I=0;I<int(Regions().size());++I){
  const auto& Defn=Regions()[I];auto& R=D["regions"][Defn.Id];bool Ally=R.Get("alliance").Truthy();
  Add(R,"rival",I==Target?5:1);Add(R,"influence",I==Target?(Ally?-1:-4):(Ally?1:0));
  if(R.Get("trade").Truthy())Add(R,"relation",1);else if(Num(R,"relation")>Defn.Relation)Add(R,"relation",-1);
  if(R.Get("sanction").Truthy()){Add(R,"relation",-2);Add(R,"stability",-2);}
  if(Ally&&Num(R,"relation")<50){R.Set("alliance",B(false));News(std::string(Defn.Name)+" suspends its security partnership after relations deteriorate.");}
 }
 News(std::string("Rival diplomatic offensive in ")+Regions()[Target].Name+". Partners reduce the influence loss.");
 for(auto& C:D["crises"].Items())if(C.Get("status").AsString()=="open"&&Num(C,"deadline")<=Q){
  C.Set("status",T("failed"));Add(D,"setbacks",1,1000);Add(D,"tension",10);Add(D,"prestige",-8);
  auto& R=D["regions"][C.Get("region").AsString()];Add(R,"stability",-15);Add(R,"influence",-8);Metric(S,C.Get("metric").AsString().c_str(),-3);Metric(S,"trust",-1);
  News(C.Get("title").AsString()+": deadline missed. Tension +10; prestige -8; domestic conditions suffer.");
 }
 if(Num(D,"tension")>=70){Metric(S,"prices",-2);Metric(S,"growth",-1);News("High global tension damages price stability and investment.");}
 Add(D,"tension",-1);
 if(Q<16&&(Q-int(Num(D,"started")))%2==0){Spawn(S,Q);Report.Push(T("A new international crisis is awaiting a response on the World Map [M]."));}
 if(Q>=16)News(View(S).Get("onCourse").Truthy()?"Foreign-policy victory: all strategic objectives achieved.":"Foreign-policy campaign ended with objectives still outstanding.");
 return Report;
}
}
