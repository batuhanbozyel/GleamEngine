#include "gpch.h"
#include "Input.h"
#include "Core/Events/KeyEvent.h"
#include <SDL3/SDL.h>

using namespace Gleam;

void Input::ShowCursor()
{
    SDL_ShowCursor();
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

void Input::HideCursor()
{
    SDL_HideCursor();
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

bool Input::CursorVisible()
{
	return SDL_CursorVisible();
}

bool Input::GetButtonDown(const KeyCode keycode)
{
	auto scancode = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(keycode));
	return mKeyboardState[scancode] == SDL_PRESSED;
}

bool Input::GetButtonDown(const MouseButton button)
{
    return mMouseState & SDL_BUTTON(static_cast<uint8_t>(button));
}

const Vector2& Input::GetMousePosition()
{
    return mMousePosition;
}

const Vector2& Input::GetAxis()
{
    return mAxis;
}

float Input::GetMouseX()
{
    return mMousePosition.x;
}

float Input::GetMouseY()
{
    return mMousePosition.y;
}

float Input::GetAxisX()
{
    return mAxis.x;
}

float Input::GetAxisY()
{
    return mAxis.y;
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
    mMouseState = SDL_GetMouseState(&mMousePosition.x, &mMousePosition.y);
    mRelativeMouseState = SDL_GetRelativeMouseState(&mAxis.x, &mAxis.y);
    mKeyboardState = SDL_GetKeyboardState(nullptr);
}

void Input::MouseMoveEventHandler(SDL_MouseMotionEvent motionEvent)
{
    EventDispatcher<MouseMovedEvent>::Publish(MouseMovedEvent(static_cast<float>(motionEvent.x), static_cast<float>(motionEvent.y)));
}

void Input::MouseScrollEventHandler(SDL_MouseWheelEvent wheelEvent)
{
    EventDispatcher<MouseScrolledEvent>::Publish(MouseScrolledEvent(wheelEvent.x, wheelEvent.y));
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

