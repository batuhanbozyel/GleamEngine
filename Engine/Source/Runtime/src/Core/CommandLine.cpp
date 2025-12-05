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

static TString NormalizeArg(const TString& arg)
{
	auto pos = arg.find_first_not_of('-');
	return arg.substr(pos);
}

CommandLine::CommandLine(int argc, char* argv[])
{
	Parse(argc, argv);
}

void CommandLine::Parse(int argc, char* argv[])
{
	// skip program name at argv[0]
	for (int i = 1; i < argc; ++i)
	{
		mArgs.emplace_back(argv[i]);
	}

	for (size_t i = 0; i < mArgs.size(); ++i)
	{
		const TString& arg = mArgs[i];
		if (IsFlag(arg))
		{
			TString normalized = NormalizeArg(arg);

			// Check if next argument is a value (not a flag)
			if (i + 1 < mArgs.size() && !IsFlag(mArgs[i + 1]))
			{
				mParams.emplace(normalized, mArgs[i + 1]);
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

bool CommandLine::HasFlag(const TString& flag) const
{
	return mFlags.find(NormalizeArg(flag)) != mFlags.end();
}

bool CommandLine::HasParam(const TString& param) const
{
	return mParams.find(NormalizeArg(param)) != mParams.end();
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