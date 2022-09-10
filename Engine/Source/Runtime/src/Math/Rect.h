#pragma once

namespace Gleam {
    
struct Rect
{
    Vector2 min{Math::NegativeInfinity, Math::NegativeInfinity};
    Vector2 max{Math::Infinity, Math::Infinity};
    
    constexpr Rect() = default;
    constexpr Rect(Rect&&) = default;
    constexpr Rect(const Rect&) = default;
    FORCE_INLINE constexpr Rect& operator=(Rect&&) = default;
    FORCE_INLINE constexpr Rect& operator=(const Rect&) = default;
    
    constexpr Rect(float min, float max)
        : min(min), max(max)
    {
        
    }
    
    constexpr Rect(const Vector2& min, const Vector2& max)
        : min(min), max(max)
    {
        
    }
    
};
    
} // namespace Gleam
