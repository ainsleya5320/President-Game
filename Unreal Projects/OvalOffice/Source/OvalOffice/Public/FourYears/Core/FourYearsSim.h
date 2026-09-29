// Four Years core: the presidency simulation, ported line for line from the browser game's
// simulation-core.js and executive-systems.js. Standard C++17 only, so it builds and is tested outside Unreal.
//
// The game state is a JsonValue with exactly the shape of a browser save, so saves move between the
// browser game and Unreal, and parity with the JavaScript can be checked state for state
// (see Tests/FourYearsCore). Actions return the same result objects as the JavaScript API:
// {ok:true, detail} or {ok:false, reason}.
#pragma once

#include "FourYears/Core/FourYearsJson.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace FourYears
{
class Simulation
{
public:
	Simulation();
	~Simulation();
	Simulation(const Simulation&) = delete;
	Simulation& operator=(const Simulation&) = delete;

	// Loads data/*.json given as (file stem, file text) pairs, e.g. ("events", "[...]"), then validates it.
	// On failure Errors lists every problem found and the simulation must not be used.
	bool LoadData(const std::vector<std::pair<std::string, std::string>>& Files, std::vector<std::string>& Errors);
	bool IsLoaded() const { return bLoaded; }
	const JsonValue& Data() const { return GameData; }

	// Terms and saves.
	JsonValue Fresh(double Seed = 6137) const;
	// Upgrades a save from any earlier browser version (or starts fresh for anything unreadable).
	JsonValue Migrate(JsonValue Old) const;

	// Quarterly decision and report.
	JsonValue Choose(JsonValue& S, double Index) const;  // Boolean.
	JsonValue Advance(JsonValue& S) const;               // The quarterly report, or false.

	// Actions.
	JsonValue ChangePolicy(JsonValue& S, const std::string& Id, double Level) const; // {ok, cost} or {ok:false, reason}
	JsonValue Meeting(JsonValue& S, const std::string& PersonId, const std::string& Response) const;
	JsonValue CampaignTrip(JsonValue& S, const std::string& RegionId, const std::string& IssueId, const std::string& PersonId) const;
	JsonValue SetPlatform(JsonValue& S, const std::vector<std::string>& GoalIds, const std::string& Signature) const;
	JsonValue Propose(JsonValue& S, const std::string& BillId) const;
	JsonValue Amend(JsonValue& S, const std::string& AmendmentId) const;
	JsonValue Lobby(JsonValue& S, const std::string& BlocId) const;
	JsonValue Vote(JsonValue& S) const;
	JsonValue RegionalVisit(JsonValue& S, const std::string& RegionId) const;
	JsonValue TeamAction(JsonValue& S, const std::string& PersonId, const std::string& Action) const;
	JsonValue RespondCrisis(JsonValue& S, const std::string& CrisisId, const std::string& Choice) const;
	JsonValue StartEncounter(JsonValue& S, const std::string& Type) const;
	JsonValue AnswerEncounter(JsonValue& S, const std::string& Choice) const;

	// Queries used by the interface.
	const JsonValue* CurrentEvent(const JsonValue& S) const;
	const JsonValue* Situation(const std::string& Name) const;
	JsonValue Electorate(const JsonValue& S) const; // {poll, overlap, groups}
	const JsonValue& VoterAtlas(const JsonValue& S) const;
	double Poll(const JsonValue& S) const;
	JsonValue Groups(const JsonValue& S) const;
	JsonValue Unrest(const JsonValue& S) const;
	JsonValue Budget(const JsonValue& S) const;
	JsonValue Election(const JsonValue& S, double BasePoll) const;
	JsonValue RegionalView(const JsonValue& S, const std::string& RegionId, double BasePoll) const;
	JsonValue Legacy(const JsonValue& S) const;
	JsonValue People(const JsonValue& S) const;
	JsonValue Votes(const JsonValue& S, const JsonValue& Bill) const;
	const JsonValue* ActiveBill(const JsonValue& S) const;
	JsonValue CrisisOptions(const JsonValue& Crisis) const;
	JsonValue EncounterOptions(const JsonValue& S, const JsonValue& Encounter) const;
	int AvailableSlots(const JsonValue& S) const;
	bool CampaignSeason(const JsonValue& S) const;
	bool EventEligible(const JsonValue& S, const JsonValue& Event) const;
	std::vector<std::string> UpdateSituations(const JsonValue& S) const;
	// What an adviser asks for this quarter: {person, policy, active?, target, done}, or undefined for an
	// unknown adviser. ActivePromise receives the index of their open promise in S.promises, or -1.
	JsonValue Request(const JsonValue& S, const std::string& PersonId, int& ActivePromise) const;
	// data/policies.json with each policy's implementation lag filled in.
	const JsonValue& Policies() const { return PolicyList; }

private:
	struct FPopulation;
	struct FVoter;
	const FPopulation& Population(double Seed) const;

	bool Validate(std::vector<std::string>& Errors) const;
	const JsonValue& D(const char* Key) const { return GameData.Get(Key); }
	const JsonValue& X(const char* Key) const { return GameData.Get("executive").Get(Key); }
	const JsonValue* Policy(const std::string& Id) const;
	const JsonValue* Group(const std::string& Id) const;
	const JsonValue* Bill(const std::string& Id) const;
	const JsonValue* Team(const std::string& Id) const;
	JsonValue Person(const JsonValue& S, const JsonValue& Base) const;
	std::vector<const JsonValue*> ActiveSituations(const JsonValue& S) const;
	double MetricValue(const JsonValue& S, const std::string& Key) const;
	JsonValue GroupMoods(const JsonValue& S) const;
	double MembershipFactor(const JsonValue& S, const JsonValue& Group) const;
	int UnrestStageIndex(double Anger) const;
	void UnrestPressure(const JsonValue& S, JsonValue& Effects, double& Revenue) const;
	void UpdateUnrest(JsonValue& S, JsonValue& Report) const;
	const JsonValue* DrawEvent(JsonValue& S) const;
	JsonValue LegacyDeck(const JsonValue& S) const;
	JsonValue PoliticalDefaults() const;
	void Upgrade(JsonValue& S) const;
	void Initialize(JsonValue& S) const;
	std::string ResolvePromise(JsonValue& S, std::size_t PromiseIndex, bool bKept) const;
	double Modifier(const JsonValue& S, const std::string& GroupId) const;
	double Spending(const JsonValue& S) const;
	double StaffFactor(const JsonValue& S, const std::string& PolicyId) const;
	void ExecutiveQuarter(JsonValue& S, JsonValue& Report) const;
	int Slots(const JsonValue& S) const;
	bool Allowed(const JsonValue& S) const;

	JsonValue GameData;
	JsonValue PolicyList; // data/policies.json with each policy's implementation lag filled in.
	bool bLoaded = false;
	mutable std::map<unsigned int, std::unique_ptr<FPopulation>> Populations;
	mutable std::string ElectoralCacheKey;
	mutable JsonValue ElectoralCache;
};
} // namespace FourYears
