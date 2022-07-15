#pragma once

namespace Gleam {

struct Matrix4
{
	union
	{
		TArray<float, 16> m;
		TArray<Vector4, 4> row;
	};

    static const Matrix4 zero;
    static const Matrix4 identity;

	constexpr Matrix4() = default;
	constexpr Matrix4(Matrix4&&) = default;
	constexpr Matrix4(const Matrix4&) = default;
	FORCE_INLINE constexpr Matrix4& operator=(Matrix4&&) = default;
	FORCE_INLINE constexpr Matrix4& operator=(const Matrix4&) = default;

	constexpr Matrix4(float m00, float m01, float m02, float m03,
					  float m10, float m11, float m12, float m13,
					  float m20, float m21, float m22, float m23,
					  float m30, float m31, float m32, float m33)
		: m{m00, m01, m02, m03,
            m10, m11, m12, m13,
            m20, m21, m22, m23,
            m30, m31, m32, m33}
	{

	}
	constexpr Matrix4(const Vector4& row0, const Vector4& row1)
		: row{ row0, row1 }
	{

	}
	constexpr Matrix4(const TArray<float, 16>& m)
		: m(m)
	{

	}
	constexpr Matrix4(const TArray<Vector4, 4>& row)
		: row(row)
	{

	}

    NO_DISCARD FORCE_INLINE constexpr Vector4& operator[](size_t i)
    {
        return row[i];
    }
    
	NO_DISCARD FORCE_INLINE constexpr const Vector4& operator[](size_t i) const
	{
		return row[i];
	}
    
    NO_DISCARD FORCE_INLINE constexpr Matrix4 operator*(const Matrix4& rhs) const
    {
        return Matrix4
        {
            rhs.m[0] * m[0] + rhs.m[1] * m[4] + rhs.m[2] * m[8] + rhs.m[3] * m[12],
            rhs.m[0] * m[1] + rhs.m[1] * m[5] + rhs.m[2] * m[9] + rhs.m[3] * m[13],
            rhs.m[0] * m[2] + rhs.m[1] * m[6] + rhs.m[2] * m[10] + rhs.m[3] * m[14],
            rhs.m[0] * m[3] + rhs.m[1] * m[7] + rhs.m[2] * m[11] + rhs.m[3] * m[15],

            rhs.m[4] * m[0] + rhs.m[5] * m[4] + rhs.m[6] * m[8] + rhs.m[7] * m[12],
            rhs.m[4] * m[1] + rhs.m[5] * m[5] + rhs.m[6] * m[9] + rhs.m[7] * m[13],
            rhs.m[4] * m[2] + rhs.m[5] * m[6] + rhs.m[6] * m[10] + rhs.m[7] * m[14],
            rhs.m[4] * m[3] + rhs.m[5] * m[7] + rhs.m[6] * m[11] + rhs.m[7] * m[15],

            rhs.m[8] * m[0] + rhs.m[9] * m[4] + rhs.m[10] * m[8] + rhs.m[11] * m[12],
            rhs.m[8] * m[1] + rhs.m[9] * m[5] + rhs.m[10] * m[9] + rhs.m[11] * m[13],
            rhs.m[8] * m[2] + rhs.m[9] * m[6] + rhs.m[10] * m[10] + rhs.m[11] * m[14],
            rhs.m[8] * m[3] + rhs.m[9] * m[7] + rhs.m[10] * m[11] + rhs.m[11] * m[15],

            rhs.m[12] * m[0] + rhs.m[13] * m[4] + rhs.m[14] * m[8] + rhs.m[15] * m[12],
            rhs.m[12] * m[1] + rhs.m[13] * m[5] + rhs.m[14] * m[9] + rhs.m[15] * m[13],
            rhs.m[12] * m[2] + rhs.m[13] * m[6] + rhs.m[14] * m[10] + rhs.m[15] * m[14],
            rhs.m[12] * m[3] + rhs.m[13] * m[7] + rhs.m[14] * m[11] + rhs.m[15] * m[15]
        };
    }
    
    NO_DISCARD FORCE_INLINE constexpr Matrix4& operator*=(const Matrix4& rhs)
    {
        return *this = *this * rhs;
    }

	NO_DISCARD FORCE_INLINE static constexpr Matrix4 Translate(const Vector3& translation)
	{
		return Matrix4
		{
			1.0f, 0.0f, 0.0f, translation.x,
			0.0f, 1.0f, 0.0f, translation.y,
			0.0f, 0.0f, 1.0f, translation.z,
			0.0f, 0.0f, 0.0f, 1.0f
		};
	}

	NO_DISCARD FORCE_INLINE static constexpr Matrix4 Rotate(const Quaternion& quat)
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

	NO_DISCARD FORCE_INLINE static constexpr Matrix4 Scale(const Vector3& scale)
	{
		return Matrix4
		{
			scale.x,	0.0f,		0.0f,		0.0f,
			0.0f,		scale.y,	0.0f,		0.0f,
			0.0f,		0.0f,		scale.z,	0.0f,
			0.0f,		0.0f,		0.0f,		1.0f
		};
	}

	NO_DISCARD FORCE_INLINE static constexpr Matrix4 TRS(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
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
    
    NO_DISCARD FORCE_INLINE static constexpr Matrix4 Frustum(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        return Matrix4
        {
            2.0f * zNear / (right - left),  0.0f,                           (right + left) / (right - left),    0.0f,
            0.0f,                           2.0f * zNear / (top - bottom),  (top + bottom) / (top - bottom),    0.0f,
            0.0f,                           0.0f,                           (zFar + zNear) / (zNear - zFar),    (2.0f * zFar * zNear) / (zNear - zFar),
            0.0f,                           0.0f,                           -1.0f,                              0.0f
        };
    }
    
    NO_DISCARD FORCE_INLINE static constexpr Matrix4 LookAt(const Vector3& from, const Vector3& to, const Vector3& up)
    {
        Vector3 front = Math::Normalize(to - from);
        Vector3 side = Math::Normalize(Math::Cross(front, up));
        Vector3 upV = Math::Cross(side, front);
        
        return Matrix4
        {
            side.x,     side.y,     side.z,     Math::Dot(-from, side),
            upV.x,      upV.y,      upV.z,      Math::Dot(-from, upV),
            front.x,    front.y,    front.z,    Math::Dot(-from, front),
            0.0f,   0.0f,   0.0f,   1.0f
        };
    }
    
    NO_DISCARD FORCE_INLINE static constexpr Matrix4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        return Matrix4
        {
            2.0f / (right - left),  0.0f,                   0.0f,                   (right + left) / (left - right),
            0.0f,                   2.0f / (top - bottom),  0.0f,                   (top + bottom) / (bottom - top),
            0.0f,                   0.0f,                   -2.0f / (zFar - zNear), (zFar + zNear) / (zNear- zFar),
            0.0f,                   0.0f,                   0.0f,                   1.0f
        };
    }
    
    NO_DISCARD FORCE_INLINE static constexpr Matrix4 Perspective(float fov, float aspect, float zNear, float zFar)
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
