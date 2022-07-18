#pragma once
#include <SDL.h>

namespace Gleam {

class Input
{
public:

	static void KeyboardEventHandler(SDL_KeyboardEvent keyboardEvent);

	static void MouseMotionEventHandler(SDL_MouseMotionEvent motionEvent);

	static void MouseWheelEventHandler(SDL_MouseWheelEvent wheelEvent);

	static void MouseButtonEventHandler(SDL_MouseButtonEvent buttonEvent);

private:



};


} // namespace Gleam