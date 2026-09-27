// Four Years core: a small JSON value with JavaScript semantics.
// Standard C++17 only (no Unreal headers, no exceptions, no RTTI) so the simulation core can be built and
// tested outside the engine. Objects keep insertion order, as JavaScript objects do, because the
// simulation iterates them and summation order affects results.
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace FourYears
{
class JsonValue
{
public:
	enum class EType { Undefined, Null, Bool, Number, String, Array, Object };

	JsonValue() = default;
	static JsonValue Null();
	static JsonValue Boolean(bool Value);
	static JsonValue Number(double Value);
	static JsonValue String(std::string Value);
	static JsonValue Array();
	static JsonValue Object();

	EType Type() const { return Kind; }
	bool IsUndefined() const { return Kind == EType::Undefined; }
	bool IsNull() const { return Kind == EType::Null; }
	bool IsBool() const { return Kind == EType::Bool; }
	bool IsNumber() const { return Kind == EType::Number; }
	bool IsString() const { return Kind == EType::String; }
	bool IsArray() const { return Kind == EType::Array; }
	bool IsObject() const { return Kind == EType::Object; }
	// Undefined or null, like JavaScript's `value == null`.
	bool IsNullish() const { return Kind == EType::Undefined || Kind == EType::Null; }
	// JavaScript truthiness.
	bool Truthy() const;

	bool AsBool() const { return Kind == EType::Bool && BoolValue; }
	// Numbers read as NaN when the value is not a number.
	double AsNumber() const;
	// `value || Fallback` for numbers.
	double NumberOr(double Fallback) const;
	const std::string& AsString() const;

	// Arrays.
	std::size_t Size() const;
	const JsonValue& operator[](std::size_t Index) const;
	JsonValue& operator[](std::size_t Index);
	void Push(JsonValue Value);
	void Insert(std::size_t Index, JsonValue Value);
	void Resize(std::size_t Count);
	std::vector<JsonValue>& Items() { return Elements; }
	const std::vector<JsonValue>& Items() const { return Elements; }

	// Objects. Get returns an undefined value for missing keys; Set appends new keys at the end.
	bool Has(const std::string& Key) const;
	const JsonValue& Get(const std::string& Key) const;
	JsonValue* Find(const std::string& Key);
	const JsonValue* Find(const std::string& Key) const;
	JsonValue& Set(const std::string& Key, JsonValue Value);
	// Returns the member, creating it as undefined when missing.
	JsonValue& operator[](const std::string& Key);
	void Remove(const std::string& Key);
	const std::vector<std::string>& Keys() const { return MemberKeys; }
	const std::vector<JsonValue>& Values() const { return Elements; }
	std::vector<JsonValue>& Values() { return Elements; }

	static bool Parse(const std::string& Text, JsonValue& Out, std::string& Error);
	std::string Dump() const;

private:
	void DumpTo(std::string& Out) const;

	EType Kind = EType::Undefined;
	bool BoolValue = false;
	double NumberValue = 0.0;
	std::string StringValue;
	std::vector<std::string> MemberKeys; // Objects only; parallel to Elements.
	std::vector<JsonValue> Elements;     // Array items, or object member values.
};

// JavaScript's Number.prototype.toString for finite doubles, e.g. 2, 0.5, -1.25, 1e+21.
std::string FormatNumber(double Value);
// JavaScript's Math.round: halves round towards positive infinity.
double JsRound(double Value);
// JavaScript's ToUint32 conversion, used by the >>> 0 and Math.imul arithmetic of the seeded generators.
unsigned int ToUint32(double Value);
} // namespace FourYears
