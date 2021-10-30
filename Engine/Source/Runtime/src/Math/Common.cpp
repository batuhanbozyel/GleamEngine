#include "gpch.h"
#include "Common.h"

using namespace Gleam;

template<typename T>
[[nodiscard]] T Math::Acos(T x)
{
	return std::acos(x);
}

template<typename T>
[[nodiscard]] T Math::Asin(T x)
{
	return std::asin(x);
}

template<typename T>
[[nodiscard]] T Math::Atan(T x)
{
	return std::atan(x);
}

template<typename T>
[[nodiscard]] T Math::Atan2(T y, T x)
{
	return std::atan2(y, x);
}

template<typename T>
[[nodiscard]] constexpr T Math::PI()
{
	return glm::pi<T>();
}

template<typename T>
[[nodiscard]] constexpr T Math::Epsilon()
{
	return std::numeric_limits<T>::epsilon();
}

template<typename T>
[[nodiscard]] constexpr T Math::Infinity()
{
	return std::numeric_limits<T>::infinity();
}

template<typename T>
[[nodiscard]] constexpr T Math::NegativeInfinity()
{
	return -std::numeric_limits<T>::infinity();
}

template<typename T>
[[nodiscard]] constexpr T Math::Abs(T x)
{
	return std::abs(x);
}

template<typename T>
[[nodiscard]] constexpr T Math::Deg2Rad(T degree)
{
	return glm::radians(degree);
}

template<typename T>
[[nodiscard]] constexpr T Math::Rad2Deg(T radian)
{
	return glm::degrees(radian);
}