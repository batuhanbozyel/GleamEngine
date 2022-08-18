#pragma once
#include <memory>

namespace Gleam {

template<typename T>
using Scope = std::unique_ptr<T>;
    
template<typename T>
using Ref = std::reference_wrapper<T>;

template<typename T>
using RefCounted = std::shared_ptr<T>;

template<typename T>
using Weak = std::weak_ptr<T>;

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
