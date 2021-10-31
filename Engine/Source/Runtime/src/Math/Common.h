#pragma once

namespace Gleam {

namespace Math {

constexpr double PI = 3.141592653589793238;
constexpr float PI_F = 3.141592653f;
constexpr float EPSILON = FLT_EPSILON;
constexpr float BIG_EPSILON = 1e-2f;
constexpr double SMALL_EPSILON = DBL_EPSILON;

template<typename T, PASS_BY_VALUE>
MATH_INLINE constexpr T Cos(T x)
{
	return std::cos(x);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE constexpr T Cos(const T& x)
{
	return std::cos(x);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE constexpr T Sin(T x)
{
	return std::sin(x);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE constexpr T Sin(const T& x)
{
	return std::sin(x);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE constexpr T Tan(T x)
{
	return std::tan(x);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE constexpr T Tan(const T& x)
{
	return std::tan(x);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE T Acos(T x)
{
	return std::acos(x);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE T Acos(const T& x)
{
	return std::acos(x);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE T Asin(T x)
{
	return std::asin(x);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE T Asin(const T& x)
{
	return std::asin(x);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE T Atan(T x)
{
	return std::atan(x);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE T Atan(const T& x)
{
	return std::atan(x);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE T Atan2(T y, T x)
{
	return std::atan2(y, x);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE T Atan2(const T& y, const T& x)
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

template<typename T, PASS_BY_VALUE>
MATH_INLINE constexpr T Abs(T x)
{
	return std::abs(x);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE constexpr T Abs(const T& x)
{
	return std::abs(x);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE constexpr T Deg2Rad(T degree)
{
	return degree * static_cast<T>(0.01745329251994329576923690768489);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE constexpr T Deg2Rad(const T& degree)
{
	return degree * static_cast<T>(0.01745329251994329576923690768489);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE constexpr T Rad2Deg(T radian)
{
	return radian * static_cast<T>(57.295779513082320876798154814105);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE constexpr T Rad2Deg(const T& radian)
{
	return radian * static_cast<T>(57.295779513082320876798154814105);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE constexpr T Min(T v0, T v1)
{
	return std::min(v0, v1);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE constexpr T Min(const T& v0, const T& v1)
{
	return std::min(v0, v1);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE constexpr T Max(T v0, T v1)
{
	return std::max(v0, v1);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE constexpr T Max(const T& v0, const T& v1)
{
	return std::max(v0, v1);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE constexpr T Clamp(T value, T min, T max)
{
	return std::min(std::max(value, min), max);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE constexpr T Clamp(const T& value, const T& min, const T& max)
{
	return std::min(std::max(value, min), max);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE constexpr T Step(T edge, T x)
{
	return x >= edge;
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE constexpr T Step(const T& edge, const T& x)
{
	return x >= edge;
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE constexpr T Smoothstep(T edge0, T edge1, T x)
{
	constexpr T temp = Clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
	return temp * temp * (T(3) - T(2) * temp);
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE constexpr T Smoothstep(const T& edge0, const T& edge1, const T& x)
{
	constexpr T temp = Clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
	return temp * temp * (T(3) - T(2) * temp);
}

template<typename T, PASS_BY_VALUE>
MATH_INLINE constexpr T Mix(T x, T y, T a)
{
	return x * (T(1) - a) + y * a;
}

template<typename T, PASS_BY_REFERENCE>
MATH_INLINE constexpr T Mix(const T& x, const T& y, const T& a)
{
	return x * (T(1) - a) + y * a;
}

}}