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
		TArray<float, 4> value{};
	};

    static const Quaternion identity;
    
	NO_DISCARD FORCE_INLINE constexpr Float3 EulerAngles() const
	{
		return Float3
		{
			Math::Atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y)),
			Math::Asin(Math::Clamp(2.0f * (w * y - z * x), -1.0f, 1.0f)),
			Math::Atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z))
		};
	}

	NO_DISCARD FORCE_INLINE constexpr Quaternion Conjugate() const
	{
		return Quaternion{ w, -x, -y, -z };
	}

	constexpr Quaternion() = default;
	constexpr Quaternion(Quaternion&&) noexcept = default;
	constexpr Quaternion(const Quaternion&) = default;
    FORCE_INLINE constexpr Quaternion& operator=(Quaternion&&) noexcept = default;
    FORCE_INLINE constexpr Quaternion& operator=(const Quaternion&) = default;

	constexpr explicit Quaternion(float w, float x, float y, float z)
		: w(w), x(x), y(y), z(z)
	{

	}
	constexpr explicit Quaternion(const TArray<float, 4>& quat)
		: w(quat[0]), x(quat[1]), y(quat[2]), z(quat[3])
	{

	}
	constexpr explicit Quaternion(float eularAngle)
	{
		float c = Math::Cos(eularAngle * 0.5f);
		float s = Math::Sin(eularAngle * 0.5f);

		float cc = c * c;
		float ss = s * s;
		float ssc = ss * c;
		float ccs = cc * s;

		w = cc * c + ss * s;
		x = ccs - ssc;
		y = ccs + ssc;
		z = x;
	}
	constexpr explicit Quaternion(const Float3& eularAngles)
	{
		Float3 c = Math::Cos(eularAngles * 0.5f);
		Float3 s = Math::Sin(eularAngles * 0.5f);
        
		w = c.x * c.y * c.z + s.x * s.y * s.z;
		x = s.x * c.y * c.z - c.x * s.y * s.z;
		y = c.x * s.y * c.z + s.x * c.y * s.z;
		z = c.x * c.y * s.z - s.x * s.y * c.z;
	}
    constexpr explicit Quaternion(float pitch, float yaw, float roll)
        : Quaternion(Float3{pitch, yaw, roll})
    {
        
    }
    
    NO_DISCARD FORCE_INLINE constexpr float& operator[](size_t i)
    {
        return value[i];
    }
    
    NO_DISCARD FORCE_INLINE constexpr const float& operator[](size_t i) const
	{
		return value[i];
	}
    
    NO_DISCARD FORCE_INLINE constexpr Quaternion operator*(const Quaternion& rhs) const
    {
		// q = [w, v]
		// w = [w1 * w2 - dot(w1, w2)]
		// v = [w2 * v1 + w1 * v2 + v1 x v2]

		return Quaternion
		{
			w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
			w * rhs.x + rhs.w * x + y * rhs.z - rhs.y * z,
			w * rhs.y + rhs.w * y + z * rhs.x - rhs.z * x,
			w * rhs.z + rhs.w * z + x * rhs.y - rhs.x * y
		};
    }
    
    FORCE_INLINE constexpr Quaternion& operator*=(const Quaternion& rhs)
    {
        return *this = *this * rhs;
    }
    
    NO_DISCARD FORCE_INLINE constexpr Quaternion operator/(float s) const
    {
        return Quaternion{ w / s, x / s, y / s, z / s };
    }
    
    FORCE_INLINE constexpr Quaternion& operator/=(float s)
    {
        w /= s;
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }
    
};

NO_DISCARD FORCE_INLINE constexpr Float3 operator*(const Quaternion& quat, const Float3& vec)
{
	Float3 quatVec{ quat.x, quat.y, quat.z };
	Float3 uv = Math::Cross(quatVec, vec);
	Float3 uuv = Math::Cross(quatVec, uv);
	return vec + ((uv * quat.w) + uuv) * 2.0f;
}

namespace Math {
    
NO_DISCARD FORCE_INLINE constexpr float Dot(const Quaternion& q1, const Quaternion& q2)
{
    return q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;
}
    
NO_DISCARD FORCE_INLINE constexpr Quaternion Inverse(const Quaternion& q)
{
    return q.Conjugate() / Dot(q, q);
}

NO_DISCARD FORCE_INLINE constexpr float LengthSquared(const Quaternion& q)
{
	return Dot(q, q);
}

NO_DISCARD FORCE_INLINE constexpr float Length(const Quaternion& q)
{
	return Sqrt(LengthSquared(q));
}

NO_DISCARD FORCE_INLINE constexpr Quaternion Normalize(const Quaternion& q)
{
	float invLength = 1.0f / Length(q);
	return Quaternion
	{
		q.w * invLength,
		q.x * invLength,
		q.y * invLength,
		q.z * invLength
	};
}
    
} // namespace Math

} // namespace Gleam

GLEAM_TYPE(Gleam::Quaternion, Guid("69ACEBFE-7CFD-4876-9D4D-DF428E49A626"))
    GLEAM_FIELD(w, Serializable())
    GLEAM_FIELD(x, Serializable())
    GLEAM_FIELD(y, Serializable())
    GLEAM_FIELD(z, Serializable())
GLEAM_END
