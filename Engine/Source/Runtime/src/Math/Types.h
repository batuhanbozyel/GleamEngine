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

	[[nodiscard]] constexpr inline float operator[](size_t i) const
	{
		return value[i];
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

	[[nodiscard]] constexpr inline float operator[](size_t i) const
	{
		return value[i];
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

	[[nodiscard]] constexpr inline float operator[](size_t i) const
	{
		return value[i];
	}

};

struct Quaternion
{
	union
	{
		struct
		{
			float w, x, y, z;
			TArray<float, 4> value;
		};
	};

	Quaternion() = default;
	constexpr Quaternion(float w, float x, float y, float z)
		: w(w), x(x), y(y), z(z)
	{

	}
	constexpr Quaternion(const TArray<float, 4>& list)
		: w(list[0]), x(list[1]), y(list[2]), z(list[3])
	{

	}
	constexpr Quaternion(const Vector3& eularAngles)
	{
		Vector3 c = Math::Cos(eularAngles * 0.5f);
		Vector3 s = Math::Sin(eularAngles * 0.5f);

		w = c.x * c.y * c.z + s.x * s.y * s.z;
		x = s.x * c.y * c.z - c.x * s.y * s.z;
		y = c.x * s.y * c.z + s.x * c.y * s.z;
		z = c.x * c.y * s.z - s.x * s.y * c.z;
	}

	[[nodiscard]] constexpr inline float operator[](size_t i) const
	{
		return value[i];
	}

};

struct Matrix2
{
	union
	{
		TArray<float, 4> value;
		TArray<Vector2, 2> row;
	};

	Matrix2() = default;
	constexpr Matrix2(float m00, float m01, float m10, float m11)
		: value{m00, m01, m10, m11}
	{

	}
	constexpr Matrix2(const Vector2& row0, const Vector2& row1)
		: row{ row0, row1 }
	{

	}
	constexpr Matrix2(const TArray<float, 4>& list)
		: value(list)
	{

	}
	constexpr Matrix2(const TArray<Vector2, 2>& list)
		: row(list)
	{

	}

	[[nodiscard]] constexpr inline const Vector2& operator[](size_t i) const
	{
		return row[i];
	}

};

struct Matrix3
{
	union
	{
		TArray<float, 9> value;
		TArray<Vector3, 3> row;
	};

	Matrix3() = default;
	constexpr Matrix3(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
		: value{ m00, m01, m02, m10, m11, m12, m20, m21, m22 }
	{

	}
	constexpr Matrix3(const Vector3& row0, const Vector3& row1, const Vector3& row2)
		: row{ row0, row1, row2 }
	{

	}
	constexpr Matrix3(const TArray<float, 9>& list)
		: value(list)
	{

	}
	constexpr Matrix3(const TArray<Vector3, 3>& list)
		: row(list)
	{

	}

	[[nodiscard]] constexpr inline const Vector3& operator[](size_t i) const
	{
		return row[i];
	}

};

struct Matrix4
{
	union
	{
		TArray<float, 16> value;
		TArray<Vector4, 4> row;
	};

	Matrix4() = default;
	constexpr Matrix4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33)
		: value{ m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33 }
	{

	}
	constexpr Matrix4(const Vector4& row0, const Vector4& row1)
		: row{ row0, row1 }
	{

	}
	constexpr Matrix4(const TArray<float, 16>& list)
		: value(list)
	{

	}
	constexpr Matrix4(const TArray<Vector4, 4>& list)
		: row(list)
	{

	}

	[[nodiscard]] constexpr inline const Vector4& operator[](size_t i) const
	{
		return row[i];
	}

	[[nodiscard]] static inline constexpr Matrix4 Identity()
	{
		return Matrix4
		{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
	}

	[[nodiscard]] static inline constexpr Matrix4 Translate(const Vector3& translation)
	{
		return Matrix4
		{
			1.0f, 0.0f, 0.0f, translation.x,
			0.0f, 1.0f, 0.0f, translation.y,
			0.0f, 0.0f, 1.0f, translation.z,
			0.0f, 0.0f, 0.0f, 1.0f
		};
	}

	[[nodiscard]] static inline constexpr Matrix4 Rotate(const Quaternion& quat)
	{
		float qxx = quat.x * quat.x;
		float qxy = quat.x * quat.y;
		float qxz = quat.x * quat.z;
		float qyy = quat.y * quat.y;
		float qyz = quat.y * quat.z;
		float qzz = quat.z * quat.z;
		float qwx = quat.w * quat.x;
		float qwy = quat.w * quat.y;
		float qwz = quat.w * quat.z;

		return Matrix4
		{
			1.0f - 2.0f * (qyy + qzz),	2.0f * (qxy + qwz),			2.0f * (qxz - qwy),			0.0f,
			2.0f * (qxy - qwz),			1.0f - 2.0f * (qxx + qzz),	2.0f * (qyz + qwx),			0.0f,
			2.0f * (qxz + qwy),			2.0f * (qyz - qwx),			1.0f - 2.0f * (qxx + qyy),	0.0f,
			0.0f,						0.0f,						0.0f,						1.0f
		};
	}

	[[nodiscard]] static inline constexpr Matrix4 Scale(const Vector3& scale)
	{
		return Matrix4
		{
			scale.x,	0.0f,		0.0f,		0.0f,
			0.0f,		scale.y,	0.0f,		0.0f,
			0.0f,		0.0f,		scale.z,	0.0f,
			0.0f,		0.0f,		0.0f,		1.0f
		};
	}

	[[nodiscard]] static inline constexpr Matrix4 TRS(const Vector3& translation, const Quaternion& quat, const Vector3& scale)
	{
		float qxx = quat.x * quat.x;
		float qxy = quat.x * quat.y;
		float qxz = quat.x * quat.z;
		float qyy = quat.y * quat.y;
		float qyz = quat.y * quat.z;
		float qzz = quat.z * quat.z;
		float qwx = quat.w * quat.x;
		float qwy = quat.w * quat.y;
		float qwz = quat.w * quat.z;

		return Matrix4
		{
			scale.x - 2.0f * scale.x * (qyy + qzz),		2.0f * (qxy + qwz),							2.0f * (qxz - qwy),						translation.x,
			2.0f * (qxy - qwz),							scale.y - 2.0f * scale.y * (qxx + qzz),		2.0f * (qyz + qwx),						translation.y,
			2.0f * (qxz + qwy),							2.0f * (qyz - qwx),							scale.z - 2.0f * scale.z * (qxx + qyy),	translation.z,
			0.0f,										0.0f,										0.0f,									1.0f
		};
	}
};

}