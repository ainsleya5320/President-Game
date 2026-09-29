#include "FourYears/Core/FourYearsDiplomacy.h"
#include <iostream>
#include <cmath>
using V=FourYears::JsonValue;
namespace D=FourYears::Diplomacy;
int Checks=0;
void Check(bool Ok,const char* Name){++Checks;if(!Ok){std::cerr<<"FAILED: "<<Name<<"\n";std::exit(1);}}
V Fresh(){V S=V::Object();S.Set("quarter",V::Number(0));S.Set("seed",V::Number(6137));S.Set("capital",V::Number(20));S.Set("debt",V::Number(80));S.Set("ended",V::Boolean(false));S.Set("agendaChoice",V::Null());S.Set("history",V::Array());S.Set("metrics",V::Object());for(auto K:{"growth","prices","trust","safety","energy"})S["metrics"].Set(K,V::Number(50));D::Initialize(S);return S;}
bool Ok(const V& R){return R.Get("ok").Truthy();}
void Next(V& S){S.Set("agendaChoice",V::Number(0));D::Advance(S);S.Set("quarter",V::Number(S.Get("quarter").AsNumber()+1));S.Set("agendaChoice",V::Null());S.Set("capital",V::Number(20));}
int main(){
 auto S=Fresh();Check(S.Get("diplomacy").Get("regions").Size()==12,"twelve partners");Check(S.Get("diplomacy").Get("crises").Size()==1,"initial crisis");
 auto Snap=S.Dump();D::Initialize(S);Check(S.Dump()==Snap,"migration is idempotent");
 Check(!Ok(D::Act(S,"bad","trade"))&&S.Dump()==Snap,"unknown region is atomic");
 Check(!Ok(D::Act(S,"canada","bad"))&&S.Dump()==Snap,"unknown action is atomic");
 Check(!Ok(D::Respond(S,-1,"fund"))&&S.Dump()==Snap,"invalid crisis is atomic");
 Check(Ok(D::SetDoctrine(S,"peace")),"select peacemaker");
 Check(Ok(D::Act(S,"canada","trade")),"trade with eligible partner");
 Check(S.Get("capital").AsNumber()==17&&S.Get("debt").AsNumber()==82,"trade costs");
 Snap=S.Dump();Check(!Ok(D::Act(S,"canada","envoy"))&&S.Dump()==Snap,"one action per partner");
 Check(!Ok(D::SetDoctrine(S,"commerce"))&&S.Dump()==Snap,"strategy locked on first action");
 Check(Ok(D::Act(S,"uk","alliance")),"eligible alliance");
 Check(Ok(D::Respond(S,0,"mediate")),"start mediation");
 Check(S.Get("diplomacy").Get("actions").AsNumber()==0,"three action limit");
 Snap=S.Dump();Check(!Ok(D::Act(S,"japan","envoy"))&&S.Dump()==Snap,"action cap is atomic");
 Check(!Ok(D::Respond(S,0,"mediate"))&&S.Dump()==Snap,"cannot finish talks in same quarter");
 Check(D::Advance(S).Size()==0&&S.Dump()==Snap,"cannot advance without domestic decision");
 double Debt=S.Get("debt").AsNumber();Next(S);Check(S.Get("debt").AsNumber()<Debt,"trade dividend");
 Check(S.Get("diplomacy").Get("actions").AsNumber()==3,"quarter refresh");
 Check(Ok(D::Respond(S,0,"mediate")),"finish mediation next quarter");
 Check(S.Get("diplomacy").Get("resolved").AsNumber()==1,"peace victory progress");
 Snap=S.Dump();Check(!Ok(D::Respond(S,0,"fund"))&&S.Dump()==Snap,"closed crisis cannot pay again");
 auto R=Fresh();Snap=R.Dump();Check(!Ok(D::Act(R,"russia","alliance"))&&R.Dump()==Snap,"alliance prerequisite");
 Check(!Ok(D::Act(R,"china","trade"))&&R.Dump()==Snap,"trade prerequisite");
 Check(Ok(D::Act(R,"china","sanction")),"sanctions available");
 Check(R.Get("diplomacy").Get("tension").AsNumber()==36,"sanctions escalate");
 Next(R);Check(Ok(D::Act(R,"china","lift")),"sanctions can be lifted");
 auto Low=Fresh();Low.Set("capital",V::Number(0));Snap=Low.Dump();Check(!Ok(D::Act(Low,"canada","envoy"))&&Low.Dump()==Snap,"capital gate");
 Low=Fresh();Low.Set("debt",V::Number(249));Snap=Low.Dump();Check(!Ok(D::Respond(Low,0,"fund"))&&Low.Dump()==Snap,"debt gate");
 auto Neglect=Fresh();Next(Neglect);Next(Neglect);Next(Neglect);
 Check(Neglect.Get("diplomacy").Get("setbacks").AsNumber()==1,"missed crisis deadline");
 Check(Neglect.Get("metrics").Get("prices").AsNumber()<50,"crisis domestic consequences");
 auto Copy=Fresh();Copy.Set("agendaChoice",V::Number(0));auto Twin=Copy;
 D::Advance(Copy);D::Advance(Twin);Check(Copy.Dump()==Twin.Dump(),"deterministic rival turn");
 Snap=Copy.Dump();Check(D::Advance(Copy).Size()==0&&Copy.Dump()==Snap,"quarter cannot process twice");
 V Loaded;std::string Error;Check(V::Parse(Neglect.Dump(),Loaded,Error),"save round trip");D::Initialize(Loaded);Check(Loaded.Dump()==Neglect.Dump(),"save restores world state");
 auto Win=Fresh();for(auto Id:{"canada","uk","europe","japan"})Win["diplomacy"]["regions"][Id].Set("trade",V::Boolean(true));
 for(const auto& Def:D::Regions())Win["diplomacy"]["regions"][Def.Id].Set("influence",V::Number(60));
 Check(D::View(Win).Get("onCourse").Truthy(),"commerce victory conditions");
 Win["diplomacy"].Set("tension",V::Number(80));Check(!D::View(Win).Get("onCourse").Truthy(),"escalation blocks victory");
 Win.Set("ended",V::Boolean(true));Snap=Win.Dump();Check(!Ok(D::Act(Win,"canada","envoy"))&&D::Advance(Win).Size()==0&&Snap==Win.Dump(),"ended presidency immutable");
 for(auto Doctrine:{"commerce","coalition","peace"}){auto Campaign=Fresh();D::SetDoctrine(Campaign,Doctrine);for(int Q=0;Q<16;++Q){for(const auto& Def:D::Regions()){auto O=D::Options(Campaign,Def.Id);for(const auto& Choice:O.Items())if(Choice.Get("available").Truthy()){D::Act(Campaign,Def.Id,Choice.Get("id").AsString());break;}}Next(Campaign);auto View=D::View(Campaign);Check(View.Get("score").AsNumber()>=0&&View.Get("score").AsNumber()<=100,"score bounded");for(const auto& Def:D::Regions())for(auto Key:{"relation","stability","influence","rival"}){double Value=Campaign.Get("diplomacy").Get("regions").Get(Def.Id).Get(Key).AsNumber();Check(std::isfinite(Value)&&Value>=0&&Value<=100,"world values bounded");}}}
 std::cout<<Checks<<" diplomacy checks passed\n";
}
