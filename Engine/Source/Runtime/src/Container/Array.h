#pragma once
#include <Reflection/Macro.h>
#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <EASTL/span.h>
#include <EASTL/fixed_vector.h>

namespace Gleam {

template<typename T, size_t size>
struct ArrayHelper
{
	typedef eastl::array<T, size> Type;
};

template<typename T>
struct ArrayHelper<T, 0>
{
	typedef eastl::vector<T> Type;
};

template<typename T, size_t size = 0>
using TArray = typename ArrayHelper<T, size>::Type;

template<typename T>
using TArrayView = eastl::span<T>;

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

namespace Gleam::Reflection::External::eastl {

template<typename T, typename Allocator = ::eastl::allocator>
class vector {};

template<>
GCLASS(vector<uint8_t>, "0413C2C5-88CB-49B8-A3EA-9AD7505E51E6", Serializable)
{

};

} // namespace Gleam::Reflection::External
