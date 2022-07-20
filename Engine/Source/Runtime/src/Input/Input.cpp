#include "gpch.h"
#include "Input.h"
#include "Core/Events/KeyEvent.h"
#include <SDL.h>

using namespace Gleam;

bool Input::IsKeyPressed(const KeyCode keycode)
{
	auto scancode = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(keycode));
	return mKeyboardState[scancode] == SDL_PRESSED;
}

bool Input::IsMouseButtonPressed(const MouseButton button)
{
    return mMouseState & static_cast<uint8_t>(button);
}

Vector2 Input::GetMousePosition()
{
    return mMousePosition;
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

void Input::Update()
{
    int x, y;
    mMouseState = SDL_GetMouseState(&x, &y);
    mMousePosition.x = static_cast<float>(x);
    mMousePosition.y = static_cast<float>(y);
    mKeyboardState = SDL_GetKeyboardState(nullptr);
}

void Input::MouseMoveEventHandler(SDL_MouseMotionEvent motionEvent)
{
    EventDispatcher<MouseMovedEvent>::Publish(MouseMovedEvent(motionEvent.x, motionEvent.y));
}

void Input::MouseScrollEventHandler(SDL_MouseWheelEvent wheelEvent)
{
    EventDispatcher<MouseScrolledEvent>::Publish(MouseScrolledEvent(wheelEvent.preciseX, wheelEvent.preciseY));
}

void Input::MouseButtonEventHandler(SDL_MouseButtonEvent buttonEvent)
{
    MouseButton button = static_cast<MouseButton>(buttonEvent.button);
    if (buttonEvent.state == SDL_PRESSED)
    {
        EventDispatcher<MouseButtonPressedEvent>::Publish(MouseButtonPressedEvent(button));
    }
    else
    {
        EventDispatcher<MouseButtonReleasedEvent>::Publish(MouseButtonReleasedEvent(button));
    }
}

