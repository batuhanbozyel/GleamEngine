#pragma once
#define MATH_INLINE [[nodiscard]] inline
#define PI 3.141592653589793238
#define PI_F 3.141592653f
#define EPSILON 1e-5f
#define BIG_EPSILON 1e-2f
#define SMALL_EPSILON 1e-7

namespace Gleam {

namespace Math {

template<typename T>
MATH_INLINE constexpr T Cos(T x)
{
	return std::cos(x);
}

template<typename T>
MATH_INLINE constexpr T Sin(T x)
{
	return std::sin(x);
}

template<typename T>
MATH_INLINE constexpr T Tan(T x)
{
	return std::tan(x);
}

template<typename T>
MATH_INLINE T Acos(T x)
{
	return std::acos(x);
}

template<typename T>
MATH_INLINE T Asin(T x)
{
	return std::asin(x);
}

template<typename T>
MATH_INLINE T Atan(T x)
{
	return std::atan(x);
}

template<typename T>
MATH_INLINE T Atan2(T y, T x)
{
	return std::atan2(y, x);
}

template<typename T>
MATH_INLINE constexpr T Epsilon()
{
	return std::numeric_limits<T>::epsilon();
}

template<typename T>
MATH_INLINE constexpr T Infinity()
{
	return std::numeric_limits<T>::infinity();
}

template<typename T>
MATH_INLINE constexpr T Abs(T x)
{
	return std::abs(x);
}

template<typename T>
MATH_INLINE constexpr T Deg2Rad(T degree)
{
	return degree * static_cast<T>(0.01745329251994329576923690768489);
}

template<typename T>
MATH_INLINE constexpr T Rad2Deg(T radian)
{
	return radian * static_cast<T>(57.295779513082320876798154814105);
}

}}