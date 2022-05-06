#pragma once

namespace Gleam {

struct Matrix4
{
	union
	{
		TArray<float, 16> value;
		TArray<Vector4, 4> row;
	};

    static const Matrix4 zero;
    static const Matrix4 identity;

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
    
    MATH_INLINE static constexpr Matrix4 Frustum(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        return Matrix4
        {
            2.0f * zNear / (right - left),  0.0f,                           (right + left) / (right - left),    0.0f,
            0.0f,                           2.0f * zNear / (top - bottom),  (top + bottom) / (top - bottom),    0.0f,
            0.0f,                           0.0f,                           (zFar + zNear) / (zNear - zFar),    (2.0f * zFar * zNear) / (zNear - zFar),
            0.0f,                           0.0f,                           -1.0f,                              0.0f
        };
    }
    
    MATH_INLINE static constexpr Matrix4 LookAt(const Vector3& from, const Vector3& to, const Vector3& up)
    {
        Vector3 n = Math::Normalize(to - from);
        Vector3 u = Math::Normalize(Math::Cross(n, up));
        Vector3 v = Math::Cross(u, n);
        
        return Matrix4
        {
            u.x,    u.y,    u.z,    Math::Dot(-to, u),
            v.x,    v.y,    v.z,    Math::Dot(-to, v),
            n.x,    n.y,    n.z,    Math::Dot(-to, n),
            0.0f,   0.0f,   0.0f,   1.0f
        };
    }
    
    MATH_INLINE static constexpr Matrix4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        return Matrix4
        {
            2.0f / (right - left),  0.0f,                   0.0f,                   -(right + left) / (right - left),
            0.0f,                   2.0f / (top - bottom),  0.0f,                   -(top + bottom) / (top - bottom),
            0.0f,                   0.0f,                   -2.0f / (zFar - zNear), -(zFar + zNear) / (zFar - zNear),
            0.0f,                   0.0f,                   0.0f,                   1.0f
        };
    }
    
    MATH_INLINE static constexpr Matrix4 Perspective(float fov, float aspect, float zNear, float zFar)
    {
        float focalLength = Math::Tan(Math::Deg2Rad(fov / 2.0f));
        return Matrix4
        {
            1.0f / (focalLength * aspect),  0.0f,                   0.0f,                               0.0f,
            0.0f,                           1.0f / focalLength,     0.0f,                               0.0f,
            0.0f,                           0.0f,                   (zFar + zNear) / (zNear - zFar),   (2.0f * zFar * zNear) / (zNear - zFar),
            0.0f,                           0.0f,                   -1.0f,                              0.0f
        };
    }
};

} // namespace Gleam
