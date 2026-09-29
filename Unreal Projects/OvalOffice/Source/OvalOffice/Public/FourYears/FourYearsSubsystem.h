// Owns the Four Years simulation for the running game: loads the shared JSON game data, keeps the current
// term, and saves it in the browser game's save format.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FourYears/Core/FourYearsSim.h"
#include "Engine/Texture2D.h"
#include "FourYearsSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FFourYearsChoice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Four Years")
	FString Label;

	// Budget cost and effects, e.g. "Budget cost 7 · Energy reliability +5 · Institutional trust +2".
	UPROPERTY(BlueprintReadOnly, Category = "Four Years")
	FString Summary;
};

USTRUCT(BlueprintType)
struct FFourYearsBriefing
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Quarter = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString QuarterLabel;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Title;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Body;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") TArray<FFourYearsChoice> Choices;
	// -1 until the quarterly response is filed.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 ChosenIndex = -1;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Approval = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Capital = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 BudgetBalance = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Debt = 0;
	// Active situations and unrest, e.g. "Recession" or "Climate advocates: extremists".
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") TArray<FString> Alerts;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") bool bTermOver = false;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Outcome;
};

USTRUCT(BlueprintType)
struct FFourYearsReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Quarter = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") TArray<FString> Changes;
	// Empty unless an election was held this quarter.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Election;
};

USTRUCT(BlueprintType)
struct FFourYearsActionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Four Years") bool bOk = false;
	// What happened, or why the action was refused.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Message;
};

// Capital, appointments and whether this quarter's decision is already filed.
USTRUCT(BlueprintType)
struct FFourYearsStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Capital = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 AppointmentsLeft = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Approval = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 BudgetBalance = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") bool bDecisionFiled = false;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") bool bTermOver = false;
};

USTRUCT(BlueprintType)
struct FFourYearsPolicy
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Id;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Name;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Department;
	// Setting from 0 to 4; 2 is the starting level.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Level = 2;
	// How far implementation has caught up with the setting.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") float Implemented = 2.f;
	// Budget units per level above 2: spending when positive, revenue when negative.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 BudgetPerLevel = 0;
	// Political capital for each one-step change.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 CapitalPerStep = 2;
	// Quarters for a change to take full effect.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Lag = 1;
	// Full strength (level 4) needs an act of Congress that has not passed yet.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") bool bNeedsAct = false;
	// Per-level effects, e.g. "Public health +2.7 · Economic mobility +1.1".
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Effects;
};

USTRUCT(BlueprintType)
struct FFourYearsMeetingOption
{
	GENERATED_BODY()

	// The response id the simulation expects: promise, compromise, listen, reassure, extend or withdraw.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Response;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Label;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Detail;
};

USTRUCT(BlueprintType)
struct FFourYearsAdviser
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Id;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Name;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Role;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Initials;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Quote;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Relationship = 50;
	// Resigned advisers cannot meet until an acting replacement is appointed; acting replacements use their own name.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") bool bResigned = false;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") bool bActing = false;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") bool bMetThisQuarter = false;
	// What they want this quarter, e.g. "Affordable construction at 3/4 (now 2/4)".
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Request;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString RequestedPolicyId;
	// Their open promise, if any, e.g. "Affordable construction at 3/4 by quarter 4".
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Promise;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") TArray<FFourYearsMeetingOption> Options;
};

USTRUCT(BlueprintType)
struct FFourYearsBloc
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Id;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Name;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Seats = 0;
	// Projected support for the active bill, 0-100, and the yes votes it brings.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Support = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 YesVotes = 0;
	// What the faction wants, e.g. "Independent oversight and a credible rollout".
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Want;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") bool bLobbied = false;
};

USTRUCT(BlueprintType)
struct FFourYearsAmendment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Id;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Name;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Detail;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") bool bAdopted = false;
};

USTRUCT(BlueprintType)
struct FFourYearsBill
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Id;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Name;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Story;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Sponsor;
	// The policy it takes to full strength.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Policy;
	// One-off debt when it passes.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Cost = 0;
	// "available", "draft", "failed" or "passed".
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FString Status;
	// Delivery progress once passed, 0-100.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Progress = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 Attempts = 0;
	// Yes votes in the most recent floor vote, or -1 before any vote.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 LastYes = -1;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") bool bVotedThisQuarter = false;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") TArray<FString> Amendments;
};

// The chamber: 100 seats, 51 votes to pass, one bill on the floor at a time.
USTRUCT(BlueprintType)
struct FFourYearsCongress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Four Years") bool bHasActiveBill = false;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") FFourYearsBill ActiveBill;
	// Whip count for the active bill.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") TArray<FFourYearsBloc> Blocs;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 ProjectedYes = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") TArray<FFourYearsAmendment> Amendments;
	// Bills that can still be introduced, and bills already law.
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") TArray<FFourYearsBill> Available;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") TArray<FFourYearsBill> Passed;
	UPROPERTY(BlueprintReadOnly, Category = "Four Years") int32 OppositionMomentum = 0;
};

UCLASS()
class OVALOFFICE_API UFourYearsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// False when the game data could not be loaded; see GetDataError.
	UFUNCTION(BlueprintPure, Category = "Four Years")
	bool IsReady() const { return bReady; }

	UFUNCTION(BlueprintPure, Category = "Four Years")
	FString GetDataError() const { return DataError; }

	UFUNCTION(BlueprintCallable, Category = "Four Years")
	void StartNewTerm();

	UFUNCTION(BlueprintPure, Category = "Four Years")
	FFourYearsBriefing GetBriefing() const;

	// Files the quarterly response. Returns false if the choice is not available.
	UFUNCTION(BlueprintCallable, Category = "Four Years")
	bool ChooseResponse(int32 ChoiceIndex);

	// Resolves the quarter once a response is filed, saves, and returns the report.
	UFUNCTION(BlueprintCallable, Category = "Four Years")
	FFourYearsReport AdvanceQuarter();

	UFUNCTION(BlueprintCallable, Category = "Four Years")
	bool SaveTerm() const;

	UFUNCTION(BlueprintPure, Category = "Four Years")
	FFourYearsStatus GetStatus() const;

	UFUNCTION(BlueprintPure, Category = "Four Years")
	TArray<FFourYearsPolicy> GetPolicies() const;

	// Moves a policy to a new level (0-4), spending political capital. Saves on success.
	UFUNCTION(BlueprintCallable, Category = "Four Years")
	FFourYearsActionResult SetPolicyLevel(const FString& PolicyId, int32 Level);

	UFUNCTION(BlueprintPure, Category = "Four Years")
	TArray<FFourYearsAdviser> GetAdvisers() const;

	// Holds a meeting in the Oval Office. Uses one of the quarter's two appointments. Saves on success.
	UFUNCTION(BlueprintCallable, Category = "Four Years")
	FFourYearsActionResult MeetAdviser(const FString& AdviserId, const FString& Response);

	// Every promise made this term, newest first, e.g. "Kept · Affordable construction at 3/4 for Maya Chen".
	UFUNCTION(BlueprintPure, Category = "Four Years")
	TArray<FString> GetPromiseRecord() const;

	// The adviser's portrait from Prototype/assets/characters, or null (acting replacements have none).
	UFUNCTION(BlueprintCallable, Category = "Four Years")
	UTexture2D* GetPortrait(const FString& AdviserName);

	UFUNCTION(BlueprintPure, Category = "Four Years|Introduction")
	bool NeedsInauguration() const;

	UFUNCTION(BlueprintCallable, Category = "Four Years|Introduction")
	void CompleteInauguration();

	UFUNCTION(BlueprintCallable, Category = "Four Years|Introduction")
	UTexture2D* GetInaugurationImage(int32 Index);

	// Presentation-only lines are authored in Prototype/data/dialogue.json, separate from simulation rules.
	UFUNCTION(BlueprintPure, Category = "Four Years")
	FString GetDialogueLine(const FString& AdviserId, const FString& Cue) const;

	UFUNCTION(BlueprintPure, Category = "Four Years")
	FFourYearsCongress GetCongress() const;

	// Introduces a bill (2 political capital). Only one bill can be on the floor at a time.
	UFUNCTION(BlueprintCallable, Category = "Four Years")
	FFourYearsActionResult IntroduceBill(const FString& BillId);

	// Adds an amendment to the active bill (1 political capital).
	UFUNCTION(BlueprintCallable, Category = "Four Years")
	FFourYearsActionResult AddAmendment(const FString& AmendmentId);

	// Meets a faction about the active bill: +15 support, one appointment and 1 political capital.
	UFUNCTION(BlueprintCallable, Category = "Four Years")
	FFourYearsActionResult LobbyBloc(const FString& BlocId);

	// Calls the floor vote on the active bill (2 political capital, once per quarter). 51 votes pass it.
	UFUNCTION(BlueprintCallable, Category = "Four Years")
	FFourYearsActionResult CallVote();

	UFUNCTION(BlueprintCallable, Category = "Four Years|Diplomacy")
	FFourYearsActionResult DiplomacyAction(const FString& Region, const FString& Action);
	UFUNCTION(BlueprintCallable, Category = "Four Years|Diplomacy")
	FFourYearsActionResult RespondDiplomacyCrisis(int32 Index, const FString& Choice);
	UFUNCTION(BlueprintCallable, Category = "Four Years|Diplomacy")
	FFourYearsActionResult ChooseDiplomacyDoctrine(const FString& Doctrine);
	UFUNCTION(BlueprintPure, Category = "Four Years|Diplomacy")
	FString GetDiplomacyJson() const;
	UFUNCTION(BlueprintCallable, Category = "Four Years|Electorate")
	FFourYearsActionResult ChooseElectoralParty(const FString& Party);
	UFUNCTION(BlueprintCallable, Category = "Four Years|Electorate")
	FFourYearsActionResult OrganizeCounty(const FString& County);
	UFUNCTION(BlueprintPure, Category = "Four Years|Electorate")
	FString GetVoterAtlasJson() const;

	// Where the president is: "oval" or "aircraft". Meetings need the Oval Office.
	UFUNCTION(BlueprintCallable, Category = "Four Years")
	void SetLocation(const FString& Location);

	// Saves live in Saved/FourYears/Term.json using the browser game's format.
	UFUNCTION(BlueprintPure, Category = "Four Years")
	FString GetSavePath() const;

	// Direct access for C++ screens that need more than the briefing.
	const FourYears::Simulation& GetSimulation() const { return Core; }
	FourYears::JsonValue& GetState() { return State; }

private:
	bool LoadGameData();
	bool LoadTerm();
	FString QuarterLabel(int32 Quarter) const;

	FFourYearsActionResult ToResult(const FourYears::JsonValue& Result);
	FFourYearsBill DescribeBill(const FourYears::JsonValue& Definition, const FourYears::JsonValue* BillState) const;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UTexture2D>> Portraits;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UTexture2D>> InaugurationImages;

	FourYears::Simulation Core;
	FourYears::JsonValue State;
	bool bReady = false;
	FString DataError;
};
