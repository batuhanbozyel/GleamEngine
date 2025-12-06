#include "gpch.h"
#include "CommandLine.h"

using namespace Gleam;

static bool IsNumber(const TStringView arg)
{
	std::istringstream istr(arg.data(), arg.size());
	double number;
	istr >> number;
	return !(istr.fail() || istr.bad());
}

static bool IsFlag(const TStringView arg)
{
	GLEAM_ASSERT(arg.empty() == false, "Command argument is empty");
	return not IsNumber(arg) && arg[0] == '-';
}

static TStringView NormalizeArg(const TStringView arg)
{
	auto pos = arg.find_first_not_of('-');
	return arg.substr(pos);
}

void CommandLine::Parse(int argc, char* argv[])
{
	// skip program name at argv[0]
	TArray<TString> args;
	for (int i = 1; i < argc; ++i)
	{
		args.emplace_back(argv[i]);
	}

	for (size_t i = 0; i < args.size(); ++i)
	{
		const TString& arg = args[i];
		if (IsFlag(arg))
		{
			TStringView normalized = NormalizeArg(arg);
			if (i + 1 < args.size() && not IsFlag(args[i + 1]))
			{
				mParams.emplace(TString(normalized), args[i + 1]);
				++i; // Skip the value in next iteration
			}
			else
			{
				mFlags.emplace(normalized);
			}
		}
		else
		{
			mPositionalArgs.emplace_back(arg);
		}
	}
}

bool CommandLine::HasFlag(const TStringView flag) const
{
	return eastl::find_if(mFlags.cbegin(), mFlags.cend(), [&](const auto& name)
	{
		return name == NormalizeArg(flag);
	}) != mFlags.cend();
}

bool CommandLine::HasParam(const TStringView param) const
{
	return eastl::find_if(mParams.cbegin(), mParams.cend(), [&](const auto& pair)
	{
		return pair.first == NormalizeArg(param);
	}) != mParams.cend();
}

const TString& CommandLine::GetPositionalArg(size_t index) const
{
	if (index < mPositionalArgs.size())
	{
		return mPositionalArgs[index];
	}
	static TString empty;
	return empty;
}