#pragma once
#include "FourYears/Core/FourYearsSim.h"
namespace FourYears { namespace Presidency {
void Initialize(JsonValue& State);
JsonValue View(const Simulation& Sim, const JsonValue& State);
JsonValue Act(const Simulation& Sim, JsonValue& State, const std::string& Action, const std::string& Target);
JsonValue BeforeAdvance(JsonValue& State);
} }
