#pragma once
#include "Vector3.h"
#include "Common.h"

namespace Gleam {

GSTRUCT(BoundingBox, "AB9094D8-003E-4868-8C9D-20336D882EAD", Serializable)
{
	GFIELD("8D502EDA-7459-40E3-886B-E1F8A7CA6C32", Serializable)
    Float3 min{Math::Infinity, Math::Infinity, Math::Infinity};

	GFIELD("15AC5B77-7DEF-4388-9A8A-3AFF72CC8F61", Serializable)
    Float3 max{Math::NegativeInfinity, Math::NegativeInfinity, Math::NegativeInfinity};
    
    constexpr BoundingBox() = default;
    constexpr BoundingBox(BoundingBox&&) noexcept = default;
    constexpr BoundingBox(const BoundingBox&) = default;
    FORCE_INLINE constexpr BoundingBox& operator=(BoundingBox&&) noexcept = default;
    FORCE_INLINE constexpr BoundingBox& operator=(const BoundingBox&) = default;
    
    constexpr BoundingBox(float min, float max)
        : min(min), max(max)
    {
        
    }
    
    constexpr BoundingBox(const Float2& min, const Float2& max)
        : min(min, 0.0f), max(max, 0.0f)
    {
        
    }
    
    constexpr BoundingBox(const Float3& min, const Float3& max)
        : min(min), max(max)
    {

    }

	NO_DISCARD FORCE_INLINE constexpr Float3 Extent() const
	{
		return max - min;
	}

    NO_DISCARD FORCE_INLINE constexpr bool IsValid() const
    {
        return min.x <= max.x and min.y <= max.y and min.z <= max.z;
    }

    constexpr void Encapsulate(const BoundingBox& other)
    {
        min = Math::Min(min, other.min);
        max = Math::Max(max, other.max);
    }

    FORCE_INLINE constexpr BoundingBox& operator+=(const BoundingBox& other)
    {
        Encapsulate(other);
        return *this;
    }

};

NO_DISCARD FORCE_INLINE constexpr BoundingBox operator+(const BoundingBox& lhs, const BoundingBox& rhs)
{
    BoundingBox result = lhs;
    result += rhs;
    return result;
}

} // namespace Gleam
