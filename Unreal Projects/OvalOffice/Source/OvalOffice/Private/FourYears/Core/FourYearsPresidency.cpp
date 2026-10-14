#include "FourYears/Core/FourYearsPresidency.h"
#include <algorithm>
#include <cmath>

namespace FourYears { namespace Presidency {
namespace {
using V = JsonValue;
double N(const V& X, const char* K, double F = 0) { const auto& A = X.Get(K); return A.IsNumber() ? A.AsNumber() : F; }
double Clamp(double X, double Max = 100) { return std::max(0., std::min(Max, X)); }
void Set(V& X, const char* K, double A) { X.Set(K, V::Number(A)); }
void Add(V& X, const std::string& K, double A, double Max = 100) { X.Set(K, V::Number(Clamp(X.Get(K).AsNumber() + A, Max))); }
V Result(bool Ok, const std::string& Text) { V R=V::Object(); R.Set("ok",V::Boolean(Ok)); R.Set(Ok?"detail":"reason",V::String(Text)); return R; }
void Record(V& S,const std::string& Title,const std::string& Detail) { V H=V::Object(); H.Set("quarter",S.Get("quarter")); H.Set("type",V::String("presidency")); H.Set("title",V::String(Title)); H.Set("detail",V::String(Detail)); S["history"].Push(std::move(H)); }
const V* Find(const V& List,const std::string& Id) { for(const auto& X:List.Items()) if(X.Get("id").AsString()==Id) return &X; return nullptr; }
bool Done(const V& S) { for(const auto& D:S.Get("presidency").Get("decisions").Items()) if(N(D,"quarter")==N(S,"quarter")) return true; return false; }
bool HadOpportunity(const V& S,const V& D,const std::string& Flag){
 const int Joined=int(N(S.Get("presidency"),"joined")),Now=int(N(S,"quarter"));
 for(int Q=Joined;Q<Now&&Q<int(D.Get("chapters").Size());++Q)for(const auto& C:D.Get("chapters")[Q].Get("choices").Items())if(C.Get("flag").AsString()==Flag)return true;
 return false;
}
std::string Effects(const V& X) { std::string R; for(std::size_t I=0;I<X.Size();++I) { if(!R.empty()) R+=" / "; double A=X.Values()[I].AsNumber(); R+=X.Keys()[I]+" "+(A>0?"+":"")+FormatNumber(A); } return R; }
std::string Summary(const V& C) { std::string R=Effects(C.Get("effects")); auto More=[&](const std::string& T){if(!R.empty())R+=" / ";R+=T;}; if(N(C,"capital"))More("capital "+std::string(N(C,"capital")>0?"+":"")+FormatNumber(N(C,"capital"))); if(N(C,"debt"))More("debt "+std::string(N(C,"debt")>0?"+":"")+FormatNumber(N(C,"debt"))); if(C.Get("relations").Size())More("caucus relationships: "+Effects(C.Get("relations"))); if(C.Get("later").Size())More("in two quarters: "+Effects(C.Get("later"))); return R; }
}
void Initialize(V& S) {
 if(S.Get("presidency").IsObject())return;
 V P=V::Object();Set(P,"version",1);Set(P,"joined",N(S,"quarter"));Set(P,"fatigue",15);Set(P,"activityQuarter",-1);Set(P,"contactQuarter",-1);
 P.Set("decisions",V::Array());
 if(!S.Get("agendaChoice").IsNullish()){V Closed=V::Object();Closed.Set("quarter",S.Get("quarter"));Closed.Set("title",V::String("Existing quarterly decision"));Closed.Set("choice",V::String("This quarter was filed before personal chapters became available."));Closed.Set("detail",V::String("Your earlier decision is preserved without a delegation penalty."));P["decisions"].Push(std::move(Closed));}
 P.Set("flags",V::Object());P.Set("place",V::String("oval"));P.Set("relations",V::Object());
 for(const char* Id:{"center","regional","fiscal","reform"})Set(P["relations"],Id,50);
 S.Set("presidency",std::move(P));
}
V View(const Simulation& Sim,const V& S) {
 const auto& P=S.Get("presidency"); const auto& D=Sim.Data().Get("presidency"); const int Q=std::min(15,int(N(S,"quarter")));
 V R=V::Object();R.Set("chapter",D.Get("chapters")[Q]);R.Set("act",D.Get("acts")[Q/4]);R.Set("done",V::Boolean(Done(S)));R.Set("fatigue",P.Get("fatigue"));R.Set("place",P.Get("place"));R.Set("mandates",D.Get("mandates"));R.Set("legacy",Sim.Legacy(S));
 R.Set("activityAvailable",V::Boolean(N(P,"activityQuarter",-1)!=N(S,"quarter")));R.Set("contactAvailable",V::Boolean(N(P,"contactQuarter",-1)!=N(S,"quarter")));
 V Leaders=D.Get("leaders"); for(auto& L:Leaders.Items()){double Bond=N(P.Get("relations"),L.Get("id").AsString().c_str(),50);Set(L,"relationship",Bond);Set(L,"bonus",(Bond-50)*.35);}R.Set("leaders",std::move(Leaders));
 for(auto& C:R["chapter"]["choices"].Items())C.Set("summary",V::String(Summary(C)));
 const auto Recall=R.Get("chapter").Get("recall").AsString();
 const bool Prepared=!Recall.empty()&&P.Get("flags").Get(Recall).Truthy();
 R.Set("context",V::String(Recall.empty()?"":Prepared?"Your earlier decision to invest in "+Recall+" gives the team room to act. This chapter earns an additional trust +2 and capital +1.":HadOpportunity(S,D,Recall)?"You reach this moment without the earlier preparation. Handling this chapter adds debt +2 to cover the gap.":"This preparation predates your current campaign. Your existing presidency continues without a catch-up penalty."));
 const auto L=Sim.Legacy(S);double Met=0;for(const auto& G:L.Get("goals").Items())if(G.Get("met").Truthy())++Met;
 double Delivery=0;for(const auto& B:S.Get("executive").Get("bills").Items())if(B.Get("status").AsString()=="passed")Delivery+=N(B,"progress")/100.;
 const double Score=std::round(Clamp(Sim.Poll(S)*.4+Met*10+std::min(3.,Delivery)*7+N(S.Get("metrics"),"trust")*.09));Set(R,"score",Score);Set(R,"goalsMet",Met);Set(R,"delivered",Delivery);
 const bool Won=S.Get("executive").Get("finalElection").Get("won").Truthy(),Tie=S.Get("executive").Get("finalElection").Get("tie").Truthy();
 R.Set("endingTitle",V::String(Tie?"The unresolved mandate":Won?(Score>=65?"A mandate renewed":"A narrow second chance"):(Score>=65?"The work outlives the office":"The peaceful handover")));
 std::string Epilogue=Tie?"No candidate reached 270 electors. The constitutional process moves to Congress. Your playable term ends here; a contingent election is not simulated.":Won?"The election desk calls the race. You have earned another term. Outside, the city continues its ordinary work. This four-year campaign is complete; the next administration begins beyond this story.":"The call comes just after midnight. You congratulate the president-elect and ask your staff to prepare the transition. Leaving office does not erase the decisions you made here.";
 Epilogue+="\n\n"+std::to_string(int(Met))+" platform targets reached. "+std::to_string(int(L.Get("completed").AsNumber()))+" major programs fully delivered. "+std::to_string(int(L.Get("kept").AsNumber()))+" personal promises kept.";
 Epilogue+=N(S.Get("metrics"),"trust")>=60?" Citizens leave your term with stronger trust in public institutions.":" Rebuilding confidence in government remains unfinished work.";
 Epilogue+=N(P,"fatigue")>=70?" The private cost has been heavy; the first quiet morning feels unfamiliar.":" You kept enough of a life beyond the office to recognize yourself when the room grows quiet.";
 R.Set("epilogue",V::String(Epilogue));return R;
}
V Act(const Simulation& Sim,V& S,const std::string& Action,const std::string& Target) {
 Initialize(S);if(S.Get("ended").Truthy())return Result(false,"Your term is complete. Explore your record or start a new presidency.");
 if(S.Get("electoral").Get("party").AsString().empty())return Result(false,"Choose your administration on the voter screen first.");
 if(!S.Get("agendaChoice").IsNull())return Result(false,"The quarterly decision is filed. Advance at the desk before making new commitments.");
 auto& P=S["presidency"];const auto& D=Sim.Data().Get("presidency");
 if(Action=="mandate") {const auto* M=Find(D.Get("mandates"),Target);if(!M)return Result(false,"Unknown mandate.");std::vector<std::string> Goals;for(const auto& G:M->Get("goals").Items())Goals.push_back(G.AsString());return Sim.SetPlatform(S,Goals,M->Get("signature").AsString());}
 if(Action=="story"||Action=="delegate") {
  if(Done(S))return Result(false,"This chapter is already on the record.");
  const int Q=int(N(S,"quarter"));if(Q<0||Q>=16)return Result(false,"No active chapter.");const V Chapter=D.Get("chapters")[Q];
  V Decision=V::Object();Decision.Set("quarter",S.Get("quarter"));Decision.Set("title",Chapter.Get("title"));
  if(Action=="delegate") {Add(S["metrics"],"trust",-1);Decision.Set("choice",V::String("Delegated to staff"));Decision.Set("detail",V::String("Staff handled the chapter. Trust -1; no personal decision benefits."));}
  else {
   if(P.Get("place").AsString()!=Chapter.Get("place").AsString())return Result(false,"Travel to "+Chapter.Get("place").AsString()+" to handle this conversation in person, or delegate it to staff.");
   int I=Target=="0"?0:Target=="1"?1:Target=="2"?2:-1;if(I<0)return Result(false,"Choose one of the three responses.");const V C=Chapter.Get("choices")[I];
   if(N(S,"capital")+N(C,"capital")<0)return Result(false,"You need more political capital for that response. Choose another approach or delegate.");
   for(std::size_t K=0;K<C.Get("effects").Size();++K)Add(S["metrics"],C.Get("effects").Keys()[K],C.Get("effects").Values()[K].AsNumber());
   Add(S,"capital",N(C,"capital"),20);Add(S,"debt",N(C,"debt"),250);Add(P,"fatigue",5);
   for(std::size_t K=0;K<C.Get("relations").Size();++K)Add(P["relations"],C.Get("relations").Keys()[K],C.Get("relations").Values()[K].AsNumber());
   const auto Recall=Chapter.Get("recall").AsString();std::string Extra;
   if(!Recall.empty()){if(P.Get("flags").Get(Recall).Truthy()){Add(S["metrics"],"trust",2);Add(S,"capital",1,20);Extra=" Earlier preparation: trust +2, capital +1.";}else if(HadOpportunity(S,D,Recall)){Add(S,"debt",2,250);Extra=" Catch-up response: debt +2.";}}
   P["flags"].Set(C.Get("flag").AsString(),V::Boolean(true));
   if(C.Get("later").Size()){V E=V::Object();Set(E,"due",Q+2);E.Set("delta",C.Get("later"));E.Set("title",V::String(Chapter.Get("title").AsString()+": longer-term result"));S["effects"].Push(std::move(E));}
   Decision.Set("choice",C.Get("label"));Decision.Set("detail",V::String(Summary(C)+Extra));
  }
  const auto Detail=Decision.Get("choice").AsString()+". "+Decision.Get("detail").AsString();Record(S,Chapter.Get("title").AsString(),Detail);S["presidency"]["decisions"].Push(std::move(Decision));return Result(true,Detail);
 }
 if(Action=="contact") {
  if(!Find(D.Get("leaders"),Target))return Result(false,"Unknown caucus leader.");
  if(N(P,"contactQuarter",-1)==N(S,"quarter")||N(S,"capital")<2)return Result(false,"One relationship meeting per quarter, costing 2 capital.");
  if(P.Get("place").AsString()!="cabinet"&&P.Get("place").AsString()!="limo")return Result(false,"Meet congressional leaders in the Cabinet Room or call from the limo.");
  Add(S,"capital",-2,20);Add(P["relations"],Target,9);Set(P,"contactQuarter",N(S,"quarter"));Record(S,"Congressional relationship",Target+" relationship +9. This affects support for every bill; it is separate from lobbying one bill.");return Result(true,"Relationship +9. The caucus's underlying support for all bills has increased.");
 }
 if(Action=="rest"||Action=="prepare") {
  if(N(P,"activityQuarter",-1)==N(S,"quarter"))return Result(false,"You have used this quarter's personal time. It opens again next quarter.");
  const bool Rest=Action=="rest";if(P.Get("place").AsString()!=(Rest?"residence":"limo"))return Result(false,Rest?"Return to the residence to rest.":"Take the briefing in the limo.");
  Set(P,"activityQuarter",N(S,"quarter"));Add(P,"fatigue",Rest?-20:4);if(!Rest)Add(S,"capital",2,20);
  const std::string Detail=Rest?"A quiet evening restores you. Fatigue -20.":"You review the route briefing with your aide. Capital +2; fatigue +4.";Record(S,"Personal time",Detail);return Result(true,Detail);
 }
 return Result(false,"Unknown presidency action.");
}
V BeforeAdvance(V& S) {
 Initialize(S);V Lines=V::Array();if(S.Get("ended").Truthy()||S.Get("agendaChoice").IsNull())return Lines;
 auto& P=S["presidency"];
 if(!Done(S)){V C=V::Object();C.Set("quarter",S.Get("quarter"));C.Set("title",V::String("Chapter delegated"));C.Set("choice",V::String("Staff handled this quarter's personal decision"));C.Set("detail",V::String("Trust -1"));P["decisions"].Push(std::move(C));Add(S["metrics"],"trust",-1);Lines.Push(V::String("Personal chapter delegated to staff: trust -1. Visit Presidency [J] before filing next time."));}
 Add(P,"fatigue",7);if(N(P,"fatigue")>=70){Add(S,"capital",-1,20);Add(S["metrics"],"trust",-1);Lines.Push(V::String("Exhaustion: capital -1, trust -1. Rest in the residence to recover."));}
 for(auto& Vv:P["relations"].Values())Vv=V::Number(50+(Vv.AsNumber()-50)*.9);
 return Lines;
}
} }
