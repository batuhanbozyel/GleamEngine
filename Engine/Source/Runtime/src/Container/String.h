#pragma once
#include <Reflection/Macro.h>
#include <Reflection/Utils.h>

#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/algorithm.h>

#include <sstream>
#include <string>

static constexpr uint32_t operator"" _hs(const char* str, size_t size)
{
    return Gleam::Reflection::Utils::HashString(str);
}

namespace Gleam {

GCLASS(TString, "CA5CFF7E-51A8-48B7-9315-1A1BF203E855", Serializable)
	: public eastl::string
{
public:
	using eastl::string::string;

	TString() : eastl::string() {}

	TString(const eastl::string& str) : eastl::string(str) {}
	TString(eastl::string&& str) : eastl::string(std::move(str)) {}

	TString(const std::string& str) : eastl::string(str.c_str(), str.size()) {}

	template<size_t N>
	explicit TString(const char(&str)[N]) : eastl::string(str, N) {}
	explicit TString(const char* str) : eastl::string(str) {}

	operator std::string() const
	{
		return std::string(this->c_str(), this->size());
	}

	operator std::string_view() const
	{
		return std::string_view(this->c_str(), this->size());
	}

	template<size_t N>
	TString& operator=(const char(&str)[N])
	{
		this->assign(str, N);
		return *this;
	}

	TString& operator=(const char* str)
	{
		this->assign(str);
		return *this;
	}

	TString& operator=(const std::string& str)
	{
		this->assign(str.c_str(), str.size());
		return *this;
	}

	TString& operator=(const std::string_view& str)
	{
		this->assign(str.data(), str.size());
		return *this;
	}

	TString& operator=(const eastl::string& str)
	{
		eastl::string::operator=(str);
		return *this;
	}

	TString& operator=(const eastl::string_view& str)
	{
		eastl::string::operator=(str);
		return *this;
	}

	TString operator+(const TString& other) const
	{
		TString result(*this);
		result.append(other.c_str(), other.length());
		return result;
	}

	TString operator+(const eastl::string& other) const
	{
		TString result(*this);
		result.append(other.c_str(), other.length());
		return result;
	}

	TString operator+(const std::string& other) const
	{
		TString result(*this);
		result.append(other.c_str(), other.size());
		return result;
	}

	TString operator+(const char* other) const
	{
		TString result(*this);
		result.append(other);
		return result;
	}

	TString operator+(const std::string_view& other) const
	{
		TString result(*this);
		result.append(other.data(), other.size());
		return result;
	}

	TString operator+(const eastl::string_view& other) const
	{
		TString result(*this);
		result.append(other.data(), other.length());
		return result;
	}

	TString& operator+=(const TString& other)
	{
		append(other.c_str(), other.length());
		return *this;
	}

	TString& operator+=(const eastl::string& other)
	{
		append(other.c_str(), other.length());
		return *this;
	}

	TString& operator+=(const std::string& other)
	{
		append(other.c_str(), other.length());
		return *this;
	}

	TString& operator+=(const char* other)
	{
		append(other);
		return *this;
	}

	TString& operator+=(const std::string_view& other)
	{
		append(other.data(), other.length());
		return *this;
	}

	TString& operator+=(const eastl::string_view& other)
	{
		append(other.data(), other.length());
		return *this;
	}
};

GCLASS(TWString, "B2A6136C-2883-4D73-9274-5B11152FDB89", Serializable)
	: public eastl::wstring
{
public:
	using eastl::wstring::wstring;

	TWString() : eastl::wstring() {}

	TWString(const eastl::wstring& str) : eastl::wstring(str) {}
	TWString(eastl::wstring&& str) : eastl::wstring(std::move(str)) {}

	TWString(const std::wstring& str) : eastl::wstring(str.c_str(), str.size()) {}

	template<size_t N>
	explicit TWString(const wchar_t(&str)[N]) : eastl::wstring(str, N) {}
	explicit TWString(const wchar_t* str) : eastl::wstring(str) {}

	operator std::wstring() const
	{
		return std::wstring(this->c_str(), this->size());
	}

	operator std::wstring_view() const
	{
		return std::wstring_view(this->c_str(), this->size());
	}

	template<size_t N>
	TWString& operator=(const wchar_t(&str)[N])
	{
		this->assign(str, N);
		return *this;
	}

	TWString& operator=(const wchar_t* str)
	{
		this->assign(str);
		return *this;
	}

	TWString& operator=(const std::wstring& str)
	{
		this->assign(str.c_str(), str.size());
		return *this;
	}

	TWString& operator=(const std::wstring_view& str)
	{
		this->assign(str.data(), str.size());
		return *this;
	}

	TWString& operator=(const eastl::wstring& str)
	{
		eastl::wstring::operator=(str);
		return *this;
	}

	TWString& operator=(const eastl::wstring_view& str)
	{
		eastl::wstring::operator=(str);
		return *this;
	}

	TWString operator+(const TWString& other) const
	{
		TWString result(*this);
		result.append(other.c_str(), other.length());
		return result;
	}

	TWString operator+(const eastl::wstring& other) const
	{
		TWString result(*this);
		result.append(other.c_str(), other.length());
		return result;
	}

	TWString operator+(const std::wstring& other) const
	{
		TWString result(*this);
		result.append(other.c_str(), other.size());
		return result;
	}

	TWString operator+(const wchar_t* other) const
	{
		TWString result(*this);
		result.append(other);
		return result;
	}

	TWString operator+(const std::wstring_view& other) const
	{
		TWString result(*this);
		result.append(other.data(), other.size());
		return result;
	}

	TWString operator+(const eastl::wstring_view& other) const
	{
		TWString result(*this);
		result.append(other.data(), other.length());
		return result;
	}

	TWString& operator+=(const TWString& other)
	{
		append(other.c_str(), other.length());
		return *this;
	}

	TWString& operator+=(const eastl::wstring& other)
	{
		append(other.c_str(), other.length());
		return *this;
	}

	TWString& operator+=(const std::wstring& other)
	{
		append(other.c_str(), other.length());
		return *this;
	}

	TWString& operator+=(const wchar_t* other)
	{
		append(other);
		return *this;
	}

	TWString& operator+=(const std::wstring_view& other)
	{
		append(other.data(), other.length());
		return *this;
	}

	TWString& operator+=(const eastl::wstring_view& other)
	{
		append(other.data(), other.length());
		return *this;
	}
};

GCLASS(TStringView, "CF685563-78EE-4F19-8CE9-662970F9BD1A", Serializable)
	: public eastl::string_view
{
public:
	using eastl::string_view::string_view;

	constexpr TStringView() : eastl::string_view() {}

	constexpr TStringView(const eastl::string_view& sv) : eastl::string_view(sv) {}

	constexpr TStringView(const std::string_view& sv) : eastl::string_view(sv.data(), sv.size()) {}

	constexpr TStringView(const std::string& str) : eastl::string_view(str.c_str(), str.size()) {}

	constexpr TStringView(const TString& str) : eastl::string_view(str.c_str(), str.size()) {}

	constexpr TStringView(const char* str) : eastl::string_view(str) {}

	constexpr operator std::string_view() const
	{
		return std::string_view(this->data(), this->size());
	}

	constexpr std::string to_string() const
	{
		return std::string(this->data(), this->size());
	}
};

GCLASS(TWStringView, "83A3C509-C3F9-4D8D-A881-F7E875C870F3", Serializable)
	 : public eastl::wstring_view
{
public:
	using eastl::wstring_view::wstring_view;

	constexpr TWStringView() : eastl::wstring_view() {}

	constexpr TWStringView(const eastl::wstring_view& sv) : eastl::wstring_view(sv) {}

	constexpr TWStringView(const std::wstring_view& sv) : eastl::wstring_view(sv.data(), sv.size()) {}

	constexpr TWStringView(const std::wstring& str) : eastl::wstring_view(str.c_str(), str.size()) {}

	constexpr TWStringView(const TWString& str) : eastl::wstring_view(str.c_str(), str.size()) {}

	constexpr TWStringView(const wchar_t* str) : eastl::wstring_view(str) {}

	constexpr operator std::wstring_view() const
	{
		return std::wstring_view(this->data(), this->size());
	}

	constexpr std::wstring to_wstring() const
	{
		return std::wstring(this->data(), this->size());
	}
};

using TStringStream = std::stringstream;

using TWStringStream = std::wstringstream;

namespace StringUtils {

static constexpr uint32_t Hash(const char* str)
{
	return Reflection::Utils::HashString(str);
}

static constexpr uint32_t Hash(const TStringView str)
{
	return Hash(str.data());
}

static constexpr uint32_t Hash(const TString& str)
{
	return Hash(str.c_str());
}

static TString ToLower(const TString& str)
{
    TString newStr = str;
	eastl::transform(newStr.begin(), newStr.end(), newStr.begin(),
        [](char c){ return std::tolower(c); });
    return newStr;
}

static TString ToUpper(const TString& str)
{
    TString newStr = str;
	eastl::transform(newStr.begin(), newStr.end(), newStr.begin(),
        [](char c){ return std::toupper(c); });
    return newStr;
}

static TWString Convert(const TString& as)
{
	if (as.empty()) return TWString();

	TWString str;
	str.append_convert(as);
	return str;
}

static TString Convert(const TWString& as)
{
    if (as.empty()) return TString();

	TString str;
	str.append_convert(as);
	return str;
}

} // namespace StringUtils

// TString comparison operators
inline bool operator==(const TString& lhs, const TString& rhs) noexcept
{
	return static_cast<const eastl::string&>(lhs) == static_cast<const eastl::string&>(rhs);
}

inline bool operator==(const TString& lhs, const TStringView& rhs) noexcept
{
	return TStringView(lhs) == rhs;
}

inline bool operator==(const TString& lhs, const char* rhs) noexcept
{
	return static_cast<const eastl::string&>(lhs) == rhs;
}

inline bool operator==(const TString& lhs, const std::string& rhs) noexcept
{
	return lhs.size() == rhs.size() && eastl::equal(lhs.begin(), lhs.end(), rhs.begin());
}

inline bool operator==(const TString& lhs, const std::string_view& rhs) noexcept
{
	return lhs.size() == rhs.size() && eastl::equal(lhs.begin(), lhs.end(), rhs.begin());
}

inline bool operator!=(const TString& lhs, const TString& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const TString& lhs, const TStringView& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const TString& lhs, const char* rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const TString& lhs, const std::string& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const TString& lhs, const std::string_view& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator<(const TString& lhs, const TString& rhs) noexcept
{
	return static_cast<const eastl::string&>(lhs) < static_cast<const eastl::string&>(rhs);
}

inline bool operator<(const TString& lhs, const TStringView& rhs) noexcept
{
	return TStringView(lhs) < rhs;
}

inline bool operator<(const TString& lhs, const char* rhs) noexcept
{
	return static_cast<const eastl::string&>(lhs) < rhs;
}

inline bool operator<(const TString& lhs, const std::string& rhs) noexcept
{
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

inline bool operator<(const TString& lhs, const std::string_view& rhs) noexcept
{
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

inline bool operator<=(const TString& lhs, const TString& rhs) noexcept
{
	return static_cast<const eastl::string&>(lhs) <= static_cast<const eastl::string&>(rhs);
}

inline bool operator<=(const TString& lhs, const TStringView& rhs) noexcept
{
	return static_cast<const eastl::string&>(lhs) <= static_cast<const eastl::string_view&>(rhs);
}

inline bool operator<=(const TString& lhs, const char* rhs) noexcept
{
	return static_cast<const eastl::string&>(lhs) <= rhs;
}

inline bool operator<=(const TString& lhs, const std::string& rhs) noexcept
{
	return !eastl::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

inline bool operator<=(const TString& lhs, const std::string_view& rhs) noexcept
{
	return !eastl::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

inline bool operator>(const TString& lhs, const TString& rhs) noexcept
{
	return static_cast<const eastl::string&>(lhs) > static_cast<const eastl::string&>(rhs);
}

inline bool operator>(const TString& lhs, const TStringView& rhs) noexcept
{
	return static_cast<const eastl::string&>(lhs) > static_cast<const eastl::string_view&>(rhs);
}

inline bool operator>(const TString& lhs, const char* rhs) noexcept
{
	return static_cast<const eastl::string&>(lhs) > rhs;
}

inline bool operator>(const TString& lhs, const std::string& rhs) noexcept
{
	return eastl::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

inline bool operator>(const TString& lhs, const std::string_view& rhs) noexcept
{
	return eastl::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

inline bool operator>=(const TString& lhs, const TString& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const TString& lhs, const TStringView& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const TString& lhs, const char* rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const TString& lhs, const std::string& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const TString& lhs, const std::string_view& rhs) noexcept
{
	return !(lhs < rhs);
}

// TWString comparison operators
inline bool operator==(const TWString& lhs, const TWString& rhs) noexcept
{
	return static_cast<const eastl::wstring&>(lhs) == static_cast<const eastl::wstring&>(rhs);
}

inline bool operator==(const TWString& lhs, const TWStringView& rhs) noexcept
{
	return TWStringView(lhs) == rhs;
}

inline bool operator==(const TWString& lhs, const wchar_t* rhs) noexcept
{
	return static_cast<const eastl::wstring&>(lhs) == rhs;
}

inline bool operator==(const TWString& lhs, const std::wstring& rhs) noexcept
{
	return lhs.size() == rhs.size() && eastl::equal(lhs.begin(), lhs.end(), rhs.begin());
}

inline bool operator==(const TWString& lhs, const std::wstring_view& rhs) noexcept
{
	return lhs.size() == rhs.size() && eastl::equal(lhs.begin(), lhs.end(), rhs.begin());
}

inline bool operator!=(const TWString& lhs, const TWString& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const TWString& lhs, const TWStringView& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const TWString& lhs, const wchar_t* rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const TWString& lhs, const std::wstring& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const TWString& lhs, const std::wstring_view& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator<(const TWString& lhs, const TWString& rhs) noexcept
{
	return static_cast<const eastl::wstring&>(lhs) < static_cast<const eastl::wstring&>(rhs);
}

inline bool operator<(const TWString& lhs, const TWStringView& rhs) noexcept
{
	return TWStringView(lhs) < rhs;
}

inline bool operator<(const TWString& lhs, const wchar_t* rhs) noexcept
{
	return static_cast<const eastl::wstring&>(lhs) < rhs;
}

inline bool operator<(const TWString& lhs, const std::wstring& rhs) noexcept
{
	return eastl::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

inline bool operator<(const TWString& lhs, const std::wstring_view& rhs) noexcept
{
	return eastl::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

inline bool operator<=(const TWString& lhs, const TWString& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const TWString& lhs, const TWStringView& rhs) noexcept
{
	return static_cast<const eastl::wstring&>(lhs) <= static_cast<const eastl::wstring_view&>(rhs);
}

inline bool operator<=(const TWString& lhs, const wchar_t* rhs) noexcept
{
	return static_cast<const eastl::wstring&>(lhs) <= rhs;
}

inline bool operator<=(const TWString& lhs, const std::wstring& rhs) noexcept
{
	return !eastl::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

inline bool operator<=(const TWString& lhs, const std::wstring_view& rhs) noexcept
{
	return !eastl::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

inline bool operator>(const TWString& lhs, const TWString& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const TWString& lhs, const TWStringView& rhs) noexcept
{
	return static_cast<const eastl::wstring&>(lhs) > static_cast<const eastl::wstring_view&>(rhs);
}

inline bool operator>(const TWString& lhs, const wchar_t* rhs) noexcept
{
	return static_cast<const eastl::wstring&>(lhs) > rhs;
}

inline bool operator>(const TWString& lhs, const std::wstring& rhs) noexcept
{
	return eastl::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

inline bool operator>(const TWString& lhs, const std::wstring_view& rhs) noexcept
{
	return eastl::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

inline bool operator>=(const TWString& lhs, const TWString& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const TWString& lhs, const TWStringView& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const TWString& lhs, const wchar_t* rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const TWString& lhs, const std::wstring& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const TWString& lhs, const std::wstring_view& rhs) noexcept
{
	return !(lhs < rhs);
}

// TStringView comparison operators
inline bool operator==(const TStringView& lhs, const TStringView& rhs) noexcept
{
	return static_cast<const eastl::string_view&>(lhs) == static_cast<const eastl::string_view&>(rhs);
}

inline bool operator!=(const TStringView& lhs, const TStringView& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator<(const TStringView& lhs, const TStringView& rhs) noexcept
{
	return static_cast<const eastl::string_view&>(lhs) < static_cast<const eastl::string_view&>(rhs);
}

inline bool operator<=(const TStringView& lhs, const TStringView& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator>(const TStringView& lhs, const TStringView& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>=(const TStringView& lhs, const TStringView& rhs) noexcept
{
	return !(lhs < rhs);
}

// TWStringView comparison operators
inline bool operator==(const TWStringView& lhs, const TWStringView& rhs) noexcept
{
	return static_cast<const eastl::wstring_view&>(lhs) == static_cast<const eastl::wstring_view&>(rhs);
}

inline bool operator!=(const TWStringView& lhs, const TWStringView& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator<(const TWStringView& lhs, const TWStringView& rhs) noexcept
{
	return static_cast<const eastl::wstring_view&>(lhs) < static_cast<const eastl::wstring_view&>(rhs);
}

inline bool operator<=(const TWStringView& lhs, const TWStringView& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator>(const TWStringView& lhs, const TWStringView& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>=(const TWStringView& lhs, const TWStringView& rhs) noexcept
{
	return !(lhs < rhs);
}

} // namespace Gleam

namespace std {

inline std::ostream& operator<<(std::ostream& os, const Gleam::TString& str)
{
	return os << str.c_str();
}

inline std::ostream& operator<<(std::ostream& os, const Gleam::TStringView& sv)
{
	return os.write(sv.data(), sv.size());
}

inline std::wostream& operator<<(std::wostream& os, const Gleam::TWString& str)
{
	return os << str.c_str();
}

inline std::wostream& operator<<(std::wostream& os, const Gleam::TWStringView& sv)
{
	return os.write(sv.data(), sv.size());
}

inline std::istream& operator>>(std::istream& is, Gleam::TString& str)
{
	std::string temp;
	is >> temp;
	str = temp;
	return is;
}

inline std::wistream& operator>>(std::wistream& is, Gleam::TWString& str)
{
	std::wstring temp;
	is >> temp;
	str = temp;
	return is;
}

inline std::istream& getline(std::istream& is, Gleam::TString& str)
{
	std::string temp;
	std::getline(is, temp);
	str = temp;
	return is;
}

inline std::istream& getline(std::istream& is, Gleam::TString& str, char delim)
{
	std::string temp;
	std::getline(is, temp, delim);
	str = temp;
	return is;
}

inline std::wistream& getline(std::wistream& is, Gleam::TWString& str)
{
	std::wstring temp;
	std::getline(is, temp);
	str = temp;
	return is;
}

inline std::wistream& getline(std::wistream& is, Gleam::TWString& str, wchar_t delim)
{
	std::wstring temp;
	std::getline(is, temp, delim);
	str = temp;
	return is;
}

template<>
struct hash<Gleam::TString>
{
    std::size_t operator()(const Gleam::TString& str) const noexcept
    {
        return std::hash<std::string_view>{}(std::string_view(str.c_str(), str.size()));
    }
};

template<>
struct hash<Gleam::TWString>
{
    std::size_t operator()(const Gleam::TWString& str) const noexcept
    {
        return std::hash<std::wstring_view>{}(std::wstring_view(str.c_str(), str.size()));
    }
};

template<>
struct hash<Gleam::TStringView>
{
    std::size_t operator()(const Gleam::TStringView& sv) const noexcept
    {
        return std::hash<std::string_view>{}(std::string_view(sv.data(), sv.size()));
    }
};

template<>
struct hash<Gleam::TWStringView>
{
    std::size_t operator()(const Gleam::TWStringView& sv) const noexcept
    {
        return std::hash<std::wstring_view>{}(std::wstring_view(sv.data(), sv.size()));
    }
};

} // namespace std

namespace eastl {

template<>
struct hash<Gleam::TString>
{
	std::size_t operator()(const Gleam::TString& str) const
	{
		return eastl::hash<eastl::string_view>{}(eastl::string_view(str.c_str(), str.size()));
	}
};

template<>
struct hash<Gleam::TWString>
{
	std::size_t operator()(const Gleam::TWString& str) const
	{
		return eastl::hash<eastl::wstring_view>{}(eastl::wstring_view(str.c_str(), str.size()));
	}
};

template<>
struct hash<Gleam::TStringView>
{
	std::size_t operator()(const Gleam::TStringView& sv) const
	{
		return eastl::hash<eastl::string_view>{}(eastl::string_view(sv.data(), sv.size()));
	}
};

template<>
struct hash<Gleam::TWStringView>
{
	std::size_t operator()(const Gleam::TWStringView& sv) const
	{
		return eastl::hash<eastl::wstring_view>{}(eastl::wstring_view(sv.data(), sv.size()));
	}
};

} // namespace eastl