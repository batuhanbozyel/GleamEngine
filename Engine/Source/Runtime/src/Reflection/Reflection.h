#pragma once
#include "Database.h"

namespace Gleam::Reflection {

template<typename T>
static constexpr const ClassDescription& GetClass()
{
    return Database::CreateClassIfNotExist<T>();
}

static const ClassDescription& GetClass(size_t hash)
{
    return Database::GetClass(hash);
}

static const ClassDescription& GetClass(const TStringView name)
{
	auto hash = Database::GetTypeHash(name);
	return GetClass(hash);
}

template<typename T, typename = std::enable_if_t<Traits::IsEnum<T>::value>>
static constexpr const EnumDescription& GetEnum()
{
    return Database::CreateEnumIfNotExist<T>();
}

static const EnumDescription& GetEnum(size_t hash)
{
    return Database::GetEnum(hash);
}

static const EnumDescription& GetEnum(const TStringView name)
{
	auto hash = Database::GetTypeHash(name);
	return GetEnum(hash);
}

template<typename T, typename = std::enable_if_t<Traits::IsArray<T>::value>>
static constexpr const ArrayDescription& GetArray()
{
    return Database::CreateArrayIfNotExist<T>();
}

static const ArrayDescription& GetArray(size_t hash)
{
    return Database::GetArray(hash);
}

template<typename T>
static constexpr T& Get(void* ptr)
{
    return *static_cast<T*>(ptr);
}

template<typename T>
static constexpr const T& Get(const void* ptr)
{
    return *static_cast<const T*>(ptr);
}

} // namespace Gleam::Reflection
