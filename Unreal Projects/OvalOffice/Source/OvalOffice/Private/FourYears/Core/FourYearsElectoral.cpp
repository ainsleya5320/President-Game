#include "FourYears/Core/FourYearsElectoral.h"
#include <algorithm>
#include <cmath>
#include <map>
namespace FourYears { namespace Electoral {
namespace {
using V=JsonValue;
double N(const V& X,const char* K,double Default=0){const auto& A=X.Get(K);return A.IsNumber()?A.AsNumber():Default;}
double Clamp(double X,double A=0,double B=100){return std::max(A,std::min(B,X));}
double Round(double X){return JsRound(X*1000)/1000;}
void Set(V& X,const char* K,double Value){X.Set(K,V::Number(Value));}
void Add(V& X,const char* K,double Value){Set(X,K,N(X,K)+Value);}
V Str(const std::string& S){return V::String(S);}
V Result(bool Ok,const std::string& Text){V R=V::Object();R.Set("ok",V::Boolean(Ok));R.Set(Ok?"detail":"reason",Str(Text));return R;}
V Unit(const std::string& Id,const std::string& Name,double Support,double Points){V R=V::Object();R.Set("id",Str(Id));R.Set("name",Str(Name));Set(R,"approval",Support);Set(R,"points",Points);R.Set("won",V::Boolean(Support>50));return R;}
}
JsonValue View(const V& D,const V& S,double Signal){
 const std::string Party=S.Get("electoral").Get("party").AsString();const int Direction=Party=="republican"?-1:1;
 V Rows=V::Array(),States=V::Array(),Nations=V::Array();std::map<std::string,std::size_t> SI,NI;std::map<std::string,const V*> Cultures;
 for(const auto& C:D.Get("cultures").Items()){const auto Id=C.Get("id").AsString();Cultures[Id]=&C;NI[Id]=Nations.Size();V A=V::Object();A.Set("id",C.Get("id"));A.Set("name",C.Get("name"));for(const char* K:{"adults","votes","supportVotes","approvalSum"})Set(A,K,0);Nations.Push(std::move(A));}
 for(const auto& Def:D.Get("states").Items()){SI[Def.Get("id").AsString()]=States.Size();V A=Def;for(const char* K:{"votes","supportVotes","approvalSum","adults","turnoutSum","counties"})Set(A,K,0);States.Push(std::move(A));}
 double NationalVotes=0,NationalSupport=0,NationalApproval=0,Adults=0,TurnoutSum=0;
 const auto& M=S.Get("metrics");
 for(const auto& C:D.Get("counties").Items()){
  const auto& Local=S.Get("executive").Get("regions").Get(C.Get("region").AsString());double Service=0,Autonomy=0,Ecology=0;
  for(std::size_t I=0;I<C.Get("cultures").Size();++I){const auto& Id=C.Get("cultures").Keys()[I];const double W=C.Get("cultures").Values()[I].AsNumber();const auto It=Cultures.find(Id);if(It==Cultures.end())continue;Service+=N(*It->second,"service")*W;Autonomy+=N(*It->second,"autonomy")*W;Ecology+=N(*It->second,"ecology")*W;}
  const auto Lev=[&](const char* Id){return N(S.Get("implemented"),Id,N(S.Get("levels"),Id,2))-2;};
  const double Policy=Service*(Lev("clinic")+Lev("insurance")+Lev("teacher")+Lev("tax"))*.48+Ecology*(Lev("carbon")+Lev("renewables")-Lev("fossil"))*.55-Autonomy*Lev("surveillance")*.6-N(C,"mining")*(Lev("carbon")-Lev("fossil"))*4-N(C,"manufacturing")*Lev("trade")*2;
  double Field=0;for(const auto& E:S.Get("electoral").Get("organizing").Items()){const double Decay=std::pow(.65,std::max(0.,N(S,"quarter")-N(E,"quarter")));if(E.Get("county").AsString()==C.Get("id").AsString())Field+=2.5*Decay;else if(E.Get("state").AsString()==C.Get("state").AsString())Field+=.18*Decay;}Field=Clamp(Field,0,5);
  const double LocalEffect=(N(Local,"housing",N(M,"housing"))-N(M,"housing")+N(Local,"jobs",N(M,"jobs"))-N(M,"jobs")+N(Local,"health",N(M,"health"))-N(M,"health")+N(Local,"energy",N(M,"energy"))-N(M,"energy"))/4*.3+N(Local,"goodwill")*.55;
  double Vote=0,Turn=0,Approval=0;V Cohorts=V::Array();
  for(int A=0;A<2;++A)for(int E=0;E<2;++E)for(int H=0;H<2;++H){
   const double W=(A?N(C,"senior"):1-N(C,"senior"))*(E?N(C,"college"):1-N(C,"college"))*(H?N(C,"homeowner"):1-N(C,"homeowner"));double Perf=0,Total=0;
   const auto& Weights=C.Get("weights");for(std::size_t I=0;I<Weights.Size();++I){const auto& Issue=Weights.Keys()[I];const double Weight=Weights.Values()[I].AsNumber();const double Factor=Issue=="health"?(A?1.65:.85):Issue=="housing"?(H?.8:1.7):Issue=="schools"?(A?.65:1.2):Issue=="climate"?(E?1.3:.85):1;Perf+=(M.Get(Issue).AsNumber()-50)*Weight*Factor;Total+=Weight*Factor;}
   const double Mood=Clamp(51+Perf/Total*.9+(Signal-51)*.35+Policy+LocalEffect,3,97);
   const double Prior=Party.empty()?50:(Direction==1?N(C,"dem2024"):100-N(C,"dem2024"));
   const double Response=.7+(1-std::abs(Prior-50)/50)*.35;
   const double Support=Clamp(Prior+2+(Mood-50)*.42*Response+Field,1,99);
   const double Baseline=Clamp(N(C,"baseTurnout")+(A-N(C,"senior"))*.08+(E-N(C,"college"))*.06+(H-N(C,"homeowner"))*.03,.15,.96);
   const double Participation=Clamp(Baseline+std::abs(Mood-50)*.0006+(N(M,"trust")-53)*.001+Field*.008,.12,.98);
   const double Ballots=N(C,"ballots")*W*Participation/Baseline;Vote+=Ballots*Support/100;Turn+=Ballots;Approval+=W*Mood;
   V Cohort=V::Object();Cohort.Set("name",Str(std::string(A?"65+":"18-64")+" / "+(E?"college":"no BA")+" / "+(H?"owner":"renter")));Set(Cohort,"share",Round(W*100));Set(Cohort,"approval",Round(Mood));Set(Cohort,"support",Round(Support));Set(Cohort,"turnout",Round(Participation*100));Cohorts.Push(std::move(Cohort));
  }
  const double Turnout=Clamp(N(C,"baseTurnout")*Turn/N(C,"ballots")*100),Support=Vote/Turn*100;
  V Row=V::Object();Row.Set("id",C.Get("id"));Row.Set("state",C.Get("state"));Set(Row,"approval",Round(Approval));Set(Row,"support",Round(Support));Set(Row,"turnout",Round(Turnout));Set(Row,"votes",Round(Turn));Set(Row,"swing",Round(Support-(Party=="republican"?100-N(C,"dem2024"):Party.empty()?50:N(C,"dem2024"))));Set(Row,"field",Round(Field));Row.Set("cohorts",std::move(Cohorts));Rows.Push(std::move(Row));
  auto& St=States[SI[C.Get("state").AsString()]];Add(St,"votes",Turn);Add(St,"supportVotes",Vote);Add(St,"approvalSum",Approval*N(C,"adults"));Add(St,"adults",N(C,"adults"));Add(St,"turnoutSum",Turnout*N(C,"adults"));Add(St,"counties",1);
  for(std::size_t I=0;I<C.Get("cultures").Size();++I){const auto& Id=C.Get("cultures").Keys()[I];const double W=C.Get("cultures").Values()[I].AsNumber();auto& Nat=Nations[NI[Id]];Add(Nat,"adults",N(C,"adults")*W);Add(Nat,"votes",Turn*W);Add(Nat,"supportVotes",Vote*W);Add(Nat,"approvalSum",Approval*N(C,"adults")*W);}
  NationalVotes+=Turn;NationalSupport+=Vote;NationalApproval+=Approval*N(C,"adults");Adults+=N(C,"adults");TurnoutSum+=Turnout*N(C,"adults");
 }
 double Points=0,Opponent=0,Tossup=0;V Units=V::Array();
 for(auto& St:States.Items()){
  Set(St,"support",Round(N(St,"supportVotes")/N(St,"votes")*100));Set(St,"approval",Round(N(St,"approvalSum")/N(St,"adults")));Set(St,"turnout",Round(N(St,"turnoutSum")/N(St,"adults")));Set(St,"margin",Round((N(St,"support")-50)*2));
  const auto Id=St.Get("id").AsString();const double Count=(Id=="ME"||Id=="NE")?2:N(St,"ev");Units.Push(Unit(Id,St.Get("name").AsString(),N(St,"support"),Count));
  if(N(St,"support")>50)Points+=Count;else if(N(St,"support")<50)Opponent+=Count;else Tossup+=Count;
  St.Set("won",V::Boolean(N(St,"support")>50));Set(St,"points",N(St,"support")>50?Count:0);
 }
 V Districts=V::Array();for(const auto& Def:D.Get("districts").Items()){
  auto& St=States[SI[Def.Get("state").AsString()]];const double Base=Party=="republican"?100-N(Def,"dem2024"):Party.empty()?50:N(Def,"dem2024"),StateBase=Party=="republican"?100-N(St,"dem2024"):Party.empty()?50:N(St,"dem2024");
  const double Support=Round(Clamp(Base+N(St,"support")-StateBase,1,99));const bool Won=Support>50;
  if(Won){++Points;Add(St,"points",1);}else if(Support<50)++Opponent;else ++Tossup;
  Units.Push(Unit(Def.Get("id").AsString(),Def.Get("id").AsString()+" district",Support,1));V District=Def;Set(District,"support",Support);District.Set("won",V::Boolean(Won));Districts.Push(std::move(District));
 }
 for(auto& Nat:Nations.Items()){Set(Nat,"support",Round(N(Nat,"supportVotes")/N(Nat,"votes")*100));Set(Nat,"approval",Round(N(Nat,"approvalSum")/N(Nat,"adults")));}
 V R=V::Object();R.Set("party",Str(Party));Set(R,"approval",Round(NationalApproval/Adults));Set(R,"support",Round(NationalSupport/NationalVotes*100));Set(R,"turnout",Round(TurnoutSum/Adults));Set(R,"adults",Adults);Set(R,"points",Points);Set(R,"opponent",Opponent);Set(R,"tossup",Tossup);R.Set("won",V::Boolean(Points>=270));R.Set("tie",V::Boolean(Points<270&&Opponent<270));R.Set("states",std::move(States));R.Set("districts",std::move(Districts));R.Set("counties",std::move(Rows));R.Set("nations",std::move(Nations));R.Set("units",std::move(Units));return R;
}
JsonValue Election(const V& View){V R=V::Object();R.Set("points",View.Get("points"));R.Set("regions",View.Get("units"));R.Set("won",View.Get("won"));R.Set("tie",View.Get("tie"));R.Set("vote",View.Get("support"));Set(R,"total",538);Set(R,"needed",270);R.Set("opponent",View.Get("opponent"));return R;}
JsonValue ChooseParty(V& S,const std::string& Party){
 if((Party!="democrat"&&Party!="republican")||!S.Get("electoral").Get("party").AsString().empty()||S.Get("ended").Truthy())return Result(false,"Choose your party once per presidency.");
 V E=V::Object();E.Set("party",Str(Party));E.Set("organizing",V::Array());E.Set("chosen",S.Get("quarter"));S.Set("electoral",std::move(E));return Result(true,std::string(Party=="democrat"?"Democratic":"Republican")+" administration selected. Your existing decisions and progress are preserved.");
}
JsonValue Organize(const V& D,V& S,const std::string& Id){
 const V* C=nullptr;for(const auto& A:D.Get("counties").Items())if(A.Get("id").AsString()==Id){C=&A;break;}
 if(!C||S.Get("electoral").Get("party").AsString().empty()||S.Get("ended").Truthy()||!S.Get("agendaChoice").IsNull())return Result(false,"Choose your party and keep the quarterly decision open.");
 bool Used=false;for(const auto& A:S.Get("electoral").Get("organizing").Items())if(N(A,"quarter")==N(S,"quarter"))Used=true;
 if(N(S,"capital")<2||Used)return Result(false,"One field operation each quarter, costing 2 political capital.");
 Set(S,"capital",N(S,"capital")-2);V Op=V::Object();Op.Set("county",Str(Id));Op.Set("state",C->Get("state"));Op.Set("quarter",S.Get("quarter"));S["electoral"]["organizing"].Push(std::move(Op));
 const std::string Detail="Field organizers arrive in "+C->Get("name").AsString()+", "+C->Get("state").AsString()+". Local enthusiasm and turnout improve; the effect fades each quarter.";
 V H=V::Object();H.Set("quarter",S.Get("quarter"));H.Set("type",Str("organizing"));H.Set("title",Str("Local organizing"));H.Set("detail",Str(Detail));S["history"].Push(std::move(H));return Result(true,Detail);
}
} }
