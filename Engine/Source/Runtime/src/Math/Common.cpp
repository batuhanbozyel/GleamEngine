#include "gpch.h"
#include "Common.h"

using namespace Gleam::Math;

template<typename T>
[[nodiscard]] T Acos(T x)
{
	return std::acos(x);
}

template<typename T>
[[nodiscard]] T Asin(T x)
{
	return std::asin(x);
}

template<typename T>
[[nodiscard]] T Atan(T x)
{
	return std::atan(x);
}

template<typename T>
[[nodiscard]] T Atan2(T y, T x)
{
	return std::atan2(y, x);
}

template<typename T>
[[nodiscard]] constexpr T PI()
{
	return glm::pi<T>();
}

template<typename T>
[[nodiscard]] constexpr T Epsilon()
{
	return std::numeric_limits<T>::epsilon();
}

template<typename T>
[[nodiscard]] constexpr T Infinity()
{
	return std::numeric_limits<T>::infinity();
}

template<typename T>
[[nodiscard]] constexpr T NegativeInfinity()
{
	return -std::numeric_limits<T>::infinity();
}

template<typename T>
[[nodiscard]] constexpr T Abs(T x)
{
	return std::abs(x);
}

template<typename T>
[[nodiscard]] constexpr T Deg2Rad(T degree)
{
	return glm::radians(degree);
}

template<typename T>
[[nodiscard]] constexpr T Rad2Deg(T radian)
{
	return glm::degrees(radian);
}