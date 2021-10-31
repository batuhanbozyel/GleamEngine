#pragma once

namespace Gleam {

struct Vector2
{
	union
	{
		struct
		{
			float x, y;
		};
		struct
		{
			float r, g;
		};
		TArray<float, 2> value;
	};

	Vector2() = default;
	constexpr Vector2(float v)
		: x(v), y(v)
	{

	}
	constexpr Vector2(float x, float y)
		: x(x), y(y)
	{

	}
	constexpr Vector2(const TArray<float, 2>& list)
		: x(list[0]), y(list[1])
	{

	}

	MATH_INLINE constexpr float operator[](size_t i) const
	{
		return value[i];
	}

	MATH_INLINE constexpr Vector2 operator*(float val) const
	{
		return Vector2{ x * val, y * val };
	}

	MATH_INLINE constexpr Vector2 operator/(float val) const
	{
		return Vector2{ x / val, y / val };
	}

	MATH_INLINE constexpr Vector2 operator+(float val) const
	{
		return Vector2{ x + val, y + val };
	}

	MATH_INLINE constexpr Vector2 operator-(float val) const
	{
		return Vector2{ x - val, y - val };
	}

	MATH_INLINE constexpr Vector2 operator*(const Vector2& vec) const
	{
		return Vector2{ x * vec.x, y * vec.y };
	}

	MATH_INLINE constexpr Vector2 operator/(const Vector2& vec) const
	{
		return Vector2{ x / vec.x, y / vec.y };
	}

	MATH_INLINE constexpr Vector2 operator+(const Vector2& vec) const
	{
		return Vector2{ x + vec.x, y + vec.y };
	}

	MATH_INLINE constexpr Vector2 operator-(const Vector2& vec) const
	{
		return Vector2{ x - vec.x, y - vec.x };
	}

	MATH_INLINE constexpr Vector2& operator*=(float val)
	{
		x *= val;
		y *= val;
		return *this;
	}

	MATH_INLINE constexpr Vector2& operator/=(float val)
	{
		x /= val;
		y /= val;
		return *this;
	}

	MATH_INLINE constexpr Vector2& operator+=(float val)
	{
		x += val;
		y += val;
		return *this;
	}

	MATH_INLINE constexpr Vector2& operator-=(float val)
	{
		x -= val;
		y -= val;
		return *this;
	}

	MATH_INLINE constexpr Vector2& operator*=(const Vector2& vec)
	{
		x *= vec.x;
		y *= vec.y;
		return *this;
	}

	MATH_INLINE constexpr Vector2& operator/=(const Vector2& vec)
	{
		x /= vec.x;
		y /= vec.y;
		return *this;
	}

	MATH_INLINE constexpr Vector2& operator+=(const Vector2& vec)
	{
		x += vec.x;
		y += vec.y;
		return *this;
	}

	MATH_INLINE constexpr Vector2& operator-=(const Vector2& vec)
	{
		x -= vec.x;
		y -= vec.y;
		return *this;
	}
	
};

struct Vector3
{
	union
	{
		struct
		{
			float x, y, z;
		};
		struct
		{
			float r, g, b;
		};
		TArray<float, 3> value;
	};

	Vector3() = default;
	constexpr Vector3(float v)
		: x(v), y(v), z(v)
	{

	}
	constexpr Vector3(float x, float y, float z)
		: x(x), y(y), z(z)
	{

	}
	constexpr Vector3(const TArray<float, 3>& list)
		: x(list[0]), y(list[1]), z(list[2])
	{

	}

	MATH_INLINE constexpr float operator[](size_t i) const
	{
		return value[i];
	}

	MATH_INLINE constexpr Vector3 operator*(float val) const
	{
		return Vector3{ x * val, y * val, z * val };
	}

	MATH_INLINE constexpr Vector3 operator/(float val) const
	{
		return Vector3{ x / val, y / val, z / val };
	}

	MATH_INLINE constexpr Vector3 operator+(float val) const
	{
		return Vector3{ x + val, y + val, z + val };
	}

	MATH_INLINE constexpr Vector3 operator-(float val) const
	{
		return Vector3{ x - val, y - val, z - val };
	}

	MATH_INLINE constexpr Vector3 operator*(const Vector3& vec) const
	{
		return Vector3{ x * vec.x, y * vec.y, z * vec.z };
	}

	MATH_INLINE constexpr Vector3 operator/(const Vector3& vec) const
	{
		return Vector3{ x / vec.x, y / vec.y, z / vec.z };
	}

	MATH_INLINE constexpr Vector3 operator+(const Vector3& vec) const
	{
		return Vector3{ x + vec.x, y + vec.y, z + vec.z };
	}

	MATH_INLINE constexpr Vector3 operator-(const Vector3& vec) const
	{
		return Vector3{ x - vec.x, y - vec.x, z - vec.z };
	}

	MATH_INLINE constexpr Vector3& operator*=(float val)
	{
		x *= val;
		y *= val;
		z *= val;
		return *this;
	}

	MATH_INLINE constexpr Vector3& operator/=(float val)
	{
		x /= val;
		y /= val;
		z /= val;
		return *this;
	}

	MATH_INLINE constexpr Vector3& operator+=(float val)
	{
		x += val;
		y += val;
		z += val;
		return *this;
	}

	MATH_INLINE constexpr Vector3& operator-=(float val)
	{
		x -= val;
		y -= val;
		z -= val;
		return *this;
	}

	MATH_INLINE constexpr Vector3& operator*=(const Vector3& vec)
	{
		x *= vec.x;
		y *= vec.y;
		z *= vec.z;
		return *this;
	}

	MATH_INLINE constexpr Vector3& operator/=(const Vector3& vec)
	{
		x /= vec.x;
		y /= vec.y;
		z /= vec.z;
		return *this;
	}

	MATH_INLINE constexpr Vector3& operator+=(const Vector3& vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
		return *this;
	}

	MATH_INLINE constexpr Vector3& operator-=(const Vector3& vec)
	{
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		return *this;
	}

};

struct Vector4
{
	union
	{
		struct
		{
			float x, y, z, w;
		};
		struct
		{
			float r, g, b, a;
		};
		TArray<float, 4> value;
	};

	Vector4() = default;
	constexpr Vector4(float v)
		: x(v), y(v), z(v), w(v)
	{

	}
	constexpr Vector4(float x, float y, float z, float w)
		: x(x), y(y), z(z), w(w)
	{

	}
	constexpr Vector4(const TArray<float, 4>& list)
		: x(list[0]), y(list[1]), z(list[2]), w(list[3])
	{

	}

	MATH_INLINE constexpr float operator[](size_t i) const
	{
		return value[i];
	}

	MATH_INLINE constexpr Vector4 operator*(float val) const
	{
		return Vector4{ x * val, y * val, z * val, w * val };
	}

	MATH_INLINE constexpr Vector4 operator/(float val) const
	{
		return Vector4{ x / val, y / val, z / val, w / val };
	}

	MATH_INLINE constexpr Vector4 operator+(float val) const
	{
		return Vector4{ x + val, y + val, z + val, w + val };
	}

	MATH_INLINE constexpr Vector4 operator-(float val) const
	{
		return Vector4{ x - val, y - val, z - val, w - val };
	}

	MATH_INLINE constexpr Vector4 operator*(const Vector4& vec) const
	{
		return Vector4{ x * vec.x, y * vec.y, z * vec.z, w * vec.w };
	}

	MATH_INLINE constexpr Vector4 operator/(const Vector4& vec) const
	{
		return Vector4{ x / vec.x, y / vec.y, z / vec.z, w / vec.w };
	}

	MATH_INLINE constexpr Vector4 operator+(const Vector4& vec) const
	{
		return Vector4{ x + vec.x, y + vec.y, z + vec.z, w + vec.w };
	}

	MATH_INLINE constexpr Vector4 operator-(const Vector4& vec) const
	{
		return Vector4{ x - vec.x, y - vec.x, z - vec.z, w - vec.w };
	}

	MATH_INLINE constexpr Vector4& operator*=(float val)
	{
		x *= val;
		y *= val;
		z *= val;
		w *= val;
		return *this;
	}

	MATH_INLINE constexpr Vector4& operator/=(float val)
	{
		x /= val;
		y /= val;
		z /= val;
		w /= val;
		return *this;
	}

	MATH_INLINE constexpr Vector4& operator+=(float val)
	{
		x += val;
		y += val;
		z += val;
		w += val;
		return *this;
	}

	MATH_INLINE constexpr Vector4& operator-=(float val)
	{
		x -= val;
		y -= val;
		z -= val;
		w -= val;
		return *this;
	}

	MATH_INLINE constexpr Vector4& operator*=(const Vector4& vec)
	{
		x *= vec.x;
		y *= vec.y;
		z *= vec.z;
		w *= vec.w;
		return *this;
	}

	MATH_INLINE constexpr Vector4& operator/=(const Vector4& vec)
	{
		x /= vec.x;
		y /= vec.y;
		z /= vec.z;
		w /= vec.w;
		return *this;
	}

	MATH_INLINE constexpr Vector4& operator+=(const Vector4& vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
		w += vec.w;
		return *this;
	}

	MATH_INLINE constexpr Vector4& operator-=(const Vector4& vec)
	{
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		w -= vec.w;
		return *this;
	}

};

namespace Math {

template<>
MATH_INLINE constexpr Vector2 Cos<Vector2>(Vector2 vec)
{
	return Vector2
	{
		std::cos(vec.x),
		std::cos(vec.y)
	};
}

template<>
MATH_INLINE constexpr Vector3 Cos<Vector3>(Vector3 vec)
{
	return Vector3
	{
		std::cos(vec.x),
		std::cos(vec.y),
		std::cos(vec.z)
	};
}

template<>
MATH_INLINE constexpr Vector4 Cos<Vector4>(Vector4 vec)
{
	return Vector4
	{
		std::cos(vec.x),
		std::cos(vec.y),
		std::cos(vec.z),
		std::cos(vec.w)
	};
}

template<>
MATH_INLINE constexpr Vector2 Sin<Vector2>(Vector2 vec)
{
	return Vector2
	{
		std::sin(vec.x),
		std::sin(vec.y)
	};
}

template<>
MATH_INLINE constexpr Vector3 Sin<Vector3>(Vector3 vec)
{
	return Vector3
	{
		std::sin(vec.x),
		std::sin(vec.y),
		std::sin(vec.z)
	};
}

template<>
MATH_INLINE constexpr Vector4 Sin<Vector4>(Vector4 vec)
{
	return Vector4
	{
		std::sin(vec.x),
		std::sin(vec.y),
		std::sin(vec.z),
		std::sin(vec.w)
	};
}

template<>
MATH_INLINE constexpr Vector2 Abs<Vector2>(Vector2 vec)
{
	return Vector2
	{
		std::abs(vec.x),
		std::abs(vec.y)
	};
}

template<>
MATH_INLINE constexpr Vector3 Abs<Vector3>(Vector3 vec)
{
	return Vector3
	{
		std::abs(vec.x),
		std::abs(vec.y),
		std::abs(vec.z)
	};
}

template<>
MATH_INLINE constexpr Vector4 Abs<Vector4>(Vector4 vec)
{
	return Vector4
	{
		std::abs(vec.x),
		std::abs(vec.y),
		std::abs(vec.z),
		std::abs(vec.w)
	};
}

}

}