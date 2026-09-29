#pragma once
#include "FourYears/Core/FourYearsJson.h"
namespace FourYears { namespace Electoral {
JsonValue View(const JsonValue& Data,const JsonValue& State,double ApprovalSignal);
JsonValue Election(const JsonValue& View);
JsonValue ChooseParty(JsonValue& State,const std::string& Party);
JsonValue Organize(const JsonValue& Data,JsonValue& State,const std::string& County);
} }
