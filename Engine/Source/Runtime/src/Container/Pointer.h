#pragma once
#include <memory>
#include <optional>

namespace Gleam {

template<typename T>
using Scope = std::unique_ptr<T>;
    
template<typename T>
using Ref = std::reference_wrapper<T>;

template<typename T>
using RefCounted = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

template<typename T>
using Optional = std::optional<T>;

static constexpr auto Null = std::nullopt;

template<typename T, typename ... Args>
NO_DISCARD FORCE_INLINE constexpr Scope<T> CreateScope(Args&& ... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename ... Args>
NO_DISCARD FORCE_INLINE constexpr  RefCounted<T> CreateRef(Args&& ... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

} // namespace Gleam
