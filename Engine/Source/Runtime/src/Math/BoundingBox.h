#pragma once

namespace Gleam {

struct BoundingBox
{
    Float3 min{Math::NegativeInfinity, Math::NegativeInfinity, Math::NegativeInfinity};
    Float3 max{Math::Infinity, Math::Infinity, Math::Infinity};
    
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
    
};

} // namespace Gleam
