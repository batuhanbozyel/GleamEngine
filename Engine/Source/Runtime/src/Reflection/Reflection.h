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

} // namespace Gleam::Reflection
