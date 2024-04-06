#pragma once
#include "Database.h"

namespace Gleam::Reflection {

template<typename T>
static constexpr const ClassDescription& GetClass()
{
    return Database::GetClass<T>();
}

static constexpr const ClassDescription& GetClass(size_t hash)
{
    return Database::GetClass(hash);
}

template<typename T, typename = std::enable_if_t<Traits::IsEnum<T>::value>>
static constexpr const EnumDescription& GetEnum()
{
    return Database::GetEnum<T>();
}

static constexpr const EnumDescription& GetEnum(size_t hash)
{
    return Database::GetEnum(hash);
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
