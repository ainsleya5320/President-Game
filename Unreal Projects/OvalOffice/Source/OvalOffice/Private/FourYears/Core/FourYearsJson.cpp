#include "FourYears/Core/FourYearsJson.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace FourYears
{
namespace
{
const JsonValue& UndefinedValue()
{
	static const JsonValue Value;
	return Value;
}

const std::string& EmptyString()
{
	static const std::string Value;
	return Value;
}

void AppendUtf8(std::string& Out, unsigned int CodePoint)
{
	if (CodePoint < 0x80)
	{
		Out += static_cast<char>(CodePoint);
	}
	else if (CodePoint < 0x800)
	{
		Out += static_cast<char>(0xC0 | (CodePoint >> 6));
		Out += static_cast<char>(0x80 | (CodePoint & 0x3F));
	}
	else if (CodePoint < 0x10000)
	{
		Out += static_cast<char>(0xE0 | (CodePoint >> 12));
		Out += static_cast<char>(0x80 | ((CodePoint >> 6) & 0x3F));
		Out += static_cast<char>(0x80 | (CodePoint & 0x3F));
	}
	else
	{
		Out += static_cast<char>(0xF0 | (CodePoint >> 18));
		Out += static_cast<char>(0x80 | ((CodePoint >> 12) & 0x3F));
		Out += static_cast<char>(0x80 | ((CodePoint >> 6) & 0x3F));
		Out += static_cast<char>(0x80 | (CodePoint & 0x3F));
	}
}

class JsonParser
{
public:
	JsonParser(const std::string& InText, std::string& InError) : Text(InText), Error(InError) {}

	bool ParseDocument(JsonValue& Out)
	{
		SkipSpace();
		if (!ParseValue(Out, 0))
		{
			return false;
		}
		SkipSpace();
		if (Position != Text.size())
		{
			return Fail("unexpected text after the JSON value");
		}
		return true;
	}

private:
	bool Fail(const char* Message)
	{
		Error = std::string(Message) + " at offset " + std::to_string(Position);
		return false;
	}

	void SkipSpace()
	{
		while (Position < Text.size() && (Text[Position] == ' ' || Text[Position] == '\t' || Text[Position] == '\n' || Text[Position] == '\r'))
		{
			++Position;
		}
	}

	bool Literal(const char* Word)
	{
		std::size_t Length = 0;
		while (Word[Length] != '\0')
		{
			++Length;
		}
		if (Text.compare(Position, Length, Word) != 0)
		{
			return false;
		}
		Position += Length;
		return true;
	}

	bool ParseHex4(unsigned int& Out)
	{
		if (Position + 4 > Text.size())
		{
			return Fail("truncated \\u escape");
		}
		Out = 0;
		for (int Digit = 0; Digit < 4; ++Digit)
		{
			const char C = Text[Position++];
			Out <<= 4;
			if (C >= '0' && C <= '9') Out |= static_cast<unsigned int>(C - '0');
			else if (C >= 'a' && C <= 'f') Out |= static_cast<unsigned int>(C - 'a' + 10);
			else if (C >= 'A' && C <= 'F') Out |= static_cast<unsigned int>(C - 'A' + 10);
			else return Fail("invalid \\u escape");
		}
		return true;
	}

	bool ParseString(std::string& Out)
	{
		++Position; // Opening quote.
		while (Position < Text.size())
		{
			const char C = Text[Position++];
			if (C == '"')
			{
				return true;
			}
			if (C != '\\')
			{
				Out += C;
				continue;
			}
			if (Position >= Text.size())
			{
				break;
			}
			const char Escape = Text[Position++];
			switch (Escape)
			{
			case '"': Out += '"'; break;
			case '\\': Out += '\\'; break;
			case '/': Out += '/'; break;
			case 'b': Out += '\b'; break;
			case 'f': Out += '\f'; break;
			case 'n': Out += '\n'; break;
			case 'r': Out += '\r'; break;
			case 't': Out += '\t'; break;
			case 'u':
			{
				unsigned int CodePoint = 0;
				if (!ParseHex4(CodePoint))
				{
					return false;
				}
				if (CodePoint >= 0xD800 && CodePoint <= 0xDBFF && Position + 1 < Text.size() && Text[Position] == '\\' && Text[Position + 1] == 'u')
				{
					Position += 2;
					unsigned int Low = 0;
					if (!ParseHex4(Low))
					{
						return false;
					}
					CodePoint = 0x10000 + ((CodePoint - 0xD800) << 10) + (Low - 0xDC00);
				}
				AppendUtf8(Out, CodePoint);
				break;
			}
			default: return Fail("invalid escape");
			}
		}
		return Fail("unterminated string");
	}

	bool ParseValue(JsonValue& Out, int Depth)
	{
		if (Depth > 256)
		{
			return Fail("nesting too deep");
		}
		if (Position >= Text.size())
		{
			return Fail("unexpected end of input");
		}
		const char C = Text[Position];
		if (C == '{')
		{
			++Position;
			Out = JsonValue::Object();
			SkipSpace();
			if (Position < Text.size() && Text[Position] == '}')
			{
				++Position;
				return true;
			}
			while (true)
			{
				SkipSpace();
				if (Position >= Text.size() || Text[Position] != '"')
				{
					return Fail("expected a member name");
				}
				std::string Key;
				if (!ParseString(Key))
				{
					return false;
				}
				SkipSpace();
				if (Position >= Text.size() || Text[Position] != ':')
				{
					return Fail("expected ':'");
				}
				++Position;
				SkipSpace();
				JsonValue Member;
				if (!ParseValue(Member, Depth + 1))
				{
					return false;
				}
				Out.Set(Key, std::move(Member));
				SkipSpace();
				if (Position < Text.size() && Text[Position] == ',')
				{
					++Position;
					continue;
				}
				if (Position < Text.size() && Text[Position] == '}')
				{
					++Position;
					return true;
				}
				return Fail("expected ',' or '}'");
			}
		}
		if (C == '[')
		{
			++Position;
			Out = JsonValue::Array();
			SkipSpace();
			if (Position < Text.size() && Text[Position] == ']')
			{
				++Position;
				return true;
			}
			while (true)
			{
				SkipSpace();
				JsonValue Item;
				if (!ParseValue(Item, Depth + 1))
				{
					return false;
				}
				Out.Push(std::move(Item));
				SkipSpace();
				if (Position < Text.size() && Text[Position] == ',')
				{
					++Position;
					continue;
				}
				if (Position < Text.size() && Text[Position] == ']')
				{
					++Position;
					return true;
				}
				return Fail("expected ',' or ']'");
			}
		}
		if (C == '"')
		{
			std::string Value;
			if (!ParseString(Value))
			{
				return false;
			}
			Out = JsonValue::String(std::move(Value));
			return true;
		}
		if (Literal("true")) { Out = JsonValue::Boolean(true); return true; }
		if (Literal("false")) { Out = JsonValue::Boolean(false); return true; }
		if (Literal("null")) { Out = JsonValue::Null(); return true; }
		if (C == '-' || (C >= '0' && C <= '9'))
		{
			const char* Start = Text.c_str() + Position;
			char* End = nullptr;
			const double Value = std::strtod(Start, &End);
			if (End == Start)
			{
				return Fail("invalid number");
			}
			Position += static_cast<std::size_t>(End - Start);
			Out = JsonValue::Number(Value);
			return true;
		}
		return Fail("unexpected character");
	}

	const std::string& Text;
	std::string& Error;
	std::size_t Position = 0;
};

void DumpString(std::string& Out, const std::string& Value)
{
	Out += '"';
	for (const char C : Value)
	{
		switch (C)
		{
		case '"': Out += "\\\""; break;
		case '\\': Out += "\\\\"; break;
		case '\n': Out += "\\n"; break;
		case '\r': Out += "\\r"; break;
		case '\t': Out += "\\t"; break;
		case '\b': Out += "\\b"; break;
		case '\f': Out += "\\f"; break;
		default:
			if (static_cast<unsigned char>(C) < 0x20)
			{
				char Buffer[8];
				std::snprintf(Buffer, sizeof(Buffer), "\\u%04x", static_cast<unsigned int>(static_cast<unsigned char>(C)));
				Out += Buffer;
			}
			else
			{
				Out += C;
			}
		}
	}
	Out += '"';
}
} // namespace

JsonValue JsonValue::Null() { JsonValue V; V.Kind = EType::Null; return V; }
JsonValue JsonValue::Boolean(bool Value) { JsonValue V; V.Kind = EType::Bool; V.BoolValue = Value; return V; }
JsonValue JsonValue::Number(double Value) { JsonValue V; V.Kind = EType::Number; V.NumberValue = Value; return V; }
JsonValue JsonValue::String(std::string Value) { JsonValue V; V.Kind = EType::String; V.StringValue = std::move(Value); return V; }
JsonValue JsonValue::Array() { JsonValue V; V.Kind = EType::Array; return V; }
JsonValue JsonValue::Object() { JsonValue V; V.Kind = EType::Object; return V; }

bool JsonValue::Truthy() const
{
	switch (Kind)
	{
	case EType::Bool: return BoolValue;
	case EType::Number: return NumberValue != 0.0 && !std::isnan(NumberValue);
	case EType::String: return !StringValue.empty();
	case EType::Array:
	case EType::Object: return true;
	default: return false;
	}
}

double JsonValue::AsNumber() const
{
	return Kind == EType::Number ? NumberValue : std::numeric_limits<double>::quiet_NaN();
}

double JsonValue::NumberOr(double Fallback) const
{
	return Truthy() && Kind == EType::Number ? NumberValue : Fallback;
}

const std::string& JsonValue::AsString() const
{
	return Kind == EType::String ? StringValue : EmptyString();
}

std::size_t JsonValue::Size() const { return Elements.size(); }

const JsonValue& JsonValue::operator[](std::size_t Index) const
{
	return Kind == EType::Array && Index < Elements.size() ? Elements[Index] : UndefinedValue();
}

JsonValue& JsonValue::operator[](std::size_t Index) { return Elements[Index]; }

void JsonValue::Push(JsonValue Value) { Elements.push_back(std::move(Value)); }

void JsonValue::Insert(std::size_t Index, JsonValue Value)
{
	Elements.insert(Elements.begin() + static_cast<std::ptrdiff_t>(Index), std::move(Value));
}

void JsonValue::Resize(std::size_t Count) { Elements.resize(Count); }

bool JsonValue::Has(const std::string& Key) const { return Find(Key) != nullptr; }

const JsonValue& JsonValue::Get(const std::string& Key) const
{
	const JsonValue* Member = Find(Key);
	return Member ? *Member : UndefinedValue();
}

JsonValue* JsonValue::Find(const std::string& Key)
{
	if (Kind != EType::Object)
	{
		return nullptr;
	}
	for (std::size_t Index = 0; Index < MemberKeys.size(); ++Index)
	{
		if (MemberKeys[Index] == Key)
		{
			return &Elements[Index];
		}
	}
	return nullptr;
}

const JsonValue* JsonValue::Find(const std::string& Key) const
{
	return const_cast<JsonValue*>(this)->Find(Key);
}

JsonValue& JsonValue::Set(const std::string& Key, JsonValue Value)
{
	if (JsonValue* Member = Find(Key))
	{
		*Member = std::move(Value);
		return *Member;
	}
	MemberKeys.push_back(Key);
	Elements.push_back(std::move(Value));
	return Elements.back();
}

JsonValue& JsonValue::operator[](const std::string& Key)
{
	if (JsonValue* Member = Find(Key))
	{
		return *Member;
	}
	return Set(Key, JsonValue());
}

void JsonValue::Remove(const std::string& Key)
{
	for (std::size_t Index = 0; Index < MemberKeys.size(); ++Index)
	{
		if (MemberKeys[Index] == Key)
		{
			MemberKeys.erase(MemberKeys.begin() + static_cast<std::ptrdiff_t>(Index));
			Elements.erase(Elements.begin() + static_cast<std::ptrdiff_t>(Index));
			return;
		}
	}
}

bool JsonValue::Parse(const std::string& Text, JsonValue& Out, std::string& Error)
{
	JsonParser Parser(Text, Error);
	return Parser.ParseDocument(Out);
}

std::string JsonValue::Dump() const
{
	std::string Out;
	DumpTo(Out);
	return Out;
}

void JsonValue::DumpTo(std::string& Out) const
{
	switch (Kind)
	{
	case EType::Undefined:
	case EType::Null: Out += "null"; break;
	case EType::Bool: Out += BoolValue ? "true" : "false"; break;
	case EType::Number: Out += std::isfinite(NumberValue) ? FormatNumber(NumberValue) : "null"; break;
	case EType::String: DumpString(Out, StringValue); break;
	case EType::Array:
		Out += '[';
		for (std::size_t Index = 0; Index < Elements.size(); ++Index)
		{
			if (Index) Out += ',';
			// Like JSON.stringify, undefined array items are written as null.
			Elements[Index].DumpTo(Out);
		}
		Out += ']';
		break;
	case EType::Object:
	{
		Out += '{';
		bool First = true;
		for (std::size_t Index = 0; Index < MemberKeys.size(); ++Index)
		{
			// Like JSON.stringify, undefined members are omitted.
			if (Elements[Index].IsUndefined()) continue;
			if (!First) Out += ',';
			First = false;
			DumpString(Out, MemberKeys[Index]);
			Out += ':';
			Elements[Index].DumpTo(Out);
		}
		Out += '}';
		break;
	}
	}
}

std::string FormatNumber(double Value)
{
	if (std::isnan(Value)) return "NaN";
	if (std::isinf(Value)) return Value > 0 ? "Infinity" : "-Infinity";
	if (Value == 0.0) return "0";
	// Find the shortest digit string that round-trips, then lay it out the way JavaScript does.
	char Buffer[40];
	int Precision = 1;
	for (; Precision <= 17; ++Precision)
	{
		std::snprintf(Buffer, sizeof(Buffer), "%.*e", Precision - 1, Value);
		if (std::strtod(Buffer, nullptr) == Value) break;
	}
	const std::string Scientific(Buffer);
	const bool Negative = Scientific[0] == '-';
	const std::size_t Mark = Scientific.find('e');
	std::string Digits;
	for (std::size_t Index = Negative ? 1 : 0; Index < Mark; ++Index)
	{
		if (Scientific[Index] != '.') Digits += Scientific[Index];
	}
	while (Digits.size() > 1 && Digits.back() == '0') Digits.pop_back();
	const int Exponent = std::atoi(Scientific.c_str() + Mark + 1);
	const int DigitCount = static_cast<int>(Digits.size());
	const int PointAt = Exponent + 1; // Digits before the decimal point.
	std::string Out = Negative ? "-" : "";
	if (PointAt >= DigitCount && PointAt <= 21)
	{
		Out += Digits + std::string(static_cast<std::size_t>(PointAt - DigitCount), '0');
	}
	else if (PointAt > 0 && PointAt <= 21)
	{
		Out += Digits.substr(0, static_cast<std::size_t>(PointAt)) + "." + Digits.substr(static_cast<std::size_t>(PointAt));
	}
	else if (PointAt <= 0 && PointAt > -6)
	{
		Out += "0." + std::string(static_cast<std::size_t>(-PointAt), '0') + Digits;
	}
	else
	{
		Out += Digits.substr(0, 1);
		if (DigitCount > 1) Out += "." + Digits.substr(1);
		Out += Exponent >= 0 ? "e+" : "e-";
		Out += std::to_string(Exponent >= 0 ? Exponent : -Exponent);
	}
	return Out;
}

double JsRound(double Value)
{
	if (!std::isfinite(Value)) return Value;
	const double Floor = std::floor(Value);
	return Value - Floor >= 0.5 ? Floor + 1.0 : Floor;
}

unsigned int ToUint32(double Value)
{
	if (!std::isfinite(Value)) return 0u;
	const double Truncated = std::trunc(Value);
	double Wrapped = std::fmod(Truncated, 4294967296.0);
	if (Wrapped < 0) Wrapped += 4294967296.0;
	return static_cast<unsigned int>(Wrapped);
}
} // namespace FourYears
