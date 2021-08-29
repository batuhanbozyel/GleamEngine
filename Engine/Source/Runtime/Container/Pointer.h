#pragma once
#include <memory>

namespace Gleam {

template<class T>
using UniquePtr = std::unique_ptr<T>;

template<class T>
using SharedPtr = std::shared_ptr<T>;

template<class T>
using WeakPtr = std::weak_ptr<T>;

template<class T, class ... Args>
constexpr UniquePtr<T> MakeUnique(Args&& ... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<class T, class ... Args>
constexpr SharedPtr<T> MakeShared(Args&& ... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

}