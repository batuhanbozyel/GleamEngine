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

MATH_INLINE constexpr Vector2 Cos(const Vector2& vec)
{
	return Vector2
	{
		std::cos(vec.x),
		std::cos(vec.y)
	};
}

MATH_INLINE constexpr Vector3 Cos(const Vector3& vec)
{
	return Vector3
	{
		std::cos(vec.x),
		std::cos(vec.y),
		std::cos(vec.z)
	};
}

MATH_INLINE constexpr Vector4 Cos(const Vector4& vec)
{
	return Vector4
	{
		std::cos(vec.x),
		std::cos(vec.y),
		std::cos(vec.z),
		std::cos(vec.w)
	};
}

MATH_INLINE constexpr Vector2 Sin(const Vector2& vec)
{
	return Vector2
	{
		std::sin(vec.x),
		std::sin(vec.y)
	};
}

MATH_INLINE constexpr Vector3 Sin(const Vector3& vec)
{
	return Vector3
	{
		std::sin(vec.x),
		std::sin(vec.y),
		std::sin(vec.z)
	};
}

MATH_INLINE constexpr Vector4 Sin(const Vector4& vec)
{
	return Vector4
	{
		std::sin(vec.x),
		std::sin(vec.y),
		std::sin(vec.z),
		std::sin(vec.w)
	};
}

MATH_INLINE constexpr Vector2 Abs(const Vector2& vec)
{
	return Vector2
	{
		std::abs(vec.x),
		std::abs(vec.y)
	};
}

MATH_INLINE constexpr Vector3 Abs(const Vector3& vec)
{
	return Vector3
	{
		std::abs(vec.x),
		std::abs(vec.y),
		std::abs(vec.z)
	};
}

MATH_INLINE constexpr Vector4 Abs(const Vector4& vec)
{
	return Vector4
	{
		std::abs(vec.x),
		std::abs(vec.y),
		std::abs(vec.z),
		std::abs(vec.w)
	};
}

MATH_INLINE constexpr Vector2 Deg2Rad(const Vector2& degrees)
{
	return Vector2
	{
		Deg2Rad(degrees.x),
		Deg2Rad(degrees.y)
	};
}

MATH_INLINE constexpr Vector3 Deg2Rad(const Vector3& degrees)
{
	return Vector3
	{
		Deg2Rad(degrees.x),
		Deg2Rad(degrees.y),
		Deg2Rad(degrees.z)
	};
}

MATH_INLINE constexpr Vector4 Deg2Rad(const Vector4& degrees)
{
	return Vector4
	{
		Deg2Rad(degrees.x),
		Deg2Rad(degrees.y),
		Deg2Rad(degrees.z),
		Deg2Rad(degrees.w)
	};
}

MATH_INLINE constexpr Vector2 Rad2Deg(const Vector2& radians)
{
	return Vector2
	{
		Rad2Deg(radians.x),
		Rad2Deg(radians.y)
	};
}

MATH_INLINE constexpr Vector3 Rad2Deg(const Vector3& radians)
{
	return Vector3
	{
		Rad2Deg(radians.x),
		Rad2Deg(radians.y),
		Rad2Deg(radians.z)
	};
}

MATH_INLINE constexpr Vector4 Rad2Deg(const Vector4& radians)
{
	return Vector4
	{
		Rad2Deg(radians.x),
		Rad2Deg(radians.y),
		Rad2Deg(radians.z),
		Rad2Deg(radians.w)
	};
}

MATH_INLINE constexpr Vector2 Min(const Vector2& v0, const Vector2& v1)
{
	return Vector2
	{
		Min(v0.x, v1.x),
		Min(v0.y, v1.y)
	};
}

MATH_INLINE constexpr Vector3 Min(const Vector3& v0, const Vector3& v1)
{
	return Vector3
	{
		Min(v0.x, v1.x),
		Min(v0.y, v1.y),
		Min(v0.z, v1.z)
	};
}

MATH_INLINE constexpr Vector4 Min(const Vector4& v0, const Vector4& v1)
{
	return Vector4
	{
		Min(v0.x, v1.x),
		Min(v0.y, v1.y),
		Min(v0.z, v1.z),
		Min(v0.w, v1.w),
	};
}

MATH_INLINE constexpr Vector2 Max(const Vector2& v0, const Vector2& v1)
{
	return Vector2
	{
		Max(v0.x, v1.x),
		Max(v0.y, v1.y)
	};
}

MATH_INLINE constexpr Vector3 Max(const Vector3& v0, const Vector3& v1)
{
	return Vector3
	{
		Max(v0.x, v1.x),
		Max(v0.y, v1.y),
		Max(v0.z, v1.z)
	};
}

MATH_INLINE constexpr Vector4 Max(const Vector4& v0, const Vector4& v1)
{
	return Vector4
	{
		Max(v0.x, v1.x),
		Max(v0.y, v1.y),
		Max(v0.z, v1.z),
		Max(v0.w, v1.w),
	};
}

}

}