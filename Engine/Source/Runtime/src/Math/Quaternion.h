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

    static const Quaternion identity;
    
	NO_DISCARD FORCE_INLINE Vector3 EularAngles() const
	{
		return Vector3
		{
			Math::Atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y)),
			Math::Asin(Math::Clamp(2.0f * (w * y - z * x), -1.0f, 1.0f)),
			Math::Atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z))
		};
	}

	Quaternion() = default;
	constexpr Quaternion(Quaternion&&) = default;
	constexpr Quaternion(const Quaternion&) = default;
    FORCE_INLINE constexpr Quaternion& operator=(Quaternion&&) = default;
    FORCE_INLINE constexpr Quaternion& operator=(const Quaternion&) = default;

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
            w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
            x * rhs.w + w * rhs.x + y * rhs.z - z * rhs.y,
            w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
            w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w
        };
    }
    
    FORCE_INLINE constexpr Quaternion& operator*=(const Quaternion& rhs)
    {
        return *this = *this * rhs;
    }
    
    Vector3 operator*(const Vector3& rhs) const
    {
        Vector3 qVec(x, y, z);
        Vector3 cross1(Math::Cross(qVec, rhs));
        Vector3 cross2(Math::Cross(qVec, cross1));

        return rhs + 2.0f * (cross1 * w + cross2);
    }
    
};

} // namespace Gleam
