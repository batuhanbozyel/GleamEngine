#pragma once

namespace Gleam {
    
struct Size
{
    float width = 0.0f;
    float height = 0.0f;
    
    static const Size zero;
    static const Size one;
    
    constexpr Size() = default;
    constexpr Size(Size&&) = default;
    constexpr Size(const Size&) = default;
    FORCE_INLINE constexpr Size& operator=(Size&&) = default;
    FORCE_INLINE constexpr Size& operator=(const Size&) = default;
    
    constexpr Size(float width, float height)
        : width(width), height(height)
    {
        
    }
    
    NO_DISCARD FORCE_INLINE constexpr bool operator==(const Size& other) const
    {
        return width == other.width && height == other.height;
    }
    
    NO_DISCARD FORCE_INLINE constexpr bool operator!=(const Size& other) const
    {
        return width != other.width || height != other.height;
    }
    
    NO_DISCARD constexpr operator Vector2() const
    {
        return Vector2{width, height};
    }
    
};
    
} // namespace Gleam
