#pragma once
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

class Input final
{
    friend class Application;
    
public:

	static bool IsKeyPressed(const KeyCode keycode);
    
    static bool IsMouseButtonPressed(const MouseButton button);
    
    static Vector2 GetMousePosition();

private:
    
    static void Update();
    
    static void KeyboardEventHandler(SDL_KeyboardEvent keyboardEvent);

    static void MouseMoveEventHandler(SDL_MouseMotionEvent motionEvent);

    static void MouseScrollEventHandler(SDL_MouseWheelEvent wheelEvent);

    static void MouseButtonEventHandler(SDL_MouseButtonEvent buttonEvent);
    
    static inline Vector2 mMousePosition = Vector2::zero;
    
    static inline uint32_t mMouseState = 0;
    
    static inline const uint8_t* mKeyboardState = nullptr;
    
};


} // namespace Gleam
