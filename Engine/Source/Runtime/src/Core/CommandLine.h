#pragma once
#include "Container/Hash.h"
#include "Container/Array.h"
#include "Container/String.h"

namespace Gleam {

class CommandLine
{
public:

	CommandLine(int argc, char* argv[]);

	void Parse(int argc, char* argv[]);

	const auto& Flags() const { return mFlags; }
	const auto& Params() const { return mParams; }
	const auto& PositionalArgs() const { return mPositionalArgs; }

	const TString& operator[](size_t index) const { return GetPositionalArg(index); }
	bool operator[](const TString& flag) const { return HasFlag(flag); }

	bool HasFlag(const TString& flag) const;
	bool HasParam(const TString& param) const;
	const TString& GetPositionalArg(size_t index) const;

private:

	TArray<TString> mArgs;
	MultiSet<TString> mFlags;
	MultiMap<TString, TString> mParams;
	TArray<TString> mPositionalArgs;
};

} // namespace Gleam
