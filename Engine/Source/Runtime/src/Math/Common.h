#pragma once

namespace Gleam {

namespace Math {

constexpr float PI = 3.14159265359f;
constexpr float PI_2 = 1.57079632679f;
constexpr float PI_4 = 0.78539816339f;
constexpr float TWO_PI = 6.28318530718f;
constexpr float Epsilon = 1e-5f;
constexpr float BigEpsilon = 1e-2f;
constexpr double SmallEpsilon = 1e-9;
constexpr float Infinity = std::numeric_limits<float>::max();
constexpr float NegativeInfinity = std::numeric_limits<float>::lowest();

template <typename T>
NO_DISCARD FORCE_INLINE constexpr T Sign(const T& val)
{
	return (T(0) <= val) - (val < T(0));
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Cos(const T& x)
{
	return std::cos(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Sin(const T& x)
{
	return std::sin(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Tan(const T& x)
{
	return std::tan(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE T Acos(const T& x)
{
	return std::acos(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE T Asin(const T& x)
{
	return std::asin(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE T Atan(const T& x)
{
	return std::atan(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE T Atan2(const T& y, const T& x)
{
	return std::atan2(y, x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Exp(const T& x)
{
	return std::exp(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Floor(const T& x)
{
	return std::floor(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Round(const T& x)
{
	return std::round(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Fract(const T& x)
{
	return x - static_cast<long>(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Abs(const T& x)
{
	return std::abs(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Deg2Rad(const T& degree)
{
	return degree * static_cast<T>(0.01745329251994329576923690768489);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Rad2Deg(const T& radian)
{
	return radian * static_cast<T>(57.295779513082320876798154814105);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Min(const T& v0, const T& v1)
{
	return std::min(v0, v1);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Max(const T& v0, const T& v1)
{
	return std::max(v0, v1);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Clamp(const T& value, const T& min, const T& max)
{
	return Min(Max(value, min), max);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Step(const T& edge, const T& x)
{
	return static_cast<T>(x >= edge);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Smoothstep(const T& edge0, const T& edge1, const T& x)
{
	constexpr T temp = Clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
	return temp * temp * (T(3) - T(2) * temp);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Mix(const T& x, const T& y, const T& a)
{
	return x * (T(1) - a) + y * a;
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Log2(const T& x)
{
	return std::log2(x);
}

} // namespace Math

} // namespace Gleam
