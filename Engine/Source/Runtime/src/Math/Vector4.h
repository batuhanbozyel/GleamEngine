#pragma once

namespace Gleam {

template<typename T>
struct Vector4Base
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
		TArray<T, 4> value;
	};

	Vector4Base() = default;
	constexpr Vector4Base(Vector4Base&&) = default;
	constexpr Vector4Base(const Vector4Base&) = default;
	inline constexpr Vector4Base& operator=(Vector4Base&&) = default;
	inline constexpr Vector4Base& operator=(const Vector4Base&) = default;

	constexpr Vector4Base(T v)
		: x(v), y(v), z(v), w(v)
	{

	}
	constexpr Vector4Base(T x, T y, T z, T w)
		: x(x), y(y), z(z), w(w)
	{

	}
	constexpr Vector4Base(const TArray<T, 4>& vec)
		: x(vec[0]), y(vec[1]), z(vec[2]), w(vec[3])
	{

	}
    
    MATH_INLINE constexpr bool operator==(const Vector4Base& vec) const
    {
        return x == vec.x && y == vec.y && z == vec.z && w == vec.w;
    }

	MATH_INLINE constexpr float operator[](size_t i) const
	{
		return value[i];
	}

    MATH_INLINE constexpr Vector4Base operator-() const
    {
        return Vector4Base
        {
            -x,
            -y,
            -z,
            -w
        };
    }
    
	MATH_INLINE constexpr Vector4Base operator*(T val) const
	{
		return Vector4Base
		{
			x * val,
			y * val,
			z * val,
			w * val
		};
	}

	MATH_INLINE constexpr Vector4Base operator/(T val) const
	{
		return Vector4Base
		{
			x / val,
			y / val,
			z / val,
			w / val
		};
	}

	MATH_INLINE constexpr Vector4Base operator+(T val) const
	{
		return Vector4Base
		{
			x + val,
			y + val,
			z + val,
			w + val
		};
	}

	MATH_INLINE constexpr Vector4Base operator-(T val) const
	{
		return Vector4Base
		{
			x - val,
			y - val,
			z - val,
			w - val
		};
	}

	MATH_INLINE constexpr Vector4Base operator*(const Vector4Base& vec) const
	{
		return Vector4Base
		{
			x * vec.x,
			y * vec.y,
			z * vec.z,
			w * vec.w
		};
	}

	MATH_INLINE constexpr Vector4Base operator/(const Vector4Base& vec) const
	{
		return Vector4Base
		{
			x / vec.x,
			y / vec.y,
			z / vec.z,
			w / vec.w
		};
	}

	MATH_INLINE constexpr Vector4Base operator+(const Vector4Base& vec) const
	{
		return Vector4Base
		{
			x + vec.x,
			y + vec.y,
			z + vec.z,
			w + vec.w
		};
	}

	MATH_INLINE constexpr Vector4Base operator-(const Vector4Base& vec) const
	{
		return Vector4Base
		{
			x - vec.x,
			y - vec.x,
			z - vec.z,
			w - vec.w
		};
	}

	MATH_INLINE constexpr Vector4Base& operator*=(T val)
	{
		x *= val;
		y *= val;
		z *= val;
		w *= val;
		return *this;
	}

	MATH_INLINE constexpr Vector4Base& operator/=(T val)
	{
		x /= val;
		y /= val;
		z /= val;
		w /= val;
		return *this;
	}

	MATH_INLINE constexpr Vector4Base& operator+=(T val)
	{
		x += val;
		y += val;
		z += val;
		w += val;
		return *this;
	}

	MATH_INLINE constexpr Vector4Base& operator-=(T val)
	{
		x -= val;
		y -= val;
		z -= val;
		w -= val;
		return *this;
	}

	MATH_INLINE constexpr Vector4Base& operator*=(const Vector4Base& vec)
	{
		x *= vec.x;
		y *= vec.y;
		z *= vec.z;
		w *= vec.w;
		return *this;
	}

	MATH_INLINE constexpr Vector4Base& operator/=(const Vector4Base& vec)
	{
		x /= vec.x;
		y /= vec.y;
		z /= vec.z;
		w /= vec.w;
		return *this;
	}

	MATH_INLINE constexpr Vector4Base& operator+=(const Vector4Base& vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
		w += vec.w;
		return *this;
	}

	MATH_INLINE constexpr Vector4Base& operator-=(const Vector4Base& vec)
	{
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		w -= vec.w;
		return *this;
	}

};

namespace Math {

template<typename T>
MATH_INLINE constexpr Vector4Base<T> Cos(const Vector4Base<T>& vec)
{
	return Vector4Base
	{
		Cos(vec.x),
		Cos(vec.y),
		Cos(vec.z),
		Cos(vec.w)
	};
}

template<typename T>
MATH_INLINE constexpr Vector4Base<T> Sin(const Vector4Base<T>& vec)
{
	return Vector4Base
	{
		Sin(vec.x),
		Sin(vec.y),
		Sin(vec.z),
		Sin(vec.w)
	};
}

template<typename T>
MATH_INLINE constexpr Vector4Base<T> Abs(const Vector4Base<T>& vec)
{
	return Vector4Base
	{
		Abs(vec.x),
		Abs(vec.y),
		Abs(vec.z),
		Abs(vec.w)
	};
}

template<typename T>
MATH_INLINE constexpr Vector4Base<T> Fract(const Vector4Base<T>& vec)
{
	return Vector4Base
	{
		vec.x - static_cast<long>(vec.x),
		vec.y - static_cast<long>(vec.y),
		vec.z - static_cast<long>(vec.z),
		vec.w - static_cast<long>(vec.w)
	};
}

template<typename T>
MATH_INLINE constexpr Vector4Base<T> Deg2Rad(const Vector4Base<T>& degrees)
{
	return Vector4Base
	{
		Deg2Rad(degrees.x),
		Deg2Rad(degrees.y),
		Deg2Rad(degrees.z),
		Deg2Rad(degrees.w)
	};
}

template<typename T>
MATH_INLINE constexpr Vector4Base<T> Rad2Deg(const Vector4Base<T>& radians)
{
	return Vector4Base
	{
		Rad2Deg(radians.x),
		Rad2Deg(radians.y),
		Rad2Deg(radians.z),
		Rad2Deg(radians.w)
	};
}

template<typename T>
MATH_INLINE constexpr Vector4Base<T> Min(const Vector4Base<T>& v0, const Vector4Base<T>& v1)
{
	return Vector4Base
	{
		Min(v0.x, v1.x),
		Min(v0.y, v1.y),
		Min(v0.z, v1.z),
		Min(v0.w, v1.w),
	};
}

template<typename T>
MATH_INLINE constexpr Vector4Base<T> Max(const Vector4Base<T>& v0, const Vector4Base<T>& v1)
{
	return Vector4Base
	{
		Max(v0.x, v1.x),
		Max(v0.y, v1.y),
		Max(v0.z, v1.z),
		Max(v0.w, v1.w),
	};
}

template<typename T>
MATH_INLINE constexpr Vector4Base<T> Mix(const Vector4Base<T>& v0, const Vector4Base<T>& v1, T a)
{
	return v0 * (T(1) - a) + v1 * a;
}

template<typename T>
MATH_INLINE constexpr Vector4Base<T> Clamp(const  Vector4Base<T>& value, T min, T max)
{
	return Vector4Base
	{
		Min(Max(value.x, min), max),
		Min(Max(value.y, min), max),
		Min(Max(value.z, min), max),
		Min(Max(value.w, min), max)
	};
}

template<typename T>
MATH_INLINE constexpr T Dot(const Vector4Base<T>& vec1, const Vector4Base<T>& vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z + vec1.w * vec2.w;
}

template<typename T>
MATH_INLINE constexpr T Length(const Vector4Base<T>& vec)
{
    return Dot(vec, vec);
}

template<typename T>
MATH_INLINE constexpr Vector4Base<T> Normalize(const Vector4Base<T>& vec)
{
    return vec / Length(vec);
}

} // namespace Math

using Vector4 = Vector4Base<float>;
using Vector4Int = Vector4Base<int>;

} // namespace Gleam
