#include "FourYears/Core/FourYearsPresidency.h"
#include "FourYears/Core/FourYearsElectoral.h"
#include "FourYears/Core/FourYearsDiplomacy.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
using V=FourYears::JsonValue;namespace P=FourYears::Presidency;
int Checks=0;void Check(bool B,const char* T){++Checks;if(!B){std::cerr<<"FAIL "<<T<<"\n";std::exit(1);}}
bool Ok(const V& R){return R.Get("ok").Truthy();}
void Place(V& S,const std::string& Id){S["presidency"].Set("place",V::String(Id));}
int main(int argc,char** argv){
 FourYears::Simulation Sim;std::vector<std::pair<std::string,std::string>> Files;
 for(const auto& E:std::filesystem::directory_iterator(argc>1?argv[1]:"Prototype/data")){if(E.path().extension()!=".json")continue;std::ifstream I(E.path());std::stringstream B;B<<I.rdbuf();Files.emplace_back(E.path().stem().string(),B.str());}
 std::vector<std::string> Errors;Check(Sim.LoadData(Files,Errors),"game data loads");
 const auto& Data=Sim.Data().Get("presidency");Check(Data.Get("chapters").Size()==16,"sixteen chapters");
 for(const auto& C:Data.Get("chapters").Items()){Check(C.Get("choices").Size()==3,"three chapter choices");for(const auto& Ch:C.Get("choices").Items())for(auto K:{"effects","later"})for(const auto& M:Ch.Get(K).Keys())Check(Sim.Data().Get("metrics").Get("labels").Has(M),"known metric");}
 auto Fresh=[&]{V S=Sim.Fresh(6137);P::Initialize(S);FourYears::Electoral::ChooseParty(S,"democrat");return S;};
 auto S=Fresh();std::string Snap=S.Dump();P::Initialize(S);Check(S.Dump()==Snap,"migration idempotent");
 Check(!Ok(P::Act(Sim,S,"story","0"))&&S.Dump()==Snap,"chapter requires correct place atomically");
 for(const auto& M:Data.Get("mandates").Items()){auto T=Fresh();Check(Ok(P::Act(Sim,T,"mandate",M.Get("id").AsString())),"all mandates valid");Check(Sim.Legacy(T).Get("goals").Size()==3,"three goals");}
 Place(S,"residence");Check(Ok(P::Act(Sim,S,"story","0")),"first chapter");Snap=S.Dump();Check(!Ok(P::Act(Sim,S,"story","1"))&&Snap==S.Dump(),"cannot repeat chapter");
 Check(Ok(P::Act(Sim,S,"rest","")),"rest available");Check(S.Get("presidency").Get("fatigue").AsNumber()==0,"fatigue restored");Place(S,"limo");Snap=S.Dump();Check(!Ok(P::Act(Sim,S,"prepare",""))&&S.Dump()==Snap,"personal time shared across places");
 S=Fresh();Place(S,"cabinet");Check(Ok(Sim.Propose(S,"housing")),"bill proposed");const auto* Bill=Sim.ActiveBill(S);auto Before=Sim.Votes(S,*Bill);Check(Ok(P::Act(Sim,S,"contact","center")),"caucus meeting");auto After=Sim.Votes(S,*Sim.ActiveBill(S));Check(After.Dump()!=Before.Dump(),"relationships affect actual whip count");Snap=S.Dump();Check(!Ok(P::Act(Sim,S,"contact","regional"))&&S.Dump()==Snap,"one caucus contact per quarter");
 S=Fresh();Snap=S.Dump();Check(P::BeforeAdvance(S).Size()==0&&S.Dump()==Snap,"no advance without filed decision");Check(Sim.Choose(S,0).Truthy(),"decision filed");Snap=S.Dump();Check(!Ok(P::Act(Sim,S,"delegate",""))&&S.Dump()==Snap,"filed quarter closes personal actions");Check(P::BeforeAdvance(S).Size()==1,"missed chapter explicitly delegated");Check(S.Get("presidency").Get("decisions").Size()==1,"delegation recorded");
 // Exercise every narrative response, including all delayed results and recall branches.
 for(int Q=0;Q<16;++Q)for(int Choice=0;Choice<3;++Choice){auto T=Fresh();T.Set("quarter",V::Number(Q));T.Set("capital",V::Number(20));const auto& C=Data.Get("chapters")[Q];Place(T,C.Get("place").AsString());Check(Ok(P::Act(Sim,T,"story",std::to_string(Choice))),"every chapter option reachable");Check(!P::View(Sim,T).Get("chapter").Get("title").AsString().empty(),"chapter view populated");}
 auto Existing=Sim.Fresh(42);Existing.Set("quarter",V::Number(4));FourYears::Electoral::ChooseParty(Existing,"democrat");P::Initialize(Existing);Place(Existing,"cabinet");Check(P::View(Sim,Existing).Get("context").AsString().find("without a catch-up penalty")!=std::string::npos,"older saves are not penalized for unavailable chapters");
 auto Closed=Sim.Fresh(42);Sim.Choose(Closed,0);P::Initialize(Closed);Check(P::View(Sim,Closed).Get("done").Truthy(),"already-filed saves bypass the new chapter without penalty");
 auto Tired=Fresh();Tired["presidency"].Set("fatigue",V::Number(70));P::Act(Sim,Tired,"delegate","");Sim.Choose(Tired,0);double Cap=Tired.Get("capital").AsNumber();P::BeforeAdvance(Tired);Check(Tired.Get("capital").AsNumber()==Cap-1,"exhaustion has a real political cost");
 auto Ending=Fresh();Ending.Set("ended",V::Boolean(true));Ending["executive"].Set("finalElection",V::Object());Ending["executive"]["finalElection"].Set("won",V::Boolean(false));Check(P::View(Sim,Ending).Get("endingTitle").AsString()=="The peaceful handover","defeat receives its own ending");Ending["executive"]["finalElection"].Set("tie",V::Boolean(true));Check(P::View(Sim,Ending).Get("endingTitle").AsString()=="The unresolved mandate","tie is not misreported as victory");
 for(const char* Party:{"democrat","republican"})for(int Path=0;Path<3;++Path){
  auto T=Sim.Fresh(6137+Path);P::Initialize(T);Check(Ok(FourYears::Electoral::ChooseParty(T,Party)),"choose party");Check(Ok(P::Act(Sim,T,"mandate",Data.Get("mandates")[Path].Get("id").AsString())),"choose campaign mandate");FourYears::Diplomacy::Initialize(T);
  for(int Q=0;Q<16;++Q){
   const auto& Chapter=Data.Get("chapters")[Q];Place(T,Chapter.Get("place").AsString());
   auto R=P::Act(Sim,T,"story",std::to_string(Path));if(!Ok(R))R=P::Act(Sim,T,"delegate","");Check(Ok(R),"quarter chapter never blocks progress");
   Place(T,"residence");P::Act(Sim,T,"rest","");
   Check(Sim.Choose(T,Path%Sim.CurrentEvent(T)->Get("choices").Size()).Truthy(),"quarter response available");P::BeforeAdvance(T);FourYears::Diplomacy::Advance(T);Check(Sim.Advance(T).IsObject(),"quarter resolves");
   if(Q==7)Check(T.Get("midterm").IsObject(),"midterm reckoning occurs");
  }
  Check(T.Get("ended").Truthy()&&T.Get("quarter").AsNumber()==16,"campaign ends at four years");Check(T.Get("presidency").Get("decisions").Size()==16,"all personal chapters recorded");Check(T.Get("executive").Get("finalElection").IsObject(),"general election recorded");auto View=P::View(Sim,T);Check(!View.Get("epilogue").AsString().empty(),"epilogue produced");Check(View.Get("score").AsNumber()>=0&&View.Get("score").AsNumber()<=100,"bounded legacy score");
  Snap=T.Dump();Check(!Ok(P::Act(Sim,T,"rest",""))&&Snap==T.Dump(),"ended term immutable");V Loaded;std::string Error;Check(V::Parse(Snap,Loaded,Error),"serialize campaign");Loaded=Sim.Migrate(Loaded);P::Initialize(Loaded);Check(Loaded.Get("presidency").Dump()==T.Get("presidency").Dump(),"save migration retains campaign");
  std::cout<<Party<<" path "<<Path<<": approval "<<Sim.Poll(T)<<", legacy "<<View.Get("score").AsNumber()<<", "<<View.Get("endingTitle").AsString()<<"\n";
 }
 std::cout<<"PASS "<<Checks<<" campaign checks\n";
}
