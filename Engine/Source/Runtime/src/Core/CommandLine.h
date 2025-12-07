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

	bool HasFlag(const TStringView flag) const;
	bool HasParam(const TStringView param) const;
	const TString& GetPositionalArg(size_t index) const;

	const TString& operator[](size_t index) const { return GetPositionalArg(index); }
	bool operator[](const TStringView flag) const { return HasFlag(flag); }
	bool operator[](std::initializer_list<char const* const> flags) const
	{
		return eastl::any_of(flags.begin(), flags.end(), [&](char const* const flag) { return HasFlag(flag); });
	}

	template<typename T>
	T GetParam(const TStringView param, T&& defaultValue) const
	{
		auto normalized = NormalizeArg(param);
		auto it = eastl::find_if(mParams.begin(), mParams.end(), [&](const auto& pair)
		{
			return pair.first == normalized;
		});

		if (mParams.end() != it)
		{
			TStringStream ss(it->second);
			T val = {};
			ss >> val;
			return val;
		}
		return defaultValue;
	}

	template<typename T>
	T GetParam(std::initializer_list<char const* const> params, T&& defaultValue) const
	{
		for (auto& param : params)
		{
			auto normalized = NormalizeArg(param);
			auto it = eastl::find_if(mParams.begin(), mParams.end(), [&](const auto& pair)
			{
				return pair.first == normalized;
			});

			if (mParams.end() != it)
			{
				TStringStream ss(it->second);
				T val = {};
				ss >> val;
				return val;
			}
		}
		return defaultValue;
	}

	template<typename T>
	T operator()(const TStringView param, T&& defaultValue) const
	{
		return GetParam<T>(param, eastl::move(defaultValue));
	}

	template<typename T>
	T operator()(std::initializer_list<char const* const> params, T&& defaultValue) const
	{
		return GetParam<T>(eastl::move(params), eastl::move(defaultValue));
	}

	auto begin() const { return mPositionalArgs.cbegin(); }
	auto end() const { return mPositionalArgs.cend(); }
	size_t size() const { return mPositionalArgs.size(); }

private:

	static bool IsNumber(const TStringView arg);
	static bool IsFlag(const TStringView arg);
	static TStringView NormalizeArg(const TStringView arg);

	MultiSet<TString> mFlags;
	MultiMap<TString, TString> mParams;
	TArray<TString> mPositionalArgs;
};

} // namespace Gleam
