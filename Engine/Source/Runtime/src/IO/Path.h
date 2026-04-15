#pragma once
#include "Container/String.h"
#include "Container/Array.h"

#include <filesystem>

namespace Gleam {

GCLASS(Path, "DA162505-C36B-4FF1-BCB7-FE8428606223", Serializable)
{
public:
	static inline constexpr wchar_t kPreferredSeparator = '/';
	static inline constexpr wchar_t kAltSeparator = '\\';

	Path() = default;
	Path(const Path&) = default;
	Path(Path&&) noexcept = default;

	Path(const std::filesystem::path& p) : mPath(p.native()) {}
	Path(std::filesystem::path&& p) : mPath(p.native()) {}

	Path(const wchar_t* wstr) : mPath(wstr) {}

	Path(const TWString& wstr) : mPath(wstr) {}
	Path(TWString&& wstr) : mPath(eastl::move(wstr)) {}
	Path(const TWStringView& wsv) : mPath(wsv) {}

	Path(const std::wstring& wstr) : mPath(wstr) {}
	Path(std::wstring&& wstr) : mPath(wstr) {}
	Path(const std::wstring_view& wsv) : mPath(wsv) {}

	Path(const char* str)
	{
		mPath.append_convert(str);
	}

	Path(const TString & str)
	{
		mPath.append_convert(str);
	}
	Path(const TStringView & str)
	{
		mPath.append_convert(str.data(), str.length());
	}

	Path(const std::string & str)
	{
		mPath.append_convert(str);
	}
	Path(const std::string_view & str)
	{
		mPath.append_convert(str.data(), str.length());
	}

	Path& operator=(const Path&) = default;
	Path& operator=(Path&&) noexcept = default;

	Path& operator=(const std::filesystem::path& p)
	{
		mPath = p.wstring();
		return *this;
	}

	Path& operator=(const std::string& str)
	{
		mPath.clear();
		mPath.append_convert(str);
		return *this;
	}

	Path& operator=(const std::string_view& sv)
	{
		mPath.clear();
		mPath.append_convert(sv.data(), sv.length());
		return *this;
	}

	Path& operator=(const char* str)
	{
		mPath.clear();
		mPath.append_convert(str);
		return *this;
	}

	Path& operator=(const TString& str)
	{
		mPath.clear();
		mPath.append_convert(str);
		return *this;
	}

	Path& operator=(const TStringView& sv)
	{
		mPath.clear();
		mPath.append_convert(sv.data(), sv.length());
		return *this;
	}

	Path& operator=(const TWString& str)
	{
		mPath = str;
		return *this;
	}

	Path& operator=(const TWStringView& sv)
	{
		mPath = sv;
		return *this;
	}

	Path& operator/=(const Path& other)
	{
		if (other.mPath.empty())
		{
			return *this;
		}

		if (mPath.empty())
		{
			mPath = other.mPath;
		}
		else
		{
			if (mPath.back() != kPreferredSeparator && mPath.back() != kAltSeparator)
			{
				mPath.push_back(kPreferredSeparator);
			}
			mPath += other.mPath;
		}
		return *this;
	}

	friend Path operator/(const Path& lhs, const Path& rhs)
	{
		Path result = lhs;
		result /= rhs;
		return result;
	}

	friend Path operator/(const Path& lhs, const char* rhs)
	{
		return lhs / Path(rhs);
	}

	friend Path operator/(const Path & lhs, const std::string_view& rhs)
	{
		return lhs / Path(rhs);
	}

	friend Path operator/(const Path& lhs, const std::string& rhs)
	{
		return lhs / Path(rhs);
	}

	friend Path operator/(const Path& lhs, const std::wstring_view& rhs)
	{
		return lhs / Path(rhs);
	}

	friend Path operator/(const Path& lhs, const std::wstring& rhs)
	{
		return lhs / Path(rhs);
	}

	friend Path operator/(const Path& lhs, const TStringView& rhs)
	{
		return lhs / Path(rhs);
	}

	friend Path operator/(const Path& lhs, const TString& rhs)
	{
		return lhs / Path(rhs);
	}

	friend Path operator/(const Path& lhs, const TWStringView& rhs)
	{
		return lhs / Path(rhs);
	}

	friend Path operator/(const Path& lhs, const TWString& rhs)
	{
		return lhs / Path(rhs);
	}

	const TWString& Native() const
	{
		return mPath;
	}

	TString String() const
	{
		TString str;
		str.append_convert(mPath);
		return str;
	}

	operator TWString() const
	{
		return Native();
	}

	operator std::filesystem::path() const
	{
		return std::filesystem::path(static_cast<const std::wstring&>(mPath));
	}

	bool HasExtension() const
	{
		if (mPath.empty())
		{
			return false;
		}

		auto lastDot = mPath.find_last_of(L'.');
		auto lastSlash = mPath.find_last_of(L"/\\");
		return (lastDot == TWString::npos || (lastSlash != TWString::npos && lastDot < lastSlash)) == false;
	}

	TWStringView Extension() const
	{
		if (mPath.empty())
		{
			return TWStringView{};
		}

		auto lastDot = mPath.find_last_of(L'.');
		auto lastSlash = mPath.find_last_of(L"/\\");
		if (lastDot == TWString::npos || (lastSlash != TWString::npos && lastDot < lastSlash))
		{
			return TWStringView{};
		}

		return TWStringView(mPath.data() + lastDot, mPath.size() - lastDot);
	}

	Path Parent() const
	{
		if (mPath.empty())
		{
			return TWStringView{};
		}

		auto lastSlash = mPath.find_last_of(L"/\\");
		if (lastSlash == TWString::npos)
		{
			return TWStringView{}; // No parent
		}
		return Path(TWStringView(mPath.data(), lastSlash));
	}

	TWStringView Filename() const
	{
		if (mPath.empty())
		{
			return TWStringView{};
		}

		auto lastSlash = mPath.find_last_of(L"/\\");
		if (lastSlash == TWString::npos)
		{
			return mPath; // No parent
		}
		auto start = lastSlash + 1;
		return TWStringView(mPath.data() + start, mPath.size() - start);
	}

	TWStringView Stem() const
	{
		if (mPath.empty())
		{
			return TWStringView{};
		}

		auto lastSlash = mPath.find_last_of(L"/\\");
		auto start = (lastSlash == TWString::npos) ? 0 : lastSlash + 1;
		auto lastDot = mPath.find_last_of(L'.');
		auto end = mPath.size();

		if (lastDot != TWString::npos && (lastSlash == TWString::npos || lastDot > lastSlash))
		{
			end = lastDot;
		}

		if (start >= end)
		{
			return TWStringView{};
		}

		return TWStringView(mPath.data() + start, end - start);
	}

	Path& RemoveFilename()
	{
		if (mPath.empty())
		{
			return *this;
		}

		auto lastSlash = mPath.find_last_of(L"/\\");
		if (lastSlash != TWString::npos)
		{
			mPath = mPath.substr(0, lastSlash);
		}
		return *this;
	}

	Path& Append(const char* str)
	{
		return operator/=(Path(str));
	}

	Path& Append(const wchar_t* str)
	{
		return operator/=(Path(str));
	}

	Path& Append(const TStringView & str)
	{
		return operator/=(Path(str));
	}

	Path& Append(const TString & str)
	{
		return operator/=(Path(str));
	}

	Path& Append(const TWStringView & str)
	{
		return operator/=(Path(str));
	}

	Path& Append(const TWString & str)
	{
		return operator/=(Path(str));
	}

	Path& Append(const std::string_view & str)
	{
		return operator/=(Path(str));
	}

	Path& Append(const std::string & str)
	{
		return operator/=(Path(str));
	}

	Path& Append(const std::wstring_view & str)
	{
		return operator/=(Path(str));
	}

	Path& Append(const std::wstring & str)
	{
		return operator/=(Path(str));
	}

	Path& Concat(const char* str)
	{
		mPath.append_convert(str);
		return *this;
	}

	Path& Concat(const wchar_t* str)
	{
		mPath += str;
		return *this;
	}

	Path& Concat(const TStringView& str)
	{
		mPath.append_convert(str.data(), str.length());
		return *this;
	}

	Path& Concat(const TString& str)
	{
		mPath.append_convert(str);
		return *this;
	}

	Path& Concat(const TWStringView& str)
	{
		mPath += str;
		return *this;
	}

	Path& Concat(const TWString& str)
	{
		mPath += str;
		return *this;
	}

	Path& Concat(const std::string_view& str)
	{
		mPath.append_convert(str.data(), str.length());
		return *this;
	}

	Path& Concat(const std::string& str)
	{
		mPath.append_convert(str);
		return *this;
	}

	Path& Concat(const std::wstring_view& str)
	{
		mPath += str;
		return *this;
	}

	Path& Concat(const std::wstring& str)
	{
		mPath += str;
		return *this;
	}

	void Clear()
	{
		mPath.clear();
	}

	bool Empty() const
	{
		return mPath.empty();
	}

	bool IsRelative() const
	{
		return std::filesystem::path(static_cast<const std::wstring&>(mPath)).is_relative();
	}

	bool IsAbsolute() const
	{
		return std::filesystem::path(static_cast<const std::wstring&>(mPath)).is_absolute();
	}

	Path& MakePreferred()
	{
		eastl::replace(mPath.begin(), mPath.end(), kAltSeparator, kPreferredSeparator);
		return *this;
	}

	TArray<TWStringView> Split() const
	{
		TArray<TWStringView> parts;
		if (mPath.empty())
		{
			return parts;
		}

		size_t start = 0;
		size_t end = 0;

		while (end < mPath.size())
		{
			while (end < mPath.size() && mPath[end] != kPreferredSeparator && mPath[end] != kAltSeparator)
			{
				++end;
			}

			if (end > start)
			{
				parts.push_back(TWStringView(mPath.data() + start, end - start));
			}

			if (end < mPath.size())
			{
				++end;
			}
			start = end;
		}

		return parts;
	}

private:

	TWString mPath;
};

inline bool operator==(const Path& lhs, const Path& rhs) noexcept
{
	const auto& lhsNative = lhs.Native();
	const auto& rhsNative = rhs.Native();

	if (lhsNative.size() != rhsNative.size())
	{
		return false;
	}

	for (size_t i = 0; i < lhsNative.size(); ++i)
	{
		wchar_t lc = lhsNative[i];
		wchar_t rc = rhsNative[i];

		if (lc == Path::kAltSeparator) lc = Path::kPreferredSeparator;
		if (rc == Path::kAltSeparator) rc = Path::kPreferredSeparator;

		if (lc != rc)
		{
			return false;
		}
	}

	return true;
}

inline bool operator!=(const Path& lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator<(const Path& lhs, const Path& rhs) noexcept
{
	const auto& lhsNative = lhs.Native();
	const auto& rhsNative = rhs.Native();

	size_t minSize = lhsNative.size() < rhsNative.size() ? lhsNative.size() : rhsNative.size();
	for (size_t i = 0; i < minSize; ++i)
	{
		wchar_t lc = lhsNative[i];
		wchar_t rc = rhsNative[i];

		if (lc == Path::kAltSeparator) lc = Path::kPreferredSeparator;
		if (rc == Path::kAltSeparator) rc = Path::kPreferredSeparator;

		if (lc < rc) return true;
		if (lc > rc) return false;
	}

	return lhsNative.size() < rhsNative.size();
}

inline bool operator<=(const Path& lhs, const Path& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator>(const Path& lhs, const Path& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>=(const Path& lhs, const Path& rhs) noexcept
{
	return !(lhs < rhs);
}

} // namespace Gleam

namespace std {

inline ostream& operator<<(ostream& os, const Gleam::Path& path)
{
	return os << path.String();
}

inline wostream& operator<<(wostream& os, const Gleam::Path& path)
{
	return os << path.Native();
}

inline istream& operator>>(istream& is, Gleam::Path& path)
{
	Gleam::TString temp;
	is >> temp;
	path = temp;
	return is;
}

inline wistream& operator>>(wistream& is, Gleam::Path& path)
{
	Gleam::TWString temp;
	is >> temp;
	path = temp;
	return is;
}

template<>
struct hash<Gleam::Path>
{
	size_t operator()(const Gleam::Path& path) const noexcept
	{
		const auto& native = path.Native();
		size_t h = 0;

		for (wchar_t c : native)
		{
			if (c == Gleam::Path::kAltSeparator)
			{
				c = Gleam::Path::kPreferredSeparator;
			}
			h = h * 31 + static_cast<size_t>(c);
		}

		return h;
	}
};

} // namespace std

template<>
struct eastl::hash<Gleam::Path>
{
	size_t operator()(const Gleam::Path& path) const noexcept
	{
		const auto& native = path.Native();
		size_t h = 0;

		for (wchar_t c : native)
		{
			if (c == Gleam::Path::kAltSeparator)
			{
				c = Gleam::Path::kPreferredSeparator;
			}
			h = h * 31 + static_cast<size_t>(c);
		}

		return h;
	}
};
