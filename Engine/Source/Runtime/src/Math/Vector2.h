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
		TArray<T, 2> value;
	};

	Vector2Base() = default;
	constexpr Vector2Base(Vector2Base&&) = default;
	constexpr Vector2Base(const Vector2Base&) = default;
	inline constexpr Vector2Base& operator=(Vector2Base&&) = default;
	inline constexpr Vector2Base& operator=(const Vector2Base&) = default;

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
    
    MATH_INLINE constexpr bool operator==(const Vector2Base& vec) const
    {
        return x == vec.x && y == vec.y;
    }

	MATH_INLINE constexpr T operator[](size_t i) const
	{
		return value[i];
	}
    
    MATH_INLINE constexpr Vector2Base operator-() const
    {
        return Vector2Base
        {
            -x,
            -y
        };
    }

	MATH_INLINE constexpr Vector2Base operator*(T val) const
	{
		return Vector2Base
		{
			x * val,
			y * val
		};
	}

	MATH_INLINE constexpr Vector2Base operator/(T val) const
	{
		return Vector2Base
		{
			x / val,
			y / val
		};
	}

	MATH_INLINE constexpr Vector2Base operator+(T val) const
	{
		return Vector2Base
		{
			x + val,
			y + val
		};
	}

	MATH_INLINE constexpr Vector2Base operator-(T val) const
	{
		return Vector2Base
		{
			x - val,
			y - val
		};
	}

	MATH_INLINE constexpr Vector2Base operator*(const Vector2Base& vec) const
	{
		return Vector2Base
		{
			x * vec.x,
			y * vec.y
		};
	}

	MATH_INLINE constexpr Vector2Base operator/(const Vector2Base& vec) const
	{
		return Vector2Base
		{
			x / vec.x,
			y / vec.y
		};
	}

	MATH_INLINE constexpr Vector2Base operator+(const Vector2Base& vec) const
	{
		return Vector2Base
		{
			x + vec.x,
			y + vec.y
		};
	}

	MATH_INLINE constexpr Vector2Base operator-(const Vector2Base& vec) const
	{
		return Vector2Base
		{
			x - vec.x,
			y - vec.x
		};
	}

	MATH_INLINE constexpr Vector2Base& operator*=(T val)
	{
		x *= val;
		y *= val;
		return *this;
	}

	MATH_INLINE constexpr Vector2Base& operator/=(T val)
	{
		x /= val;
		y /= val;
		return *this;
	}

	MATH_INLINE constexpr Vector2Base& operator+=(T val)
	{
		x += val;
		y += val;
		return *this;
	}

	MATH_INLINE constexpr Vector2Base& operator-=(T val)
	{
		x -= val;
		y -= val;
		return *this;
	}

	MATH_INLINE constexpr Vector2Base& operator*=(const Vector2Base& vec)
	{
		x *= vec.x;
		y *= vec.y;
		return *this;
	}

	MATH_INLINE constexpr Vector2Base& operator/=(const Vector2Base& vec)
	{
		x /= vec.x;
		y /= vec.y;
		return *this;
	}

	MATH_INLINE constexpr Vector2Base& operator+=(const Vector2Base& vec)
	{
		x += vec.x;
		y += vec.y;
		return *this;
	}

	MATH_INLINE constexpr Vector2Base& operator-=(const Vector2Base& vec)
	{
		x -= vec.x;
		y -= vec.y;
		return *this;
	}
	
};

namespace Math {

template<typename T>
MATH_INLINE constexpr Vector2Base<T> Cos(const Vector2Base<T>& vec)
{
	return Vector2Base
	{
		Cos(vec.x),
		Cos(vec.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector2Base<T> Sin(const Vector2Base<T>& vec)
{
	return Vector2Base
	{
		Sin(vec.x),
		Sin(vec.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector2Base<T> Abs(const Vector2Base<T>& vec)
{
	return Vector2Base
	{
		Abs(vec.x),
		Abs(vec.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector2Base<T> Fract(const Vector2Base<T>& vec)
{
	return Vector2Base
	{
		vec.x - static_cast<long>(vec.x),
		vec.y - static_cast<long>(vec.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector2Base<T> Deg2Rad(const Vector2Base<T>& degrees)
{
	return Vector2Base
	{
		Deg2Rad(degrees.x),
		Deg2Rad(degrees.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector2Base<T> Rad2Deg(const Vector2Base<T>& radians)
{
	return Vector2Base
	{
		Rad2Deg(radians.x),
		Rad2Deg(radians.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector2Base<T> Min(const Vector2Base<T>& v0, const Vector2Base<T>& v1)
{
	return Vector2Base
	{
		Min(v0.x, v1.x),
		Min(v0.y, v1.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector2Base<T> Max(const Vector2Base<T>& v0, const Vector2Base<T>& v1)
{
	return Vector2Base
	{
		Max(v0.x, v1.x),
		Max(v0.y, v1.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector2Base<T> Mix(const Vector2Base<T>& v0, const Vector2Base<T>& v1, T a)
{
	return v0 * (T(1) - a) + v1 * a;
}

template<typename T>
MATH_INLINE constexpr Vector2Base<T> Clamp(const Vector2Base<T>& value, T min, T max)
{
	return Vector2Base
	{
		Min(Max(value.x, min), max),
		Min(Max(value.y, min), max)
	};
}

template<typename T>
MATH_INLINE constexpr T Dot(const Vector2Base<T>& vec1, const Vector2Base<T>& vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y;
}
    
template<typename T>
MATH_INLINE constexpr T Length(const Vector2Base<T>& vec)
{
    return Dot(vec, vec);
}

template<typename T>
MATH_INLINE constexpr Vector2Base<T> Normalize(const Vector2Base<T>& vec)
{
    return vec / Length(vec);
}

} // namespace Math

using Vector2 = Vector2Base<float>;
using Vector2Int = Vector2Base<int>;

} // namespace Gleam
