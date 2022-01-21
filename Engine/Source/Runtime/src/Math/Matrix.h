#pragma once

namespace Gleam {

struct Quaternion
{
	union
	{
		struct
		{
			float w, x, y, z;
		};
		TArray<float, 4> value;
	};

	MATH_INLINE Vector3 EularAngles() const
	{
		return Vector3
		{
			Math::Atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y)),
			Math::Asin(Math::Clamp(2.0f * (w * y - z * x), -1.0f, 1.0f)),
			Math::Atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z))
		};
	}

	MATH_INLINE static constexpr Quaternion Identity()
	{
		return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	}

	Quaternion() = default;
	constexpr Quaternion(Quaternion&&) = default;
	constexpr Quaternion(const Quaternion&) = default;
	inline constexpr Quaternion& operator=(Quaternion&&) = default;
	inline constexpr Quaternion& operator=(const Quaternion&) = default;

	constexpr Quaternion(float w, float x, float y, float z)
		: w(w), x(x), y(y), z(z)
	{

	}
	constexpr Quaternion(const TArray<float, 4>& quat)
		: w(quat[0]), x(quat[1]), y(quat[2]), z(quat[3])
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

	MATH_INLINE constexpr float operator[](size_t i) const
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

	MATH_INLINE static constexpr Matrix2 Identity()
	{
		return Matrix2
		{
			1.0f, 0.0f,
			0.0f, 1.0f
		};
	}

	Matrix2() = default;
	constexpr Matrix2(Matrix2&&) = default;
	constexpr Matrix2(const Matrix2&) = default;
	inline constexpr Matrix2& operator=(Matrix2&&) = default;
	inline constexpr Matrix2& operator=(const Matrix2&) = default;

	constexpr Matrix2(float m00, float m01,
					  float m10, float m11)
		: value{m00, m01,
				m10, m11}
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

	MATH_INLINE constexpr const Vector2& operator[](size_t i) const
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

	MATH_INLINE static constexpr Matrix3 Identity()
	{
		return Matrix3
		{
			1.0f, 0.0f,0.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 1.0f
		};
	}

	Matrix3() = default;
	constexpr Matrix3(Matrix3&&) = default;
	constexpr Matrix3(const Matrix3&) = default;
	inline constexpr Matrix3& operator=(Matrix3&&) = default;
	inline constexpr Matrix3& operator=(const Matrix3&) = default;

	constexpr Matrix3(float m00, float m01, float m02,
					  float m10, float m11, float m12,
					  float m20, float m21, float m22)
		: value{m00, m01, m02,
				m10, m11, m12,
				m20, m21, m22}
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

	MATH_INLINE constexpr const Vector3& operator[](size_t i) const
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

	MATH_INLINE static constexpr Matrix4 Identity()
	{
		return Matrix4
		{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
	}

	Matrix4() = default;
	constexpr Matrix4(Matrix4&&) = default;
	constexpr Matrix4(const Matrix4&) = default;
	inline constexpr Matrix4& operator=(Matrix4&&) = default;
	inline constexpr Matrix4& operator=(const Matrix4&) = default;

	constexpr Matrix4(float m00, float m01, float m02, float m03,
					  float m10, float m11, float m12, float m13,
					  float m20, float m21, float m22, float m23,
					  float m30, float m31, float m32, float m33)
		: value{m00, m01, m02, m03,
				m10, m11, m12, m13,
				m20, m21, m22, m23,
				m30, m31, m32, m33}
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

	MATH_INLINE constexpr const Vector4& operator[](size_t i) const
	{
		return row[i];
	}

	MATH_INLINE static constexpr Matrix4 Translate(const Vector3& translation)
	{
		return Matrix4
		{
			1.0f, 0.0f, 0.0f, translation.x,
			0.0f, 1.0f, 0.0f, translation.y,
			0.0f, 0.0f, 1.0f, translation.z,
			0.0f, 0.0f, 0.0f, 1.0f
		};
	}

	MATH_INLINE static constexpr Matrix4 Rotate(const Quaternion& quat)
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

	MATH_INLINE static constexpr Matrix4 Scale(const Vector3& scale)
	{
		return Matrix4
		{
			scale.x,	0.0f,		0.0f,		0.0f,
			0.0f,		scale.y,	0.0f,		0.0f,
			0.0f,		0.0f,		scale.z,	0.0f,
			0.0f,		0.0f,		0.0f,		1.0f
		};
	}

	MATH_INLINE static constexpr Matrix4 TRS(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
	{
		float qxx = rotation.x * rotation.x;
		float qxy = rotation.x * rotation.y;
		float qxz = rotation.x * rotation.z;
		float qyy = rotation.y * rotation.y;
		float qyz = rotation.y * rotation.z;
		float qzz = rotation.z * rotation.z;
		float qwx = rotation.w * rotation.x;
		float qwy = rotation.w * rotation.y;
		float qwz = rotation.w * rotation.z;

		return Matrix4
		{
			scale.x - 2.0f * scale.x * (qyy + qzz),		2.0f * (qxy + qwz),							2.0f * (qxz - qwy),						translation.x,
			2.0f * (qxy - qwz),							scale.y - 2.0f * scale.y * (qxx + qzz),		2.0f * (qyz + qwx),						translation.y,
			2.0f * (qxz + qwy),							2.0f * (qyz - qwx),							scale.z - 2.0f * scale.z * (qxx + qyy),	translation.z,
			0.0f,										0.0f,										0.0f,									1.0f
		};
	}
};

} // namespace Gleam