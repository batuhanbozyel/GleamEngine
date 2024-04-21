#pragma once
#include "Core/Subsystem.h"
#include "KeyCode.h"
#include "MouseButton.h"
#include "JoystickButton.h"

struct SDL_Mouse;
struct SDL_Keyboard;
struct SDL_KeyboardEvent;
struct SDL_MouseMotionEvent;
struct SDL_MouseWheelEvent;
struct SDL_MouseButtonEvent;

namespace Gleam {

class EventSystem;

class InputSystem final : public Subsystem
{
	friend class Application;
    friend class EventSystem;
public:
    
    void ShowCursor() const;
    
    void HideCursor() const;
    
    bool CursorVisible() const;
    
    bool GetButtonDown(const KeyCode keycode) const;
    
    bool GetButtonDown(const MouseButton button) const;
    
    const Float2& GetMousePosition() const;
    
    const Float2& GetAxis() const;

private:
    
    void Update();
    
    void KeyboardEventHandler(SDL_KeyboardEvent keyboardEvent) const;

    void MouseMoveEventHandler(SDL_MouseMotionEvent motionEvent) const;

    void MouseScrollEventHandler(SDL_MouseWheelEvent wheelEvent) const;

    void MouseButtonEventHandler(SDL_MouseButtonEvent buttonEvent) const;

    Float2 mMousePosition = Float2::zero;
    
    Float2 mAxis = Float2::zero;
    
    uint32_t mMouseState = 0;

	const uint8_t* mKeyboardState = nullptr;

	mutable bool mCursorHidden = false;

	mutable Float2 mCursorHidePosition = Float2::zero;
    
};


} // namespace Gleam
