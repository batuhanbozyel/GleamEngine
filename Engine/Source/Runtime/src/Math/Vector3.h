#pragma once

namespace Gleam {
	
template<typename T>
struct Vector3Base
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
		TArray<T, 3> value;
	};
    
    static const Vector3Base zero;
    static const Vector3Base one;
    static const Vector3Base left;
    static const Vector3Base right;
    static const Vector3Base up;
    static const Vector3Base down;
    static const Vector3Base forward;
    static const Vector3Base back;

	constexpr Vector3Base() = default;
	constexpr Vector3Base(Vector3Base&&) = default;
	constexpr Vector3Base(const Vector3Base&) = default;
	FORCE_INLINE constexpr Vector3Base& operator=(Vector3Base&&) = default;
	FORCE_INLINE constexpr Vector3Base& operator=(const Vector3Base&) = default;

	constexpr Vector3Base(T v)
		: x(v), y(v), z(v)
	{

	}
	constexpr Vector3Base(T x, T y, T z)
		: x(x), y(y), z(z)
	{

	}
	constexpr Vector3Base(const TArray<T, 3>& vec)
		: x(vec[0]), y(vec[1]), z(vec[2])
	{

	}
    
    NO_DISCARD FORCE_INLINE constexpr bool operator==(const Vector3Base& vec) const
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

    NO_DISCARD FORCE_INLINE constexpr Vector3Base operator-() const
    {
        return Vector3Base
        {
            -x,
            -y,
            -z
        };
    }
    
	NO_DISCARD FORCE_INLINE constexpr Vector3Base operator*(T val) const
	{
		return Vector3Base
		{
			x * val,
			y * val,
			z * val
		};
	}

	NO_DISCARD FORCE_INLINE constexpr Vector3Base operator/(T val) const
	{
		return Vector3Base
		{
			x / val,
			y / val,
			z / val
		};
	}

	NO_DISCARD FORCE_INLINE constexpr Vector3Base operator+(T val) const
	{
		return Vector3Base
		{
			x + val,
			y + val,
			z + val
		};
	}

	NO_DISCARD FORCE_INLINE constexpr Vector3Base operator-(T val) const
	{
		return Vector3Base
		{
			x - val,
			y - val,
			z - val
		};
	}

	NO_DISCARD FORCE_INLINE constexpr Vector3Base operator*(const Vector3Base& vec) const
	{
		return Vector3Base
		{
			x * vec.x,
			y * vec.y,
			z * vec.z
		};
	}

	NO_DISCARD FORCE_INLINE constexpr Vector3Base operator/(const Vector3Base& vec) const
	{
		return Vector3Base
		{
			x / vec.x,
			y / vec.y,
			z / vec.z
		};
	}

	NO_DISCARD FORCE_INLINE constexpr Vector3Base operator+(const Vector3Base& vec) const
	{
		return Vector3Base
		{
			x + vec.x,
			y + vec.y,
			z + vec.z
		};
	}

	NO_DISCARD FORCE_INLINE constexpr Vector3Base operator-(const Vector3Base& vec) const
	{
		return Vector3Base
		{
			x - vec.x,
			y - vec.x,
			z - vec.z
		};
	}

	FORCE_INLINE constexpr Vector3Base& operator*=(T val)
	{
		x *= val;
		y *= val;
		z *= val;
		return *this;
	}

	FORCE_INLINE constexpr Vector3Base& operator/=(T val)
	{
		x /= val;
		y /= val;
		z /= val;
		return *this;
	}

	FORCE_INLINE constexpr Vector3Base& operator+=(T val)
	{
		x += val;
		y += val;
		z += val;
		return *this;
	}

	FORCE_INLINE constexpr Vector3Base& operator-=(T val)
	{
		x -= val;
		y -= val;
		z -= val;
		return *this;
	}

	FORCE_INLINE constexpr Vector3Base& operator*=(const Vector3Base& vec)
	{
		x *= vec.x;
		y *= vec.y;
		z *= vec.z;
		return *this;
	}

	FORCE_INLINE constexpr Vector3Base& operator/=(const Vector3Base& vec)
	{
		x /= vec.x;
		y /= vec.y;
		z /= vec.z;
		return *this;
	}

	FORCE_INLINE constexpr Vector3Base& operator+=(const Vector3Base& vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
		return *this;
	}

	FORCE_INLINE constexpr Vector3Base& operator-=(const Vector3Base& vec)
	{
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		return *this;
	}

};
    
template<typename T>
Vector3Base<T> operator+(T val, const Vector3Base<T>& vec)
{
    return vec + val;
}

template<typename T>
Vector3Base<T> operator-(T val, const Vector3Base<T>& vec)
{
    return vec - val;
}
    
template<typename T>
Vector3Base<T> operator*(T val, const Vector3Base<T>& vec)
{
    return vec * val;
}
    
template<typename T>
Vector3Base<T> operator/(T val, const Vector3Base<T>& vec)
{
    return vec / val;
}

namespace Math {

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3Base<T> Cos(const Vector3Base<T>& vec)
{
    return Vector3Base
    {
        Cos(vec.x),
        Cos(vec.y),
        Cos(vec.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3Base<T> Sin(const Vector3Base<T>& vec)
{
    return Vector3Base
    {
        Sin(vec.x),
        Sin(vec.y),
        Sin(vec.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3Base<T> Abs(const Vector3Base<T>& vec)
{
    return Vector3Base
    {
        Abs(vec.x),
        Abs(vec.y),
        Abs(vec.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3Base<T> Fract(const Vector3Base<T>& vec)
{
    return Vector3Base
    {
        vec.x - static_cast<long>(vec.x),
        vec.y - static_cast<long>(vec.y),
        vec.z - static_cast<long>(vec.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3Base<T> Deg2Rad(const Vector3Base<T>& degrees)
{
    return Vector3Base
    {
        Deg2Rad(degrees.x),
        Deg2Rad(degrees.y),
        Deg2Rad(degrees.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3Base<T> Rad2Deg(const Vector3Base<T>& radians)
{
    return Vector3Base
    {
        Rad2Deg(radians.x),
        Rad2Deg(radians.y),
        Rad2Deg(radians.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3Base<T> Min(const Vector3Base<T>& v0, const Vector3Base<T>& v1)
{
    return Vector3Base
    {
        Min(v0.x, v1.x),
        Min(v0.y, v1.y),
        Min(v0.z, v1.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3Base<T> Max(const Vector3Base<T>& v0, const Vector3Base<T>& v1)
{
    return Vector3Base
    {
        Max(v0.x, v1.x),
        Max(v0.y, v1.y),
        Max(v0.z, v1.z)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3Base<T> Mix(const Vector3Base<T>& v0, const Vector3Base<T>& v1, T a)
{
    return v0 * (T(1) - a) + v1 * a;
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3Base<T> Clamp(const Vector3Base<T>& value, T min, T max)
{
    return Vector3Base
    {
        Min(Max(value.x, min), max),
        Min(Max(value.y, min), max),
        Min(Max(value.z, min), max)
    };
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Dot(const Vector3Base<T>& vec1, const Vector3Base<T>& vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
}
    
template<typename T>
NO_DISCARD FORCE_INLINE constexpr T Length(const Vector3Base<T>& vec)
{
    return Dot(vec, vec);
}
    
template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3Base<T> Normalize(const Vector3Base<T>& vec)
{
    return vec / Length(vec);
}

template<typename T>
NO_DISCARD FORCE_INLINE constexpr Vector3Base<T> Cross(const Vector3Base<T>& vec1, const Vector3Base<T>& vec2)
{
    return Vector3Base
    {
        (vec1.y * vec2.z - vec2.y * vec1.z),
        (vec1.z * vec2.x - vec2.z * vec1.x),
        (vec1.x * vec2.y - vec2.x * vec1.y)
    };
}

} // namespace Math

using Vector3 = Vector3Base<float>;
using Vector3Int = Vector3Base<int>;
    
template<typename T>
const Vector3Base<T> Vector3Base<T>::zero{T(0), T(0), T(0)};
template<typename T>
const Vector3Base<T> Vector3Base<T>::one{T(1), T(1), T(1)};
template<typename T>
const Vector3Base<T> Vector3Base<T>::left{T(-1), T(0), T(0)};
template<typename T>
const Vector3Base<T> Vector3Base<T>::right{T(1), T(0), T(0)};
template<typename T>
const Vector3Base<T> Vector3Base<T>::up{T(0), T(1), T(0)};
template<typename T>
const Vector3Base<T> Vector3Base<T>::down{T(0), T(-1), T(0)};
template<typename T>
const Vector3Base<T> Vector3Base<T>::forward{T(0), T(0), T(1)};
template<typename T>
const Vector3Base<T> Vector3Base<T>::back{T(0), T(0), T(-1)};
	
} // namespace Gleam
