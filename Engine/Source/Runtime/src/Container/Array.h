#pragma once
#include <array>
#include <vector>

namespace Gleam {

template<typename T, size_t size>
struct ArrayHelper
{
	typedef std::array<T, size> Type;
};

template<typename T>
struct ArrayHelper<T, 0>
{
	typedef std::vector<T> Type;
};

template<typename T, size_t size = 0>
using TArray = typename ArrayHelper<T, size>::Type;

}