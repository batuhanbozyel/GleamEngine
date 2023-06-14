#pragma once

namespace Gleam {

struct BoundingBox
{
    Vector3 min{Math::NegativeInfinity, Math::NegativeInfinity, Math::NegativeInfinity};
    Vector3 max{Math::Infinity, Math::Infinity, Math::Infinity};
    
    constexpr BoundingBox() = default;
    constexpr BoundingBox(BoundingBox&&) = default;
    constexpr BoundingBox(const BoundingBox&) = default;
    FORCE_INLINE constexpr BoundingBox& operator=(BoundingBox&&) = default;
    FORCE_INLINE constexpr BoundingBox& operator=(const BoundingBox&) = default;
    
    constexpr BoundingBox(float min, float max)
        : min(min), max(max)
    {
        
    }
    
    constexpr BoundingBox(const Vector2& min, const Vector2& max)
        : min(min, 0.0f), max(max, 0.0f)
    {
        
    }
    
    constexpr BoundingBox(const Vector3& min, const Vector3& max)
        : min(min), max(max)
    {
        
    }
    
};

} // namespace Gleam
