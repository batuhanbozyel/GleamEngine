#pragma once
#include <Reflection/Reflection.h>

#include "Container/Hash.h"
#include "Container/Array.h"
#include "Container/String.h"

namespace Gleam {

GCLASS(Guid, "54539D64-BE9B-49AC-B31E-E90FF8967441", Serializable) : public Reflection::Attribute::Guid
{
public:

	static Guid NewGuid();
    static Guid Combine(const Guid& guid1, const Guid& guid2);

	constexpr Guid() = default;
	constexpr Guid(const Reflection::Attribute::Guid& guid)
		: Reflection::Attribute::Guid(guid)
	{

	}
	constexpr Guid(Reflection::Attribute::Guid&& guid)
		: Reflection::Attribute::Guid(std::move(guid))
	{

	}

	constexpr Guid& operator=(const Reflection::Attribute::Guid& guid)
	{
		Reflection::Attribute::Guid::operator=(guid);
		return *this;
	}
	constexpr Guid& operator=(Reflection::Attribute::Guid&& guid)
	{
		Reflection::Attribute::Guid::operator=(std::move(guid));
		return *this;
	}

	template<size_t N>
	explicit constexpr Guid(const char(&str)[N])
		: Reflection::Attribute::Guid(str)
	{
		static_assert(N == 37, "Guid string must be 36 characters long plus null terminator.");
	}

	explicit constexpr Guid(const char* str)
		: Reflection::Attribute::Guid(str)
	{
	}

	Guid(const TString& guid)
		: Reflection::Attribute::Guid(guid.c_str())
	{

	}
	Guid(const std::string& guid)
		: Reflection::Attribute::Guid(guid.c_str())
	{

	}
	Guid& operator=(const TString& guid)
	{
		*this = Guid(guid);
		return *this;
	}
	Guid& operator=(const std::string& guid)
	{
		*this = Guid(guid);
		return *this;
	}
};

inline bool operator==(const Reflection::Attribute::Guid& lhs, const Guid& rhs)
{
	return (Reflection::Attribute::Guid)rhs == lhs;
}

inline bool operator!=(const Reflection::Attribute::Guid& lhs, const Guid& rhs)
{
	return (Reflection::Attribute::Guid)rhs != lhs;
}

} // namespace Gleam

template <>
struct std::hash<Gleam::Guid>
{
	size_t operator()(const Gleam::Guid& guid) const
	{
		return std::hash<Gleam::Reflection::Attribute::Guid>()(guid);
	}
};

template <>
struct eastl::hash<Gleam::Guid>
{
	size_t operator()(const Gleam::Guid& guid) const
	{
		return std::hash<Gleam::Reflection::Attribute::Guid>()(guid);
	}
};
