#pragma once
#include <SDL.h>

namespace Gleam {

enum class MouseButton
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

} // namespace