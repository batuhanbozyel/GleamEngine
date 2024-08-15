#include "gpch.h"
#include "InputSystem.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/Application.h"
#include "Core/WindowSystem.h"
#include "Core/Events/KeyEvent.h"
#include <SDL3/SDL.h>

using namespace Gleam;

void InputSystem::Update()
{
    mKeyboardState = SDL_GetKeyboardState(nullptr);
    mMouseState = SDL_GetMouseState(&mMousePosition.x, &mMousePosition.y);
    SDL_GetRelativeMouseState(&mAxis.x, &mAxis.y);
}

void InputSystem::ShowCursor() const
{
	static auto windowSystem = Globals::Engine->GetSubsystem<WindowSystem>();
	SDL_WarpMouseInWindow(windowSystem->GetSDLWindow(), mCursorHidePosition.x, mCursorHidePosition.y);
	SDL_SetWindowRelativeMouseMode(windowSystem->GetSDLWindow(), SDL_FALSE);
    SDL_ShowCursor();
	mCursorHidden = false;
}

void InputSystem::HideCursor() const
{
	static auto windowSystem = Globals::Engine->GetSubsystem<WindowSystem>();
    SDL_HideCursor();
	SDL_SetWindowRelativeMouseMode(windowSystem->GetSDLWindow(), SDL_TRUE);
	SDL_GetMouseState(&mCursorHidePosition.x, &mCursorHidePosition.y);
	mCursorHidden = true;
}

bool InputSystem::CursorVisible() const
{
	return SDL_CursorVisible();
}

bool InputSystem::GetButtonDown(const KeyCode keycode) const
{
	SDL_Keymod modifier = SDL_KMOD_NONE;
	auto scancode = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(keycode), &modifier);
	return mKeyboardState[scancode] == SDL_PRESSED;
}

bool InputSystem::GetButtonDown(const MouseButton button) const
{
    return mMouseState & SDL_BUTTON(static_cast<uint8_t>(button));
}

const Float2& InputSystem::GetMousePosition() const
{
    return mMousePosition;
}

const Float2& InputSystem::GetAxis() const
{
    return mAxis;
}

void InputSystem::KeyboardEventHandler(SDL_KeyboardEvent keyboardEvent) const
{
    KeyCode keycode = static_cast<KeyCode>(keyboardEvent.key);
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

