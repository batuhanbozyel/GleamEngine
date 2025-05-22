#pragma once
#include "Core/Macro.h"
#include "Reflection/Macro.h"

namespace Gleam {
    
GSTRUCT(Rect, "CB0D2255-72B8-433A-A169-AE80581A0478", Serializable)
{
	GFIELD("BD8E08CD-752B-4BA2-83F7-83B15CEBFAAD", Serializable)
    Size size{1.0f, 1.0f};

	GFIELD("9CC392D6-8B00-4026-8DB7-0EF3456DD66C", Serializable)
    Float2 offset{0.0f, 0.0f};
    
    constexpr Rect() = default;
    constexpr Rect(Rect&&) noexcept = default;
    constexpr Rect(const Rect&) = default;
    FORCE_INLINE constexpr Rect& operator=(Rect&&) noexcept = default;
    FORCE_INLINE constexpr Rect& operator=(const Rect&) = default;
    
    constexpr Rect(float width, float height)
        : size(width, height)
    {
        
    }
    
    constexpr Rect(const Size& size, const Float2& offset)
        : size(size), offset(offset)
    {
        
    }
    
};
    
} // namespace Gleam
