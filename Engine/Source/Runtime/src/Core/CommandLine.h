#pragma once
#include "Container/Hash.h"
#include "Container/Array.h"
#include "Container/String.h"

namespace Gleam {

class CommandLine
{
public:

	CommandLine() = default;
	~CommandLine() = default;

	void Parse(int argc, char* argv[]);

	const auto& Flags() const { return mFlags; }
	const auto& Params() const { return mParams; }
	const auto& PositionalArgs() const { return mPositionalArgs; }

	const TString& operator[](size_t index) const { return GetPositionalArg(index); }
	bool operator[](const TStringView flag) const { return HasFlag(flag); }
	bool operator[](std::initializer_list<char const* const> flags) const
	{
		return eastl::any_of(flags.begin(), flags.end(), [&](char const* const flag) { return HasFlag(flag); });
	}

	bool HasFlag(const TStringView flag) const;
	bool HasParam(const TStringView param) const;
	const TString& GetPositionalArg(size_t index) const;

	auto begin() const { return mPositionalArgs.cbegin(); }
	auto end() const { return mPositionalArgs.cend(); }
	size_t size() const { return mPositionalArgs.size(); }

private:

	MultiSet<TString> mFlags;
	MultiMap<TString, TString> mParams;
	TArray<TString> mPositionalArgs;
};

} // namespace Gleam
