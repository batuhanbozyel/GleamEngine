#pragma once

namespace Gleam {
	
template<typename T>
struct Vector2
{
    union
    {
        struct
        {
            T x, y;
        };
        struct
        {
            T r, g;
        };
        TArray<T, 2> value{};
    };

    static const Vector2 zero;
    static const Vector2 one;

    constexpr Vector2() = default;
    constexpr Vector2(Vector2&&) noexcept = default;
    constexpr Vector2(const Vector2&) = default;
    FORCE_INLINE constexpr Vector2& operator=(Vector2&&) noexcept = default;
    FORCE_INLINE constexpr Vector2& operator=(const Vector2&) = default;

    constexpr Vector2(T v)
        : x(v), y(v)
    {

    }
    constexpr Vector2(T x, T y)
        : x(x), y(y)
    {

    }
    constexpr Vector2(const TArray<T, 2>& vec)
        : x(vec[0]), y(vec[1])
    {

    }

    NO_DISCARD FORCE_INLINE constexpr bool operator==(const Vector2& vec) const
    {
        return x == vec.x && y == vec.y;
    }

    NO_DISCARD FORCE_INLINE constexpr T& operator[](size_t i)
    {
        return value[i];
    }

    NO_DISCARD FORCE_INLINE constexpr const T& operator[](size_t i) const
    {
        return value[i];
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2 operator-() const
    {
        return Vector2
        {
            -x,
            -y
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2 operator*(T val) const
    {
        return Vector2
        {
            x * val,
            y * val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2 operator/(T val) const
    {
        return Vector2
        {
            x / val,
            y / val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2 operator+(T val) const
    {
        return Vector2
        {
            x + val,
            y + val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2 operator-(T val) const
    {
        return Vector2
        {
            x - val,
            y - val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2 operator*(const Vector2& vec) const
    {
        return Vector2
        {
            x * vec.x,
            y * vec.y
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2 operator/(const Vector2& vec) const
    {
        return Vector2
        {
            x / vec.x,
            y / vec.y
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2 operator+(const Vector2& vec) const
    {
        return Vector2
        {
            x + vec.x,
            y + vec.y
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2 operator-(const Vector2& vec) const
    {
        return Vector2
        {
            x - vec.x,
            y - vec.y
        };
    }

    FORCE_INLINE constexpr Vector2& operator*=(T val)
    {
        x *= val;
        y *= val;
        return *this;
    }

    FORCE_INLINE constexpr Vector2& operator/=(T val)
    {
        x /= val;
        y /= val;
        return *this;
    }

    FORCE_INLINE constexpr Vector2& operator+=(T val)
    {
        x += val;
        y += val;
        return *this;
    }

    FORCE_INLINE constexpr Vector2& operator-=(T val)
    {
        x -= val;
        y -= val;
        return *this;
    }

    FORCE_INLINE constexpr Vector2& operator*=(const Vector2& vec)
    {
        x *= vec.x;
        y *= vec.y;
        return *this;
    }

    FORCE_INLINE constexpr Vector2& operator/=(const Vector2& vec)
    {
        x /= vec.x;
        y /= vec.y;
        return *this;
    }

    FORCE_INLINE constexpr Vector2& operator+=(const Vector2& vec)
    {
        x += vec.x;
        y += vec.y;
        return *this;
    }

    FORCE_INLINE constexpr Vector2& operator-=(const Vector2& vec)
    {
        x -= vec.x;
        y -= vec.y;
        return *this;
    }
	
};

template<typename T>
NO_DISCARD FORCE_INLINE Vector2<T> operator+(T val, const Vector2<T>& vec)
{
    return vec + val;
}

template<typename T>
NO_DISCARD FORCE_INLINE Vector2<T> operator-(T val, const Vector2<T>& vec)
{
    return vec - val;
}
    
template<typename T>
NO_DISCARD FORCE_INLINE Vector2<T> operator*(T val, const Vector2<T>& vec)
{
    return vec * val;
}
    
template<typename T>
NO_DISCARD FORCE_INLINE Vector2<T> operator/(T val, const Vector2<T>& vec)
{
    return vec / val;
}
    
namespace Math {

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2<T> Cos(const Vector2<T>& vec)
{
    return Vector2
    {
        Cos(vec.x),
        Cos(vec.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2<T> Sin(const Vector2<T>& vec)
{
    return Vector2
    {
        Sin(vec.x),
        Sin(vec.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2<T> Abs(const Vector2<T>& vec)
{
    return Vector2
    {
        Abs(vec.x),
        Abs(vec.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2<T> Fract(const Vector2<T>& vec)
{
    return Vector2
    {
        vec.x - static_cast<long>(vec.x),
        vec.y - static_cast<long>(vec.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2<T> Deg2Rad(const Vector2<T>& degrees)
{
    return Vector2
    {
        Deg2Rad(degrees.x),
        Deg2Rad(degrees.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2<T> Rad2Deg(const Vector2<T>& radians)
{
    return Vector2
    {
        Rad2Deg(radians.x),
        Rad2Deg(radians.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2<T> Min(const Vector2<T>& v0, const Vector2<T>& v1)
{
    return Vector2
    {
        Min(v0.x, v1.x),
        Min(v0.y, v1.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2<T> Max(const Vector2<T>& v0, const Vector2<T>& v1)
{
    return Vector2
    {
        Max(v0.x, v1.x),
        Max(v0.y, v1.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2<T> Mix(const Vector2<T>& v0, const Vector2<T>& v1, T a)
{
    return v0 * (T(1) - a) + v1 * a;
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2<T> Clamp(const Vector2<T>& value, T min, T max)
{
    return Vector2
    {
        Min(Max(value.x, min), max),
        Min(Max(value.y, min), max)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Dot(const Vector2<T>& vec1, const Vector2<T>& vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y;
}
    
template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Length(const Vector2<T>& vec)
{
    return Sqrt(Dot(vec, vec));
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2<T> Normalize(const Vector2<T>& vec)
{
    return vec / Length(vec);
}

} // namespace Math

using Float2 = Vector2<float>;
using Int2 = Vector2<int>;

template<typename T>
const Vector2<T> Vector2<T>::zero{T(0), T(0)};
template<typename T>
const Vector2<T> Vector2<T>::one{T(1), T(1)};

} // namespace Gleam
