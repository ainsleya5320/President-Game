#include "FourYears/FourYearsSubsystem.h"

#include "HAL/FileManager.h"
#include "ImageUtils.h"
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

FFourYearsActionResult UFourYearsSubsystem::ToResult(const JsonValue& Result)
{
	FFourYearsActionResult Out;
	Out.bOk = Result.Get("ok").Truthy();
	Out.Message = ToFString(Result.Get(Out.bOk ? "detail" : "reason").AsString());
	if (Out.bOk)
	{
		SaveTerm();
	}
	return Out;
}

void UFourYearsSubsystem::SetLocation(const FString& Location)
{
	if (bReady && (Location == TEXT("oval") || Location == TEXT("aircraft")))
	{
		State.Set("location", JsonValue::String(ToUtf8(Location)));
	}
}

FFourYearsStatus UFourYearsSubsystem::GetStatus() const
{
	FFourYearsStatus Status;
	if (!bReady)
	{
		return Status;
	}
	Status.Capital = static_cast<int32>(State.Get("capital").AsNumber());
	Status.AppointmentsLeft = Core.AvailableSlots(State);
	Status.Approval = static_cast<int32>(Core.Poll(State));
	Status.BudgetBalance = static_cast<int32>(Core.Budget(State).Get("balance").AsNumber());
	Status.bDecisionFiled = !State.Get("agendaChoice").IsNull();
	Status.bTermOver = State.Get("ended").Truthy();
	return Status;
}

TArray<FFourYearsPolicy> UFourYearsSubsystem::GetPolicies() const
{
	TArray<FFourYearsPolicy> Out;
	if (!bReady)
	{
		return Out;
	}
	const JsonValue& Labels = Core.Data().Get("metrics").Get("labels");
	for (const JsonValue& Policy : Core.Policies().Items())
	{
		const std::string Id = Policy.Get("id").AsString();
		FFourYearsPolicy Row;
		Row.Id = ToFString(Id);
		Row.Name = ToFString(Policy.Get("name").AsString());
		Row.Department = ToFString(Policy.Get("department").AsString());
		Row.Level = static_cast<int32>(State.Get("levels").Get(Id).AsNumber());
		Row.Implemented = static_cast<float>(State.Get("implemented").Get(Id).AsNumber());
		Row.BudgetPerLevel = static_cast<int32>(Policy.Get("cost").AsNumber());
		Row.Lag = static_cast<int32>(Policy.Get("lag").AsNumber());
		Row.CapitalPerStep = Row.Lag > 1 ? 3 : 2;
		// Mirrors changePolicy: full strength needs a passed act when a bill covers this policy.
		bool bCoveredByBill = false, bAuthorized = false;
		for (const JsonValue& Bill : Core.Data().Get("executive").Get("bills").Items())
		{
			if (Bill.Get("policy").AsString() != Id) continue;
			bCoveredByBill = true;
			for (const JsonValue& Passed : State.Get("executive").Get("bills").Items())
			{
				if (Passed.Get("id").AsString() == Bill.Get("id").AsString() && Passed.Get("status").AsString() == "passed") bAuthorized = true;
			}
		}
		Row.bNeedsAct = bCoveredByBill && !bAuthorized;
		const JsonValue& Effects = Policy.Get("effects");
		TArray<FString> Parts;
		for (size_t Index = 0; Index < Effects.Keys().size(); ++Index)
		{
			Parts.Add(ToFString(Labels.Get(Effects.Keys()[Index]).AsString()) + TEXT(" ") + Signed(Effects.Values()[Index].AsNumber()));
		}
		Row.Effects = FString::Join(Parts, TEXT(" · "));
		Out.Add(Row);
	}
	return Out;
}

FFourYearsActionResult UFourYearsSubsystem::SetPolicyLevel(const FString& PolicyId, int32 Level)
{
	if (!bReady)
	{
		return FFourYearsActionResult();
	}
	const JsonValue Result = Core.ChangePolicy(State, ToUtf8(PolicyId), Level);
	FFourYearsActionResult Out = ToResult(Result);
	if (Out.bOk)
	{
		const JsonValue* Policy = nullptr;
		for (const JsonValue& Candidate : Core.Policies().Items())
		{
			if (Candidate.Get("id").AsString() == ToUtf8(PolicyId)) Policy = &Candidate;
		}
		Out.Message = FString::Printf(TEXT("%s set to %d/4 for %s political capital. Effects build over the coming quarters."),
			Policy ? *ToFString(Policy->Get("name").AsString()) : *PolicyId, Level, *ToFString(FourYears::FormatNumber(Result.Get("cost").AsNumber())));
	}
	return Out;
}

TArray<FFourYearsAdviser> UFourYearsSubsystem::GetAdvisers() const
{
	TArray<FFourYearsAdviser> Out;
	if (!bReady)
	{
		return Out;
	}
	const JsonValue People = Core.People(State);
	for (const JsonValue& Person : People.Items())
	{
		const std::string Id = Person.Get("id").AsString();
		int32 Active = -1;
		const JsonValue Request = Core.Request(State, Id, Active);
		const JsonValue& Cabinet = State.Get("executive").Get("cabinet").Get(Id);
		FFourYearsAdviser Adviser;
		Adviser.Id = ToFString(Id);
		Adviser.Name = ToFString(Person.Get("name").AsString());
		Adviser.Role = ToFString(Person.Get("role").AsString());
		Adviser.Initials = ToFString(Person.Get("initials").AsString());
		Adviser.Quote = ToFString(Person.Get("quote").AsString());
		Adviser.Relationship = static_cast<int32>(FourYears::JsRound(State.Get("relationships").Get(Id).AsNumber()));
		Adviser.bResigned = Cabinet.Get("status").AsString() == "resigned";
		Adviser.bActing = Cabinet.Get("acting").Truthy();
		Adviser.bMetThisQuarter = Request.Get("done").Truthy();
		const JsonValue& Wanted = Request.Get("policy");
		const std::string WantedId = Wanted.Get("id").AsString();
		Adviser.RequestedPolicyId = ToFString(WantedId);
		const int32 Now = static_cast<int32>(State.Get("levels").Get(WantedId).AsNumber());
		const int32 Target = static_cast<int32>(Request.Get("target").AsNumber());
		Adviser.Request = FString::Printf(TEXT("%s at %d/4 (now %d/4)"), *ToFString(Wanted.Get("name").AsString()), Target, Now);
		if (Active >= 0)
		{
			const JsonValue& Promise = State.Get("promises")[static_cast<size_t>(Active)];
			Adviser.Promise = FString::Printf(TEXT("%s at %d/4 by quarter %d%s"), *ToFString(Wanted.Get("name").AsString()),
				static_cast<int32>(Promise.Get("target").AsNumber()), static_cast<int32>(Promise.Get("due").AsNumber()), Promise.Get("extended").Truthy() ? TEXT(" (already extended)") : TEXT(""));
			const auto Add = [&Adviser](const TCHAR* Response, const TCHAR* Label, const TCHAR* Detail) {
				FFourYearsMeetingOption Option;
				Option.Response = Response;
				Option.Label = Label;
				Option.Detail = Detail;
				Adviser.Options.Add(Option);
			};
			Add(TEXT("reassure"), TEXT("Reassure them"), TEXT("Costs 1 political capital · relationship +3 · the deadline stands"));
			if (!Promise.Get("extended").Truthy() && Promise.Get("due").AsNumber() < 16) Add(TEXT("extend"), TEXT("Ask for more time"), TEXT("One more quarter · relationship −3 · only once"));
			Add(TEXT("withdraw"), TEXT("Withdraw the promise"), TEXT("Counts as a broken promise: trust and relationship suffer"));
		}
		else
		{
			const bool bFull = Now >= 4;
			const auto Add = [&Adviser](const TCHAR* Response, const TCHAR* Label, const FString& Detail) {
				FFourYearsMeetingOption Option;
				Option.Response = Response;
				Option.Label = Label;
				Option.Detail = Detail;
				Adviser.Options.Add(Option);
			};
			if (!bFull)
			{
				Add(TEXT("promise"), TEXT("Promise to deliver"), FString::Printf(TEXT("Commit to %d/4 within 2 quarters · capital +3 · relationship +4"), Target));
				Add(TEXT("compromise"), TEXT("Offer a compromise"), FString::Printf(TEXT("Commit to %d/4 within 3 quarters · capital +1 · relationship +2"), Target));
			}
			Add(TEXT("listen"), TEXT("Listen without committing"), TEXT("Relationship +1 · no promise, no support"));
		}
		Out.Add(Adviser);
	}
	return Out;
}

FFourYearsActionResult UFourYearsSubsystem::MeetAdviser(const FString& AdviserId, const FString& Response)
{
	if (!bReady)
	{
		return FFourYearsActionResult();
	}
	return ToResult(Core.Meeting(State, ToUtf8(AdviserId), ToUtf8(Response)));
}

TArray<FString> UFourYearsSubsystem::GetPromiseRecord() const
{
	TArray<FString> Out;
	if (!bReady)
	{
		return Out;
	}
	const JsonValue People = Core.People(State);
	const JsonValue& Promises = State.Get("promises");
	for (size_t Index = Promises.Size(); Index-- > 0;)
	{
		const JsonValue& Promise = Promises[Index];
		FString Who = ToFString(Promise.Get("person").AsString());
		for (const JsonValue& Person : People.Items())
		{
			if (Person.Get("id").AsString() == Promise.Get("person").AsString()) Who = ToFString(Person.Get("name").AsString());
		}
		FString PolicyName = ToFString(Promise.Get("policy").AsString());
		for (const JsonValue& Policy : Core.Policies().Items())
		{
			if (Policy.Get("id").AsString() == Promise.Get("policy").AsString()) PolicyName = ToFString(Policy.Get("name").AsString());
		}
		const std::string Status = Promise.Get("status").AsString();
		const FString Label = Status == "kept" ? TEXT("Kept") : Status == "broken" ? TEXT("Broken") : TEXT("Open");
		Out.Add(FString::Printf(TEXT("%s · %s at %d/4 for %s · due quarter %d"), *Label, *PolicyName,
			static_cast<int32>(Promise.Get("target").AsNumber()), *Who, static_cast<int32>(Promise.Get("due").AsNumber())));
	}
	return Out;
}

UTexture2D* UFourYearsSubsystem::GetPortrait(const FString& AdviserName)
{
	if (const TObjectPtr<UTexture2D>* Cached = Portraits.Find(AdviserName))
	{
		return Cached->Get();
	}
	// Portraits are named after the person: "Maya Chen" -> maya-chen.png.
	const FString File = AdviserName.ToLower().Replace(TEXT(" "), TEXT("-")) + TEXT(".png");
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Prototype"), TEXT("assets"), TEXT("characters"), File);
	UTexture2D* Texture = FPaths::FileExists(Path) ? FImageUtils::ImportFileAsTexture2D(Path) : nullptr;
	Portraits.Add(AdviserName, Texture);
	return Texture;
}

FString UFourYearsSubsystem::GetDialogueLine(const FString& AdviserId, const FString& Cue) const
{
	const JsonValue& Dialogue = Core.Data().Get("dialogue");
	const JsonValue& Character = Dialogue.Get("characters").Get(ToUtf8(AdviserId));
	const JsonValue& Line = Character.Get(ToUtf8(Cue));
	if (Line.IsString() && !Line.AsString().empty()) return ToFString(Line.AsString());
	const JsonValue& Fallback = Dialogue.Get("fallback").Get(ToUtf8(Cue));
	return Fallback.IsString() ? ToFString(Fallback.AsString()) : TEXT("Let's talk about what happens next, Mr. President.");
}

FFourYearsBill UFourYearsSubsystem::DescribeBill(const JsonValue& Definition, const JsonValue* BillState) const
{
	FFourYearsBill Bill;
	Bill.Id = ToFString(Definition.Get("id").AsString());
	Bill.Name = ToFString(Definition.Get("name").AsString());
	Bill.Story = ToFString(Definition.Get("story").AsString());
	Bill.Cost = static_cast<int32>(Definition.Get("cost").AsNumber());
	const JsonValue People = Core.People(State);
	for (const JsonValue& Person : People.Items())
	{
		if (Person.Get("id").AsString() == Definition.Get("sponsor").AsString()) Bill.Sponsor = ToFString(Person.Get("name").AsString());
	}
	for (const JsonValue& Policy : Core.Policies().Items())
	{
		if (Policy.Get("id").AsString() == Definition.Get("policy").AsString()) Bill.Policy = ToFString(Policy.Get("name").AsString());
	}
	Bill.Status = TEXT("available");
	if (BillState)
	{
		Bill.Status = ToFString(BillState->Get("status").AsString());
		Bill.Progress = static_cast<int32>(BillState->Get("progress").AsNumber());
		Bill.Attempts = static_cast<int32>(BillState->Get("attempts").AsNumber());
		Bill.LastYes = BillState->Get("yes").IsNumber() ? static_cast<int32>(BillState->Get("yes").AsNumber()) : -1;
		Bill.bVotedThisQuarter = BillState->Get("lastVote").AsNumber() == State.Get("quarter").AsNumber();
		for (const JsonValue& Id : BillState->Get("amendments").Items())
		{
			for (const JsonValue& Amendment : Core.Data().Get("executive").Get("amendments").Items())
			{
				if (Amendment.Get("id").AsString() == Id.AsString()) Bill.Amendments.Add(ToFString(Amendment.Get("name").AsString()));
			}
		}
	}
	return Bill;
}

FFourYearsCongress UFourYearsSubsystem::GetCongress() const
{
	FFourYearsCongress Congress;
	if (!bReady)
	{
		return Congress;
	}
	const JsonValue& Executive = Core.Data().Get("executive");
	Congress.OppositionMomentum = static_cast<int32>(FourYears::JsRound(State.Get("executive").Get("opposition").Get("momentum").AsNumber()));
	for (const JsonValue& Definition : Executive.Get("bills").Items())
	{
		const JsonValue* BillState = nullptr;
		for (const JsonValue& Candidate : State.Get("executive").Get("bills").Items())
		{
			if (Candidate.Get("id").AsString() == Definition.Get("id").AsString()) BillState = &Candidate;
		}
		const FFourYearsBill Bill = DescribeBill(Definition, BillState);
		if (!BillState) Congress.Available.Add(Bill);
		else if (Bill.Status == TEXT("passed")) Congress.Passed.Add(Bill);
	}
	const JsonValue* Active = Core.ActiveBill(State);
	if (!Active)
	{
		return Congress;
	}
	Congress.bHasActiveBill = true;
	for (const JsonValue& Definition : Executive.Get("bills").Items())
	{
		if (Definition.Get("id").AsString() == Active->Get("id").AsString()) Congress.ActiveBill = DescribeBill(Definition, Active);
	}
	const JsonValue Rows = Core.Votes(State, *Active);
	for (const JsonValue& Row : Rows.Items())
	{
		FFourYearsBloc Bloc;
		Bloc.Id = ToFString(Row.Get("id").AsString());
		Bloc.Name = ToFString(Row.Get("name").AsString());
		Bloc.Seats = static_cast<int32>(Row.Get("seats").AsNumber());
		Bloc.Support = static_cast<int32>(Row.Get("support").AsNumber());
		Bloc.YesVotes = static_cast<int32>(Row.Get("yes").AsNumber());
		Bloc.Want = ToFString(Row.Get("want").AsString());
		for (const JsonValue& Id : Active->Get("lobbied").Items())
		{
			if (Id.AsString() == Row.Get("id").AsString()) Bloc.bLobbied = true;
		}
		Congress.ProjectedYes += Bloc.YesVotes;
		Congress.Blocs.Add(Bloc);
	}
	for (const JsonValue& Definition : Executive.Get("amendments").Items())
	{
		FFourYearsAmendment Amendment;
		Amendment.Id = ToFString(Definition.Get("id").AsString());
		Amendment.Name = ToFString(Definition.Get("name").AsString());
		Amendment.Detail = ToFString(Definition.Get("detail").AsString());
		for (const JsonValue& Id : Active->Get("amendments").Items())
		{
			if (Id.AsString() == Definition.Get("id").AsString()) Amendment.bAdopted = true;
		}
		Congress.Amendments.Add(Amendment);
	}
	return Congress;
}

FFourYearsActionResult UFourYearsSubsystem::IntroduceBill(const FString& BillId)
{
	return bReady ? ToResult(Core.Propose(State, ToUtf8(BillId))) : FFourYearsActionResult();
}

FFourYearsActionResult UFourYearsSubsystem::AddAmendment(const FString& AmendmentId)
{
	return bReady ? ToResult(Core.Amend(State, ToUtf8(AmendmentId))) : FFourYearsActionResult();
}

FFourYearsActionResult UFourYearsSubsystem::LobbyBloc(const FString& BlocId)
{
	return bReady ? ToResult(Core.Lobby(State, ToUtf8(BlocId))) : FFourYearsActionResult();
}

FFourYearsActionResult UFourYearsSubsystem::CallVote()
{
	return bReady ? ToResult(Core.Vote(State)) : FFourYearsActionResult();
}
