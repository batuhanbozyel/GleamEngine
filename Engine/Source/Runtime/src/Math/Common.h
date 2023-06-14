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
NO_DISCARD FORCE_INLINE constexpr T Sign(T val)
{
    return (T(0) <= val) - (val < T(0));
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Cos(T x)
{
    return std::cos(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Sin(T x)
{
    return std::sin(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Tan(T x)
{
    return std::tan(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE T Acos(T x)
{
    return std::acos(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE T Asin(T x)
{
    return std::asin(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE T Atan(T x)
{
    return std::atan(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE T Atan2(T y, T x)
{
    return std::atan2(y, x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Exp(T x)
{
    return std::exp(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Floor(T x)
{
    return static_cast<T>(std::floor(x));
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Round(T x)
{
    return static_cast<T>(std::round(x));
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Fract(T x)
{
    return x - static_cast<long>(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Abs(T x)
{
    return std::abs(x);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Deg2Rad(T degree)
{
    return degree * static_cast<T>(0.01745329251994329576923690768489);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Rad2Deg(T radian)
{
    return radian * static_cast<T>(57.295779513082320876798154814105);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Min(T v0, T v1)
{
    return std::min(v0, v1);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Max(T v0, T v1)
{
    return std::max(v0, v1);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Clamp(T value, T min, T max)
{
    return Min(Max(value, min), max);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Step(T edge, T x)
{
    return static_cast<T>(x >= edge);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Smoothstep(T edge0, T edge1, T x)
{
    constexpr T temp = Clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
    return temp * temp * (T(3) - T(2) * temp);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Mix(T x, T y, T a)
{
    return x * (T(1) - a) + y * a;
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Log2(T x)
{
    return static_cast<T>(std::log2(x));
}

template<typename T>
NO_DISCARD FORCE_INLINE T constexpr Sqrt(T v)
{
    return static_cast<T>(std::sqrt(v));
}

} // namespace Math

} // namespace Gleam
