#pragma once

namespace Gleam {

namespace Math {

template<typename T>
[[nodiscard]] T Acos(T x);

template<typename T>
[[nodiscard]] T Asin(T x);

template<typename T>
[[nodiscard]] T Atan(T x);

template<typename T>
[[nodiscard]] T Atan2(T y, T x);

template<typename T>
[[nodiscard]] constexpr T PI();

template<typename T>
[[nodiscard]] constexpr T Epsilon();

template<typename T>
[[nodiscard]] constexpr T Infinity();

template<typename T>
[[nodiscard]] constexpr T NegativeInfinity();

template<typename T>
[[nodiscard]] constexpr T Abs(T x);

template<typename T>
[[nodiscard]] constexpr T Deg2Rad(T degree);

template<typename T>
[[nodiscard]] constexpr T Rad2Deg(T radian);

}
}