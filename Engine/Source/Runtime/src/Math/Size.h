#pragma once
#include "Vector2.h"
#include "Common.h"

namespace Gleam {

GSTRUCT(Size, "6FAF00C4-631C-4F00-832B-6554DCFB3078", Serializable, PrettyName("Size"))
{
	GFIELD("9A2D96E6-31EF-4C2F-8763-788C0A385C26", Serializable, PrettyName("Width"))
    float width = 0.0f;

	GFIELD("A5486918-7D7F-4B50-9250-E202F1DAC0B2", Serializable, PrettyName("Height"))
    float height = 0.0f;
    
    static const Size zero;
    static const Size one;
    
    constexpr Size() = default;
    constexpr Size(Size&&) noexcept = default;
    constexpr Size(const Size&) = default;
    FORCE_INLINE constexpr Size& operator=(Size&&) noexcept = default;
    FORCE_INLINE constexpr Size& operator=(const Size&) = default;

	constexpr Size(float size)
		: width(size), height(size)
	{

	}

    constexpr Size(float width, float height)
        : width(width), height(height)
    {
        
    }

	constexpr Size(const Float2& size)
		: width(size.x), height(size.y)
	{

	}
    
    NO_DISCARD FORCE_INLINE constexpr Size operator*(float value) const
    {
        return Size{ width * value, height * value };
    }
    
    NO_DISCARD FORCE_INLINE constexpr Size operator*(const Size& other) const
    {
        return Size{ width * other.width, height * other.height };
    }
    
    NO_DISCARD FORCE_INLINE constexpr Size operator/(float value) const
    {
        return Size{ width / value, height / value };
    }
    
    NO_DISCARD FORCE_INLINE constexpr Size operator/(const Size& other) const
    {
        return Size{ width / other.width, height / other.height };
    }
    
    NO_DISCARD FORCE_INLINE constexpr Size operator+(float value) const
    {
        return Size{ width + value, height + value };
    }
    
    NO_DISCARD FORCE_INLINE constexpr Size operator+(const Size& other) const
    {
        return Size{ width + other.width, height + other.height };
    }
    
    NO_DISCARD FORCE_INLINE constexpr Size operator-(float value) const
    {
        return Size{ width - value, height - value };
    }
    
    NO_DISCARD FORCE_INLINE constexpr Size operator-(const Size& other) const
    {
        return Size{ width - other.width, height - other.height };
    }
    
    FORCE_INLINE constexpr Size& operator*=(float value)
    {
        width *= value;
        height *= value;
        return *this;
    }
    
    FORCE_INLINE constexpr Size& operator*=(const Size& other)
    {
        width *= other.width;
        height *= other.height;
        return *this;
    }
    
    FORCE_INLINE constexpr Size& operator/=(float value)
    {
        width /= value;
        height /= value;
        return *this;
    }
    
    FORCE_INLINE constexpr Size& operator/=(const Size& other)
    {
        width /= other.width;
        height /= other.height;
        return *this;
    }
    
    FORCE_INLINE constexpr Size& operator+=(float value)
    {
        width += value;
        height += value;
        return *this;
    }
    
    FORCE_INLINE constexpr Size& operator+=(const Size& other)
    {
        width += other.width;
        height += other.height;
        return *this;
    }
    
    FORCE_INLINE constexpr Size& operator-=(float value)
    {
        width -= value;
        height -= value;
        return *this;
    }
    
    FORCE_INLINE constexpr Size& operator-=(const Size& other)
    {
        width -= other.width;
        height -= other.height;
        return *this;
    }
    
    NO_DISCARD FORCE_INLINE constexpr bool operator==(const Size& other) const
    {
        return (Math::Abs(width - other.width) < Math::Epsilon) && (Math::Abs(height - other.height) < Math::Epsilon);
    }
    
    NO_DISCARD FORCE_INLINE constexpr bool operator!=(const Size& other) const
    {
        return (Math::Abs(width - other.width) >= Math::Epsilon) || (Math::Abs(height - other.height) >= Math::Epsilon);
    }
    
    NO_DISCARD FORCE_INLINE constexpr operator Float2() const
    {
        return Float2{width, height};
    }
    
};
    
} // namespace Gleam
