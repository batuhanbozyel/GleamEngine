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


namespace ArrayUtils {

template<typename T>
static TArray<T> Combine(const TArray<T>& m, const TArray<T>& n)
{
    TArray<T> v;
    v.reserve(m.size() + n.size());
    v.insert(v.end(), m.begin(), m.end());
    v.insert(v.end(), n.begin(), n.end());
    return v;
}

template<typename T>
static TArray<T>& Append(TArray<T>& m, const TArray<T>& n)
{
    m.reserve(m.size() + n.size());
    m.insert(m.end(), n.begin(), n.end());
    return m;
}

} // namespace ArrayUtils

} // namespace Gleam
