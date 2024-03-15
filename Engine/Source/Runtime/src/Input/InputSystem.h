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

class InputSystem final : public Subsystem
{
    friend class Application;
    
public:
    
    void ShowCursor() const;
    
    void HideCursor() const;
    
    bool CursorVisible() const;
    
    bool GetButtonDown(const KeyCode keycode) const;
    
    bool GetButtonDown(const MouseButton button) const;
    
    const Vector2& GetMousePosition() const;
    
    const Vector2& GetAxis() const;

private:
    
    void Update();
    
    void KeyboardEventHandler(SDL_KeyboardEvent keyboardEvent) const;

    void MouseMoveEventHandler(SDL_MouseMotionEvent motionEvent) const;

    void MouseScrollEventHandler(SDL_MouseWheelEvent wheelEvent) const;

    void MouseButtonEventHandler(SDL_MouseButtonEvent buttonEvent) const;

    Vector2 mMousePosition = Vector2::zero;
    
    Vector2 mAxis = Vector2::zero;
    
    uint32_t mMouseState = 0;

	const uint8_t* mKeyboardState = nullptr;

	mutable bool mCursorHidden = false;

	mutable Vector2 mCursorHidePosition = Vector2::zero;
    
};


} // namespace Gleam
