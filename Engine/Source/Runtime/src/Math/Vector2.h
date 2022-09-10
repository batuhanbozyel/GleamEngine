#pragma once

namespace Gleam {
	
template<typename T>
struct Vector2Base
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

    static const Vector2Base zero;
    static const Vector2Base one;

    constexpr Vector2Base() = default;
    constexpr Vector2Base(Vector2Base&&) = default;
    constexpr Vector2Base(const Vector2Base&) = default;
    FORCE_INLINE constexpr Vector2Base& operator=(Vector2Base&&) = default;
    FORCE_INLINE constexpr Vector2Base& operator=(const Vector2Base&) = default;

    constexpr Vector2Base(T v)
        : x(v), y(v)
    {

    }
    constexpr Vector2Base(T x, T y)
        : x(x), y(y)
    {

    }
    constexpr Vector2Base(const TArray<T, 2>& vec)
        : x(vec[0]), y(vec[1])
    {

    }

    NO_DISCARD FORCE_INLINE constexpr bool operator==(const Vector2Base& vec) const
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

    NO_DISCARD FORCE_INLINE constexpr Vector2Base operator-() const
    {
        return Vector2Base
        {
            -x,
            -y
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2Base operator*(T val) const
    {
        return Vector2Base
        {
            x * val,
            y * val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2Base operator/(T val) const
    {
        return Vector2Base
        {
            x / val,
            y / val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2Base operator+(T val) const
    {
        return Vector2Base
        {
            x + val,
            y + val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2Base operator-(T val) const
    {
        return Vector2Base
        {
            x - val,
            y - val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2Base operator*(const Vector2Base& vec) const
    {
        return Vector2Base
        {
            x * vec.x,
            y * vec.y
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2Base operator/(const Vector2Base& vec) const
    {
        return Vector2Base
        {
            x / vec.x,
            y / vec.y
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2Base operator+(const Vector2Base& vec) const
    {
        return Vector2Base
        {
            x + vec.x,
            y + vec.y
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector2Base operator-(const Vector2Base& vec) const
    {
        return Vector2Base
        {
            x - vec.x,
            y - vec.y
        };
    }

    FORCE_INLINE constexpr Vector2Base& operator*=(T val)
    {
        x *= val;
        y *= val;
        return *this;
    }

    FORCE_INLINE constexpr Vector2Base& operator/=(T val)
    {
        x /= val;
        y /= val;
        return *this;
    }

    FORCE_INLINE constexpr Vector2Base& operator+=(T val)
    {
        x += val;
        y += val;
        return *this;
    }

    FORCE_INLINE constexpr Vector2Base& operator-=(T val)
    {
        x -= val;
        y -= val;
        return *this;
    }

    FORCE_INLINE constexpr Vector2Base& operator*=(const Vector2Base& vec)
    {
        x *= vec.x;
        y *= vec.y;
        return *this;
    }

    FORCE_INLINE constexpr Vector2Base& operator/=(const Vector2Base& vec)
    {
        x /= vec.x;
        y /= vec.y;
        return *this;
    }

    FORCE_INLINE constexpr Vector2Base& operator+=(const Vector2Base& vec)
    {
        x += vec.x;
        y += vec.y;
        return *this;
    }

    FORCE_INLINE constexpr Vector2Base& operator-=(const Vector2Base& vec)
    {
        x -= vec.x;
        y -= vec.y;
        return *this;
    }
	
};

template<typename T>
NO_DISCARD FORCE_INLINE Vector2Base<T> operator+(T val, const Vector2Base<T>& vec)
{
    return vec + val;
}

template<typename T>
NO_DISCARD FORCE_INLINE Vector2Base<T> operator-(T val, const Vector2Base<T>& vec)
{
    return vec - val;
}
    
template<typename T>
NO_DISCARD FORCE_INLINE Vector2Base<T> operator*(T val, const Vector2Base<T>& vec)
{
    return vec * val;
}
    
template<typename T>
NO_DISCARD FORCE_INLINE Vector2Base<T> operator/(T val, const Vector2Base<T>& vec)
{
    return vec / val;
}
    
namespace Math {

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2Base<T> Cos(const Vector2Base<T>& vec)
{
    return Vector2Base
    {
        Cos(vec.x),
        Cos(vec.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2Base<T> Sin(const Vector2Base<T>& vec)
{
    return Vector2Base
    {
        Sin(vec.x),
        Sin(vec.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2Base<T> Abs(const Vector2Base<T>& vec)
{
    return Vector2Base
    {
        Abs(vec.x),
        Abs(vec.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2Base<T> Fract(const Vector2Base<T>& vec)
{
    return Vector2Base
    {
        vec.x - static_cast<long>(vec.x),
        vec.y - static_cast<long>(vec.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2Base<T> Deg2Rad(const Vector2Base<T>& degrees)
{
    return Vector2Base
    {
        Deg2Rad(degrees.x),
        Deg2Rad(degrees.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2Base<T> Rad2Deg(const Vector2Base<T>& radians)
{
    return Vector2Base
    {
        Rad2Deg(radians.x),
        Rad2Deg(radians.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2Base<T> Min(const Vector2Base<T>& v0, const Vector2Base<T>& v1)
{
    return Vector2Base
    {
        Min(v0.x, v1.x),
        Min(v0.y, v1.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2Base<T> Max(const Vector2Base<T>& v0, const Vector2Base<T>& v1)
{
    return Vector2Base
    {
        Max(v0.x, v1.x),
        Max(v0.y, v1.y)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2Base<T> Mix(const Vector2Base<T>& v0, const Vector2Base<T>& v1, T a)
{
    return v0 * (T(1) - a) + v1 * a;
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2Base<T> Clamp(const Vector2Base<T>& value, T min, T max)
{
    return Vector2Base
    {
        Min(Max(value.x, min), max),
        Min(Max(value.y, min), max)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Dot(const Vector2Base<T>& vec1, const Vector2Base<T>& vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y;
}
    
template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Length(const Vector2Base<T>& vec)
{
    return Sqrt(Dot(vec, vec));
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector2Base<T> Normalize(const Vector2Base<T>& vec)
{
    return vec / Length(vec);
}

} // namespace Math

using Vector2 = Vector2Base<float>;
using Vector2Int = Vector2Base<int>;

template<typename T>
const Vector2Base<T> Vector2Base<T>::zero{T(0), T(0)};
template<typename T>
const Vector2Base<T> Vector2Base<T>::one{T(1), T(1)};

} // namespace Gleam
