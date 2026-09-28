#include "FourYears/Core/FourYearsSim.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <set>

// Comments name the JavaScript function each block mirrors. Arithmetic keeps the JavaScript's operation
// order (including left-to-right summation), because the parity test compares states exactly.
namespace FourYears
{
namespace
{
using V = JsonValue;

double Bound(double N, double A = 0.0, double B = 100.0) { return std::max(A, std::min(B, N)); }
V Num(double N) { return V::Number(N); }
V Str(const std::string& Text) { return V::String(Text); }
V Bool(bool Value) { return V::Boolean(Value); }
std::string Fmt(double N) { return FormatNumber(N); }

V Ok(const std::string& Detail)
{
	V Result = V::Object();
	Result.Set("ok", Bool(true));
	Result.Set("detail", Str(Detail));
	return Result;
}

V Fail(const std::string& Reason)
{
	V Result = V::Object();
	Result.Set("ok", Bool(false));
	Result.Set("reason", Str(Reason));
	return Result;
}

const V* FindById(const V& List, const std::string& Id)
{
	for (const V& Item : List.Items())
	{
		if (Item.Get("id").AsString() == Id) return &Item;
	}
	return nullptr;
}

bool Contains(const V& List, const std::string& Text)
{
	for (const V& Item : List.Items())
	{
		if (Item.IsString() && Item.AsString() == Text) return true;
	}
	return false;
}

bool ContainsNumber(std::initializer_list<double> List, double N)
{
	return std::find(List.begin(), List.end(), N) != List.end();
}

V StringArray(const std::vector<std::string>& Items)
{
	V Result = V::Array();
	for (const std::string& Item : Items) Result.Push(Str(Item));
	return Result;
}

void Record(V& S, const std::string& Type, const std::string& Title, const std::string& Detail)
{
	V Entry = V::Object();
	Entry.Set("quarter", S.Get("quarter"));
	Entry.Set("type", Str(Type));
	Entry.Set("title", Str(Title));
	Entry.Set("detail", Str(Detail));
	S["history"].Push(std::move(Entry));
}

std::string Join(const std::vector<std::string>& Parts, const std::string& Separator)
{
	std::string Out;
	for (std::size_t Index = 0; Index < Parts.size(); ++Index)
	{
		if (Index) Out += Separator;
		Out += Parts[Index];
	}
	return Out;
}

std::string Lower(std::string Text)
{
	for (char& C : Text)
	{
		if (C >= 'A' && C <= 'Z') C = static_cast<char>(C - 'A' + 'a');
	}
	return Text;
}

std::string ReplaceFirst(std::string Text, const std::string& Token, const std::string& With)
{
	const std::size_t At = Text.find(Token);
	if (At != std::string::npos) Text.replace(At, Token.size(), With);
	return Text;
}

bool IsInteger(const V& Value)
{
	return Value.IsNumber() && std::isfinite(Value.AsNumber()) && std::trunc(Value.AsNumber()) == Value.AsNumber();
}

// Math.imul and the 32-bit mixing used by the seeded generators.
unsigned int Imul(unsigned int A, unsigned int B) { return A * B; }

double Mix(unsigned int T)
{
	T = Imul(T ^ (T >> 15), T | 1u);
	T ^= T + Imul(T ^ (T >> 7), T | 61u);
	return static_cast<double>(T ^ (T >> 14)) / 4294967296.0;
}

// roll(seed, quarter)
double Roll(unsigned int Seed, double Quarter)
{
	return Mix(Seed ^ Imul(ToUint32(Quarter + 1), 0x9e3779b1u));
}

// stream(seed)
struct FStream
{
	explicit FStream(unsigned int Seed) : State(Seed) {}
	double Next()
	{
		State += 0x6d2b79f5u;
		return Mix(State);
	}
	unsigned int State;
};

// Objects whose members are numbers: Object.entries(...) as (key, value) pairs.
std::vector<std::pair<std::string, double>> NumberEntries(const V& Object)
{
	std::vector<std::pair<std::string, double>> Out;
	if (!Object.IsObject()) return Out;
	for (std::size_t Index = 0; Index < Object.Keys().size(); ++Index)
	{
		Out.emplace_back(Object.Keys()[Index], Object.Values()[Index].AsNumber());
	}
	return Out;
}

V SliceHistory(const V& Array, std::size_t Count)
{
	V Out = V::Array();
	for (std::size_t Index = 0; Index < Array.Size() && Index < Count; ++Index) Out.Push(Array[Index]);
	return Out;
}
} // namespace

struct Simulation::FVoter
{
	std::vector<std::string> Traits; // One value per electorate trait, in data order.
	double Lean = 0.0;
	std::vector<double> Draws;       // One draw per group.
};

struct Simulation::FPopulation
{
	std::vector<FVoter> Voters;
	std::vector<std::vector<double>> Affinity; // [group][voter]
	std::vector<double> Norms;
};

Simulation::Simulation() = default;
Simulation::~Simulation() = default;

bool Simulation::LoadData(const std::vector<std::pair<std::string, std::string>>& Files, std::vector<std::string>& Errors)
{
	bLoaded = false;
	Populations.clear();
	GameData = V::Object();
	for (const auto& File : Files)
	{
		V Parsed;
		std::string Error;
		if (!V::Parse(File.second, Parsed, Error))
		{
			Errors.push_back(File.first + ".json: " + Error);
			continue;
		}
		GameData.Set(File.first, std::move(Parsed));
	}
	for (const char* Required : {"metrics", "policies", "groups", "people", "campaign", "events", "situations", "electorate", "unrest", "executive"})
	{
		if (!GameData.Has(Required)) Errors.push_back(std::string("missing data file ") + Required + ".json");
	}
	if (!Errors.empty()) return false;
	// Each policy is a five-position lever. Effects are per quarter, after implementation catches up.
	PolicyList = V::Array();
	for (const V& Source : D("policies").Items())
	{
		V Lever = Source;
		if (!Lever.Has("lag") || Lever.Get("lag").IsNullish())
		{
			const double Cost = Lever.Get("cost").AsNumber();
			Lever.Set("lag", Num(Cost >= 8 ? 3 : Cost >= 5 ? 2 : 1));
		}
		PolicyList.Push(std::move(Lever));
	}
	if (!Validate(Errors)) return false;
	bLoaded = true;
	return true;
}

// validate(): hand-edited data fails loudly with every problem listed.
bool Simulation::Validate(std::vector<std::string>& Errors) const
{
	const V& Metrics = D("metrics").Get("labels");
	const auto Metric = [&](const std::string& K) { return Metrics.Has(K); };
	const auto MetricOrDebt = [&](const std::string& K) { return Metric(K) || K == "debt"; };
	std::set<std::string> PolicyIds, GroupIds, Names, EventIds;
	std::set<double> Fixed;
	for (const V& P : PolicyList.Items()) PolicyIds.insert(P.Get("id").AsString());
	for (const V& G : D("groups").Items()) GroupIds.insert(G.Get("id").AsString());
	for (const V& Sit : D("situations").Items()) Names.insert(Sit.Get("name").AsString());
	const auto KeysOk = [&](const std::string& Where, const V& Object, const auto& Ok) {
		if (!Object.IsObject()) return;
		for (const std::string& K : Object.Keys())
		{
			if (!Ok(K)) Errors.push_back(Where + ": unknown key \"" + K + "\"");
		}
	};
	const auto IdsOk = [&](const std::string& Where, const V& List, const std::set<std::string>& Known) {
		for (const V& Id : List.Items())
		{
			if (!Known.count(Id.AsString())) Errors.push_back(Where + ": unknown id \"" + Id.AsString() + "\"");
		}
	};
	for (const V& P : PolicyList.Items()) KeysOk("policy " + P.Get("id").AsString(), P.Get("effects"), Metric);
	const V& ElectorateData = D("electorate");
	if (!(IsInteger(ElectorateData.Get("voters")) && ElectorateData.Get("voters").AsNumber() > 0)) Errors.push_back("electorate: voters must be a positive whole number");
	if (!ElectorateData.Get("bar").IsNumber() || !ElectorateData.Get("lean").IsNumber()) Errors.push_back("electorate: bar and lean must be numbers");
	const V& Traits = ElectorateData.Get("traits");
	for (std::size_t T = 0; T < Traits.Keys().size(); ++T)
	{
		double Sum = 0;
		for (const V& Share : Traits.Values()[T].Values()) Sum += Share.AsNumber();
		if (std::fabs(Sum - 1) > .001) Errors.push_back("electorate trait " + Traits.Keys()[T] + ": shares must add up to 1");
	}
	for (const V& G : D("groups").Items())
	{
		const std::string Id = G.Get("id").AsString();
		KeysOk("group " + Id + " priorities", G.Get("priorities"), Metric);
		IdsOk("group " + Id, G.Get("fav"), PolicyIds);
		IdsOk("group " + Id, G.Get("opp"), PolicyIds);
		KeysOk("group " + Id + " drivers", G.Get("drivers"), Metric);
		KeysOk("group " + Id + " disruption", G.Get("disruption").Get("effects"), Metric);
		const double Share = G.Get("share").AsNumber();
		if (!(Share > 0 && Share <= 100)) Errors.push_back("group " + Id + ": share must be a percentage of voters above 0");
		const V& GroupTraits = G.Get("traits");
		for (std::size_t T = 0; T < GroupTraits.Keys().size(); ++T)
		{
			const std::string& K = GroupTraits.Keys()[T];
			if (!Traits.Has(K)) Errors.push_back("group " + Id + ": unknown trait \"" + K + "\"");
			else KeysOk("group " + Id + " trait " + K, GroupTraits.Values()[T], [&](const std::string& Value) { return Traits.Get(K).Has(Value); });
		}
	}
	const V& Stages = D("unrest").Get("stages");
	for (std::size_t Index = 1; Index < Stages.Size(); ++Index)
	{
		if (Stages[Index].Get("at").AsNumber() <= Stages[Index - 1].Get("at").AsNumber())
		{
			Errors.push_back("unrest: stages must be in rising order");
			break;
		}
	}
	for (const V& Stage : Stages.Items()) KeysOk("unrest stage " + Stage.Get("name").AsString(), Stage.Get("effects"), Metric);
	const V& Attempt = D("unrest").Get("attempt");
	IdsOk("unrest attempt security", StringArray(Attempt.Get("security").Keys()), PolicyIds);
	const V& Outcomes = Attempt.Get("outcomes");
	for (std::size_t Index = 0; Index < Outcomes.Keys().size(); ++Index) KeysOk("unrest outcome " + Outcomes.Keys()[Index], Outcomes.Values()[Index].Get("effects"), Metric);
	for (const V& P : D("people").Items())
	{
		IdsOk("person " + P.Get("id").AsString() + " policies", P.Get("policies"), PolicyIds);
		V GroupList = V::Array();
		GroupList.Push(P.Get("group"));
		IdsOk("person " + P.Get("id").AsString() + " group", GroupList, GroupIds);
	}
	const V& Issues = D("campaign").Get("issues");
	for (const V& R : D("campaign").Get("regions").Items())
	{
		IdsOk("region " + R.Get("id").AsString() + " groups", R.Get("groups"), GroupIds);
		for (const V& Issue : R.Get("issues").Items())
		{
			if (!Issues.Has(Issue.AsString())) Errors.push_back("region " + R.Get("id").AsString() + " issues: unknown key \"" + Issue.AsString() + "\"");
		}
	}
	for (const V& Sit : D("situations").Items())
	{
		const std::string Name = Sit.Get("name").AsString();
		if (!MetricOrDebt(Sit.Get("metric").AsString())) Errors.push_back("situation " + Name + ": unknown metric \"" + Sit.Get("metric").AsString() + "\"");
		const bool HasBelow = Sit.Has("below"), HasAbove = Sit.Has("above");
		if (HasBelow == HasAbove) Errors.push_back("situation " + Name + ": set exactly one of \"below\" or \"above\"");
		else if (!Sit.Get("clearsAt").IsNumber() || (HasBelow ? Sit.Get("clearsAt").AsNumber() < Sit.Get("below").AsNumber() : Sit.Get("clearsAt").AsNumber() > Sit.Get("above").AsNumber()))
			Errors.push_back("situation " + Name + ": clearsAt must sit on the recovery side of the trigger");
		KeysOk("situation " + Name + " effects", Sit.Get("effects"), Metric);
		KeysOk("situation " + Name + " groups", Sit.Get("groups"), [&](const std::string& K) { return GroupIds.count(K) > 0; });
	}
	for (const V& E : D("events").Items())
	{
		const std::string Id = E.Get("id").AsString();
		if (EventIds.count(Id)) Errors.push_back("event " + Id + ": duplicate id");
		EventIds.insert(Id);
		if (E.Has("fixed"))
		{
			const double Q = E.Get("fixed").AsNumber();
			if (!IsInteger(E.Get("fixed")) || Q < 0 || Q > 15 || Fixed.count(Q)) Errors.push_back("event " + Id + ": fixed must be a unique quarter from 0 to 15");
			Fixed.insert(Q);
		}
		const V& W = E.Get("when");
		KeysOk("event " + Id + " when.below", W.Get("below"), MetricOrDebt);
		KeysOk("event " + Id + " when.above", W.Get("above"), MetricOrDebt);
		for (const char* ListKey : {"situations", "absent"})
		{
			for (const V& Name : W.Get(ListKey).Items())
			{
				if (!Names.count(Name.AsString())) Errors.push_back("event " + Id + ": unknown situation \"" + Name.AsString() + "\"");
			}
		}
		if (W.Has("unrest") && !W.Get("unrest").IsNumber()) Errors.push_back("event " + Id + ": when.unrest must be a number");
		if (!E.Get("choices").Size()) Errors.push_back("event " + Id + ": needs at least one choice");
		for (const V& C : E.Get("choices").Items())
		{
			const std::string Label = C.Get("label").AsString();
			KeysOk("event " + Id + " choice \"" + Label + "\"", C.Get("effects"), Metric);
			if (!C.Get("cost").IsNumber()) Errors.push_back("event " + Id + " choice \"" + Label + "\": cost must be a number");
			if (C.Has("calm") && !C.Get("calm").IsNumber()) Errors.push_back("event " + Id + " choice \"" + Label + "\": calm must be a number");
		}
	}
	if (!Errors.empty()) Errors.insert(Errors.begin(), "Four Years data errors:");
	return Errors.empty();
}

const V* Simulation::Policy(const std::string& Id) const { return FindById(PolicyList, Id); }
const V* Simulation::Group(const std::string& Id) const { return FindById(D("groups"), Id); }
const V* Simulation::Bill(const std::string& Id) const { return FindById(X("bills"), Id); }
const V* Simulation::Team(const std::string& Id) const { return FindById(X("team"), Id); }

const V* Simulation::Situation(const std::string& Name) const
{
	for (const V& Sit : D("situations").Items())
	{
		if (Sit.Get("name").AsString() == Name) return &Sit;
	}
	return nullptr;
}

std::vector<const V*> Simulation::ActiveSituations(const V& S) const
{
	std::vector<const V*> Out;
	for (const V& Name : S.Get("situations").Items())
	{
		if (const V* Sit = Situation(Name.AsString())) Out.push_back(Sit);
	}
	return Out;
}

double Simulation::MetricValue(const V& S, const std::string& Key) const
{
	return Key == "debt" ? S.Get("debt").AsNumber() : S.Get("metrics").Get(Key).AsNumber();
}

// updateSituations(s): a situation starts past its trigger and only clears once the condition recovers past clearsAt.
std::vector<std::string> Simulation::UpdateSituations(const V& S) const
{
	std::vector<std::string> Out;
	const V& Before = S.Get("situations");
	for (const V& Sit : D("situations").Items())
	{
		const double Value = MetricValue(S, Sit.Get("metric").AsString());
		const bool On = Contains(Before, Sit.Get("name").AsString());
		const bool Active = Sit.Has("below") ? Value < (On ? Sit.Get("clearsAt").AsNumber() : Sit.Get("below").AsNumber())
		                                     : Value > (On ? Sit.Get("clearsAt").AsNumber() : Sit.Get("above").AsNumber());
		if (Active) Out.push_back(Sit.Get("name").AsString());
	}
	return Out;
}

// eventEligible(s, e)
bool Simulation::EventEligible(const V& S, const V& Event) const
{
	const V& W = Event.Get("when");
	const double Q = S.Get("quarter").AsNumber();
	if ((W.Has("from") && Q < W.Get("from").AsNumber()) || (W.Has("until") && Q > W.Get("until").AsNumber())) return false;
	for (const auto& Entry : NumberEntries(W.Get("below")))
	{
		if (!(MetricValue(S, Entry.first) < Entry.second)) return false;
	}
	for (const auto& Entry : NumberEntries(W.Get("above")))
	{
		if (!(MetricValue(S, Entry.first) > Entry.second)) return false;
	}
	if (W.Has("unrest"))
	{
		bool Any = false;
		for (const V& G : D("groups").Items())
		{
			if (S.Get("unrest").Get("anger").Get(G.Get("id").AsString()).NumberOr(0) >= W.Get("unrest").AsNumber()) Any = true;
		}
		if (!Any) return false;
	}
	for (const V& Name : W.Get("situations").Items())
	{
		if (!Contains(S.Get("situations"), Name.AsString())) return false;
	}
	for (const V& Name : W.Get("absent").Items())
	{
		if (Contains(S.Get("situations"), Name.AsString())) return false;
	}
	return true;
}

// drawEvent(s): fixed events keep their quarter; the rest are drawn by weight from eligible events, seeded per save.
const V* Simulation::DrawEvent(V& S) const
{
	const V& Events = D("events");
	std::set<std::string> Used;
	for (const V& Id : S.Get("deck").Get("used").Items()) Used.insert(Id.AsString());
	std::vector<const V*> Open, Flexible;
	for (const V& E : Events.Items())
	{
		if (Used.count(E.Get("id").AsString())) continue;
		Open.push_back(&E);
		if (!E.Has("fixed")) Flexible.push_back(&E);
	}
	const V* Event = nullptr;
	for (const V* E : Open)
	{
		if (E->Has("fixed") && E->Get("fixed").AsNumber() == S.Get("quarter").AsNumber())
		{
			Event = E;
			break;
		}
	}
	if (!Event)
	{
		std::vector<const V*> Eligible;
		for (const V* E : Flexible)
		{
			if (EventEligible(S, *E)) Eligible.push_back(E);
		}
		std::vector<const V*> Pool = !Eligible.empty() ? Eligible : Flexible;
		if (Pool.empty())
		{
			for (const V& E : Events.Items())
			{
				if (!E.Has("fixed")) Pool.push_back(&E);
			}
		}
		const auto Weight = [](const V* E) { return E->Has("weight") && !E->Get("weight").IsNullish() ? E->Get("weight").AsNumber() : 1.0; };
		double Total = 0;
		for (const V* E : Pool) Total += Weight(E);
		double R = Roll(ToUint32(S.Get("seed").AsNumber()), S.Get("quarter").AsNumber()) * Total;
		for (const V* E : Pool)
		{
			if ((R -= Weight(E)) < 0)
			{
				Event = E;
				break;
			}
		}
		if (!Event) Event = Pool.back();
	}
	V& Deck = S["deck"];
	Deck.Set("current", Event->Get("id"));
	if (!Used.count(Event->Get("id").AsString())) Deck["used"].Push(Event->Get("id"));
	return Event;
}

const V* Simulation::CurrentEvent(const V& S) const
{
	const V& Current = S.Get("deck").Get("current");
	if (!Current.IsString()) return nullptr;
	return FindById(D("events"), Current.AsString());
}

// legacyDeck(s): saves from before the deck continue with the event they were already shown.
V Simulation::LegacyDeck(const V& S) const
{
	V Deck = V::Object();
	V Used = V::Array();
	V Current = V::Null();
	const double Q = S.Get("quarter").AsNumber();
	for (const V& E : D("events").Items())
	{
		if (E.Has("legacyQuarter") && E.Get("legacyQuarter").AsNumber() <= Q) Used.Push(E.Get("id"));
	}
	if (!S.Get("ended").Truthy())
	{
		for (const V& E : D("events").Items())
		{
			if (E.Has("legacyQuarter") && E.Get("legacyQuarter").AsNumber() == Q)
			{
				Current = E.Get("id");
				break;
			}
		}
	}
	Deck.Set("used", std::move(Used));
	Deck.Set("current", std::move(Current));
	return Deck;
}

// population(seed): a seeded population of individual voters whose group memberships overlap.
const Simulation::FPopulation& Simulation::Population(double Seed) const
{
	const unsigned int Key = ToUint32(Seed);
	auto Found = Populations.find(Key);
	if (Found != Populations.end()) return *Found->second;
	std::unique_ptr<FPopulation> Pop(new FPopulation());
	FStream Stream(Key ^ 0x5bd1e995u);
	const V& ElectorateData = D("electorate");
	const V& Traits = ElectorateData.Get("traits");
	const V& GroupList = D("groups");
	const std::size_t Count = static_cast<std::size_t>(ElectorateData.Get("voters").AsNumber());
	Pop->Voters.resize(Count);
	for (FVoter& Voter : Pop->Voters)
	{
		for (const V& Distribution : Traits.Values())
		{
			double Draw = Stream.Next();
			std::string Chosen = Distribution.Keys().empty() ? std::string() : Distribution.Keys().back();
			for (std::size_t Index = 0; Index < Distribution.Keys().size(); ++Index)
			{
				if ((Draw -= Distribution.Values()[Index].AsNumber()) < 0)
				{
					Chosen = Distribution.Keys()[Index];
					break;
				}
			}
			Voter.Traits.push_back(Chosen);
		}
		Voter.Lean = (Stream.Next() * 2 - 1) * ElectorateData.Get("lean").AsNumber();
		for (std::size_t G = 0; G < GroupList.Size(); ++G) Voter.Draws.push_back(Stream.Next());
	}
	// Normalize trait affinity so each group's average membership matches its data share.
	for (const V& G : GroupList.Items())
	{
		std::vector<double> Affinity;
		double Sum = 0;
		const V& GroupTraits = G.Get("traits");
		for (const FVoter& Voter : Pop->Voters)
		{
			double M = 1;
			for (std::size_t T = 0; T < GroupTraits.Keys().size(); ++T)
			{
				std::size_t TraitIndex = 0;
				while (TraitIndex < Traits.Keys().size() && Traits.Keys()[TraitIndex] != GroupTraits.Keys()[T]) ++TraitIndex;
				const V& Multiplier = TraitIndex < Voter.Traits.size() ? GroupTraits.Values()[T].Get(Voter.Traits[TraitIndex]) : V();
				M = M * (Multiplier.IsNullish() ? 1.0 : Multiplier.AsNumber());
			}
			Affinity.push_back(M);
			Sum += M;
		}
		const double Mean = Sum / static_cast<double>(Affinity.size());
		Pop->Norms.push_back(Mean != 0 && !std::isnan(Mean) ? Mean : 1.0);
		Pop->Affinity.push_back(std::move(Affinity));
	}
	const FPopulation& Result = *Pop;
	Populations[Key] = std::move(Pop);
	return Result;
}

// membershipFactor(s, g): membership drifts with conditions.
double Simulation::MembershipFactor(const V& S, const V& G) const
{
	double Sum = 0;
	for (const auto& Entry : NumberEntries(G.Get("drivers"))) Sum = Sum + Entry.second * (S.Get("metrics").Get(Entry.first).AsNumber() - 50) / 50;
	return Bound(1 + Sum, .3, 2);
}

// groupMoods(s)
V Simulation::GroupMoods(const V& S) const
{
	V Out = V::Array();
	const std::vector<const V*> Active = ActiveSituations(S);
	for (const V& G : D("groups").Items())
	{
		const std::string Id = G.Get("id").AsString();
		double Weighted = 0, Total = 0;
		for (const auto& Entry : NumberEntries(G.Get("priorities")))
		{
			Weighted += (S.Get("metrics").Get(Entry.first).AsNumber() - 50) * Entry.second;
			Total += Entry.second;
		}
		double Stance = 0;
		for (const V& P : G.Get("fav").Items()) Stance += (S.Get("levels").Get(P.AsString()).AsNumber() - 2) * 1.8;
		for (const V& P : G.Get("opp").Items()) Stance -= (S.Get("levels").Get(P.AsString()).AsNumber() - 2) * 1.8;
		double SituationSum = 0;
		for (const V* Sit : Active) SituationSum = SituationSum + Sit->Get("groups").Get(Id).NumberOr(0);
		const double Approval = Bound(JsRound(51 + Weighted / std::max(1.0, Total) * .85 + Stance + S.Get("goodwill").Get(Id).NumberOr(0) + S.Get("campaign").Get("boosts").Get(Id).NumberOr(0) + SituationSum + Modifier(S, Id)), 5, 95);
		V Mood = G;
		Mood.Set("approval", Num(Approval));
		Mood.Set("turnout", Num(Bound(JsRound(58 + std::fabs(Approval - 50) * .3 + G.Get("turnout").NumberOr(0)), 40, 85)));
		Out.Push(std::move(Mood));
	}
	return Out;
}

// electorate(s): each voter weighs the groups they belong to; the poll is the turnout-weighted share who approve.
V Simulation::Electorate(const V& S) const
{
	const FPopulation& Pop = Population(S.Get("seed").AsNumber());
	V Moods = GroupMoods(S);
	const V& GroupList = D("groups");
	const std::size_t N = GroupList.Size(), Count = Pop.Voters.size();
	std::vector<double> Members(N, 0), Supporters(N, 0), Scale(N, 0), Approval(N, 0), Turnout(N, 0);
	double ApprovalShare = 0, ShareTotal = 0;
	for (std::size_t J = 0; J < N; ++J)
	{
		ApprovalShare = ApprovalShare + Moods[J].Get("approval").AsNumber() * Moods[J].Get("share").AsNumber();
		ShareTotal = ShareTotal + Moods[J].Get("share").AsNumber();
	}
	const double National = ApprovalShare / ShareTotal;
	for (std::size_t J = 0; J < N; ++J)
	{
		Scale[J] = GroupList[J].Get("share").AsNumber() / 100 / Pop.Norms[J] * MembershipFactor(S, GroupList[J]);
		Approval[J] = Moods[J].Get("approval").AsNumber();
		Turnout[J] = Moods[J].Get("turnout").AsNumber();
	}
	const double Bar = D("electorate").Get("bar").AsNumber();
	std::vector<std::size_t> Joined(N);
	double ApprovingTurnout = 0, TurnoutSum = 0, Memberships = 0;
	for (std::size_t I = 0; I < Count; ++I)
	{
		const FVoter& Voter = Pop.Voters[I];
		std::size_t K = 0;
		double Score = 0, Turn = 0;
		for (std::size_t J = 0; J < N; ++J)
		{
			if (Voter.Draws[J] < Scale[J] * Pop.Affinity[J][I])
			{
				Joined[K++] = J;
				Score += Approval[J];
				Turn += Turnout[J];
			}
		}
		Score = K ? Score / static_cast<double>(K) : National;
		Turn = K ? Turn / static_cast<double>(K) : 55;
		const bool bApprove = Score + Voter.Lean >= Bar;
		for (std::size_t Index = 0; Index < K; ++Index)
		{
			Members[Joined[Index]]++;
			if (bApprove) Supporters[Joined[Index]]++;
		}
		Memberships += static_cast<double>(K);
		TurnoutSum += Turn;
		if (bApprove) ApprovingTurnout += Turn;
	}
	V Result = V::Object();
	Result.Set("poll", Num(JsRound(ApprovingTurnout / TurnoutSum * 100)));
	Result.Set("overlap", Num(JsRound(Memberships / static_cast<double>(Count) * 10) / 10));
	V Out = V::Array();
	for (std::size_t J = 0; J < N; ++J)
	{
		V G = Moods[J];
		const double Anger = S.Get("unrest").Get("anger").Get(G.Get("id").AsString()).NumberOr(0);
		G.Set("share", Num(JsRound(Members[J] / static_cast<double>(Count) * 100)));
		G.Set("support", Num(Members[J] ? JsRound(Supporters[J] / Members[J] * 100) : 0));
		G.Set("anger", Num(JsRound(Anger)));
		const int Stage = UnrestStageIndex(Anger);
		G.Set("stage", Stage >= 0 ? D("unrest").Get("stages")[static_cast<std::size_t>(Stage)].Get("name") : V::Null());
		Out.Push(std::move(G));
	}
	Result.Set("groups", std::move(Out));
	return Result;
}

double Simulation::Poll(const V& S) const { return Electorate(S).Get("poll").AsNumber(); }
V Simulation::Groups(const V& S) const { return Electorate(S).Get("groups"); }

// unrestStage(anger): the highest stage reached, or -1.
int Simulation::UnrestStageIndex(double Anger) const
{
	int Stage = -1;
	const V& Stages = D("unrest").Get("stages");
	for (std::size_t Index = 0; Index < Stages.Size(); ++Index)
	{
		if (Anger >= Stages[Index].Get("at").AsNumber()) Stage = static_cast<int>(Index);
	}
	return Stage;
}

// unrestPressure(s)
void Simulation::UnrestPressure(const V& S, V& Effects, double& Revenue) const
{
	Effects = V::Object();
	Revenue = 0;
	for (const V& G : D("groups").Items())
	{
		const int StageIndex = UnrestStageIndex(S.Get("unrest").Get("anger").Get(G.Get("id").AsString()).NumberOr(0));
		if (StageIndex < 0) continue;
		const V& Stage = D("unrest").Get("stages")[static_cast<std::size_t>(StageIndex)];
		const double K = Bound(G.Get("share").AsNumber() / 20, .3, 1.5);
		const auto Add = [&](const V& Fx) {
			for (const auto& Entry : NumberEntries(Fx)) Effects.Set(Entry.first, Num(Effects.Get(Entry.first).NumberOr(0) + Entry.second * K));
		};
		Add(Stage.Get("effects"));
		if (Stage.Get("disruption").Truthy())
		{
			Add(G.Get("disruption").Get("effects"));
			Revenue += G.Get("disruption").Get("revenue").NumberOr(0) * K;
		}
	}
}

// unrest(s): groups with an active unrest stage, angriest first.
V Simulation::Unrest(const V& S) const
{
	std::vector<V> Active;
	const V Current = Groups(S);
	for (const V& G : Current.Items())
	{
		if (G.Get("stage").Truthy()) Active.push_back(G);
	}
	std::stable_sort(Active.begin(), Active.end(), [](const V& A, const V& B) { return B.Get("anger").AsNumber() < A.Get("anger").AsNumber(); });
	V Out = V::Array();
	for (V& G : Active) Out.Push(std::move(G));
	return Out;
}

// ----- State lifecycle -----

V Simulation::PoliticalDefaults() const
{
	V Defaults = V::Object();
	V Relationships = V::Object();
	for (const V& P : D("people").Items()) Relationships.Set(P.Get("id").AsString(), Num(50));
	Defaults.Set("relationships", std::move(Relationships));
	Defaults.Set("appointments", V::Array());
	Defaults.Set("promises", V::Array());
	Defaults.Set("goodwill", V::Object());
	V Campaign = V::Object();
	Campaign.Set("boosts", V::Object());
	Campaign.Set("trips", V::Array());
	Defaults.Set("campaign", std::move(Campaign));
	return Defaults;
}

// ExecutiveSim.initialize(s)
void Simulation::Initialize(V& S) const
{
	if (S.Get("executive").Truthy()) return;
	V Regions = V::Object();
	for (const V& R : X("regions").Items())
	{
		V Region = V::Object();
		const V& Offset = R.Get("offset");
		for (std::size_t Index = 0; Index < Offset.Keys().size(); ++Index)
		{
			const std::string& K = Offset.Keys()[Index];
			Region.Set(K, Num(Bound(S.Get("metrics").Get(K).AsNumber() + Offset.Values()[Index].AsNumber())));
		}
		Region.Set("goodwill", Num(0));
		Region.Set("news", V::Array());
		Region.Set("visited", Num(-1));
		Regions.Set(R.Get("id").AsString(), std::move(Region));
	}
	V Bills = V::Array();
	for (const V& B : X("bills").Items())
	{
		if (S.Get("levels").Get(B.Get("policy").AsString()).AsNumber() != 4) continue;
		V Inherited = V::Object();
		Inherited.Set("id", B.Get("id"));
		Inherited.Set("status", Str("passed"));
		Inherited.Set("amendments", V::Array());
		Inherited.Set("lobbied", V::Array());
		Inherited.Set("introduced", S.Get("quarter"));
		Inherited.Set("passed", S.Get("quarter"));
		Inherited.Set("lastVote", Num(-1));
		Inherited.Set("attempts", Num(0));
		Inherited.Set("progress", Num(Bound(JsRound((S.Get("implemented").Get(B.Get("policy").AsString()).NumberOr(2) - 2) * 50))));
		Inherited.Set("inherited", Bool(true));
		Bills.Push(std::move(Inherited));
	}
	V Cabinet = V::Object();
	for (const V& T : X("team").Items())
	{
		V Member = V::Object();
		Member.Set("competence", T.Get("competence"));
		Member.Set("status", Str("serving"));
		Member.Set("strain", Num(0));
		Member.Set("acting", Bool(false));
		Member.Set("lastAction", Num(-1));
		Cabinet.Set(T.Get("id").AsString(), std::move(Member));
	}
	V Executive = V::Object();
	Executive.Set("regions", std::move(Regions));
	Executive.Set("bills", std::move(Bills));
	Executive.Set("platform", V::Null());
	Executive.Set("cabinet", std::move(Cabinet));
	Executive.Set("opposition", X("opposition"));
	Executive.Set("crises", V::Array());
	Executive.Set("encounters", V::Array());
	Executive.Set("started", S.Get("quarter"));
	Executive.Set("mandate", Num(0));
	S.Set("executive", std::move(Executive));
}

// upgrade(s)
void Simulation::Upgrade(V& S) const
{
	V Defaults = PoliticalDefaults();
	for (std::size_t Index = 0; Index < Defaults.Keys().size(); ++Index)
	{
		if (S.Get(Defaults.Keys()[Index]).IsUndefined()) S.Set(Defaults.Keys()[Index], Defaults.Values()[Index]);
	}
	if (!IsInteger(S.Get("seed"))) S.Set("seed", Num(6137));
	if (!S.Get("situations").Truthy()) S.Set("situations", V::Array());
	if (!S.Get("deck").Truthy()) S.Set("deck", LegacyDeck(S));
	if (!S.Get("unrest").Truthy())
	{
		V UnrestState = V::Object();
		UnrestState.Set("anger", V::Object());
		UnrestState.Set("incidents", V::Array());
		S.Set("unrest", std::move(UnrestState));
	}
	S.Set("version", Num(6));
	Initialize(S);
}

// fresh(seed)
V Simulation::Fresh(double Seed) const
{
	V Levels = V::Object();
	for (const V& P : PolicyList.Items()) Levels.Set(P.Get("id").AsString(), Num(2));
	V S = V::Object();
	S.Set("version", Num(6));
	S.Set("quarter", Num(0));
	S.Set("location", Str("oval"));
	S.Set("levels", Levels);
	S.Set("implemented", Levels);
	S.Set("metrics", D("metrics").Get("base"));
	S.Set("capital", Num(12));
	S.Set("debt", Num(72));
	S.Set("budget", V::Null());
	S.Set("agendaChoice", V::Null());
	S.Set("history", V::Array());
	S.Set("effects", V::Array());
	S.Set("situations", V::Array());
	S.Set("polls", V::Array());
	S.Set("midterm", V::Null());
	S.Set("ended", Bool(false));
	S.Set("legacy", V::Null());
	S.Set("seed", Num(Seed));
	V Deck = V::Object();
	Deck.Set("used", V::Array());
	Deck.Set("current", V::Null());
	S.Set("deck", std::move(Deck));
	V UnrestState = V::Object();
	UnrestState.Set("anger", V::Object());
	UnrestState.Set("incidents", V::Array());
	S.Set("unrest", std::move(UnrestState));
	V Defaults = PoliticalDefaults();
	for (std::size_t Index = 0; Index < Defaults.Keys().size(); ++Index) S.Set(Defaults.Keys()[Index], Defaults.Values()[Index]);
	Initialize(S);
	DrawEvent(S);
	return S;
}

// migrate(old)
V Simulation::Migrate(V Old) const
{
	if (!Old.IsObject()) return Fresh();
	if (ContainsNumber({2, 3, 4, 5, 6}, Old.Get("version").AsNumber()))
	{
		Upgrade(Old);
		if (!Old.Get("ended").Truthy() && !CurrentEvent(Old))
		{
			Old.Set("agendaChoice", V::Null());
			DrawEvent(Old);
		}
		return Old;
	}
	// Month-based saves from the first prototype.
	V S = Fresh();
	S.Set("location", Str(Old.Get("location").AsString() == "aircraft" ? "aircraft" : "oval"));
	S.Set("quarter", Num(Bound(std::floor(Old.Get("month").NumberOr(0) / 3), 0, 15)));
	const auto NumberFrom = [](const V& Value) {
		if (Value.IsNumber()) return Value.AsNumber();
		if (Value.IsBool()) return Value.AsBool() ? 1.0 : 0.0;
		if (Value.IsNull()) return 0.0;
		return std::nan("");
	};
	const auto Or = [](double Value, double Fallback) { return Value != 0 && !std::isnan(Value) ? Value : Fallback; };
	S["metrics"].Set("trust", Num(Bound(Or(NumberFrom(Old.Get("trust")), 53))));
	S["metrics"].Set("growth", Num(Bound(Or(NumberFrom(Old.Get("fiscal")), 50))));
	S.Set("capital", Num(Bound(JsRound(Or(NumberFrom(Old.Get("capital")), 7) * 1.5), 0, 20)));
	S.Set("deck", LegacyDeck(S));
	V LegacyRecord = V::Object();
	LegacyRecord.Set("months", Num(Old.Get("month").NumberOr(0)));
	LegacyRecord.Set("history", Old.Get("history").Truthy() ? Old.Get("history") : V::Array());
	LegacyRecord.Set("trust", Old.Get("trust"));
	LegacyRecord.Set("fiscal", Old.Get("fiscal"));
	S.Set("legacy", std::move(LegacyRecord));
	V Entry = V::Object();
	Entry.Set("quarter", S.Get("quarter"));
	Entry.Set("type", Str("legacy"));
	Entry.Set("title", Str("First-year record preserved"));
	Entry.Set("detail", Str(Fmt(S.Get("legacy").Get("months").AsNumber()) + " monthly briefings from the earlier prototype are archived below."));
	S["history"].Push(std::move(Entry));
	return S;
}

// ----- Executive systems -----

bool Simulation::Allowed(const V& S) const { return !S.Get("ended").Truthy() && S.Get("agendaChoice").IsNull(); }

int Simulation::Slots(const V& S) const
{
	int Used = 0;
	for (const V& A : S.Get("appointments").Items())
	{
		if (A.Get("quarter").AsNumber() == S.Get("quarter").AsNumber()) ++Used;
	}
	return std::max(0, 2 - Used);
}

int Simulation::AvailableSlots(const V& S) const { return Slots(S); }

bool Simulation::CampaignSeason(const V& S) const
{
	return !S.Get("ended").Truthy() && ContainsNumber({6, 7, 14, 15}, S.Get("quarter").AsNumber());
}

const V* Simulation::ActiveBill(const V& S) const
{
	for (const V& B : S.Get("executive").Get("bills").Items())
	{
		const std::string& Status = B.Get("status").AsString();
		if (Status == "draft" || Status == "failed") return &B;
	}
	return nullptr;
}

namespace
{
void News(V& S, const std::string& RegionId, const std::string& Text)
{
	V& Region = S["executive"]["regions"][RegionId];
	V Entry = V::Object();
	Entry.Set("quarter", S.Get("quarter"));
	Entry.Set("text", Str(Text));
	V& Feed = Region["news"];
	Feed.Insert(0, std::move(Entry));
	Region.Set("news", SliceHistory(Region.Get("news"), 8));
}

V* MutableActiveBill(V& S)
{
	for (V& B : S["executive"]["bills"].Items())
	{
		const std::string& Status = B.Get("status").AsString();
		if (Status == "draft" || Status == "failed") return &B;
	}
	return nullptr;
}

void PushAppointment(V& S, const char* Type)
{
	V Appointment = V::Object();
	Appointment.Set("quarter", S.Get("quarter"));
	Appointment.Set("type", Str(Type));
	S["appointments"].Push(std::move(Appointment));
}
} // namespace

// platform(s, ids, signature)
V Simulation::SetPlatform(V& S, const std::vector<std::string>& GoalIds, const std::string& Signature) const
{
	if (!Allowed(S) || S.Get("executive").Get("platform").Truthy()) return Fail("Your platform has already been set, or this quarter is closed.");
	bool Unknown = false;
	for (const std::string& Id : GoalIds)
	{
		if (!FindById(X("goals"), Id)) Unknown = true;
	}
	if (std::set<std::string>(GoalIds.begin(), GoalIds.end()).size() != 3 || Unknown || !Bill(Signature)) return Fail("Choose three distinct promises and one signature project.");
	V Platform = V::Object();
	Platform.Set("goals", StringArray(GoalIds));
	Platform.Set("signature", Str(Signature));
	Platform.Set("chosen", S.Get("quarter"));
	S["executive"].Set("platform", std::move(Platform));
	std::vector<std::string> Names;
	for (const std::string& Id : GoalIds) Names.push_back(FindById(X("goals"), Id)->Get("name").AsString());
	Record(S, "mandate", "Your public mandate", Join(Names, " · "));
	return Ok("Your mandate is on the record. Track it throughout your term.");
}

// propose(s, id)
V Simulation::Propose(V& S, const std::string& BillId) const
{
	if (!Allowed(S)) return Fail("File legislation before closing the quarter.");
	bool Introduced = false;
	for (const V& B : S.Get("executive").Get("bills").Items())
	{
		if (B.Get("id").AsString() == BillId) Introduced = true;
	}
	if (!Bill(BillId) || ActiveBill(S) || Introduced) return Fail("Finish your active bill before proposing another; each act can be introduced once.");
	if (S.Get("capital").AsNumber() < 2) return Fail("Introducing legislation costs 2 political capital.");
	S.Set("capital", Num(S.Get("capital").AsNumber() - 2));
	V NewBill = V::Object();
	NewBill.Set("id", Str(BillId));
	NewBill.Set("status", Str("draft"));
	NewBill.Set("amendments", V::Array());
	NewBill.Set("lobbied", V::Array());
	NewBill.Set("introduced", S.Get("quarter"));
	NewBill.Set("lastVote", Num(-1));
	NewBill.Set("attempts", Num(0));
	NewBill.Set("progress", Num(0));
	S["executive"]["bills"].Push(std::move(NewBill));
	Record(S, "legislation", "Bill introduced", Bill(BillId)->Get("name").AsString());
	return Ok("Bill introduced. Negotiate amendments and inspect the whip count before calling a vote.");
}

// votes(s, bill)
V Simulation::Votes(const V& S, const V& BillState) const
{
	const V& Def = *Bill(BillState.Get("id").AsString());
	const std::string Sponsor = Def.Get("sponsor").AsString();
	const double Bond = S.Get("relationships").Get(Sponsor).AsNumber();
	const V& TeamState = S.Get("executive").Get("cabinet").Get(Sponsor);
	V Out = V::Array();
	for (const V& Bloc : X("blocs").Items())
	{
		const std::string Id = Bloc.Get("id").AsString();
		double Support = Bloc.Get("base").AsNumber() + (Bond - 50) * .25 + S.Get("executive").Get("mandate").AsNumber() - S.Get("executive").Get("opposition").Get("momentum").AsNumber() * .15 + (TeamState.Get("status").AsString() == "resigned" ? -10 : 0);
		if (Contains(BillState.Get("lobbied"), Id)) Support += 15;
		for (const V& A : X("amendments").Items())
		{
			if (Contains(BillState.Get("amendments"), A.Get("id").AsString())) Support += A.Get("support").Get(Id).NumberOr(0);
		}
		double Goodwill = 0;
		for (const V& Region : S.Get("executive").Get("regions").Values()) Goodwill = Goodwill + Region.Get("goodwill").AsNumber();
		const double RegionalBoost = Goodwill / 4;
		if (Id == "regional") Support += RegionalBoost;
		Support = Bound(Support);
		V Row = Bloc;
		Row.Set("support", Num(JsRound(Support)));
		Row.Set("yes", Num(JsRound(Bloc.Get("seats").AsNumber() * Support / 100)));
		Out.Push(std::move(Row));
	}
	return Out;
}

// amend(s, id)
V Simulation::Amend(V& S, const std::string& AmendmentId) const
{
	V* B = MutableActiveBill(S);
	const V* Def = FindById(X("amendments"), AmendmentId);
	if (!Allowed(S) || !B || !Def || Contains(B->Get("amendments"), AmendmentId)) return Fail("That amendment is unavailable.");
	const std::string Raises = Def->Get("raisesPolicy").AsString();
	if (!Raises.empty() && S.Get("levels").Get(Raises).AsNumber() >= 4) return Fail("The " + Lower(Policy(Raises)->Get("name").AsString()) + " is already at its maximum. Seek a different coalition.");
	if (S.Get("capital").AsNumber() < 1) return Fail("Negotiating an amendment costs 1 political capital.");
	S.Set("capital", Num(S.Get("capital").AsNumber() - 1));
	(*B)["amendments"].Push(Str(AmendmentId));
	Record(S, "legislation", "Amendment accepted", Def->Get("name").AsString());
	return Ok("Amendment accepted. The whip count and cost forecast have changed.");
}

// lobby(s, id)
V Simulation::Lobby(V& S, const std::string& BlocId) const
{
	V* B = MutableActiveBill(S);
	const V* Bloc = FindById(X("blocs"), BlocId);
	if (!Allowed(S) || !B || !Bloc || Contains(B->Get("lobbied"), BlocId)) return Fail("That faction cannot be lobbied again for this bill.");
	if (S.Get("location").AsString() != "oval" || Slots(S) < 1 || S.Get("capital").AsNumber() < 1) return Fail("Lobbying requires an Oval Office appointment and 1 capital.");
	S.Set("capital", Num(S.Get("capital").AsNumber() - 1));
	PushAppointment(S, "lobby");
	B = MutableActiveBill(S);
	(*B)["lobbied"].Push(Str(BlocId));
	Record(S, "legislation", "Congressional meeting", Bloc->Get("name").AsString() + " support increased.");
	return Ok("The faction agrees to support more of the package. Whip count updated.");
}

// vote(s)
V Simulation::Vote(V& S) const
{
	V* B = MutableActiveBill(S);
	if (!Allowed(S) || !B || B->Get("lastVote").AsNumber() == S.Get("quarter").AsNumber()) return Fail("Call at most one floor vote on this bill each quarter.");
	if (S.Get("capital").AsNumber() < 2) return Fail("A floor vote costs 2 political capital.");
	S.Set("capital", Num(S.Get("capital").AsNumber() - 2));
	B = MutableActiveBill(S);
	B->Set("lastVote", S.Get("quarter"));
	B->Set("attempts", Num(B->Get("attempts").AsNumber() + 1));
	B->Set("rollcall", Votes(S, *B));
	double Yes = 0;
	for (const V& Row : B->Get("rollcall").Items()) Yes = Yes + Row.Get("yes").AsNumber();
	B->Set("yes", Num(Yes));
	const V& Def = *Bill(B->Get("id").AsString());
	const std::string Tally = Fmt(Yes) + "–" + Fmt(100 - Yes);
	if (Yes >= 51)
	{
		B->Set("status", Str("passed"));
		B->Set("passed", S.Get("quarter"));
		std::vector<std::string> Raised;
		for (const V& A : X("amendments").Items())
		{
			if (!A.Get("raisesPolicy").AsString().empty() && Contains(B->Get("amendments"), A.Get("id").AsString())) Raised.push_back(A.Get("raisesPolicy").AsString());
		}
		S["levels"].Set(Def.Get("policy").AsString(), Num(4));
		S.Set("debt", Num(Bound(S.Get("debt").AsNumber() + Def.Get("cost").AsNumber(), 0, 250)));
		for (const std::string& PolicyId : Raised) S["levels"].Set(PolicyId, Num(std::min(4.0, S.Get("levels").Get(PolicyId).AsNumber() + 1)));
		S["metrics"].Set("trust", Num(Bound(S.Get("metrics").Get("trust").AsNumber() + 2)));
		const std::string Sponsor = Def.Get("sponsor").AsString();
		S["relationships"].Set(Sponsor, Num(Bound(S.Get("relationships").Get(Sponsor).AsNumber() + 5)));
		Record(S, "legislation", Def.Get("name").AsString() + " passes", Tally + ". Implementation begins next quarter. Initial debt +" + Fmt(Def.Get("cost").AsNumber()) + ".");
		return Ok("Passed " + Tally + ". The program now has legal authority; follow delivery on the national map.");
	}
	B->Set("status", Str("failed"));
	V& Opposition = S["executive"]["opposition"];
	Opposition.Set("momentum", Num(Bound(Opposition.Get("momentum").AsNumber() + 6)));
	Record(S, "legislation", Def.Get("name").AsString() + " defeated", Tally + ". Amend the package and retry next quarter.");
	return Ok("Defeated " + Tally + ". The opposition gains momentum. You can revise the bill and retry next quarter.");
}

// regionalView(s, id, basePoll)
V Simulation::RegionalView(const V& S, const std::string& RegionId, double BasePoll) const
{
	const V& Def = *FindById(X("regions"), RegionId);
	const V& R = S.Get("executive").Get("regions").Get(RegionId);
	const V& M = S.Get("metrics");
	const double Local = (R.Get("housing").AsNumber() - M.Get("housing").AsNumber() + R.Get("jobs").AsNumber() - M.Get("jobs").AsNumber() + R.Get("health").AsNumber() - M.Get("health").AsNumber() + R.Get("energy").AsNumber() - M.Get("energy").AsNumber()) / 4;
	V View = Def;
	for (std::size_t Index = 0; Index < R.Keys().size(); ++Index) View.Set(R.Keys()[Index], R.Values()[Index]);
	View.Set("approval", Num(JsRound(Bound(BasePoll + Local * .55 + R.Get("goodwill").AsNumber(), 5, 95))));
	return View;
}

// election(s, basePoll)
V Simulation::Election(const V& S, double BasePoll) const
{
	double Points = 0;
	V Regions = V::Array();
	for (const V& Def : X("regions").Items())
	{
		const V View = RegionalView(S, Def.Get("id").AsString(), BasePoll);
		const bool bWon = View.Get("approval").AsNumber() >= 50;
		if (bWon) Points = Points + View.Get("points").AsNumber();
		V Row = V::Object();
		Row.Set("id", View.Get("id"));
		Row.Set("name", View.Get("name"));
		Row.Set("approval", View.Get("approval"));
		Row.Set("points", View.Get("points"));
		Row.Set("won", Bool(bWon));
		Regions.Push(std::move(Row));
	}
	V Result = V::Object();
	Result.Set("points", Num(Points));
	Result.Set("regions", std::move(Regions));
	Result.Set("won", Bool(Points > 50));
	return Result;
}

// regionalVisit(s, id)
V Simulation::RegionalVisit(V& S, const std::string& RegionId) const
{
	const V* R = S.Get("executive").Get("regions").Find(RegionId);
	if (!Allowed(S) || !R || S.Get("location").AsString() != "aircraft" || Slots(S) < 1 || S.Get("capital").AsNumber() < 2 || R->Get("visited").AsNumber() == S.Get("quarter").AsNumber())
		return Fail("A regional visit needs an Air Force One appointment, 2 capital, and an unvisited region this quarter.");
	S.Set("capital", Num(S.Get("capital").AsNumber() - 2));
	S.Set("debt", Num(Bound(S.Get("debt").AsNumber() + 3, 0, 250)));
	PushAppointment(S, "regional");
	V& Region = S["executive"]["regions"][RegionId];
	Region.Set("visited", S.Get("quarter"));
	Region.Set("goodwill", Num(Bound(Region.Get("goodwill").AsNumber() + 3, -15, 15)));
	std::string Weakest = "housing";
	for (const char* K : {"jobs", "energy", "health"})
	{
		if (Region.Get(K).AsNumber() < Region.Get(Weakest).AsNumber()) Weakest = K;
	}
	Region.Set(Weakest, Num(Bound(Region.Get(Weakest).AsNumber() + 2)));
	const V& Def = *FindById(X("regions"), RegionId);
	const std::string Text = "Federal visit brings targeted " + Weakest + " assistance and a meeting with " + Def.Get("governor").AsString() + ".";
	News(S, RegionId, Text);
	Record(S, "region", Def.Get("name").AsString(), Text);
	return Ok("Visit completed. The weakest local service improves by 2; regional goodwill +3; debt +3.");
}

// modifier(s, group)
double Simulation::Modifier(const V& S, const std::string& GroupId) const
{
	if (!S.Get("executive").Truthy()) return 0;
	const V* Region = nullptr;
	for (const V& R : X("regions").Items())
	{
		if (Contains(R.Get("groups"), GroupId))
		{
			Region = &R;
			break;
		}
	}
	const V* R = Region ? S.Get("executive").Get("regions").Find(Region->Get("id").AsString()) : nullptr;
	const V& M = S.Get("metrics");
	const double Local = R ? (R->Get("housing").AsNumber() - M.Get("housing").AsNumber() + R->Get("jobs").AsNumber() - M.Get("jobs").AsNumber() + R->Get("health").AsNumber() - M.Get("health").AsNumber() + R->Get("energy").AsNumber() - M.Get("energy").AsNumber()) * .025 + R->Get("goodwill").AsNumber() * .2 : 0;
	return Local - S.Get("executive").Get("opposition").Get("momentum").AsNumber() * .035;
}

// spending(s)
double Simulation::Spending(const V& S) const
{
	if (!S.Get("executive").Truthy()) return 0;
	double Total = 0;
	for (const V& B : S.Get("executive").Get("bills").Items())
	{
		if (B.Get("status").AsString() != "passed") continue;
		double BillCost = 0;
		for (const V& Id : B.Get("amendments").Items()) BillCost = BillCost + FindById(X("amendments"), Id.AsString())->Get("cost").AsNumber();
		Total = Total + BillCost;
	}
	return Total;
}

// staffFactor(s, policy)
double Simulation::StaffFactor(const V& S, const std::string& PolicyId) const
{
	if (!S.Get("executive").Truthy()) return 1;
	for (const V& T : X("team").Items())
	{
		if (!Contains(T.Get("policies"), PolicyId)) continue;
		const V& P = S.Get("executive").Get("cabinet").Get(T.Get("id").AsString());
		return P.Get("status").AsString() == "resigned" ? .55 : .65 + P.Get("competence").AsNumber() / 200;
	}
	return 1;
}

// teamAction(s, id, action)
V Simulation::TeamAction(V& S, const std::string& PersonId, const std::string& Action) const
{
	const V* P = S.Get("executive").Get("cabinet").Find(PersonId);
	if (!Allowed(S) || !P || (Action != "support" && Action != "train" && Action != "replace") || P->Get("lastAction").AsNumber() == S.Get("quarter").AsNumber() || Slots(S) < 1 || S.Get("location").AsString() != "oval")
		return Fail("Team decisions need one Oval Office appointment per person per quarter.");
	const double Cost = Action == "replace" ? 3 : 2;
	if (S.Get("capital").AsNumber() < Cost) return Fail("This action costs " + Fmt(Cost) + " political capital.");
	if (Action != "replace" && P->Get("status").AsString() == "resigned") return Fail("Appoint an acting replacement for this vacant role.");
	if (Action == "replace" && P->Get("acting").Truthy() && P->Get("status").AsString() != "resigned") return Fail("An acting replacement is already in place.");
	S.Set("capital", Num(S.Get("capital").AsNumber() - Cost));
	PushAppointment(S, "team");
	V& Member = S["executive"]["cabinet"][PersonId];
	Member.Set("lastAction", S.Get("quarter"));
	if (Action == "support")
	{
		S["relationships"].Set(PersonId, Num(Bound(S.Get("relationships").Get(PersonId).AsNumber() + 10)));
		Member.Set("strain", Num(0));
	}
	if (Action == "train") Member.Set("competence", Num(Bound(Member.Get("competence").AsNumber() + 10)));
	if (Action == "replace")
	{
		Member.Set("status", Str("serving"));
		Member.Set("acting", Bool(true));
		Member.Set("competence", Num(68));
		Member.Set("strain", Num(0));
		S["relationships"].Set(PersonId, Num(50));
	}
	const V& Def = *Team(PersonId);
	Record(S, "cabinet", "Government team", Def.Get("portfolio").AsString() + ": " + Action + ".");
	if (Action == "replace") return Ok(Def.Get("deputy").AsString() + " takes over. Competence 68; relationship 50.");
	return Ok(Action == "train" ? "Delivery competence +10." : "Relationship +10. Immediate resignation pressure relieved.");
}

// person(s, base): an acting replacement takes the adviser's place.
V Simulation::Person(const V& S, const V& Base) const
{
	const V& TeamState = S.Get("executive").Get("cabinet").Get(Base.Get("id").AsString());
	if (!TeamState.Get("acting").Truthy()) return Base;
	const std::string Deputy = Team(Base.Get("id").AsString())->Get("deputy").AsString();
	std::vector<std::string> Words;
	std::size_t Start = 0;
	while (true)
	{
		const std::size_t Space = Deputy.find(' ', Start);
		Words.push_back(Deputy.substr(Start, Space == std::string::npos ? std::string::npos : Space - Start));
		if (Space == std::string::npos) break;
		Start = Space + 1;
	}
	std::string Initials;
	for (std::size_t Index = Words.size() >= 2 ? Words.size() - 2 : 0; Index < Words.size(); ++Index)
	{
		if (!Words[Index].empty()) Initials += Words[Index][0];
	}
	V Replacement = Base;
	Replacement.Set("name", Str(Deputy));
	Replacement.Set("initials", Str(Initials));
	return Replacement;
}

V Simulation::People(const V& S) const
{
	V Out = V::Array();
	for (const V& P : D("people").Items()) Out.Push(Person(S, P));
	return Out;
}

// crisisOptions(c)
V Simulation::CrisisOptions(const V& Crisis) const
{
	const V* Stage = X("crises").Find(Crisis.Get("stage").AsString());
	V Options = Stage ? *Stage : X("crises").Get("investigation");
	std::string DamageText = Crisis.Get("damage").IsNumber() ? Fmt(Crisis.Get("damage").AsNumber()) : "undefined";
	Options.Set("body", Str(ReplaceFirst(Options.Get("body").AsString(), "{damage}", DamageText)));
	return Options;
}

// respondCrisis(s, id, choice)
V Simulation::RespondCrisis(V& S, const std::string& CrisisId, const std::string& Choice) const
{
	V* Crisis = nullptr;
	for (V& C : S["executive"]["crises"].Items())
	{
		if (C.Get("id").AsString() == CrisisId)
		{
			Crisis = &C;
			break;
		}
	}
	bool Offered = false;
	if (Crisis)
	{
		const V Options = CrisisOptions(*Crisis);
		for (const V& Option : Options.Get("choices").Items())
		{
			if (Option.Get("id").AsString() == Choice) Offered = true;
		}
	}
	if (!Allowed(S) || !Crisis || Crisis->Get("stage").AsString() == "resolved" || Crisis->Get("response").Truthy() || !Offered) return Fail("This crisis response is unavailable.");
	Crisis->Set("response", Str(Choice));
	V& R = S["executive"]["regions"][Crisis->Get("region").AsString()];
	if (Choice == "prepare") S.Set("debt", Num(Bound(S.Get("debt").AsNumber() + 4, 0, 250)));
	if (Choice == "full" || Choice == "target")
	{
		const double Amount = Choice == "full" ? 7 : 4;
		S.Set("debt", Num(Bound(S.Get("debt").AsNumber() + (Choice == "full" ? 8 : 4), 0, 250)));
		const double Damage = Crisis->Get("damage").AsNumber();
		R.Set("housing", Num(Bound(R.Get("housing").AsNumber() + std::min(Damage, Amount))));
		R.Set("energy", Num(Bound(R.Get("energy").AsNumber() + std::min(Damage, Amount))));
		R.Set("goodwill", Num(Bound(R.Get("goodwill").AsNumber() + 2, -15, 15)));
	}
	if (Choice == "publish")
	{
		S.Set("debt", Num(Bound(S.Get("debt").AsNumber() + 2, 0, 250)));
		S["metrics"].Set("trust", Num(Bound(S.Get("metrics").Get("trust").AsNumber() + 2)));
		S["relationships"].Set("chen", Num(Bound(S.Get("relationships").Get("chen").AsNumber() - 4)));
	}
	if (Choice == "defend")
	{
		S["metrics"].Set("trust", Num(Bound(S.Get("metrics").Get("trust").AsNumber() - 4)));
		V& Opposition = S["executive"]["opposition"];
		Opposition.Set("momentum", Num(Bound(Opposition.Get("momentum").AsNumber() + 6)));
	}
	const V Options = CrisisOptions(*Crisis);
	std::string ChoiceTitle;
	for (const V& Option : Options.Get("choices").Items())
	{
		if (Option.Get("id").AsString() == Choice) ChoiceTitle = Option.Get("title").AsString();
	}
	Record(S, "crisis", Options.Get("title").AsString(), ChoiceTitle);
	return Ok("Response recorded. The next quarterly report will show how the crisis develops.");
}

// encounterOptions(s, e)
V Simulation::EncounterOptions(const V& S, const V& Encounter) const
{
	const V& Def = X("encounters").Get(Encounter.Get("type").AsString() == "debate" ? "debate" : "press");
	const V& Stage = Def.Get("stages")[static_cast<std::size_t>(std::min(Encounter.Get("stage").AsNumber(), 1.0))];
	const V& After = Stage.Get("questionAfter").Get(Encounter.Get("answers")[0].AsString());
	const std::string Question = After.IsNullish() ? Stage.Get("question").AsString() : After.AsString();
	V Options = V::Object();
	Options.Set("title", Def.Get("title"));
	Options.Set("speaker", Stage.Get("speaker"));
	Options.Set("question", Str(ReplaceFirst(Question, "{issue}", S.Get("executive").Get("opposition").Get("issue").AsString())));
	Options.Set("choices", Stage.Get("choices"));
	return Options;
}

// startEncounter(s, type)
V Simulation::StartEncounter(V& S, const std::string& Type) const
{
	bool Appeared = false;
	for (const V& E : S.Get("executive").Get("encounters").Items())
	{
		if (E.Get("quarter").AsNumber() == S.Get("quarter").AsNumber()) Appeared = true;
	}
	if (!Allowed(S) || (Type != "press" && Type != "debate") || S.Get("location").AsString() != "oval" || Slots(S) < 1 || Appeared)
		return Fail("One media appearance per quarter requires an Oval Office appointment.");
	if (Type == "debate" && !ContainsNumber({7, 15}, S.Get("quarter").AsNumber())) return Fail("Debates take place in the final quarter before each election.");
	PushAppointment(S, "media");
	V Encounter = V::Object();
	Encounter.Set("type", Str(Type));
	Encounter.Set("quarter", S.Get("quarter"));
	Encounter.Set("stage", Num(0));
	Encounter.Set("answers", V::Array());
	Encounter.Set("done", Bool(false));
	S["executive"]["encounters"].Push(std::move(Encounter));
	return Ok("The cameras are live. Choose your answer, then handle the follow-up.");
}

// answerEncounter(s, choice)
V Simulation::AnswerEncounter(V& S, const std::string& Choice) const
{
	V* Encounter = nullptr;
	for (V& E : S["executive"]["encounters"].Items())
	{
		if (E.Get("quarter").AsNumber() == S.Get("quarter").AsNumber() && !E.Get("done").Truthy())
		{
			Encounter = &E;
			break;
		}
	}
	bool Offered = false;
	if (Encounter)
	{
		const V Options = EncounterOptions(S, *Encounter);
		for (const V& Option : Options.Get("choices").Items())
		{
			if (Option.Get("id").AsString() == Choice) Offered = true;
		}
	}
	if (!Allowed(S) || !Encounter || !Offered) return Fail("That answer is unavailable.");
	double Trust = 0, Momentum = 0;
	if (Choice == "record") Trust = S.Get("metrics").Get(S.Get("executive").Get("opposition").Get("issue").AsString()).AsNumber() >= 52 ? 2 : -2;
	if (Choice == "own")
	{
		Trust = 1;
		Momentum = -2;
	}
	if (Choice == "attack" || Choice == "deflect")
	{
		Trust = -2;
		S.Set("capital", Num(Bound(S.Get("capital").AsNumber() + 1, 0, 20)));
		Momentum = Choice == "deflect" ? 2 : 0;
	}
	if (Choice == "specific")
	{
		int Met = 0;
		const V Standing = Legacy(S);
		for (const V& Goal : Standing.Get("goals").Items())
		{
			if (Goal.Get("met").Truthy()) ++Met;
		}
		if (Met >= 2) Momentum = -4;
		else Trust = -1;
	}
	if (Choice == "listen")
	{
		Trust = 1;
		Momentum = -1;
	}
	if (Choice == "timeline") Trust = 1;
	if (Choice == "victory")
	{
		bool Completed = false;
		for (const V& B : S.Get("executive").Get("bills").Items())
		{
			if (B.Get("progress").AsNumber() >= 100) Completed = true;
		}
		Trust = Completed ? 2 : -3;
	}
	if (Choice == "transparent")
	{
		Trust = 2;
		Momentum = -2;
	}
	S["metrics"].Set("trust", Num(Bound(S.Get("metrics").Get("trust").AsNumber() + Trust)));
	V& Opposition = S["executive"]["opposition"];
	Opposition.Set("momentum", Num(Bound(Opposition.Get("momentum").AsNumber() + Momentum)));
	for (V& E : S["executive"]["encounters"].Items())
	{
		if (E.Get("quarter").AsNumber() == S.Get("quarter").AsNumber() && !E.Get("done").Truthy())
		{
			Encounter = &E;
			break;
		}
	}
	(*Encounter)["answers"].Push(Str(Choice));
	Encounter->Set("stage", Num(Encounter->Get("stage").AsNumber() + 1));
	Encounter->Set("done", Bool(Encounter->Get("stage").AsNumber() >= 2));
	const bool bDone = Encounter->Get("done").AsBool();
	const std::string Title = Encounter->Get("type").AsString() == "press" ? "Press conference" : "Presidential debate";
	Record(S, "media", Title, Choice + ": trust " + (Trust >= 0 ? "+" : "") + Fmt(Trust) + ", opposition momentum " + (Momentum >= 0 ? "+" : "") + Fmt(Momentum) + ".");
	return Ok(bDone ? "Appearance complete. Your answers are in the presidential record." : "Answer recorded. The moderator has a follow-up.");
}

// legacy(s)
V Simulation::Legacy(const V& S) const
{
	const V& Platform = S.Get("executive").Get("platform");
	V Goals = V::Array();
	for (const V& Id : Platform.Get("goals").Items())
	{
		const V& G = *FindById(X("goals"), Id.AsString());
		const bool bDebt = G.Get("id").AsString() == "debt";
		const double Value = bDebt ? S.Get("debt").AsNumber() : S.Get("metrics").Get(G.Get("metric").AsString()).AsNumber();
		V Goal = G;
		Goal.Set("value", Num(JsRound(Value)));
		Goal.Set("met", Bool(bDebt ? Value <= G.Get("target").AsNumber() : Value >= G.Get("target").AsNumber()));
		Goals.Push(std::move(Goal));
	}
	V Result = V::Object();
	Result.Set("goals", std::move(Goals));
	V Project;
	for (const V& B : S.Get("executive").Get("bills").Items())
	{
		if (Platform.Get("signature").IsString() && B.Get("id").AsString() == Platform.Get("signature").AsString())
		{
			Project = B;
			break;
		}
	}
	Result.Set("project", std::move(Project));
	double Kept = 0, Broken = 0, Completed = 0;
	for (const V& P : S.Get("promises").Items())
	{
		if (P.Get("status").AsString() == "kept") ++Kept;
		if (P.Get("status").AsString() == "broken") ++Broken;
	}
	for (const V& B : S.Get("executive").Get("bills").Items())
	{
		if (B.Get("progress").AsNumber() >= 100) ++Completed;
	}
	Result.Set("kept", Num(Kept));
	Result.Set("broken", Num(Broken));
	Result.Set("completed", Num(Completed));
	return Result;
}

// ----- Appointments, promises and campaigns -----

// request(s, id): what an adviser asks for this quarter. ActivePromise is the index of their open promise, or -1.
V Simulation::Request(const V& S, const std::string& PersonId, int& ActivePromise) const
{
	ActivePromise = -1;
	const V* Base = FindById(D("people"), PersonId);
	if (!Base) return V();
	const V Who = Person(S, *Base);
	const V& Promises = S.Get("promises");
	for (std::size_t Index = 0; Index < Promises.Size(); ++Index)
	{
		if (Promises[Index].Get("person").AsString() == PersonId && Promises[Index].Get("status").AsString() == "open")
		{
			ActivePromise = static_cast<int>(Index);
			break;
		}
	}
	const V& Policies = Who.Get("policies");
	std::vector<std::string> Candidates;
	for (std::size_t Index = 0; Index < Policies.Size(); ++Index)
	{
		const std::size_t Rotated = (Index + static_cast<std::size_t>(S.Get("quarter").AsNumber())) % Policies.Size();
		Candidates.push_back(Policies[Rotated].AsString());
	}
	std::string PolicyId;
	if (ActivePromise >= 0) PolicyId = Promises[static_cast<std::size_t>(ActivePromise)].Get("policy").AsString();
	if (PolicyId.empty())
	{
		for (const std::string& Candidate : Candidates)
		{
			if (S.Get("levels").Get(Candidate).AsNumber() < 4)
			{
				PolicyId = Candidate;
				break;
			}
		}
	}
	if (PolicyId.empty() && !Candidates.empty()) PolicyId = Candidates[0];
	bool Done = false;
	for (const V& A : S.Get("appointments").Items())
	{
		if (A.Get("quarter").AsNumber() == S.Get("quarter").AsNumber() && A.Get("person").AsString() == PersonId) Done = true;
	}
	V Result = V::Object();
	Result.Set("person", Who);
	Result.Set("policy", *Policy(PolicyId));
	if (ActivePromise >= 0) Result.Set("active", Promises[static_cast<std::size_t>(ActivePromise)]);
	Result.Set("target", Num(std::min(4.0, S.Get("levels").Get(PolicyId).AsNumber() + 1)));
	Result.Set("done", Bool(Done));
	return Result;
}

// resolvePromise(s, p, kept)
std::string Simulation::ResolvePromise(V& S, std::size_t PromiseIndex, bool bKept) const
{
	V& P = S["promises"][PromiseIndex];
	const std::string PersonId = P.Get("person").AsString();
	const V Who = Person(S, *FindById(D("people"), PersonId));
	P.Set("status", Str(bKept ? "kept" : "broken"));
	P.Set("resolved", S.Get("quarter"));
	const std::string PolicyName = Policy(P.Get("policy").AsString())->Get("name").AsString();
	S["relationships"].Set(PersonId, Num(Bound(S.Get("relationships").Get(PersonId).AsNumber() + (bKept ? 8 : -12))));
	const std::string GroupId = Who.Get("group").AsString();
	S["goodwill"].Set(GroupId, Num(Bound(S.Get("goodwill").Get(GroupId).NumberOr(0) + (bKept ? 2 : -3), -12, 12)));
	S["metrics"].Set("trust", Num(Bound(S.Get("metrics").Get("trust").AsNumber() + (bKept ? 1 : -3))));
	if (bKept) S.Set("capital", Num(Bound(S.Get("capital").AsNumber() + 2, 0, 20)));
	const std::string Title = std::string(bKept ? "Promise kept" : "Promise broken") + ": " + PolicyName;
	const std::string Name = Who.Get("name").AsString();
	Record(S, "promise", Title, Name + " " + (bKept ? "backs your administration" : "questions your word") + ". Relationship " + (bKept ? "+8" : "−12") + "; " + Group(GroupId)->Get("name").AsString() + " goodwill " + (bKept ? "+2" : "−3") + ".");
	return Title + ". " + Name + " " + (bKept ? "celebrates delivery" : "demands an explanation") + ".";
}

// meeting(s, id, response)
V Simulation::Meeting(V& S, const std::string& PersonId, const std::string& Response) const
{
	int Active = -1;
	const V R = Request(S, PersonId, Active);
	if (R.IsUndefined() || S.Get("executive").Get("cabinet").Get(PersonId).Get("status").AsString() == "resigned" || R.Get("done").AsBool() || S.Get("ended").Truthy() || !S.Get("agendaChoice").IsNull() || !Slots(S) || S.Get("location").AsString() != "oval")
		return Fail("Meetings need an open appointment slot in the Oval Office before filing the quarterly decision.");
	const bool bOffered = Active >= 0 ? (Response == "reassure" || Response == "extend" || Response == "withdraw") : (Response == "promise" || Response == "compromise" || Response == "listen");
	if (!bOffered) return Fail("Choose a response offered in this meeting.");
	const V& ActivePromise = R.Get("active");
	if (Response == "extend" && (ActivePromise.Get("extended").Truthy() || ActivePromise.Get("due").AsNumber() >= 16)) return Fail("This deadline cannot be extended again.");
	if (Response == "reassure" && S.Get("capital").AsNumber() < 1) return Fail("Reassurance costs 1 political capital.");
	const std::string PolicyId = R.Get("policy").Get("id").AsString();
	if ((Response == "promise" || Response == "compromise") && S.Get("levels").Get(PolicyId).AsNumber() >= 4) return Fail("This program is already at full strength. Choose a listening meeting.");
	V Appointment = V::Object();
	Appointment.Set("quarter", S.Get("quarter"));
	Appointment.Set("person", Str(PersonId));
	Appointment.Set("type", Str("meeting"));
	S["appointments"].Push(std::move(Appointment));
	std::string Detail;
	if (Response == "promise" || Response == "compromise")
	{
		const bool bPromise = Response == "promise";
		const double Support = bPromise ? 3 : 1;
		const double Due = std::min(16.0, S.Get("quarter").AsNumber() + (bPromise ? 2 : 3));
		V Promise = V::Object();
		Promise.Set("id", Str(Fmt(S.Get("quarter").AsNumber()) + "-" + PersonId));
		Promise.Set("person", Str(PersonId));
		Promise.Set("policy", Str(PolicyId));
		Promise.Set("target", R.Get("target"));
		Promise.Set("due", Num(Due));
		Promise.Set("status", Str("open"));
		Promise.Set("extended", Bool(false));
		S["promises"].Push(std::move(Promise));
		S.Set("capital", Num(Bound(S.Get("capital").AsNumber() + Support, 0, 20)));
		S["relationships"].Set(PersonId, Num(Bound(S.Get("relationships").Get(PersonId).AsNumber() + (bPromise ? 4 : 2))));
		Detail = R.Get("policy").Get("name").AsString() + " at " + Fmt(R.Get("target").AsNumber()) + "/4 by the end of term quarter " + Fmt(Due) + "/16. " + (bPromise ? "A firm pledge earns immediate support." : "A longer deadline earns narrower support.") + " Political capital +" + Fmt(Support) + ".";
	}
	if (Response == "listen")
	{
		S["relationships"].Set(PersonId, Num(Bound(S.Get("relationships").Get(PersonId).AsNumber() + 1)));
		Detail = "You listened without making a commitment. Relationship +1; no political support pledged.";
	}
	if (Response == "reassure")
	{
		S.Set("capital", Num(S.Get("capital").AsNumber() - 1));
		S["relationships"].Set(PersonId, Num(Bound(S.Get("relationships").Get(PersonId).AsNumber() + 3)));
		Detail = "You invested political attention in the relationship. Capital −1; relationship +3. The original deadline still stands.";
	}
	if (Response == "extend")
	{
		V& Promise = S["promises"][static_cast<std::size_t>(Active)];
		Promise.Set("due", Num(std::min(16.0, Promise.Get("due").AsNumber() + 1)));
		Promise.Set("extended", Bool(true));
		S["relationships"].Set(PersonId, Num(Bound(S.Get("relationships").Get(PersonId).AsNumber() - 3)));
		Detail = "A reluctant extension to quarter " + Fmt(Promise.Get("due").AsNumber()) + ". Relationship −3.";
	}
	if (Response == "withdraw")
	{
		ResolvePromise(S, static_cast<std::size_t>(Active), false);
		Detail = "You withdrew the promise. Trust and the relationship take the same hit as a missed deadline.";
	}
	Record(S, "meeting", "Meeting with " + R.Get("person").Get("name").AsString(), Detail);
	return Ok(Detail);
}

// campaignTrip(s, regionId, issueId, personId), including tripPreview.
V Simulation::CampaignTrip(V& S, const std::string& RegionId, const std::string& IssueId, const std::string& PersonId) const
{
	const V* Region = FindById(D("campaign").Get("regions"), RegionId);
	const V* Issue = D("campaign").Get("issues").Find(IssueId);
	const V* Base = FindById(D("people"), PersonId);
	if (!Region || !Issue || !Base || S.Get("executive").Get("cabinet").Get(PersonId).Get("status").AsString() == "resigned") return Fail("Choose a region, a speech and a traveling adviser.");
	const V Who = Person(S, *Base);
	if (!CampaignSeason(S) || !S.Get("agendaChoice").IsNull()) return Fail("Campaign flights open in the two quarters before each election, before filing the quarterly decision.");
	if (S.Get("location").AsString() != "aircraft") return Fail("Board Air Force One to hold your campaign briefing.");
	bool TripThisQuarter = false;
	for (const V& Trip : S.Get("campaign").Get("trips").Items())
	{
		if (Trip.Get("quarter").AsNumber() == S.Get("quarter").AsNumber()) TripThisQuarter = true;
	}
	if (!Slots(S) || TripThisQuarter) return Fail("You need an open appointment slot. One campaign trip is allowed each quarter.");
	if (S.Get("capital").AsNumber() < 2) return Fail("A campaign trip requires 2 political capital.");
	const bool bRelevant = Contains(Region->Get("issues"), IssueId);
	const bool bCredible = S.Get("metrics").Get(Issue->Get("metric").AsString()).AsNumber() >= 55;
	const bool bExpert = Contains(Who.Get("expertise"), IssueId);
	const bool bTrusted = S.Get("relationships").Get(PersonId).AsNumber() >= 60;
	double Broken = 0;
	for (const V& P : S.Get("promises").Items())
	{
		if (P.Get("status").AsString() == "broken" && P.Get("person").AsString() == PersonId) ++Broken;
	}
	const double Strength = std::max(1.0, (bRelevant ? 2 : 1) + (bCredible ? 1 : 0) + (bExpert ? 1 : 0) + (bTrusted ? 1 : 0) - std::min(2.0, Broken));
	S.Set("capital", Num(S.Get("capital").AsNumber() - 2));
	S.Set("debt", Num(Bound(S.Get("debt").AsNumber() + 2, 0, 250)));
	V& Local = S["executive"]["regions"][RegionId];
	Local.Set("goodwill", Num(Bound(Local.Get("goodwill").AsNumber() + Strength * .5, -15, 15)));
	PushAppointment(S, "trip");
	V Trip = V::Object();
	Trip.Set("quarter", S.Get("quarter"));
	Trip.Set("region", Str(RegionId));
	Trip.Set("issue", Str(IssueId));
	Trip.Set("person", Str(PersonId));
	Trip.Set("strength", Num(Strength));
	S["campaign"]["trips"].Push(std::move(Trip));
	V& Boosts = S["campaign"]["boosts"];
	for (const V& GroupId : Region->Get("groups").Items()) Boosts.Set(GroupId.AsString(), Num(Bound(Boosts.Get(GroupId.AsString()).NumberOr(0) + Strength, 0, 8)));
	const std::string Detail = Who.Get("name").AsString() + " joins you in " + Region->Get("name").AsString() + ". Speech: " + Issue->Get("name").AsString() + ". Local voter support +" + Fmt(Strength) + "; capital −2; debt +2. Campaign enthusiasm fades by 25% each quarter.";
	Record(S, "campaign", "On the road: " + Region->Get("name").AsString(), Detail);
	return Ok(Detail);
}

// ----- Policies, budget and the quarterly decision -----

// budget(s)
V Simulation::Budget(const V& S) const
{
	double Revenue = 87, Spent = 85;
	for (const V& P : PolicyList.Items())
	{
		const double Contribution = (S.Get("levels").Get(P.Get("id").AsString()).AsNumber() - 2) * P.Get("cost").AsNumber();
		if (P.Get("cost").AsNumber() < 0) Revenue -= Contribution;
		else Spent += Contribution;
	}
	double SituationRevenue = 0;
	for (const V* Sit : ActiveSituations(S)) SituationRevenue = SituationRevenue + Sit->Get("revenue").NumberOr(0);
	V Pressure;
	double UnrestRevenue = 0;
	UnrestPressure(S, Pressure, UnrestRevenue);
	Revenue += (S.Get("metrics").Get("growth").AsNumber() - 50) * .2 + SituationRevenue + UnrestRevenue;
	Spent += S.Get("debt").AsNumber() * .045 + Spending(S);
	V Result = V::Object();
	Result.Set("revenue", Num(JsRound(Revenue)));
	Result.Set("spending", Num(JsRound(Spent)));
	Result.Set("balance", Num(JsRound(Revenue - Spent)));
	Result.Set("interest", Num(JsRound(S.Get("debt").AsNumber() * .045)));
	return Result;
}

// changePolicy(s, id, level)
V Simulation::ChangePolicy(V& S, const std::string& Id, double Level) const
{
	const V* P = Policy(Id);
	if (!P || S.Get("ended").Truthy() || !S.Get("agendaChoice").IsNull() || Level < 0 || Level > 4 || std::trunc(Level) != Level) return Fail("This policy cannot be changed now.");
	if (Level == 4)
	{
		bool NeedsAct = false, Authorized = false;
		for (const V& B : X("bills").Items())
		{
			if (B.Get("policy").AsString() == Id) NeedsAct = true;
		}
		for (const V& B : S.Get("executive").Get("bills").Items())
		{
			if (B.Get("status").AsString() == "passed" && Bill(B.Get("id").AsString())->Get("policy").AsString() == Id) Authorized = true;
		}
		if (NeedsAct && !Authorized) return Fail("Full-strength funding needs an act of Congress. Open Congress to introduce the bill.");
	}
	const double Previous = S.Get("levels").Get(Id).AsNumber();
	const double Steps = std::fabs(Level - Previous);
	if (!Steps) return Fail("That setting is already in force.");
	const double Cost = Steps * (2 + (P->Get("lag").AsNumber() > 1 ? 1 : 0));
	if (S.Get("capital").AsNumber() < Cost) return Fail("Requires " + Fmt(Cost) + " political capital.");
	S.Set("capital", Num(S.Get("capital").AsNumber() - Cost));
	S["levels"].Set(Id, Num(Level));
	Record(S, "policy", P->Get("name").AsString(), "Set to " + Fmt(Level) + "/4 · " + Fmt(Cost) + " political capital");
	V Result = V::Object();
	Result.Set("ok", Bool(true));
	Result.Set("cost", Num(Cost));
	return Result;
}

// choose(s, index)
V Simulation::Choose(V& S, double Index) const
{
	if (S.Get("ended").Truthy() || !S.Get("agendaChoice").IsNull()) return Bool(false);
	const V* Event = CurrentEvent(S);
	if (!Event || Index < 0 || std::trunc(Index) != Index || Index >= static_cast<double>(Event->Get("choices").Size())) return Bool(false);
	const V& Choice = Event->Get("choices")[static_cast<std::size_t>(Index)];
	S.Set("agendaChoice", Num(Index));
	V Effect = V::Object();
	Effect.Set("due", Num(S.Get("quarter").AsNumber() + 1));
	Effect.Set("delta", Choice.Get("effects"));
	Effect.Set("title", Str(Event->Get("title").AsString() + ": " + Choice.Get("label").AsString()));
	S["effects"].Push(std::move(Effect));
	S.Set("debt", Num(Bound(S.Get("debt").AsNumber() + Choice.Get("cost").AsNumber() * .45, 0, 250)));
	if (Choice.Get("calm").Truthy())
	{
		std::string Top;
		double TopAnger = -1;
		for (const V& G : D("groups").Items())
		{
			const double Anger = S.Get("unrest").Get("anger").Get(G.Get("id").AsString()).NumberOr(0);
			if (Anger > TopAnger)
			{
				Top = G.Get("id").AsString();
				TopAnger = Anger;
			}
		}
		S["unrest"]["anger"].Set(Top, Num(Bound(TopAnger - Choice.Get("calm").AsNumber())));
	}
	const double Cost = Choice.Get("cost").AsNumber();
	V Entry = V::Object();
	Entry.Set("quarter", S.Get("quarter"));
	Entry.Set("type", Str("agenda"));
	Entry.Set("event", Event->Get("id"));
	Entry.Set("title", Event->Get("title"));
	Entry.Set("detail", Str(Choice.Get("label").AsString() + " · " + (Cost < 0 ? "Saves " + Fmt(-Cost) : "Cost " + Fmt(Cost)) + " budget units; effects arrive next quarter."));
	S["history"].Push(std::move(Entry));
	return Bool(true);
}

// updateUnrest(s, report): anger builds below the threshold and decays otherwise; extremists risk an attempt.
void Simulation::UpdateUnrest(V& S, V& Report) const
{
	const V& UnrestData = D("unrest");
	const V& Stages = UnrestData.Get("stages");
	const V Current = Groups(S);
	for (const V& G : Current.Items())
	{
		const std::string Id = G.Get("id").AsString();
		const double Before = S.Get("unrest").Get("anger").Get(Id).NumberOr(0);
		const double After = JsRound(Bound(Before * (1 - UnrestData.Get("decay").AsNumber()) + std::max(0.0, UnrestData.Get("threshold").AsNumber() - G.Get("approval").AsNumber()) * UnrestData.Get("rate").AsNumber()) * 10) / 10;
		const int Was = UnrestStageIndex(Before), Now = UnrestStageIndex(After);
		S["unrest"]["anger"].Set(Id, Num(After));
		if (Now >= 0 && Now != Was && After > Before)
		{
			const V& Stage = Stages[static_cast<std::size_t>(Now)];
			const V& Disruption = G.Get("disruption");
			std::string Detail = G.Get("name").AsString() + ": " + Lower(Stage.Get("name").AsString());
			if (Stage.Get("disruption").Truthy() && Disruption.Truthy()) Detail += " (" + Lower(Disruption.Get("name").AsString()) + ")";
			Detail += ".";
			Report["changes"].Push(Str("Unrest rising. " + Detail));
			Record(S, "unrest", "Unrest rising", Detail);
		}
		if (Now < 0 && Was >= 0)
		{
			Report["changes"].Push(Str(G.Get("name").AsString() + " unrest has calmed."));
			Record(S, "unrest", "Unrest calmed", G.Get("name").AsString());
		}
	}
	const V& Attempt = UnrestData.Get("attempt");
	std::vector<const V*> Angry;
	for (const V& G : D("groups").Items())
	{
		if (S.Get("unrest").Get("anger").Get(G.Get("id").AsString()).AsNumber() >= Attempt.Get("at").AsNumber()) Angry.push_back(&G);
	}
	std::stable_sort(Angry.begin(), Angry.end(), [&](const V* A, const V* B) {
		return S.Get("unrest").Get("anger").Get(B->Get("id").AsString()).AsNumber() < S.Get("unrest").Get("anger").Get(A->Get("id").AsString()).AsNumber();
	});
	if (Angry.empty()) return;
	// Police and surveillance lower the odds of an attempt; weaker security raises them.
	double SecuritySum = 0;
	for (const auto& Entry : NumberEntries(Attempt.Get("security"))) SecuritySum = SecuritySum + Entry.second * (S.Get("levels").Get(Entry.first).AsNumber() - 2);
	const double Security = Bound(1 - SecuritySum, .2, 1.6);
	const double Chance = Bound(Attempt.Get("chance").AsNumber() * Security * (1 + (static_cast<double>(Angry.size()) - 1) * .25), 0, .9);
	const unsigned int Seed = ToUint32(S.Get("seed").AsNumber());
	if (Roll(Seed ^ 0x2545f491u, S.Get("quarter").AsNumber()) >= Chance) return;
	const std::string Kind = Roll(Seed ^ 0x68e31da4u, S.Get("quarter").AsNumber()) < Attempt.Get("foiled").AsNumber() ? "foiled" : "attack";
	const V& Outcome = Attempt.Get("outcomes").Get(Kind);
	V Effect = V::Object();
	Effect.Set("due", Num(S.Get("quarter").AsNumber() + 1));
	Effect.Set("delta", Outcome.Get("effects"));
	Effect.Set("title", Outcome.Get("title"));
	S["effects"].Push(std::move(Effect));
	S.Set("capital", Num(Bound(S.Get("capital").AsNumber() + Outcome.Get("capital").NumberOr(0), 0, 20)));
	V Incident = V::Object();
	Incident.Set("quarter", S.Get("quarter"));
	Incident.Set("group", Angry[0]->Get("id"));
	Incident.Set("outcome", Str(Kind));
	S["unrest"]["incidents"].Push(std::move(Incident));
	Report["changes"].Push(Str(Outcome.Get("title").AsString() + ". " + Outcome.Get("detail").AsString()));
	Record(S, "security", Outcome.Get("title").AsString(), Outcome.Get("detail").AsString() + " Investigators link it to extremists among " + Lower(Angry[0]->Get("name").AsString()) + ".");
}

// ExecutiveSim.quarter(s, report)
void Simulation::ExecutiveQuarter(V& S, V& Report) const
{
	V& Exec = S["executive"];
	if (!Exec.Get("platform").Truthy())
	{
		V Platform = V::Object();
		Platform.Set("goals", StringArray({"housing", "health", "trust"}));
		Platform.Set("signature", Str("housing"));
		Platform.Set("chosen", Num(S.Get("quarter").AsNumber() - 1));
		Exec.Set("platform", std::move(Platform));
	}
	const auto Changes = [&](const std::string& Text) { Report["changes"].Push(Str(Text)); };
	for (const V& Def : X("regions").Items())
	{
		V& R = Exec["regions"][Def.Get("id").AsString()];
		for (const char* K : {"housing", "jobs", "health", "energy"})
		{
			R.Set(K, Num(Bound(R.Get(K).AsNumber() + (S.Get("metrics").Get(K).AsNumber() + Def.Get("offset").Get(K).AsNumber() - R.Get(K).AsNumber()) * .12)));
		}
		R.Set("goodwill", Num(R.Get("goodwill").AsNumber() * .9));
	}
	for (V& B : Exec["bills"].Items())
	{
		if (B.Get("status").AsString() != "passed" || B.Get("progress").AsNumber() >= 100) continue;
		const V& Def = *Bill(B.Get("id").AsString());
		const V& TeamState = Exec.Get("cabinet").Get(Def.Get("sponsor").AsString());
		const double Step = TeamState.Get("status").AsString() == "resigned" ? 12 : JsRound(18 + TeamState.Get("competence").AsNumber() * .2);
		const double Gain = std::min(Step, 100 - B.Get("progress").AsNumber());
		B.Set("progress", Num(B.Get("progress").AsNumber() + Gain));
		bool bAllRegions = false, bProtected = false;
		for (const V& A : X("amendments").Items())
		{
			if (!Contains(B.Get("amendments"), A.Get("id").AsString())) continue;
			if (A.Get("allRegions").Truthy()) bAllRegions = true;
			if (A.Get("preventsInvestigation").Truthy()) bProtected = true;
		}
		std::vector<std::string> Areas;
		if (bAllRegions)
		{
			for (const V& R : X("regions").Items()) Areas.push_back(R.Get("id").AsString());
		}
		else
		{
			for (const V& R : Def.Get("regions").Items()) Areas.push_back(R.AsString());
		}
		const std::string Metric = Def.Get("metric").AsString();
		for (const std::string& Id : Areas)
		{
			V& R = Exec["regions"][Id];
			if (R.Has(Metric)) R.Set(Metric, Num(Bound(R.Get(Metric).AsNumber() + Gain * .09)));
			R.Set("jobs", Num(Bound(R.Get("jobs").AsNumber() + Gain * .02)));
			News(S, Id, Def.Get("name").AsString() + ": " + Fmt(B.Get("progress").AsNumber()) + "% delivered.");
		}
		if (B.Get("progress").AsNumber() >= 100)
		{
			Changes(Def.Get("name").AsString() + ": delivery complete.");
			Record(S, "delivery", Def.Get("name").AsString(), "Construction and service rollout complete.");
		}
		bool bInvestigated = false;
		for (const V& C : Exec.Get("crises").Items())
		{
			if (C.Get("id").AsString() == "contracts") bInvestigated = true;
		}
		if (B.Get("id").AsString() == "housing" && B.Get("progress").AsNumber() >= 50 && !bProtected && !bInvestigated)
		{
			V Crisis = V::Object();
			Crisis.Set("id", Str("contracts"));
			Crisis.Set("region", Str("coast"));
			Crisis.Set("stage", Str("investigation"));
			Crisis.Set("since", S.Get("quarter"));
			Crisis.Set("response", V::Null());
			Exec["crises"].Push(std::move(Crisis));
			Changes("Housing procurement contracts face an investigation.");
		}
	}
	for (std::size_t Index = 0; Index < Exec.Get("cabinet").Keys().size(); ++Index)
	{
		const std::string Id = Exec.Get("cabinet").Keys()[Index];
		V& P = Exec["cabinet"][Id];
		const std::string Portfolio = Team(Id)->Get("portfolio").AsString();
		P.Set("strain", Num(S.Get("relationships").Get(Id).AsNumber() < 30 ? P.Get("strain").AsNumber() + 1 : 0));
		if (P.Get("strain").AsNumber() == 1 && P.Get("status").AsString() == "serving")
			Changes(Portfolio + ": your colleague publicly questions the administration. Repair the relationship before the next quarter.");
		if (P.Get("strain").AsNumber() >= 2 && P.Get("status").AsString() == "serving")
		{
			P.Set("status", Str("resigned"));
			Changes(Portfolio + ": " + (Id == "chen" || Id == "brooks" ? "the secretary resigns" : "your ally withdraws cooperation") + ". Delivery slows until a replacement is appointed.");
			Record(S, "cabinet", Portfolio, "Support withdrawn after two quarters of strained relations.");
		}
	}
	for (V& C : Exec["crises"].Items())
	{
		if (C.Get("stage").AsString() == "resolved" || !(C.Get("since").AsNumber() < S.Get("quarter").AsNumber())) continue;
		V& R = Exec["regions"][C.Get("region").AsString()];
		const std::string Stage = C.Get("stage").AsString();
		const std::string Response = C.Get("response").AsString();
		if (Stage == "warning")
		{
			const double Damage = std::max(2.0, 13 - S.Get("levels").Get("grid").AsNumber() * 2 - (Response == "prepare" ? 5 : 0));
			C.Set("damage", Num(Damage));
			R.Set("housing", Num(Bound(R.Get("housing").AsNumber() - Damage)));
			R.Set("energy", Num(Bound(R.Get("energy").AsNumber() - Damage)));
			R.Set("goodwill", Num(Bound(R.Get("goodwill").AsNumber() - 2, -15, 15)));
			C.Set("stage", Str("landfall"));
			Changes("The coastal storm makes landfall. Local housing and energy fall " + Fmt(Damage) + " points.");
			News(S, C.Get("region").AsString(), "Storm damage: " + Fmt(Damage) + " points. Relief decisions are on the president’s desk.");
		}
		else if (Stage == "landfall")
		{
			if (!C.Get("response").Truthy())
			{
				R.Set("goodwill", Num(Bound(R.Get("goodwill").AsNumber() - 5, -15, 15)));
				S["metrics"].Set("trust", Num(Bound(S.Get("metrics").Get("trust").AsNumber() - 3)));
				Changes("Delayed storm relief draws criticism from coastal governors.");
			}
			if (Response == "full" && S.Get("levels").Get("transparency").AsNumber() < 3)
			{
				C.Set("stage", Str("investigation"));
				Changes("Emergency contracts trigger a procurement investigation.");
			}
			else
			{
				C.Set("stage", Str("resolved"));
				Changes("Coastal recovery moves to local agencies.");
			}
		}
		else
		{
			if (!C.Get("response").Truthy())
			{
				S["metrics"].Set("trust", Num(Bound(S.Get("metrics").Get("trust").AsNumber() - 3)));
				V& Opposition = Exec["opposition"];
				Opposition.Set("momentum", Num(Bound(Opposition.Get("momentum").AsNumber() + 5)));
			}
			C.Set("stage", Str("resolved"));
			Changes(Response == "publish" ? "The procurement report is published; corrective work begins." : "The procurement controversy damages confidence in the administration.");
		}
		C.Set("response", V::Null());
		C.Set("since", S.Get("quarter"));
	}
	bool bStorm = false;
	for (const V& C : Exec.Get("crises").Items())
	{
		if (C.Get("id").AsString() == "storm") bStorm = true;
	}
	const double Q = S.Get("quarter").AsNumber();
	if (Q >= 2 && !bStorm && (Q >= 3 || S.Get("metrics").Get("energy").AsNumber() < 50 || S.Get("metrics").Get("climate").AsNumber() < 44))
	{
		V Crisis = V::Object();
		Crisis.Set("id", Str("storm"));
		Crisis.Set("region", Str("coast"));
		Crisis.Set("stage", Str("warning"));
		Crisis.Set("since", S.Get("quarter"));
		Crisis.Set("response", V::Null());
		Exec["crises"].Push(std::move(Crisis));
		Changes("Storm watch issued for the coast. Preparation is available in the Situation Room.");
	}
	std::string Issue = "housing";
	for (const char* K : {"health", "jobs", "trust"})
	{
		if (S.Get("metrics").Get(K).AsNumber() < S.Get("metrics").Get(Issue).AsNumber()) Issue = K;
	}
	V& Opposition = Exec["opposition"];
	Opposition.Set("issue", Str(Issue));
	Opposition.Set("momentum", Num(Bound(Opposition.Get("momentum").AsNumber() + (S.Get("metrics").Get(Issue).AsNumber() < 45 ? 3 : -1) + (S.Get("debt").AsNumber() > 130 ? 2 : 0))));
	for (V& E : Exec["encounters"].Items())
	{
		if (E.Get("done").Truthy() || !(E.Get("quarter").AsNumber() < Q)) continue;
		E.Set("done", Bool(true));
		E.Set("abandoned", Bool(true));
		S["metrics"].Set("trust", Num(Bound(S.Get("metrics").Get("trust").AsNumber() - 1)));
		Changes("An unfinished media appearance draws questions about transparency.");
	}
}

// advance(s): resolves the quarter and returns the report.
V Simulation::Advance(V& S) const
{
	if (S.Get("ended").Truthy() || S.Get("agendaChoice").IsNull()) return Bool(false);
	Upgrade(S);
	V Report = V::Object();
	Report.Set("quarter", Num(S.Get("quarter").AsNumber() + 1));
	Report.Set("changes", V::Array());
	Report.Set("situations", V::Array());
	Report.Set("budget", V::Null());
	Report.Set("election", V::Null());
	S.Set("quarter", Num(S.Get("quarter").AsNumber() + 1));
	const double Q = S.Get("quarter").AsNumber();
	std::vector<std::size_t> Open;
	for (std::size_t Index = 0; Index < S.Get("promises").Size(); ++Index)
	{
		if (S.Get("promises")[Index].Get("status").AsString() == "open") Open.push_back(Index);
	}
	for (const std::size_t Index : Open)
	{
		const V& P = S.Get("promises")[Index];
		if (S.Get("levels").Get(P.Get("policy").AsString()).AsNumber() >= P.Get("target").AsNumber()) Report["changes"].Push(Str(ResolvePromise(S, Index, true)));
		else if (Q >= P.Get("due").AsNumber()) Report["changes"].Push(Str(ResolvePromise(S, Index, false)));
	}
	V& Boosts = S["campaign"]["boosts"];
	for (V& Boost : Boosts.Values()) Boost = Num(Boost.AsNumber() * .75);
	for (const V& P : PolicyList.Items())
	{
		const std::string Id = P.Get("id").AsString();
		const double Current = S.Get("implemented").Get(Id).AsNumber(), Target = S.Get("levels").Get(Id).AsNumber();
		S["implemented"].Set(Id, Num(Current + (Target - Current) * std::min(1.0, StaffFactor(S, Id) / P.Get("lag").AsNumber())));
	}
	const V& Labels = D("metrics").Get("labels");
	const V& Base = D("metrics").Get("base");
	V Incoming = V::Object();
	for (const std::string& K : Base.Keys()) Incoming.Set(K, Num(0));
	const auto Add = [&](const std::string& K, double Value) { Incoming.Set(K, Num(Incoming.Get(K).AsNumber() + Value)); };
	for (const V& P : PolicyList.Items())
	{
		for (const auto& Entry : NumberEntries(P.Get("effects"))) Add(Entry.first, (S.Get("implemented").Get(P.Get("id").AsString()).AsNumber() - 2) * Entry.second);
	}
	V Remaining = V::Array();
	const V Effects = S.Get("effects");
	for (const V& Effect : Effects.Items())
	{
		if (Effect.Get("due").AsNumber() != Q) continue;
		std::vector<std::string> Parts;
		for (const auto& Entry : NumberEntries(Effect.Get("delta")))
		{
			Add(Entry.first, Entry.second);
			Parts.push_back(Labels.Get(Entry.first).AsString() + " " + (Entry.second > 0 ? "+" : "") + Fmt(Entry.second));
		}
		Report["changes"].Push(Effect.Get("title"));
		Record(S, "effect", Effect.Get("title").AsString(), Join(Parts, " · "));
	}
	for (const V& Effect : S.Get("effects").Items())
	{
		if (Effect.Get("due").AsNumber() > Q) Remaining.Push(Effect);
	}
	S.Set("effects", std::move(Remaining));
	// Active situations and unrest are part of the web: they push on conditions until they clear.
	for (const V* Sit : ActiveSituations(S))
	{
		for (const auto& Entry : NumberEntries(Sit->Get("effects"))) Add(Entry.first, Entry.second);
	}
	V Pressure;
	double PressureRevenue = 0;
	UnrestPressure(S, Pressure, PressureRevenue);
	for (const auto& Entry : NumberEntries(Pressure)) Add(Entry.first, Entry.second);
	// Conditions feed back into each other, while inertia stops a lever from solving a crisis instantly.
	const V& M = S.Get("metrics");
	const double Debt = S.Get("debt").AsNumber();
	Add("growth", (M.Get("jobs").AsNumber() - 50) * .018 - std::max(0.0, Debt - 100) * .025);
	Add("jobs", (M.Get("growth").AsNumber() - 50) * .025);
	Add("housing", (M.Get("jobs").AsNumber() - 50) * .025);
	Add("health", (M.Get("housing").AsNumber() - 50) * .015);
	Add("energy", (M.Get("climate").AsNumber() - 50) * .018);
	const double LastBalance = S.Get("budget").Get("balance").NumberOr(0);
	Add("trust", (M.Get("jobs").AsNumber() - 50) * .03 + (M.Get("prices").AsNumber() - 50) * .025 - std::max(0.0, Debt - 110) * .02 - std::max(0.0, -LastBalance - 8) * .03);
	const V Previous = S.Get("metrics");
	for (const std::string& K : Base.Keys())
	{
		const double Current = S.Get("metrics").Get(K).AsNumber();
		S["metrics"].Set(K, Num(Bound(JsRound((Current + (Base.Get(K).AsNumber() + Incoming.Get(K).AsNumber() - Current) * .36) * 10) / 10, 0, 100)));
	}
	for (const std::string& K : Base.Keys())
	{
		const double Now = S.Get("metrics").Get(K).AsNumber(), Was = Previous.Get(K).AsNumber();
		if (std::fabs(Now - Was) >= 1.4) Report["changes"].Push(Str(Labels.Get(K).AsString() + " " + (Now > Was ? "rose" : "fell") + " to " + Fmt(JsRound(Now))));
	}
	const V B = Budget(S);
	S.Set("budget", B);
	S.Set("debt", Num(Bound(JsRound((S.Get("debt").AsNumber() - B.Get("balance").AsNumber() * .3) * 10) / 10, 0, 250)));
	Report.Set("budget", B);
	const V Before = S.Get("situations");
	S.Set("situations", StringArray(UpdateSituations(S)));
	Report.Set("situations", S.Get("situations"));
	for (const V& Name : S.Get("situations").Items())
	{
		if (!Contains(Before, Name.AsString())) Report["changes"].Push(Str("New situation: " + Name.AsString()));
	}
	for (const V& Name : Before.Items())
	{
		if (!Contains(S.Get("situations"), Name.AsString()) && Situation(Name.AsString())) Report["changes"].Push(Str("Situation eased: " + Name.AsString()));
	}
	ExecutiveQuarter(S, Report);
	UpdateUnrest(S, Report);
	const double CurrentPoll = Poll(S);
	V PollEntry = V::Object();
	PollEntry.Set("quarter", Num(Q));
	PollEntry.Set("approval", Num(CurrentPoll));
	S["polls"].Push(std::move(PollEntry));
	double SituationCapital = 0;
	for (const V* Sit : ActiveSituations(S)) SituationCapital = SituationCapital + Sit->Get("capital").NumberOr(0);
	S.Set("capital", Num(Bound(S.Get("capital").AsNumber() + (CurrentPoll >= 55 ? 4 : CurrentPoll >= 42 ? 3 : 2) + SituationCapital, 0, 20)));
	if (Q == 8)
	{
		const V Regional = Election(S, CurrentPoll);
		V Midterm = V::Object();
		Midterm.Set("vote", Num(CurrentPoll));
		Midterm.Set("majority", Regional.Get("won"));
		Midterm.Set("regional", Regional);
		S.Set("midterm", Midterm);
		S["executive"].Set("mandate", Num(Regional.Get("won").AsBool() ? 5 : -5));
		if (!Midterm.Get("majority").Truthy()) S.Set("capital", Num(Bound(S.Get("capital").AsNumber() - 3, 0, 20)));
		V ElectionReport = V::Object();
		ElectionReport.Set("type", Str("midterm"));
		for (std::size_t Index = 0; Index < Midterm.Keys().size(); ++Index) ElectionReport.Set(Midterm.Keys()[Index], Midterm.Values()[Index]);
		Report.Set("election", std::move(ElectionReport));
	}
	if (Q == 16)
	{
		S.Set("ended", Bool(true));
		V ElectionReport = V::Object();
		ElectionReport.Set("type", Str("general"));
		ElectionReport.Set("vote", Num(CurrentPoll));
		const V Result = Election(S, CurrentPoll);
		for (std::size_t Index = 0; Index < Result.Keys().size(); ++Index) ElectionReport.Set(Result.Keys()[Index], Result.Values()[Index]);
		Report.Set("election", ElectionReport);
		S["executive"].Set("finalElection", ElectionReport);
	}
	S.Set("agendaChoice", V::Null());
	if (!S.Get("ended").Truthy()) DrawEvent(S);
	const double Balance = B.Get("balance").AsNumber();
	Record(S, "report", "Quarter " + Fmt(Q) + " report", "Polling " + Fmt(CurrentPoll) + "% · budget " + (Balance >= 0 ? "+" : "") + Fmt(Balance) + " · debt " + Fmt(JsRound(S.Get("debt").AsNumber())));
	return Report;
}
} // namespace FourYears
