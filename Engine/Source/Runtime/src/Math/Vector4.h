#pragma once

namespace Gleam {

template<typename T>
struct Vector4
{
    union
    {
        struct
        {
            T x, y, z, w;
        };
        struct
        {
            T r, g, b, a;
        };
        TArray<T, 4> value{};
    };

    static const Vector4 zero;
    static const Vector4 one;

    constexpr Vector4() = default;
    constexpr Vector4(Vector4&&) noexcept = default;
    constexpr Vector4(const Vector4&) = default;
    FORCE_INLINE constexpr Vector4& operator=(Vector4&&) noexcept = default;
    FORCE_INLINE constexpr Vector4& operator=(const Vector4&) = default;

    constexpr Vector4(T v)
        : x(v), y(v), z(v), w(v)
    {

    }
    constexpr Vector4(T x, T y, T z, T w)
        : x(x), y(y), z(z), w(w)
    {

    }
    constexpr Vector4(const TArray<T, 4>& vec)
        : x(vec[0]), y(vec[1]), z(vec[2]), w(vec[3])
    {

    }

    constexpr Vector4(const Float3& vec, float w)
        : x(vec.x), y(vec.y), z(vec.z), w(w)
    {

    }

    NO_DISCARD FORCE_INLINE constexpr bool operator==(const Vector4& vec) const
    {
        return x == vec.x && y == vec.y && z == vec.z && w == vec.w;
    }

    NO_DISCARD FORCE_INLINE constexpr T& operator[](size_t i)
    {
        return value[i];
    }

    NO_DISCARD FORCE_INLINE constexpr const T& operator[](size_t i) const
    {
        return value[i];
    }

    NO_DISCARD FORCE_INLINE constexpr Vector4 operator-() const
    {
        return Vector4
        {
            -x,
            -y,
            -z,
            -w
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector4 operator*(T val) const
    {
        return Vector4
        {
            x * val,
            y * val,
            z * val,
            w * val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector4 operator/(T val) const
    {
        return Vector4
        {
            x / val,
            y / val,
            z / val,
            w / val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector4 operator+(T val) const
    {
        return Vector4
        {
            x + val,
            y + val,
            z + val,
            w + val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector4 operator-(T val) const
    {
        return Vector4
        {
            x - val,
            y - val,
            z - val,
            w - val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector4 operator*(const Vector4& vec) const
    {
        return Vector4
        {
            x * vec.x,
            y * vec.y,
            z * vec.z,
            w * vec.w
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector4 operator/(const Vector4& vec) const
    {
        return Vector4
        {
            x / vec.x,
            y / vec.y,
            z / vec.z,
            w / vec.w
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector4 operator+(const Vector4& vec) const
    {
        return Vector4
        {
            x + vec.x,
            y + vec.y,
            z + vec.z,
            w + vec.w
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector4 operator-(const Vector4& vec) const
    {
        return Vector4
        {
            x - vec.x,
            y - vec.y,
            z - vec.z,
            w - vec.w
        };
    }

    FORCE_INLINE constexpr Vector4& operator*=(T val)
    {
        x *= val;
        y *= val;
        z *= val;
        w *= val;
        return *this;
    }

    FORCE_INLINE constexpr Vector4& operator/=(T val)
    {
        x /= val;
        y /= val;
        z /= val;
        w /= val;
        return *this;
    }

    FORCE_INLINE constexpr Vector4& operator+=(T val)
    {
        x += val;
        y += val;
        z += val;
        w += val;
        return *this;
    }

    FORCE_INLINE constexpr Vector4& operator-=(T val)
    {
        x -= val;
        y -= val;
        z -= val;
        w -= val;
        return *this;
    }

    FORCE_INLINE constexpr Vector4& operator*=(const Vector4& vec)
    {
        x *= vec.x;
        y *= vec.y;
        z *= vec.z;
        w *= vec.w;
        return *this;
    }

    FORCE_INLINE constexpr Vector4& operator/=(const Vector4& vec)
    {
        x /= vec.x;
        y /= vec.y;
        z /= vec.z;
        w /= vec.w;
        return *this;
    }

    FORCE_INLINE constexpr Vector4& operator+=(const Vector4& vec)
    {
        x += vec.x;
        y += vec.y;
        z += vec.z;
        w += vec.w;
        return *this;
    }

    FORCE_INLINE constexpr Vector4& operator-=(const Vector4& vec)
    {
        x -= vec.x;
        y -= vec.y;
        z -= vec.z;
        w -= vec.w;
        return *this;
    }

};

template<typename T>
NO_DISCARD FORCE_INLINE Vector4<T> operator+(T val, const Vector4<T>& vec)
{
    return vec + val;
}

template<typename T>
NO_DISCARD FORCE_INLINE Vector4<T> operator-(T val, const Vector4<T>& vec)
{
    return vec - val;
}
    
template<typename T>
NO_DISCARD FORCE_INLINE Vector4<T> operator*(T val, const Vector4<T>& vec)
{
    return vec * val;
}
    
template<typename T>
NO_DISCARD FORCE_INLINE Vector4<T> operator/(T val, const Vector4<T>& vec)
{
    return vec / val;
}
    
namespace Math {

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector4<T> Cos(const Vector4<T>& vec)
{
    return Vector4
    {
        Cos(vec.x),
        Cos(vec.y),
        Cos(vec.z),
        Cos(vec.w)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector4<T> Sin(const Vector4<T>& vec)
{
    return Vector4
    {
        Sin(vec.x),
        Sin(vec.y),
        Sin(vec.z),
        Sin(vec.w)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector4<T> Abs(const Vector4<T>& vec)
{
    return Vector4
    {
        Abs(vec.x),
        Abs(vec.y),
        Abs(vec.z),
        Abs(vec.w)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector4<T> Fract(const Vector4<T>& vec)
{
    return Vector4
    {
        vec.x - static_cast<long>(vec.x),
        vec.y - static_cast<long>(vec.y),
        vec.z - static_cast<long>(vec.z),
        vec.w - static_cast<long>(vec.w)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector4<T> Deg2Rad(const Vector4<T>& degrees)
{
    return Vector4
    {
        Deg2Rad(degrees.x),
        Deg2Rad(degrees.y),
        Deg2Rad(degrees.z),
        Deg2Rad(degrees.w)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector4<T> Rad2Deg(const Vector4<T>& radians)
{
    return Vector4
    {
        Rad2Deg(radians.x),
        Rad2Deg(radians.y),
        Rad2Deg(radians.z),
        Rad2Deg(radians.w)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector4<T> Min(const Vector4<T>& v0, const Vector4<T>& v1)
{
    return Vector4
    {
        Min(v0.x, v1.x),
        Min(v0.y, v1.y),
        Min(v0.z, v1.z),
        Min(v0.w, v1.w),
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector4<T> Max(const Vector4<T>& v0, const Vector4<T>& v1)
{
    return Vector4
    {
        Max(v0.x, v1.x),
        Max(v0.y, v1.y),
        Max(v0.z, v1.z),
        Max(v0.w, v1.w),
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector4<T> Mix(const Vector4<T>& v0, const Vector4<T>& v1, T a)
{
    return v0 * (T(1) - a) + v1 * a;
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector4<T> Clamp(const  Vector4<T>& value, T min, T max)
{
    return Vector4
    {
        Min(Max(value.x, min), max),
        Min(Max(value.y, min), max),
        Min(Max(value.z, min), max),
        Min(Max(value.w, min), max)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Dot(const Vector4<T>& vec1, const Vector4<T>& vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z + vec1.w * vec2.w;
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Length(const Vector4<T>& vec)
{
    return Sqrt(Dot(vec, vec));
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector4<T> Normalize(const Vector4<T>& vec)
{
    return vec / Length(vec);
}

} // namespace Math

using Float4 = Vector4<float>;
using Int4 = Vector4<int>;
    
template<typename T>
const Vector4<T> Vector4<T>::zero{T(0), T(0), T(0), T(0)};
template<typename T>
const Vector4<T> Vector4<T>::one{T(1), T(1), T(1), T(1)};

} // namespace Gleam
