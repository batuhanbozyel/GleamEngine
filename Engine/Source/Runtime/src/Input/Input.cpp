#include "gpch.h"
#include "Input.h"
#include "Core/Events/KeyEvent.h"

using namespace Gleam;

bool Input::IsKeyPressed(const KeyCode keycode)
{
	auto scancode = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(keycode));
	auto keyboardState = SDL_GetKeyboardState(nullptr);
	return keyboardState[scancode] == SDL_PRESSED;
}

void Input::KeyboardEventHandler(SDL_KeyboardEvent keyboardEvent)
{
    KeyCode keycode = static_cast<KeyCode>(keyboardEvent.keysym.sym);
    if (keyboardEvent.state == SDL_PRESSED)
    {
        EventDispatcher<KeyPressedEvent>::Publish(KeyPressedEvent(keycode, keyboardEvent.repeat));
    }
    else
    {
        EventDispatcher<KeyReleasedEvent>::Publish(KeyReleasedEvent(keycode));
    }
}

void Input::MouseMotionEventHandler(SDL_MouseMotionEvent motionEvent)
{
    
}

void Input::MouseWheelEventHandler(SDL_MouseWheelEvent wheelEvent)
{
    
}

void Input::MouseButtonEventHandler(SDL_MouseButtonEvent buttonEvent)
{
    
}

