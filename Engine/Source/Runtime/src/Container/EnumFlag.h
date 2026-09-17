#pragma once
#include <Reflection/Macro.h>

#include <EASTL/functional.h>

#include <cstdint>
#include <functional>
#include <type_traits>

namespace Gleam {

template<typename T, bool = std::is_enum_v<T>>
struct IsScopedEnum : std::false_type {};

template<typename T>
struct IsScopedEnum<T, true> : std::bool_constant<not std::is_convertible_v<T, std::underlying_type_t<T>>> {};

template<typename T>
GCLASS(EnumFlag, "FED89785-D89D-4519-A3E5-7B013F801447", Serializable)
{
	static_assert(std::is_enum_v<T>, "EnumFlag requires an enum type.");

public:

	using EnumType = T;
	using MaskType = std::make_unsigned_t<std::underlying_type_t<T>>;

	constexpr EnumFlag() = default;

	constexpr EnumFlag(T flag)
		: mMask(static_cast<MaskType>(flag))
	{

	}

	explicit constexpr EnumFlag(MaskType mask)
		: mMask(mask)
	{

	}

	constexpr MaskType Value() const
	{
		return mMask;
	}

	constexpr bool Empty() const
	{
		return mMask == 0;
	}

	constexpr bool Has(EnumFlag flags) const
	{
		return (mMask & flags.mMask) == flags.mMask;
	}

	constexpr bool HasAny(EnumFlag flags) const
	{
		return (mMask & flags.mMask) != 0;
	}

	constexpr EnumFlag& Set(EnumFlag flags)
	{
		mMask |= flags.mMask;
		return *this;
	}

	constexpr EnumFlag& Unset(EnumFlag flags)
	{
		mMask &= static_cast<MaskType>(~flags.mMask);
		return *this;
	}

	constexpr EnumFlag& Toggle(EnumFlag flags)
	{
		mMask ^= flags.mMask;
		return *this;
	}

	constexpr void Clear()
	{
		mMask = 0;
	}

	constexpr EnumFlag operator|(EnumFlag rhs) const
	{
		return EnumFlag(static_cast<MaskType>(mMask | rhs.mMask));
	}

	constexpr EnumFlag operator&(EnumFlag rhs) const
	{
		return EnumFlag(static_cast<MaskType>(mMask & rhs.mMask));
	}

	constexpr EnumFlag operator^(EnumFlag rhs) const
	{
		return EnumFlag(static_cast<MaskType>(mMask ^ rhs.mMask));
	}

	constexpr EnumFlag operator~() const
	{
		return EnumFlag(static_cast<MaskType>(~mMask));
	}

	constexpr EnumFlag& operator|=(EnumFlag rhs)
	{
		return Set(rhs);
	}

	constexpr EnumFlag& operator&=(EnumFlag rhs)
	{
		mMask &= rhs.mMask;
		return *this;
	}

	constexpr EnumFlag& operator^=(EnumFlag rhs)
	{
		return Toggle(rhs);
	}

	constexpr bool operator==(const EnumFlag& rhs) const
	{
		return mMask == rhs.mMask;
	}

	constexpr bool operator!=(const EnumFlag& rhs) const
	{
		return mMask != rhs.mMask;
	}

	explicit constexpr operator bool() const
	{
		return mMask != 0;
	}

private:

	MaskType mMask = 0;
};

GENUM(EnumFlagPlaceholder, "04EF4833-2AFF-49E6-AA05-0EC894E5B6E6")
{
	GITEM(None, "ECD82A52-E91F-43BD-834C-1C78291D2A23") = 0
};

template class EnumFlag<EnumFlagPlaceholder>;

// Scoped enums only, so the unscoped flag enums keep integer promotion
template<typename T, std::enable_if_t<IsScopedEnum<T>::value, bool> = true>
constexpr EnumFlag<T> operator|(T lhs, T rhs)
{
	return EnumFlag<T>(lhs) | rhs;
}

template<typename T, std::enable_if_t<IsScopedEnum<T>::value, bool> = true>
constexpr EnumFlag<T> operator&(T lhs, T rhs)
{
	return EnumFlag<T>(lhs) & rhs;
}

template<typename T, std::enable_if_t<IsScopedEnum<T>::value, bool> = true>
constexpr EnumFlag<T> operator^(T lhs, T rhs)
{
	return EnumFlag<T>(lhs) ^ rhs;
}

template<typename T, std::enable_if_t<IsScopedEnum<T>::value, bool> = true>
constexpr EnumFlag<T> operator~(T flag)
{
	return ~EnumFlag<T>(flag);
}

template<typename T, std::enable_if_t<IsScopedEnum<T>::value, bool> = true>
constexpr EnumFlag<T> operator|(T lhs, EnumFlag<T> rhs)
{
	return EnumFlag<T>(lhs) | rhs;
}

template<typename T, std::enable_if_t<IsScopedEnum<T>::value, bool> = true>
constexpr EnumFlag<T> operator&(T lhs, EnumFlag<T> rhs)
{
	return EnumFlag<T>(lhs) & rhs;
}

template<typename T, std::enable_if_t<IsScopedEnum<T>::value, bool> = true>
constexpr EnumFlag<T> operator^(T lhs, EnumFlag<T> rhs)
{
	return EnumFlag<T>(lhs) ^ rhs;
}

} // namespace Gleam

namespace std {

template<typename T>
struct hash<Gleam::EnumFlag<T>>
{
	size_t operator()(const Gleam::EnumFlag<T>& flags) const
	{
		return hash<typename Gleam::EnumFlag<T>::MaskType>()(flags.Value());
	}
};

} // namespace std

namespace eastl {

template<typename T>
struct hash<Gleam::EnumFlag<T>>
{
	size_t operator()(const Gleam::EnumFlag<T>& flags) const
	{
		return std::hash<Gleam::EnumFlag<T>>()(flags);
	}
};

} // namespace eastl
