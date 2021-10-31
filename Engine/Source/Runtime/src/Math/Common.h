#pragma once
#define PI 3.14159265359f

namespace Gleam {

namespace Math {

template<typename T>
[[nodiscard]] inline constexpr T Cos(T x)
{
	return std::cos(x);
}

template<typename T>
[[nodiscard]] inline constexpr T Sin(T x)
{
	return std::sin(x);
}

template<typename T>
[[nodiscard]] inline constexpr T Tan(T x)
{
	return std::tan(x);
}

template<typename T>
[[nodiscard]] inline T Acos(T x)
{
	return std::acos(x);
}

template<typename T>
[[nodiscard]] inline T Asin(T x)
{
	return std::asin(x);
}

template<typename T>
[[nodiscard]] inline T Atan(T x)
{
	return std::atan(x);
}

template<typename T>
[[nodiscard]] inline T Atan2(T y, T x)
{
	return std::atan2(y, x);
}

template<typename T>
[[nodiscard]] inline constexpr T Epsilon()
{
	return std::numeric_limits<T>::epsilon();
}

template<typename T>
[[nodiscard]] inline constexpr T Infinity()
{
	return std::numeric_limits<T>::infinity();
}

template<typename T>
[[nodiscard]] inline constexpr T Abs(T x)
{
	return std::abs(x);
}

template<typename T>
[[nodiscard]] inline constexpr T Deg2Rad(T degree)
{
	return degree * static_cast<T>(0.01745329251994329576923690768489);
}

template<typename T>
[[nodiscard]] inline constexpr T Rad2Deg(T radian)
{
	return radian * static_cast<T>(57.295779513082320876798154814105);
}

}}