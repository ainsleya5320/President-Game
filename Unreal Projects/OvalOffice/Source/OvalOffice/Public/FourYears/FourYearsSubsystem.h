// Owns the Four Years simulation for the running game: loads the shared JSON game data, keeps the current
// term, and saves it in the browser game's save format.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FourYears/Core/FourYearsSim.h"
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

	FourYears::Simulation Core;
	FourYears::JsonValue State;
	bool bReady = false;
	FString DataError;
};
