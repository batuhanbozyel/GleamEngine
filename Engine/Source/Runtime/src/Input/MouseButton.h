#pragma once
#include <SDL3/SDL.h>

namespace Gleam {

enum class MouseButton : uint8_t
{
	M1 = SDL_BUTTON_LEFT,
	M2 = SDL_BUTTON_RIGHT,
	M3 = SDL_BUTTON_MIDDLE,
	M4 = SDL_BUTTON_X1,
	M5 = SDL_BUTTON_X2,

	Left = M1,
	Right = M2,
	Middle = M3,
	Backward = M4,
	Forward = M5
};

static TStringView ToString(const MouseButton button)
{
    switch (button)
    {
        case MouseButton::M1: return "M1";
        case MouseButton::M2: return "M2";
        case MouseButton::M3: return "M3";
        case MouseButton::M4: return "M4";
        case MouseButton::M5: return "M5";
        default: return "";
    }
}

} // namespace
