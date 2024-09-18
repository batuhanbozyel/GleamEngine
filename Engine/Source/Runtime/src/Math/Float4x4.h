#pragma once

namespace Gleam {

struct Float4x4
{
    union
    {
        TArray<float, 16> m;
        TArray<Float4, 4> row{};
    };

    static const Float4x4 zero;
    static const Float4x4 identity;

    constexpr Float4x4() = default;
    constexpr Float4x4(Float4x4&&) noexcept = default;
    constexpr Float4x4(const Float4x4&) = default;
    FORCE_INLINE constexpr Float4x4& operator=(Float4x4&&) noexcept = default;
    FORCE_INLINE constexpr Float4x4& operator=(const Float4x4&) = default;

    constexpr Float4x4(float m00, float m01, float m02, float m03,
                      float m10, float m11, float m12, float m13,
                      float m20, float m21, float m22, float m23,
                      float m30, float m31, float m32, float m33)
        : m{m00, m01, m02, m03,
            m10, m11, m12, m13,
            m20, m21, m22, m23,
            m30, m31, m32, m33}
    {

    }
    constexpr Float4x4(const Float4& row0, const Float4& row1, const Float4& row2, const Float4& row3)
        : row{ row0, row1, row2, row3 }
    {

    }
    constexpr Float4x4(const TArray<float, 16>& m)
        : m(m)
    {

    }
    constexpr Float4x4(const TArray<Float4, 4>& row)
        : row(row)
    {

    }

    NO_DISCARD FORCE_INLINE constexpr Float4& operator[](size_t i)
    {
        return row[i];
    }

    NO_DISCARD FORCE_INLINE constexpr const Float4& operator[](size_t i) const
    {
        return row[i];
    }

    NO_DISCARD FORCE_INLINE constexpr Float3 operator*(const Float3& vec) const
    {
        float invW = 1.0f / (vec.x * m[3] + vec.y * m[7] + vec.z * m[11] + m[15]);
        return Float3
        {
            (vec.x * m[0] + vec.y * m[4] + vec.z * m[8] + m[12]) * invW,
            (vec.x * m[1] + vec.y * m[5] + vec.z * m[9] + m[13]) * invW,
            (vec.x * m[2] + vec.y * m[6] + vec.z * m[10] + m[14]) * invW
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Float4 operator*(const Float4& vec) const
    {
        return Float4
        {
            vec.x * m[0] + vec.y * m[4] + vec.z * m[8] + vec.w * m[12],
            vec.x * m[1] + vec.y * m[5] + vec.z * m[9] + vec.w * m[13],
            vec.x * m[2] + vec.y * m[6] + vec.z * m[10] + vec.w * m[14],
            vec.x * m[3] + vec.y * m[7] + vec.z * m[11] + vec.w * m[15]
        };
    }

    NO_DISCARD FORCE_INLINE constexpr Float4x4 operator*(const Float4x4& rhs) const
    {
        return Float4x4
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
    
    NO_DISCARD FORCE_INLINE constexpr Quaternion operator*(const Quaternion& quat)
    {
        float trace = row[0][0] + row[1][1] + row[2][2];
        if (trace > 0.0f)
        {
            float s = 0.5f / Math::Sqrt(trace + 1.0f);
            return quat * Quaternion(0.25f / s,
                                     (row[2][1] - row[1][2]) * s,
                                     (row[0][2] - row[2][0]) * s,
                                     (row[1][0] - row[0][1]) * s);
        }
        else
        {
            if (row[0][0] > row[1][1] && row[0][0] > row[2][2])
            {
                float s = 2.0f * Math::Sqrt(1.0f + row[0][0] - row[1][1] - row[2][2]);
                return quat * Quaternion((row[2][1] - row[1][2]) / s,
                                         0.25f * s,
                                         (row[0][1] + row[1][0]) / s,
                                         (row[0][2] + row[2][0]) / s);
            }
            else if (row[1][1] > row[2][2])
            {
                float s = 2.0f * Math::Sqrt(1.0f + row[1][1] - row[0][0] - row[2][2]);
                return quat * Quaternion((row[0][2] - row[2][0]) / s,
                                         (row[0][1] + row[1][0]) / s,
                                         0.25f * s,
                                         (row[1][2] + row[2][1]) / s);
            }
            else
            {
                float s = 2.0f * Math::Sqrt(1.0f + row[2][2] - row[0][0] - row[1][1]);
                return quat * Quaternion((row[1][0] - row[0][1]) / s,
                                         (row[0][2] + row[2][0]) / s,
                                         (row[1][2] + row[2][1]) / s,
                                         0.25f * s);
            }
        }
    }

    NO_DISCARD FORCE_INLINE constexpr Float4x4& operator*=(const Float4x4& rhs)
    {
        return *this = *this * rhs;
    }

	NO_DISCARD FORCE_INLINE constexpr Float4x4 Adjugate() const
	{
		return Float4x4
		{
			Float4
			{
				row[1].y * row[2].z * row[3].w + row[3].y * row[1].z * row[2].w + row[2].y * row[3].z * row[1].w - row[1].y * row[3].z * row[2].w
						- row[2].y * row[1].z * row[3].w - row[3].y * row[2].z * row[1].w,
				row[0].y * row[3].z * row[2].w + row[2].y * row[0].z * row[3].w + row[3].y * row[2].z * row[0].w - row[3].y * row[0].z * row[2].w
						- row[2].y * row[3].z * row[0].w - row[0].y * row[2].z * row[3].w,
				row[0].y * row[1].z * row[3].w + row[3].y * row[0].z * row[1].w + row[1].y * row[3].z * row[0].w - row[0].y * row[3].z * row[1].w
						- row[1].y * row[0].z * row[3].w - row[3].y * row[1].z * row[0].w,
				row[0].y * row[2].z * row[1].w + row[1].y * row[0].z * row[2].w + row[2].y * row[1].z * row[0].w - row[0].y * row[1].z * row[2].w
						- row[2].y * row[0].z * row[1].w - row[1].y * row[2].z * row[0].w
			},
			Float4
			{
				row[1].z * row[3].w * row[2].x + row[2].z * row[1].w * row[3].x + row[3].z * row[2].w * row[1].x - row[1].z * row[2].w * row[3].x
						- row[3].z * row[1].w * row[2].x - row[2].z * row[3].w * row[1].x,
				row[0].z * row[2].w * row[3].x + row[3].z * row[0].w * row[2].x + row[2].z * row[3].w * row[0].x - row[0].z * row[3].w * row[2].x
						- row[2].z * row[0].w * row[3].x - row[3].z * row[2].w * row[0].x,
				row[0].z * row[3].w * row[1].x + row[1].z * row[0].w * row[3].x + row[3].z * row[1].w * row[0].x - row[0].z * row[1].w * row[3].x
						- row[3].z * row[0].w * row[1].x - row[1].z * row[3].w * row[0].x,
				row[0].z * row[1].w * row[2].x + row[2].z * row[0].w * row[1].x + row[1].z * row[2].w * row[0].x - row[0].z * row[2].w * row[1].x
						- row[1].z * row[0].w * row[2].x - row[2].z * row[1].w * row[0].x
			},
			Float4
			{
				row[1].w * row[2].x * row[3].y + row[3].w * row[1].x * row[2].y + row[2].w * row[3].x * row[1].y - row[1].w * row[3].x * row[2].y
						- row[2].w * row[1].x * row[3].y - row[3].w * row[2].x * row[1].y,
				row[0].w * row[3].x * row[2].y + row[2].w * row[0].x * row[3].y + row[3].w * row[2].x * row[0].y - row[0].w * row[2].x * row[3].y
						- row[3].w * row[0].x * row[2].y - row[2].w * row[3].x * row[0].y,
				row[0].w * row[1].x * row[3].y + row[3].w * row[0].x * row[1].y + row[1].w * row[3].x * row[0].y - row[0].w * row[3].x * row[1].y
						- row[1].w * row[0].x * row[3].y - row[3].w * row[1].x * row[0].y,
				row[0].w * row[2].x * row[1].y + row[1].w * row[0].x * row[2].y + row[2].w * row[1].x * row[0].y - row[0].w * row[1].x * row[2].y
						- row[2].w * row[0].x * row[1].y - row[1].w * row[2].x * row[0].y
			},
			Float4
			{
				row[1].x * row[3].y * row[2].z + row[2].x * row[1].y * row[3].z + row[3].x * row[2].y * row[1].z - row[1].x * row[2].y * row[3].z
						- row[3].x * row[1].y * row[2].z - row[2].x * row[3].y * row[1].z,
				row[0].x * row[2].y * row[3].z + row[3].x * row[0].y * row[2].z + row[2].x * row[3].y * row[0].z - row[0].x * row[3].y * row[2].z
						- row[2].x * row[0].y * row[3].z - row[3].x * row[2].y * row[0].z,
				row[0].x * row[3].y * row[1].z + row[1].x * row[0].y * row[3].z + row[3].x * row[1].y * row[0].z - row[0].x * row[1].y * row[3].z
						- row[3].x * row[0].y * row[1].z - row[1].x * row[3].y * row[0].z,
				row[0].x * row[1].y * row[2].z + row[2].x * row[0].y * row[1].z + row[1].x * row[2].y * row[0].z - row[0].x * row[2].y * row[1].z
						- row[1].x * row[0].y * row[2].z - row[2].x * row[1].y * row[0].z
			}
		};
	}

	NO_DISCARD FORCE_INLINE constexpr float Determinant() const
	{
		return row[0].x
			* (row[1].y * row[2].z * row[3].w + row[3].y * row[1].z * row[2].w + row[2].y * row[3].z * row[1].w - row[1].y * row[3].z * row[2].w
				- row[2].y * row[1].z * row[3].w - row[3].y * row[2].z * row[1].w)
			+ row[0].y
			* (row[1].z * row[3].w * row[2].x + row[2].z * row[1].w * row[3].x + row[3].z * row[2].w * row[1].x - row[1].z * row[2].w * row[3].x
				- row[3].z * row[1].w * row[2].x - row[2].z * row[3].w * row[1].x)
			+ row[0].z
			* (row[1].w * row[2].x * row[3].y + row[3].w * row[1].x * row[2].y + row[2].w * row[3].x * row[1].y - row[1].w * row[3].x * row[2].y
				- row[2].w * row[1].x * row[3].y - row[3].w * row[2].x * row[1].y)
			+ row[0].w
			* (row[1].x * row[3].y * row[2].z + row[2].x * row[1].y * row[3].z + row[3].x * row[2].y * row[1].z - row[1].x * row[2].y * row[3].z
				- row[3].x * row[1].y * row[2].z - row[2].x * row[3].y * row[1].z);
	}

    NO_DISCARD FORCE_INLINE static constexpr Float4x4 Translate(const Float3& translation)
    {
        return Float4x4
        {
            1.0f,           0.0f,           0.0f,           0.0f,
            0.0f,           1.0f,           0.0f,           0.0f,
            0.0f,           0.0f,           1.0f,           0.0f,
            translation.x,  translation.y,  translation.z,  1.0f
        };
    }

    NO_DISCARD FORCE_INLINE static constexpr Float4x4 Rotate(const Quaternion& quat)
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

        return Float4x4
        {
            1.0f - 2.0f * (qyy + qzz),	2.0f * (qxy + qwz),			2.0f * (qxz - qwy),			0.0f,
            2.0f * (qxy - qwz),			1.0f - 2.0f * (qxx + qzz),	2.0f * (qyz + qwx),			0.0f,
            2.0f * (qxz + qwy),			2.0f * (qyz - qwx),			1.0f - 2.0f * (qxx + qyy),	0.0f,
            0.0f,						0.0f,						0.0f,						1.0f
        };
    }

    NO_DISCARD FORCE_INLINE static constexpr Float4x4 Scale(const Float3& scale)
    {
        return Float4x4
        {
            scale.x,	0.0f,		0.0f,		0.0f,
            0.0f,		scale.y,	0.0f,		0.0f,
            0.0f,		0.0f,		scale.z,	0.0f,
            0.0f,		0.0f,		0.0f,		1.0f
        };
    }

    NO_DISCARD FORCE_INLINE static constexpr Float4x4 TRS(const Float3& translation, const Quaternion& rotation, const Float3& scale)
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

        return Float4x4
        {
            scale.x - 2.0f * scale.x * (qyy + qzz),		2.0f * (qxy + qwz),							2.0f * (qxz - qwy),						0.0f,
            2.0f * (qxy - qwz),							scale.y - 2.0f * scale.y * (qxx + qzz),		2.0f * (qyz + qwx),						0.0f,
            2.0f * (qxz + qwy),							2.0f * (qyz - qwx),							scale.z - 2.0f * scale.z * (qxx + qyy),	0.0f,
            translation.x,								translation.y,								translation.z,							1.0f
        };
    }

    NO_DISCARD FORCE_INLINE static constexpr Float4x4 LookTo(const Float3& eye, const Float3& to, const Float3& up)
    {
        Float3 front = Math::Normalize(to);
        Float3 side = Math::Normalize(Math::Cross(up, front));
        Float3 upV = Math::Cross(front, side);
        
        return Float4x4
        {
            side.x,                     upV.x,                  front.x,                    0.0f,
            side.y,                     upV.y,                  front.y,                    0.0f,
            side.z,                     upV.z,                  front.z,                    0.0f,
            -Math::Dot(side, eye),     -Math::Dot(upV, eye),    -Math::Dot(front, eye),     1.0f
        };
    }

    NO_DISCARD FORCE_INLINE static constexpr Float4x4 LookAt(const Float3& eye, const Float3& at, const Float3& up)
    {
        return LookTo(eye, at - eye, up);
    }

    NO_DISCARD FORCE_INLINE static constexpr Float4x4 Ortho(float width, float height, float zNear, float zFar)
    {
        float fRange = 1.0f / (zFar - zNear);
        return Float4x4
        {
            2.0f / width,   0.0f,           0.0f,               0.0f,
            0.0f,           2.0f / height,  0.0f,               0.0f,
            0.0f,           0.0f,           fRange,             0.0f,
            0.0f,           0.0f,           -zNear * fRange,    1.0f
        };
    }

    NO_DISCARD FORCE_INLINE static constexpr Float4x4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        float width = 1.0f / (right - left);
        float height = 1.0f / (top - bottom);
        float fRange = 1.0f / (zFar - zNear);
        return Float4x4
        {
            width + width,              0.0f,                      0.0f,            0.0f,
            0.0f,                       height + height,           0.0f,            0.0f,
            0.0f,                       0.0f,                      fRange,          0.0f,
            -(left + right) * width,    -(top + bottom) * height,  -zNear * fRange, 1.0f
        };
    }

    NO_DISCARD FORCE_INLINE static constexpr Float4x4 Perspective(float fov, float aspect, float zNear, float zFar)
    {
        float fRange = zFar / (zFar - zNear);
        float height = 1.0f / Math::Tan(Math::Deg2Rad(fov) / 2.0f);
        float width = height / aspect;
        return Float4x4
        {
            width,  0.0f,   0.0f,            0.0f,
            0.0f,   height, 0.0f,            0.0f,
            0.0f,   0.0f,   fRange,          1.0f,
            0.0f,   0.0f,   -zNear * fRange, 0.0f
        };
    }
};

namespace Math {

NO_DISCARD FORCE_INLINE static constexpr Float4x4 Inverse(const Float4x4& m)
{
	Float4x4 adjudate = m.Adjugate();
	float invDet = 1.0f / m.Determinant();
	return Float4x4
	{
		adjudate.row[0] * invDet,
		adjudate.row[1] * invDet,
		adjudate.row[2] * invDet,
		adjudate.row[3] * invDet
	};
}

} // namespace Math

} // namespace Gleam

GLEAM_TYPE(Gleam::Float4x4, Guid("770BABFC-E66A-4CE5-8453-A505EB3016BE"))
	GLEAM_FIELD(row, Serializable())
GLEAM_END