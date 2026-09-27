#include "FourYears/FourYearsSubsystem.h"

#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogFourYears, Log, All);

namespace
{
using FourYears::JsonValue;

FString ToFString(const std::string& Text)
{
	return FString(UTF8_TO_TCHAR(Text.c_str()));
}

std::string ToUtf8(const FString& Text)
{
	const FTCHARToUTF8 Converted(*Text);
	return std::string(Converted.Get(), Converted.Length());
}

bool ReadUtf8File(const FString& Path, std::string& Out)
{
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path))
	{
		return false;
	}
	Out.assign(reinterpret_cast<const char*>(Bytes.GetData()), static_cast<std::size_t>(Bytes.Num()));
	return true;
}

FString Signed(double Value)
{
	return (Value > 0 ? TEXT("+") : TEXT("")) + ToFString(FourYears::FormatNumber(Value));
}
} // namespace

void UFourYearsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bReady = LoadGameData();
	if (!bReady)
	{
		UE_LOG(LogFourYears, Error, TEXT("%s"), *DataError);
		return;
	}
	if (!LoadTerm())
	{
		StartNewTerm();
	}
}

bool UFourYearsSubsystem::LoadGameData()
{
	// The browser game and Unreal share one copy of the content: Prototype/data. A packaged build can ship
	// the same files under Content/FourYears/Data.
	const TArray<FString> Candidates = {
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Prototype"), TEXT("data")),
		FPaths::Combine(FPaths::ProjectContentDir(), TEXT("FourYears"), TEXT("Data")),
	};
	for (const FString& Directory : Candidates)
	{
		TArray<FString> Names;
		IFileManager::Get().FindFiles(Names, *FPaths::Combine(Directory, TEXT("*.json")), true, false);
		if (Names.Num() == 0)
		{
			continue;
		}
		std::vector<std::pair<std::string, std::string>> Files;
		for (const FString& Name : Names)
		{
			std::string Text;
			if (!ReadUtf8File(FPaths::Combine(Directory, Name), Text))
			{
				DataError = FString::Printf(TEXT("Four Years could not read %s"), *FPaths::Combine(Directory, Name));
				return false;
			}
			Files.emplace_back(ToUtf8(FPaths::GetBaseFilename(Name)), std::move(Text));
		}
		std::vector<std::string> Errors;
		if (!Core.LoadData(Files, Errors))
		{
			DataError.Reset();
			for (const std::string& Error : Errors)
			{
				DataError += ToFString(Error) + TEXT("\n");
			}
			return false;
		}
		UE_LOG(LogFourYears, Log, TEXT("Loaded Four Years data from %s"), *Directory);
		return true;
	}
	DataError = TEXT("Four Years game data was not found. Expected Prototype/data/*.json next to OvalOffice.uproject.");
	return false;
}

FString UFourYearsSubsystem::GetSavePath() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("FourYears"), TEXT("Term.json"));
}

bool UFourYearsSubsystem::LoadTerm()
{
	std::string Text;
	if (!ReadUtf8File(GetSavePath(), Text))
	{
		return false;
	}
	JsonValue Saved;
	std::string Error;
	if (!JsonValue::Parse(Text, Saved, Error))
	{
		UE_LOG(LogFourYears, Warning, TEXT("Ignoring unreadable save %s: %s"), *GetSavePath(), *ToFString(Error));
		return false;
	}
	// Migrate upgrades saves from any earlier version, including saves copied from the browser game.
	State = Core.Migrate(std::move(Saved));
	return true;
}

bool UFourYearsSubsystem::SaveTerm() const
{
	if (!bReady)
	{
		return false;
	}
	const std::string Text = State.Dump();
	TArray<uint8> Bytes;
	Bytes.Append(reinterpret_cast<const uint8*>(Text.data()), static_cast<int32>(Text.size()));
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(GetSavePath()), true);
	return FFileHelper::SaveArrayToFile(Bytes, *GetSavePath());
}

void UFourYearsSubsystem::StartNewTerm()
{
	if (!bReady)
	{
		return;
	}
	const double Seed = static_cast<double>(FDateTime::Now().GetTicks() % 2147483647);
	State = Core.Fresh(Seed);
	SaveTerm();
}

FString UFourYearsSubsystem::QuarterLabel(int32 Quarter) const
{
	if (Quarter >= 16)
	{
		return TEXT("The term is complete");
	}
	return FString::Printf(TEXT("Year %d · Quarter %d of 16"), Quarter / 4 + 1, Quarter + 1);
}

FFourYearsBriefing UFourYearsSubsystem::GetBriefing() const
{
	FFourYearsBriefing Briefing;
	if (!bReady)
	{
		Briefing.Title = TEXT("Game data unavailable");
		Briefing.Body = DataError;
		return Briefing;
	}
	const JsonValue& Labels = Core.Data().Get("metrics").Get("labels");
	const JsonValue Budget = Core.Budget(State);
	Briefing.Quarter = static_cast<int32>(State.Get("quarter").AsNumber());
	Briefing.QuarterLabel = QuarterLabel(Briefing.Quarter);
	Briefing.Approval = static_cast<int32>(Core.Poll(State));
	Briefing.Capital = static_cast<int32>(State.Get("capital").AsNumber());
	Briefing.BudgetBalance = static_cast<int32>(Budget.Get("balance").AsNumber());
	Briefing.Debt = static_cast<int32>(FourYears::JsRound(State.Get("debt").AsNumber()));
	for (const JsonValue& Name : State.Get("situations").Items())
	{
		Briefing.Alerts.Add(ToFString(Name.AsString()));
	}
	for (const JsonValue& Group : Core.Unrest(State).Items())
	{
		Briefing.Alerts.Add(ToFString(Group.Get("name").AsString()) + TEXT(": ") + ToFString(Group.Get("stage").AsString()).ToLower());
	}
	Briefing.bTermOver = State.Get("ended").Truthy();
	if (Briefing.bTermOver)
	{
		const JsonValue& Final = State.Get("executive").Get("finalElection");
		Briefing.Title = Final.Get("won").Truthy() ? TEXT("Re-elected") : TEXT("Defeated");
		Briefing.Outcome = FString::Printf(TEXT("Final vote %s%% · %s of 100 regional electoral points."),
			*ToFString(FourYears::FormatNumber(Final.Get("vote").AsNumber())), *ToFString(FourYears::FormatNumber(Final.Get("points").AsNumber())));
		Briefing.Body = TEXT("Your four years are on the record.");
		return Briefing;
	}
	const JsonValue* Event = Core.CurrentEvent(State);
	if (!Event)
	{
		Briefing.Title = TEXT("No decision is waiting");
		return Briefing;
	}
	Briefing.Title = ToFString(Event->Get("title").AsString());
	Briefing.Body = ToFString(Event->Get("body").AsString());
	for (const JsonValue& Choice : Event->Get("choices").Items())
	{
		FFourYearsChoice Option;
		Option.Label = ToFString(Choice.Get("label").AsString());
		const double Cost = Choice.Get("cost").AsNumber();
		Option.Summary = Cost < 0 ? FString::Printf(TEXT("Budget savings %s"), *ToFString(FourYears::FormatNumber(-Cost)))
		                          : FString::Printf(TEXT("Budget cost %s"), *ToFString(FourYears::FormatNumber(Cost)));
		const JsonValue& Effects = Choice.Get("effects");
		for (size_t Index = 0; Index < Effects.Keys().size(); ++Index)
		{
			Option.Summary += TEXT(" · ") + ToFString(Labels.Get(Effects.Keys()[Index]).AsString()) + TEXT(" ") + Signed(Effects.Values()[Index].AsNumber());
		}
		Briefing.Choices.Add(Option);
	}
	const JsonValue& Chosen = State.Get("agendaChoice");
	Briefing.ChosenIndex = Chosen.IsNumber() ? static_cast<int32>(Chosen.AsNumber()) : -1;
	return Briefing;
}

bool UFourYearsSubsystem::ChooseResponse(int32 ChoiceIndex)
{
	if (!bReady || !Core.Choose(State, ChoiceIndex).AsBool())
	{
		return false;
	}
	SaveTerm();
	return true;
}

FFourYearsReport UFourYearsSubsystem::AdvanceQuarter()
{
	FFourYearsReport Report;
	if (!bReady)
	{
		return Report;
	}
	const JsonValue Result = Core.Advance(State);
	if (!Result.IsObject())
	{
		return Report;
	}
	SaveTerm();
	Report.Quarter = static_cast<int32>(Result.Get("quarter").AsNumber());
	for (const JsonValue& Change : Result.Get("changes").Items())
	{
		Report.Changes.Add(ToFString(Change.AsString()));
	}
	const JsonValue& Election = Result.Get("election");
	if (Election.IsObject())
	{
		const bool bMidterm = Election.Get("type").AsString() == "midterm";
		const bool bWon = bMidterm ? Election.Get("majority").Truthy() : Election.Get("won").Truthy();
		const JsonValue& Points = bMidterm ? Election.Get("regional").Get("points") : Election.Get("points");
		Report.Election = FString::Printf(TEXT("%s: %s. Vote %s%% · %s of 100 regional points."),
			bMidterm ? TEXT("Midterm election") : TEXT("General election"),
			bWon ? (bMidterm ? TEXT("your coalition holds a majority") : TEXT("you are re-elected")) : (bMidterm ? TEXT("your coalition loses its majority") : TEXT("you are defeated")),
			*ToFString(FourYears::FormatNumber(Election.Get("vote").AsNumber())), *ToFString(FourYears::FormatNumber(Points.AsNumber())));
	}
	return Report;
}
