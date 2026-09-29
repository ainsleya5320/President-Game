#pragma once
#include "FourYears/Core/FourYearsJson.h"
#include <vector>

namespace FourYears::Diplomacy
{
// Fictional strategic scenario values, not claims about present-day governments.
struct Region { const char* Id; const char* Name; const char* Interest; double Lon, Lat; int Relation, Influence, Rival, TradeValue; };
const std::vector<Region>& Regions();
void Initialize(JsonValue& State);
JsonValue View(const JsonValue& State);
JsonValue Options(const JsonValue& State, const std::string& RegionId);
JsonValue Act(JsonValue& State, const std::string& RegionId, const std::string& Action);
JsonValue SetDoctrine(JsonValue& State, const std::string& Doctrine);
JsonValue CrisisOptions(const JsonValue& State, int Index);
JsonValue Respond(JsonValue& State, int Index, const std::string& Choice);
// Called once immediately before the native domestic quarter resolves. Does not change State.quarter.
JsonValue Advance(JsonValue& State);
}
