#pragma once
#include "Core/Macro.h"
#include "Container/Hash.h"

#include <Reflection/Reflection.h>

namespace Gleam {

uint64_t ResolveTypeLayoutHash(Reflection::MetaType type, uint32_t typeHash);

template<typename T>
NO_DISCARD uint64_t TypeLayoutHash()
{
	if constexpr (Reflection::Traits::IsPrimitive<T>::value)
	{
		constexpr auto primitive = Reflection::GetPrimitive<T>();
		return ResolveTypeLayoutHash(Reflection::MetaType::Primitive, primitive.TypeHash());
	}
	else if constexpr (std::is_enum_v<T>)
	{
		return ResolveTypeLayoutHash(Reflection::MetaType::Enum, Reflection::GetEnum<T>().TypeHash());
	}
	else
	{
		return ResolveTypeLayoutHash(Reflection::MetaType::Class, Reflection::GetClass<T>().TypeHash());
	}
}

template<typename ... Ts>
struct AssetDataLayout
{
	NO_DISCARD static uint64_t Hash()
	{
		static const uint64_t hash = []
		{
			size_t seed = 0;
			(hash_combine(seed, TypeLayoutHash<Ts>()), ...);
			return static_cast<uint64_t>(seed);
		}();
		return hash;
	}
};

} // namespace Gleam
