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

	constexpr Quaternion(float w, float x, float y, float z)
		: w(w), x(x), y(y), z(z)
	{

	}
	constexpr Quaternion(const TArray<float, 4>& quat)
		: w(quat[0]), x(quat[1]), y(quat[2]), z(quat[3])
	{

	}
	constexpr Quaternion(float eularAngle)
	{
		float c = Math::Cos(eularAngle * 0.5f);
		float s = Math::Sin(eularAngle * 0.5f);

		float cc = c * c;
		float ss = s * s;
		float ssc = ss * c;
		float ccs = cc * s;

		w = cc * c + ss * s;
		x = ccs + ssc;
		y = ccs - ssc;
		z = y;
	}
	constexpr Quaternion(const Float3& eularAngles)
	{
		Float3 c = Math::Cos(eularAngles * 0.5f);
		Float3 s = Math::Sin(eularAngles * 0.5f);
        
		w = c.x * c.y * c.z + s.x * s.y * s.z;
		x = s.x * c.y * c.z + c.x * s.y * s.z;
		y = c.x * s.y * c.z - s.x * c.y * s.z;
		z = c.x * c.y * s.z - s.x * s.y * c.z;
	}
    constexpr Quaternion(float pitch, float yaw, float roll)
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
        return Quaternion
        {
            rhs.w * w - rhs.x * x - rhs.y * y - rhs.z * z,
            rhs.w * x + rhs.x * w + rhs.y * z - rhs.z * y,
            rhs.w * y - rhs.x * z + rhs.y * w + rhs.z * x,
            rhs.w * z + rhs.x * y - rhs.y * x + rhs.z * w
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
    Quaternion v{0.0f, vec.x, vec.y, vec.z};
    auto result = quat.Conjugate() * v * quat;
    return Float3{result.x, result.y, result.z};
}

namespace Math {
    
NO_DISCARD FORCE_INLINE constexpr float Dot(const Quaternion& q1, const Quaternion& q2)
{
    return q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;
}
    
NO_DISCARD FORCE_INLINE constexpr Quaternion Inverse(const Quaternion& q)
{
    return q.Conjugate() / Math::Max(Dot(q, q), Math::Epsilon);
}
    
} // namespace Math

} // namespace Gleam

GLEAM_TYPE(Gleam::Quaternion, Guid("69ACEBFE-7CFD-4876-9D4D-DF428E49A626"))
    GLEAM_FIELD(w, Serializable())
    GLEAM_FIELD(x, Serializable())
    GLEAM_FIELD(y, Serializable())
    GLEAM_FIELD(z, Serializable())
GLEAM_END
