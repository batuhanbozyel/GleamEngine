#pragma once
#include "Container/String.h"

#include <filesystem>

namespace Gleam {

GCLASS(Path, "DA162505-C36B-4FF1-BCB7-FE8428606223", Serializable) 
	: public std::filesystem::path
{
public:
    using std::filesystem::path::path;
    
    Path() : std::filesystem::path() {}
    
    Path(const std::filesystem::path& p) : std::filesystem::path(p) {}
    Path(std::filesystem::path&& p) : std::filesystem::path(std::move(p)) {}
    
    Path(const std::string& str) : std::filesystem::path(str) {}
    Path(std::string&& str) : std::filesystem::path(std::move(str)) {}
    Path(const std::string_view& sv) : std::filesystem::path(sv) {}
    Path(const char* str) : std::filesystem::path(str) {}

	Path(const std::wstring& wstr) : std::filesystem::path(wstr) {}
	Path(std::wstring&& wstr) : std::filesystem::path(std::move(wstr)) {}
    Path(const std::wstring_view& wsv) : std::filesystem::path(wsv) {}
    Path(const wchar_t* wstr) : std::filesystem::path(wstr) {}
    
	Path(const TWString& str) : Path(std::move(std::wstring(str))) {}
    Path(const TString& str) : Path(std::move(std::string(str))) {}
    Path(const TStringView& sv) : Path(std::string_view(sv)) {}
    
    Path& operator=(const std::filesystem::path& p)
	{
        std::filesystem::path::operator=(p);
        return *this;
    }
    
    Path& operator=(const std::string& str)
	{
        std::filesystem::path::operator=(str);
        return *this;
    }
    
    Path& operator=(const std::string_view& sv)
	{
        std::filesystem::path::operator=(sv);
        return *this;
    }
    
    Path& operator=(const char* str)
	{
        std::filesystem::path::operator=(str);
        return *this;
    }
    
    Path& operator=(const TString& str)
	{
        std::filesystem::path::operator=(std::string(str));
        return *this;
    }
    
    Path& operator=(const TStringView& sv)
	{
        std::filesystem::path::operator=(std::string_view(sv));
        return *this;
    }
    
    Path& operator/=(const Path& other)
	{
        std::filesystem::path::operator/=(other);
        return *this;
    }

	friend Path operator/(const Path& lhs, const Path& rhs)
	{
		return Path(std::filesystem::path(lhs) / std::filesystem::path(rhs));
	}

	friend Path operator/(const Path& lhs, const char* rhs)
	{
		return Path(std::filesystem::path(lhs) / std::filesystem::path(rhs));
	}

	friend Path operator/(const Path& lhs, const std::string_view& rhs)
	{
		return Path(std::filesystem::path(lhs) / std::filesystem::path(rhs));
	}

	friend Path operator/(const Path& lhs, const std::string& rhs)
	{
		return Path(std::filesystem::path(lhs) / std::filesystem::path(rhs));
	}

	friend Path operator/(const Path& lhs, const std::wstring_view& rhs)
	{
		return Path(std::filesystem::path(lhs) / std::filesystem::path(rhs));
	}

	friend Path operator/(const Path& lhs, const std::wstring& rhs)
	{
		return Path(std::filesystem::path(lhs) / std::filesystem::path(rhs));
	}

	friend Path operator/(const Path& lhs, const TStringView& rhs)
	{
		return Path(std::filesystem::path(lhs) / std::filesystem::path(std::string_view(rhs)));
	}

	friend Path operator/(const Path& lhs, const TString& rhs)
	{
		return Path(std::filesystem::path(lhs) / std::filesystem::path(std::string(rhs)));
	}

	friend Path operator/(const Path& lhs, const TWStringView& rhs)
	{
		return Path(std::filesystem::path(lhs) / std::filesystem::path(std::wstring_view(rhs)));
	}

	friend Path operator/(const Path& lhs, const TWString& rhs)
	{
		return Path(std::filesystem::path(lhs) / std::filesystem::path(std::wstring(rhs)));
	}

    TString String() const
	{
        return TString(this->string());
    }
    
    operator TString() const
	{
        return String();
    }

	Path Extension() const
	{
		return extension();
	}

	Path Parent() const
	{
		return parent_path();
	}

	Path Stem() const
	{
		return stem();
	}

	template<size_t N>
	Path& Append(const char(&str)[N])
	{
		append(std::string_view(str, N));
		return *this;
	}

	Path& Append(const char* str)
	{
		append(str);
		return *this;
	}

	Path& Append(const TStringView& str)
	{
		append(std::string_view(str));
		return *this;
	}

	Path& Append(const TString& str)
	{
		append(std::string(str));
		return *this;
	}
};

// == operators
inline bool operator==(const Path& lhs, const Path& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) == static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator==(const Path& lhs, const std::filesystem::path& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) == rhs;
}

inline bool operator==(const std::filesystem::path& lhs, const Path& rhs) noexcept
{
	return lhs == static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator==(const Path& lhs, const std::string& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) == std::filesystem::path(rhs);
}

inline bool operator==(const std::string& lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(lhs) == static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator==(const Path& lhs, const std::string_view& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) == std::filesystem::path(rhs);
}

inline bool operator==(const std::string_view& lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(lhs) == static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator==(const Path& lhs, const char* rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) == std::filesystem::path(rhs);
}

inline bool operator==(const char* lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(lhs) == static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator==(const Path& lhs, const std::wstring& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) == std::filesystem::path(rhs);
}

inline bool operator==(const std::wstring& lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(lhs) == static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator==(const Path& lhs, const std::wstring_view& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) == std::filesystem::path(rhs);
}

inline bool operator==(const std::wstring_view& lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(lhs) == static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator==(const Path& lhs, const wchar_t* rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) == std::filesystem::path(rhs);
}

inline bool operator==(const wchar_t* lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(lhs) == static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator==(const Path& lhs, const TString& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) == std::filesystem::path(std::string(rhs));
}

inline bool operator==(const TString& lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(std::string(lhs)) == static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator==(const Path& lhs, const TStringView& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) == std::filesystem::path(std::string_view(rhs));
}

inline bool operator==(const TStringView& lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(std::string_view(lhs)) == static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator==(const Path& lhs, const TWString& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) == std::filesystem::path(std::wstring(rhs));
}

inline bool operator==(const TWString& lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(std::wstring(lhs)) == static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator==(const Path& lhs, const TWStringView& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) == std::filesystem::path(std::wstring_view(rhs));
}

inline bool operator==(const TWStringView& lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(std::wstring_view(lhs)) == static_cast<const std::filesystem::path&>(rhs);
}

// != operators
inline bool operator!=(const Path& lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const Path& lhs, const std::filesystem::path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const std::filesystem::path& lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const Path& lhs, const std::string& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const std::string& lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const Path& lhs, const std::string_view& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const std::string_view& lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const Path& lhs, const char* rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const char* lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const Path& lhs, const std::wstring& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const std::wstring& lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const Path& lhs, const std::wstring_view& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const std::wstring_view& lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const Path& lhs, const wchar_t* rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const wchar_t* lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const Path& lhs, const TString& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const TString& lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const Path& lhs, const TStringView& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const TStringView& lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const Path& lhs, const TWString& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const TWString& lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const Path& lhs, const TWStringView& rhs) noexcept
{
	return !(lhs == rhs);
}

inline bool operator!=(const TWStringView& lhs, const Path& rhs) noexcept
{
	return !(lhs == rhs);
}

// < operators
inline bool operator<(const Path& lhs, const Path& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) < static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator<(const Path& lhs, const std::filesystem::path& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) < rhs;
}

inline bool operator<(const std::filesystem::path& lhs, const Path& rhs) noexcept
{
	return lhs < static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator<(const Path& lhs, const std::string& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) < std::filesystem::path(rhs);
}

inline bool operator<(const std::string& lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(lhs) < static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator<(const Path& lhs, const std::string_view& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) < std::filesystem::path(rhs);
}

inline bool operator<(const std::string_view& lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(lhs) < static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator<(const Path& lhs, const char* rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) < std::filesystem::path(rhs);
}

inline bool operator<(const char* lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(lhs) < static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator<(const Path& lhs, const TString& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) < std::filesystem::path(std::string(rhs));
}

inline bool operator<(const TString& lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(std::string(lhs)) < static_cast<const std::filesystem::path&>(rhs);
}

inline bool operator<(const Path& lhs, const TStringView& rhs) noexcept
{
	return static_cast<const std::filesystem::path&>(lhs) < std::filesystem::path(std::string_view(rhs));
}

inline bool operator<(const TStringView& lhs, const Path& rhs) noexcept
{
	return std::filesystem::path(std::string_view(lhs)) < static_cast<const std::filesystem::path&>(rhs);
}

// <= operators
inline bool operator<=(const Path& lhs, const Path& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const Path& lhs, const std::filesystem::path& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const std::filesystem::path& lhs, const Path& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const Path& lhs, const std::string& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const std::string& lhs, const Path& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const Path& lhs, const std::string_view& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const std::string_view& lhs, const Path& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const Path& lhs, const char* rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const char* lhs, const Path& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const Path& lhs, const TString& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const TString& lhs, const Path& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const Path& lhs, const TStringView& rhs) noexcept
{
	return !(rhs < lhs);
}

inline bool operator<=(const TStringView& lhs, const Path& rhs) noexcept
{
	return !(rhs < lhs);
}

// > operators
inline bool operator>(const Path& lhs, const Path& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const Path& lhs, const std::filesystem::path& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const std::filesystem::path& lhs, const Path& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const Path& lhs, const std::string& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const std::string& lhs, const Path& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const Path& lhs, const std::string_view& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const std::string_view& lhs, const Path& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const Path& lhs, const char* rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const char* lhs, const Path& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const Path& lhs, const TString& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const TString& lhs, const Path& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const Path& lhs, const TStringView& rhs) noexcept
{
	return rhs < lhs;
}

inline bool operator>(const TStringView& lhs, const Path& rhs) noexcept
{
	return rhs < lhs;
}

// >= operators
inline bool operator>=(const Path& lhs, const Path& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const Path& lhs, const std::filesystem::path& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const std::filesystem::path& lhs, const Path& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const Path& lhs, const std::string& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const std::string& lhs, const Path& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const Path& lhs, const std::string_view& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const std::string_view& lhs, const Path& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const Path& lhs, const char* rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const char* lhs, const Path& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const Path& lhs, const TString& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const TString& lhs, const Path& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const Path& lhs, const TStringView& rhs) noexcept
{
	return !(lhs < rhs);
}

inline bool operator>=(const TStringView& lhs, const Path& rhs) noexcept
{
	return !(lhs < rhs);
}

} // namespace Gleam

template<>
struct std::hash<Gleam::Path>
{
	size_t operator()(const Gleam::Path& path) const noexcept
	{
		return std::hash<std::filesystem::path>()(static_cast<const std::filesystem::path&>(path));
	}
};

template<>
struct eastl::hash<Gleam::Path>
{
	size_t operator()(const Gleam::Path& path) const noexcept
	{
		return std::hash<Gleam::Path>()(path);
	}
};