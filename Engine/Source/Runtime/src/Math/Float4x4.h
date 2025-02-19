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

	constexpr Float4x4(float mat[16])
		: m{mat[0], mat[1], mat[2], mat[3],
			mat[4], mat[5], mat[6], mat[7],
			mat[8], mat[9], mat[10], mat[11],
			mat[12], mat[13], mat[14], mat[15]}
	{

	}

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
        return Float4x4{ *this * rhs.row[0], *this * rhs.row[1], *this * rhs.row[2], *this * rhs.row[3] };
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

    NO_DISCARD FORCE_INLINE static constexpr Float4x4 TRS(const Float3& translation, const Quaternion& rotation, float scale)
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
			scale - 2.0f * scale * (qyy + qzz),		2.0f * scale * (qxy + qwz),				2.0f * scale * (qxz - qwy),			0.0f,
			2.0f * scale * (qxy - qwz),				scale - 2.0f * scale * (qxx + qzz),		2.0f * scale * (qyz + qwx),			0.0f,
			2.0f * scale * (qxz + qwy),				2.0f * scale * (qyz - qwx),				scale - 2.0f * scale * (qxx + qyy),	0.0f,
			translation.x,							translation.y,							translation.z,						1.0f
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

FORCE_INLINE static void Decompose(const Float4x4& transform, Float3& translation, Quaternion& rotation, float& scale)
{
	translation = Float3(transform.m[12], transform.m[13], transform.m[14]);

	Float3 xAxis(transform.m[0], transform.m[1], transform.m[2]);
	Float3 yAxis(transform.m[4], transform.m[5], transform.m[6]);
	Float3 zAxis(transform.m[8], transform.m[9], transform.m[10]);

	scale = (Length(xAxis) + Length(yAxis) + Length(zAxis)) / 3.0f;
	GLEAM_ASSERT(scale > Epsilon);

	Float3x3 rotMatrix;
	rotMatrix[0] = xAxis / scale;
	rotMatrix[1] = yAxis / scale;
	rotMatrix[2] = zAxis / scale;

	float trace = rotMatrix[0][0] + rotMatrix[1][1] + rotMatrix[2][2];
	if (trace > 0.0f)
	{
		float s = Math::Sqrt(trace + 1.0f);
		float w = s * 0.5f;
		s = 0.5f / s;
		rotation = Quaternion(
			w,
			(rotMatrix[2][1] - rotMatrix[1][2]) * s,
			(rotMatrix[0][2] - rotMatrix[2][0]) * s,
			(rotMatrix[1][0] - rotMatrix[0][1]) * s
		);
	}
	else
	{
		// Find the largest diagonal element to avoid division by zero
		int i = 0;
		if (rotMatrix[1][1] > rotMatrix[0][0]) i = 1;
		if (rotMatrix[2][2] > rotMatrix[i][i]) i = 2;

		const int next[3] = { 1, 2, 0 };
		int j = next[i];
		int k = next[j];

		float s = Math::Sqrt(rotMatrix[i][i] - rotMatrix[j][j] - rotMatrix[k][k] + 1.0f);
		float q[4];
		q[i + 1] = s * 0.5f;

		if (s != 0.0f) s = 0.5f / s;

		q[0] = (rotMatrix[k][j] - rotMatrix[j][k]) * s;
		q[j + 1] = (rotMatrix[j][i] + rotMatrix[i][j]) * s;
		q[k + 1] = (rotMatrix[k][i] + rotMatrix[i][k]) * s;

		rotation = Quaternion(q[0], q[1], q[2], q[3]);
	}
	rotation = Normalize(rotation);
}

} // namespace Math

} // namespace Gleam

GLEAM_TYPE(Gleam::Float4x4, Guid("770BABFC-E66A-4CE5-8453-A505EB3016BE"))
	GLEAM_FIELD(row, Serializable())
GLEAM_END
