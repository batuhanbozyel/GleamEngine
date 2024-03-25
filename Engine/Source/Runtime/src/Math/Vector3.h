#pragma once

namespace Gleam {

template<typename T>
struct Vector3
{
    union
    {
        struct
        {
            T x, y, z;
        };
        struct
        {
            T r, g, b;
        };
        TArray<T, 3> value{};
    };

    static const Vector3 zero;
    static const Vector3 one;
    static const Vector3 left;
    static const Vector3 right;
    static const Vector3 up;
    static const Vector3 down;
    static const Vector3 forward;
    static const Vector3 back;

    constexpr Vector3() = default;
    constexpr Vector3(Vector3&&) noexcept = default;
    constexpr Vector3(const Vector3&) = default;
    FORCE_INLINE constexpr Vector3& operator=(Vector3&&) noexcept = default;
    FORCE_INLINE constexpr Vector3& operator=(const Vector3&) = default;

    constexpr Vector3(T v)
        : x(v), y(v), z(v)
    {

    }

    constexpr Vector3(T x, T y)
        : x(x), y(y), z(0.0f)
    {

    }

    constexpr Vector3(T x, T y, T z)
        : x(x), y(y), z(z)
    {

    }

    constexpr Vector3(const TArray<T, 3>& vec)
        : x(vec[0]), y(vec[1]), z(vec[2])
    {

    }

    constexpr Vector3(const Float2& vec)
        : x(vec.x), y(vec.y), z(0.0f)
    {

    }

    constexpr Vector3(const Float2& vec, float z)
        : x(vec.x), y(vec.y), z(z)
    {

    }

    NO_DISCARD FORCE_INLINE constexpr bool operator==(const Vector3& vec) const
    {
        return x == vec.x && y == vec.y && z == vec.z;
    }

    NO_DISCARD FORCE_INLINE constexpr T& operator[](size_t i)
    {
        return value[i];
    }

    NO_DISCARD FORCE_INLINE constexpr const T& operator[](size_t i) const
    {
        return value[i];
    }

    NO_DISCARD FORCE_INLINE constexpr Vector3 operator-() const
    {
        return Vector3
        {
            -x,
            -y,
            -z
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector3 operator*(T val) const
    {
        return Vector3
        {
            x * val,
            y * val,
            z * val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector3 operator/(T val) const
    {
        return Vector3
        {
            x / val,
            y / val,
            z / val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector3 operator+(T val) const
    {
        return Vector3
        {
            x + val,
            y + val,
            z + val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector3 operator-(T val) const
    {
        return Vector3
        {
            x - val,
            y - val,
            z - val
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector3 operator*(const Vector3& vec) const
    {
        return Vector3
        {
            x * vec.x,
            y * vec.y,
            z * vec.z
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector3 operator/(const Vector3& vec) const
    {
        return Vector3
        {
            x / vec.x,
            y / vec.y,
            z / vec.z
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector3 operator+(const Vector3& vec) const
    {
        return Vector3
        {
            x + vec.x,
            y + vec.y,
            z + vec.z
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Vector3 operator-(const Vector3& vec) const
    {
        return Vector3
        {
            x - vec.x,
            y - vec.y,
            z - vec.z
        };
    }

    FORCE_INLINE constexpr Vector3& operator*=(T val)
    {
        x *= val;
        y *= val;
        z *= val;
        return *this;
    }

    FORCE_INLINE constexpr Vector3& operator/=(T val)
    {
        x /= val;
        y /= val;
        z /= val;
        return *this;
    }

    FORCE_INLINE constexpr Vector3& operator+=(T val)
    {
        x += val;
        y += val;
        z += val;
        return *this;
    }

    FORCE_INLINE constexpr Vector3& operator-=(T val)
    {
        x -= val;
        y -= val;
        z -= val;
        return *this;
    }

    FORCE_INLINE constexpr Vector3& operator*=(const Vector3& vec)
    {
        x *= vec.x;
        y *= vec.y;
        z *= vec.z;
        return *this;
    }

    FORCE_INLINE constexpr Vector3& operator/=(const Vector3& vec)
    {
        x /= vec.x;
        y /= vec.y;
        z /= vec.z;
        return *this;
    }

    FORCE_INLINE constexpr Vector3& operator+=(const Vector3& vec)
    {
        x += vec.x;
        y += vec.y;
        z += vec.z;
        return *this;
    }

    FORCE_INLINE constexpr Vector3& operator-=(const Vector3& vec)
    {
        x -= vec.x;
        y -= vec.y;
        z -= vec.z;
        return *this;
    }

};
    
template<typename T>
NO_DISCARD FORCE_INLINE Vector3<T> operator+(T val, const Vector3<T>& vec)
{
    return vec + val;
}

template<typename T>
NO_DISCARD FORCE_INLINE Vector3<T> operator-(T val, const Vector3<T>& vec)
{
    return vec - val;
}
    
template<typename T>
NO_DISCARD FORCE_INLINE Vector3<T> operator*(T val, const Vector3<T>& vec)
{
    return vec * val;
}
    
template<typename T>
NO_DISCARD FORCE_INLINE Vector3<T> operator/(T val, const Vector3<T>& vec)
{
    return vec / val;
}

namespace Math {

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3<T> Cos(const Vector3<T>& vec)
{
    return Vector3
    {
        Cos(vec.x),
        Cos(vec.y),
        Cos(vec.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3<T> Sin(const Vector3<T>& vec)
{
    return Vector3
    {
        Sin(vec.x),
        Sin(vec.y),
        Sin(vec.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3<T> Abs(const Vector3<T>& vec)
{
    return Vector3
    {
        Abs(vec.x),
        Abs(vec.y),
        Abs(vec.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3<T> Fract(const Vector3<T>& vec)
{
    return Vector3
    {
        vec.x - static_cast<long>(vec.x),
        vec.y - static_cast<long>(vec.y),
        vec.z - static_cast<long>(vec.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3<T> Deg2Rad(const Vector3<T>& degrees)
{
    return Vector3
    {
        Deg2Rad(degrees.x),
        Deg2Rad(degrees.y),
        Deg2Rad(degrees.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3<T> Rad2Deg(const Vector3<T>& radians)
{
    return Vector3
    {
        Rad2Deg(radians.x),
        Rad2Deg(radians.y),
        Rad2Deg(radians.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3<T> Min(const Vector3<T>& v0, const Vector3<T>& v1)
{
    return Vector3
    {
        Min(v0.x, v1.x),
        Min(v0.y, v1.y),
        Min(v0.z, v1.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3<T> Max(const Vector3<T>& v0, const Vector3<T>& v1)
{
    return Vector3
    {
        Max(v0.x, v1.x),
        Max(v0.y, v1.y),
        Max(v0.z, v1.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3<T> Mix(const Vector3<T>& v0, const Vector3<T>& v1, T a)
{
    return v0 * (T(1) - a) + v1 * a;
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3<T> Clamp(const Vector3<T>& value, T min, T max)
{
    return Vector3
    {
        Min(Max(value.x, min), max),
        Min(Max(value.y, min), max),
        Min(Max(value.z, min), max)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Dot(const Vector3<T>& vec1, const Vector3<T>& vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
}
    
template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Length(const Vector3<T>& vec)
{
	return Sqrt(Dot(vec, vec));
}
    
template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3<T> Normalize(const Vector3<T>& vec)
{
    return vec / Length(vec);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3<T> Cross(const Vector3<T>& vec1, const Vector3<T>& vec2)
{
    return Vector3
    {
        (vec1.y * vec2.z - vec2.y * vec1.z),
        (vec1.z * vec2.x - vec2.z * vec1.x),
        (vec1.x * vec2.y - vec2.x * vec1.y)
    };
}

} // namespace Math

using Float3 = Vector3<float>;
using Int3 = Vector3<int>;
    
template<typename T>
const Vector3<T> Vector3<T>::zero{T(0), T(0), T(0)};
template<typename T>
const Vector3<T> Vector3<T>::one{T(1), T(1), T(1)};
template<typename T>
const Vector3<T> Vector3<T>::left{T(-1), T(0), T(0)};
template<typename T>
const Vector3<T> Vector3<T>::right{T(1), T(0), T(0)};
template<typename T>
const Vector3<T> Vector3<T>::up{T(0), T(1), T(0)};
template<typename T>
const Vector3<T> Vector3<T>::down{T(0), T(-1), T(0)};
template<typename T>
const Vector3<T> Vector3<T>::forward{T(0), T(0), T(1)};
template<typename T>
const Vector3<T> Vector3<T>::back{T(0), T(0), T(-1)};
	
} // namespace Gleam
