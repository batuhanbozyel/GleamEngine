#pragma once

namespace Gleam {
    
struct Rect
{
    Size size{1.0f, 1.0f};
    Vector2 offset{0.0f, 0.0f};
    
    constexpr Rect() = default;
    constexpr Rect(Rect&&) noexcept = default;
    constexpr Rect(const Rect&) = default;
    FORCE_INLINE constexpr Rect& operator=(Rect&&) noexcept = default;
    FORCE_INLINE constexpr Rect& operator=(const Rect&) = default;
    
    constexpr Rect(float width, float height)
        : size(width, height)
    {
        
    }
    
    constexpr Rect(const Size& size, const Vector2& offset)
        : size(size), offset(offset)
    {
        
    }
    
};
    
} // namespace Gleam
