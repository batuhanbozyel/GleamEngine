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

	MATH_INLINE constexpr T operator[](size_t i) const
	{
		return value[i];
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

	Vector3Base() = default;
	constexpr Vector3Base(Vector3Base&&) = default;
	constexpr Vector3Base(const Vector3Base&) = default;
	inline constexpr Vector3Base& operator=(Vector3Base&&) = default;
	inline constexpr Vector3Base& operator=(const Vector3Base&) = default;

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

	MATH_INLINE constexpr T operator[](size_t i) const
	{
		return value[i];
	}

	MATH_INLINE constexpr Vector3Base operator*(T val) const
	{
		return Vector3Base
		{
			x * val,
			y * val,
			z * val
		};
	}

	MATH_INLINE constexpr Vector3Base operator/(T val) const
	{
		return Vector3Base
		{
			x / val,
			y / val,
			z / val
		};
	}

	MATH_INLINE constexpr Vector3Base operator+(T val) const
	{
		return Vector3Base
		{
			x + val,
			y + val,
			z + val
		};
	}

	MATH_INLINE constexpr Vector3Base operator-(T val) const
	{
		return Vector3Base
		{
			x - val,
			y - val,
			z - val
		};
	}

	MATH_INLINE constexpr Vector3Base operator*(const Vector3Base& vec) const
	{
		return Vector3Base
		{
			x * vec.x,
			y * vec.y,
			z * vec.z
		};
	}

	MATH_INLINE constexpr Vector3Base operator/(const Vector3Base& vec) const
	{
		return Vector3Base
		{
			x / vec.x,
			y / vec.y,
			z / vec.z
		};
	}

	MATH_INLINE constexpr Vector3Base operator+(const Vector3Base& vec) const
	{
		return Vector3Base
		{
			x + vec.x,
			y + vec.y,
			z + vec.z
		};
	}

	MATH_INLINE constexpr Vector3Base operator-(const Vector3Base& vec) const
	{
		return Vector3Base
		{
			x - vec.x,
			y - vec.x,
			z - vec.z
		};
	}

	MATH_INLINE constexpr Vector3Base& operator*=(T val)
	{
		x *= val;
		y *= val;
		z *= val;
		return *this;
	}

	MATH_INLINE constexpr Vector3Base& operator/=(T val)
	{
		x /= val;
		y /= val;
		z /= val;
		return *this;
	}

	MATH_INLINE constexpr Vector3Base& operator+=(T val)
	{
		x += val;
		y += val;
		z += val;
		return *this;
	}

	MATH_INLINE constexpr Vector3Base& operator-=(T val)
	{
		x -= val;
		y -= val;
		z -= val;
		return *this;
	}

	MATH_INLINE constexpr Vector3Base& operator*=(const Vector3Base& vec)
	{
		x *= vec.x;
		y *= vec.y;
		z *= vec.z;
		return *this;
	}

	MATH_INLINE constexpr Vector3Base& operator/=(const Vector3Base& vec)
	{
		x /= vec.x;
		y /= vec.y;
		z /= vec.z;
		return *this;
	}

	MATH_INLINE constexpr Vector3Base& operator+=(const Vector3Base& vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
		return *this;
	}

	MATH_INLINE constexpr Vector3Base& operator-=(const Vector3Base& vec)
	{
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		return *this;
	}

};

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

	MATH_INLINE constexpr float operator[](size_t i) const
	{
		return value[i];
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
MATH_INLINE constexpr Vector2Base<T> Cos(const Vector2Base<T>& vec)
{
	return Vector2Base
	{
		Cos(vec.x),
		Cos(vec.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector3Base<T> Cos(const Vector3Base<T>& vec)
{
	return Vector3Base
	{
		Cos(vec.x),
		Cos(vec.y),
		Cos(vec.z)
	};
}

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
MATH_INLINE constexpr Vector2Base<T> Sin(const Vector2Base<T>& vec)
{
	return Vector2Base
	{
		Sin(vec.x),
		Sin(vec.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector3Base<T> Sin(const Vector3Base<T>& vec)
{
	return Vector3Base
	{
		Sin(vec.x),
		Sin(vec.y),
		Sin(vec.z)
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
MATH_INLINE constexpr Vector2Base<T> Abs(const Vector2Base<T>& vec)
{
	return Vector2Base
	{
		Abs(vec.x),
		Abs(vec.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector3Base<T> Abs(const Vector3Base<T>& vec)
{
	return Vector3Base
	{
		Abs(vec.x),
		Abs(vec.y),
		Abs(vec.z)
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
MATH_INLINE constexpr Vector2Base<T> Fract(const Vector2Base<T>& vec)
{
	return Vector2Base
	{
		vec.x - static_cast<long>(vec.x),
		vec.y - static_cast<long>(vec.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector3Base<T> Fract(const Vector3Base<T>& vec)
{
	return Vector3Base
	{
		vec.x - static_cast<long>(vec.x),
		vec.y - static_cast<long>(vec.y),
		vec.z - static_cast<long>(vec.z)
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
MATH_INLINE constexpr Vector2Base<T> Deg2Rad(const Vector2Base<T>& degrees)
{
	return Vector2Base
	{
		Deg2Rad(degrees.x),
		Deg2Rad(degrees.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector3Base<T> Deg2Rad(const Vector3Base<T>& degrees)
{
	return Vector3Base
	{
		Deg2Rad(degrees.x),
		Deg2Rad(degrees.y),
		Deg2Rad(degrees.z)
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
MATH_INLINE constexpr Vector2Base<T> Rad2Deg(const Vector2Base<T>& radians)
{
	return Vector2Base
	{
		Rad2Deg(radians.x),
		Rad2Deg(radians.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector3Base<T> Rad2Deg(const Vector3Base<T>& radians)
{
	return Vector3Base
	{
		Rad2Deg(radians.x),
		Rad2Deg(radians.y),
		Rad2Deg(radians.z)
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
MATH_INLINE constexpr Vector2Base<T> Min(const Vector2Base<T>& v0, const Vector2Base<T>& v1)
{
	return Vector2Base
	{
		Min(v0.x, v1.x),
		Min(v0.y, v1.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector3Base<T> Min(const Vector3Base<T>& v0, const Vector3Base<T>& v1)
{
	return Vector3Base
	{
		Min(v0.x, v1.x),
		Min(v0.y, v1.y),
		Min(v0.z, v1.z)
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
MATH_INLINE constexpr Vector2Base<T> Max(const Vector2Base<T>& v0, const Vector2Base<T>& v1)
{
	return Vector2Base
	{
		Max(v0.x, v1.x),
		Max(v0.y, v1.y)
	};
}

template<typename T>
MATH_INLINE constexpr Vector3Base<T> Max(const Vector3Base<T>& v0, const Vector3Base<T>& v1)
{
	return Vector3Base
	{
		Max(v0.x, v1.x),
		Max(v0.y, v1.y),
		Max(v0.z, v1.z)
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
MATH_INLINE constexpr Vector2Base<T> Mix(const Vector2Base<T>& v0, const Vector2Base<T>& v1, T a)
{
	return v0 * (T(1) - a) + v1 * a;
}

template<typename T>
MATH_INLINE constexpr Vector3Base<T> Mix(const Vector3Base<T>& v0, const Vector3Base<T>& v1, T a)
{
	return v0 * (T(1) - a) + v1 * a;
}

template<typename T>
MATH_INLINE constexpr Vector4Base<T> Mix(const Vector4Base<T>& v0, const Vector4Base<T>& v1, T a)
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
MATH_INLINE constexpr Vector3Base<T> Clamp(const Vector3Base<T>& value, T min, T max)
{
	return Vector3Base
	{
		Min(Max(value.x, min), max),
		Min(Max(value.y, min), max),
		Min(Max(value.z, min), max)
	};
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

} // namespace Math

using Vector2 = Vector2Base<float>;
using Vector2Int = Vector2Base<int>;

using Vector3 = Vector3Base<float>;
using Vector3Int = Vector3Base<int>;

using Vector4 = Vector4Base<float>;
using Vector4Int = Vector4Base<int>;

} // namespace Gleam