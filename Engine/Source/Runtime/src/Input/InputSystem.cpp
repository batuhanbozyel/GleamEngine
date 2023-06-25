#include "gpch.h"
#include "InputSystem.h"
#include "Core/Events/KeyEvent.h"
#include <SDL3/SDL.h>

using namespace Gleam;

void InputSystem::Update()
{
    mMouseState = SDL_GetMouseState(&mMousePosition.x, &mMousePosition.y);
    mRelativeMouseState = SDL_GetRelativeMouseState(&mAxis.x, &mAxis.y);
    mKeyboardState = SDL_GetKeyboardState(nullptr);
}

void InputSystem::ShowCursor() const
{
    SDL_ShowCursor();
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

void InputSystem::HideCursor() const
{
    SDL_HideCursor();
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

bool InputSystem::CursorVisible() const
{
	return SDL_CursorVisible();
}

bool InputSystem::GetButtonDown(const KeyCode keycode) const
{
	auto scancode = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(keycode));
	return mKeyboardState[scancode] == SDL_PRESSED;
}

bool InputSystem::GetButtonDown(const MouseButton button) const
{
    return mMouseState & SDL_BUTTON(static_cast<uint8_t>(button));
}

const Vector2& InputSystem::GetMousePosition() const
{
    return mMousePosition;
}

const Vector2& InputSystem::GetAxis() const
{
    return mAxis;
}

void InputSystem::KeyboardEventHandler(SDL_KeyboardEvent keyboardEvent) const
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

void InputSystem::MouseMoveEventHandler(SDL_MouseMotionEvent motionEvent) const
{
    EventDispatcher<MouseMovedEvent>::Publish(MouseMovedEvent(static_cast<float>(motionEvent.x), static_cast<float>(motionEvent.y)));
}

void InputSystem::MouseScrollEventHandler(SDL_MouseWheelEvent wheelEvent) const
{
    EventDispatcher<MouseScrolledEvent>::Publish(MouseScrolledEvent(wheelEvent.x, wheelEvent.y));
}

void InputSystem::MouseButtonEventHandler(SDL_MouseButtonEvent buttonEvent) const
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

