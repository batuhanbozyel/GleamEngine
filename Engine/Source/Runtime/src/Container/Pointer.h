#pragma once
#include "Core/Macro.h"

#include <EASTL/memory.h>
#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/weak_ptr.h>

namespace Gleam {

template<typename T, typename Deleter = eastl::default_delete<T>>
using Scope = eastl::unique_ptr<T>;
    
template<typename T>
using Ref = eastl::reference_wrapper<T>;

template<typename T>
using RefCounted = eastl::shared_ptr<T>;

template<typename T>
using WeakPtr = eastl::weak_ptr<T>;

template<typename T>
using Optional = eastl::optional<T>;

static constexpr auto Null = eastl::nullopt;

template<typename T, typename ... Args>
NO_DISCARD FORCE_INLINE static constexpr Scope<T> CreateScope(Args&& ... args) noexcept
{
	return eastl::make_unique<T>(eastl::forward<Args>(args)...);
}

template<typename T, typename ... Args>
NO_DISCARD FORCE_INLINE static constexpr  RefCounted<T> CreateRef(Args&& ... args) noexcept
{
	return eastl::make_shared<T>(eastl::forward<Args>(args)...);
}

NO_DISCARD FORCE_INLINE static constexpr void* OffsetPointer(void* ptr, size_t offset)
{
    return static_cast<char*>(ptr) + offset;
}

NO_DISCARD FORCE_INLINE static constexpr const void* OffsetPointer(const void* ptr, size_t offset)
{
    return static_cast<const char*>(ptr) + offset;
}

} // namespace Gleam