#pragma once
#include "KeyCode.h"
#include "MouseButton.h"
#include "JoystickButton.h"
#include <SDL_events.h>

namespace Gleam {

class Input
{
public:

	static bool IsKeyPressed(const KeyCode keycode);

	static void KeyboardEventHandler(SDL_KeyboardEvent keyboardEvent);

	static void MouseMotionEventHandler(SDL_MouseMotionEvent motionEvent);

	static void MouseWheelEventHandler(SDL_MouseWheelEvent wheelEvent);

	static void MouseButtonEventHandler(SDL_MouseButtonEvent buttonEvent);

private:


};


} // namespace Gleam