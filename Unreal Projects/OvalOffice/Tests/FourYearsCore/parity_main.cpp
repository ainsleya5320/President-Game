// Replays the browser game's reference trace through the C++ simulation core and reports any difference.
// Build and run with: node Tests/FourYearsCore/run_parity.cjs
#include "FourYears/Core/FourYearsSim.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using FourYears::JsonValue;

namespace
{
std::string ReadFile(const std::filesystem::path& Path)
{
	std::ifstream In(Path, std::ios::binary);
	std::stringstream Buffer;
	Buffer << In.rdbuf();
	return Buffer.str();
}

// Structural comparison: key order is ignored and undefined members count as absent, like JSON.stringify.
bool Same(const JsonValue& A, const JsonValue& B, const std::string& Path, std::string& Where)
{
	const bool NullA = A.IsNullish(), NullB = B.IsNullish();
	if (A.IsUndefined() != B.IsUndefined() || (NullA && NullB && A.IsNull() != B.IsNull()))
	{
		Where = Path + ": " + A.Dump() + " vs " + B.Dump();
		return false;
	}
	if (A.Type() != B.Type())
	{
		Where = Path + ": " + A.Dump() + " vs " + B.Dump();
		return false;
	}
	switch (A.Type())
	{
	case JsonValue::EType::Number:
		if (A.AsNumber() != B.AsNumber())
		{
			Where = Path + ": " + A.Dump() + " vs " + B.Dump();
			return false;
		}
		return true;
	case JsonValue::EType::Bool:
	case JsonValue::EType::String:
		if (A.Dump() != B.Dump())
		{
			Where = Path + ": " + A.Dump() + " vs " + B.Dump();
			return false;
		}
		return true;
	case JsonValue::EType::Array:
		if (A.Size() != B.Size())
		{
			Where = Path + ": array length " + std::to_string(A.Size()) + " vs " + std::to_string(B.Size());
			return false;
		}
		for (std::size_t Index = 0; Index < A.Size(); ++Index)
		{
			if (!Same(A[Index], B[Index], Path + "[" + std::to_string(Index) + "]", Where)) return false;
		}
		return true;
	case JsonValue::EType::Object:
		for (const std::string& Key : A.Keys())
		{
			if (!Same(A.Get(Key), B.Get(Key), Path + "." + Key, Where)) return false;
		}
		for (const std::string& Key : B.Keys())
		{
			if (!A.Has(Key) && !B.Get(Key).IsUndefined())
			{
				Where = Path + "." + Key + ": missing vs " + B.Get(Key).Dump();
				return false;
			}
		}
		return true;
	default:
		return true;
	}
}

void Merge(JsonValue& Target, const JsonValue& Patch)
{
	for (std::size_t Index = 0; Index < Patch.Keys().size(); ++Index)
	{
		const std::string& Key = Patch.Keys()[Index];
		const JsonValue& Value = Patch.Values()[Index];
		JsonValue* Existing = Target.Find(Key);
		if (Value.IsObject() && Existing && Existing->IsObject()) Merge(*Existing, Value);
		else Target.Set(Key, Value);
	}
}

std::vector<std::string> Strings(const JsonValue& Array)
{
	std::vector<std::string> Out;
	for (const JsonValue& Item : Array.Items()) Out.push_back(Item.AsString());
	return Out;
}

// Mirrors derived() in Prototype/parity/make_trace.cjs.
JsonValue Derived(const FourYears::Simulation& Sim, const JsonValue& S)
{
	const JsonValue Electorate = Sim.Electorate(S);
	JsonValue Out = JsonValue::Object();
	Out.Set("poll", Electorate.Get("poll"));
	Out.Set("overlap", Electorate.Get("overlap"));
	JsonValue Groups = JsonValue::Array();
	for (const JsonValue& G : Electorate.Get("groups").Items())
	{
		JsonValue Row = JsonValue::Object();
		for (const char* Key : {"id", "approval", "turnout", "share", "support", "anger", "stage"}) Row.Set(Key, G.Get(Key));
		Groups.Push(Row);
	}
	Out.Set("groups", Groups);
	Out.Set("budget", Sim.Budget(S));
	Out.Set("election", Sim.Election(S, Electorate.Get("poll").AsNumber()));
	const JsonValue* Event = Sim.CurrentEvent(S);
	Out.Set("event", Event ? Event->Get("id") : JsonValue::Null());
	Out.Set("slots", JsonValue::Number(Sim.AvailableSlots(S)));
	Out.Set("legacy", Sim.Legacy(S));
	return Out;
}

JsonValue Run(const FourYears::Simulation& Sim, JsonValue& S, const std::string& Action, const JsonValue& Args)
{
	const auto A = [&](std::size_t Index) { return Args[Index].AsString(); };
	if (Action == "changePolicy") return Sim.ChangePolicy(S, A(0), Args[1].AsNumber());
	if (Action == "meeting") return Sim.Meeting(S, A(0), A(1));
	if (Action == "campaignTrip") return Sim.CampaignTrip(S, A(0), A(1), A(2));
	if (Action == "platform") return Sim.SetPlatform(S, Strings(Args[0]), A(1));
	if (Action == "propose") return Sim.Propose(S, A(0));
	if (Action == "amend") return Sim.Amend(S, A(0));
	if (Action == "lobby") return Sim.Lobby(S, A(0));
	if (Action == "vote") return Sim.Vote(S);
	if (Action == "regionalVisit") return Sim.RegionalVisit(S, A(0));
	if (Action == "teamAction") return Sim.TeamAction(S, A(0), A(1));
	if (Action == "respondCrisis") return Sim.RespondCrisis(S, A(0), A(1));
	if (Action == "startEncounter") return Sim.StartEncounter(S, A(0));
	if (Action == "answerEncounter") return Sim.AnswerEncounter(S, A(0));
	if (Action == "setLocation")
	{
		S.Set("location", Args[0]);
		JsonValue Ok = JsonValue::Object();
		Ok.Set("ok", JsonValue::Boolean(true));
		return Ok;
	}
	if (Action == "choose") return Sim.Choose(S, Args[0].AsNumber());
	if (Action == "advance") return Sim.Advance(S);
	std::printf("Unknown action %s\n", Action.c_str());
	return JsonValue();
}
} // namespace

int main(int Argc, char** Argv)
{
	if (Argc < 3)
	{
		std::printf("usage: parity <data directory> <trace.json>\n");
		return 2;
	}
	std::vector<std::pair<std::string, std::string>> Files;
	for (const auto& Entry : std::filesystem::directory_iterator(Argv[1]))
	{
		if (Entry.path().extension() == ".json") Files.emplace_back(Entry.path().stem().string(), ReadFile(Entry.path()));
	}
	FourYears::Simulation Sim;
	std::vector<std::string> Errors;
	if (!Sim.LoadData(Files, Errors))
	{
		for (const std::string& Error : Errors) std::printf("%s\n", Error.c_str());
		return 1;
	}
	JsonValue Trace;
	std::string ParseError;
	if (!JsonValue::Parse(ReadFile(Argv[2]), Trace, ParseError))
	{
		std::printf("trace: %s\n", ParseError.c_str());
		return 1;
	}
	int Failed = 0, Steps = 0, States = 0;
	for (const JsonValue& Scenario : Trace.Get("scenarios").Items())
	{
		const std::string Label = Scenario.Get("label").AsString();
		const JsonValue& Start = Scenario.Get("start");
		JsonValue S = Start.Has("fresh") ? Sim.Fresh(Start.Get("fresh").AsNumber()) : Sim.Migrate(Start.Get("migrate"));
		if (Start.Has("patch")) Merge(S, Start.Get("patch"));
		std::string Where;
		if (!Same(S, Scenario.Get("initial"), "state", Where) || !Same(Derived(Sim, S), Scenario.Get("initialDerived"), "derived", Where))
		{
			std::printf("FAIL %s at start: %s\n", Label.c_str(), Where.c_str());
			++Failed;
			continue;
		}
		const JsonValue& ScenarioSteps = Scenario.Get("steps");
		for (std::size_t Index = 0; Index < ScenarioSteps.Size(); ++Index)
		{
			const JsonValue& Step = ScenarioSteps[Index];
			const std::string Action = Step.Get("action").AsString();
			const JsonValue Result = Run(Sim, S, Action, Step.Get("args"));
			++Steps;
			bool bOk = Same(Result, Step.Get("result"), "result", Where);
			if (bOk && Step.Has("state"))
			{
				++States;
				bOk = Same(S, Step.Get("state"), "state", Where) && Same(Derived(Sim, S), Step.Get("derived"), "derived", Where);
			}
			if (!bOk)
			{
				std::printf("FAIL %s, step %zu (%s %s): %s\n", Label.c_str(), Index, Action.c_str(), Step.Get("args").Dump().c_str(), Where.c_str());
				++Failed;
				break;
			}
		}
	}
	std::printf("%s: %zu terms, %d actions and %d quarter-end states compared, %d failing terms.\n", Failed ? "PARITY FAILED" : "Parity passed", Trace.Get("scenarios").Size(), Steps, States, Failed);
	return Failed ? 1 : 0;
}
