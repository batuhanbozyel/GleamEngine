#include "gpch.h"
#include "Input.h"
#include "Core/Events/KeyEvent.h"

using namespace Gleam;

void Input::KeyboardEventHandler(SDL_KeyboardEvent keyboardEvent)
{
    KeyCode keycode = static_cast<KeyCode>(SDL_GetKeyFromScancode(keyboardEvent.keysym.scancode));
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

