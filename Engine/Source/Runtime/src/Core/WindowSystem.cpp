#include "gpch.h"
#include "WindowSystem.h"

#include "Events/WindowEvent.h"

using namespace Gleam;

void WindowSystem::Initialize()
{
    int initSucess = SDL_Init(SDL_INIT_EVERYTHING);
    GLEAM_ASSERT(initSucess == 0, "Window subsystem initialization failed!");
}

void WindowSystem::Shutdown()
{
    if (mWindow) { SDL_DestroyWindow(mWindow); }
    SDL_Quit();
}

void WindowSystem::CreateWindow(const WindowProperties& props)
{
    // destroy old window if exists
    if (mWindow) { SDL_DestroyWindow(mWindow); }
    
    mProperties = props;
    
	// query display info to create window if not provided by the user
	if (props.display.width == 0 || props.display.height == 0)
	{
		mProperties.windowFlag = WindowFlag::MaximizedWindow;
	}

	// create window
	mWindow = SDL_CreateWindow(mProperties.title.c_str(),
                               mProperties.display.width, mProperties.display.height,
                               static_cast<uint32_t>(mProperties.windowFlag));
	GLEAM_ASSERT(mWindow, "Window creation failed!");

	// update window props with the created window info
	int monitor = SDL_GetDisplayForWindow(mWindow);
	GLEAM_ASSERT(monitor >= 0, "Window display index is invalid!");

	mProperties.display = WindowConfig::GetCurrentDisplayMode(monitor);

	EventDispatcher<WindowResizeEvent>::Subscribe([this](const WindowResizeEvent& e)
	{
		mProperties.display.width = e.GetWidth();
		mProperties.display.height = e.GetHeight();
	});
}

void WindowSystem::EventHandler(SDL_WindowEvent windowEvent)
{
	switch (windowEvent.type)
	{
		case SDL_EVENT_WINDOW_MOVED:
		{
			EventDispatcher<WindowMovedEvent>::Publish(WindowMovedEvent(SDL_GetWindowFromID(windowEvent.windowID), windowEvent.data1, windowEvent.data2));
			break;
		}
		case SDL_EVENT_WINDOW_RESIZED:
		{
			EventDispatcher<WindowResizeEvent>::Publish(WindowResizeEvent(SDL_GetWindowFromID(windowEvent.windowID), windowEvent.data1, windowEvent.data2));
			break;
		}
		case SDL_EVENT_WINDOW_MINIMIZED:
		{
			EventDispatcher<WindowMinimizeEvent>::Publish(WindowMinimizeEvent(SDL_GetWindowFromID(windowEvent.windowID)));
			break;
		}
		case SDL_EVENT_WINDOW_MAXIMIZED:
		{
			EventDispatcher<WindowMaximizeEvent>::Publish(WindowMaximizeEvent(SDL_GetWindowFromID(windowEvent.windowID)));
			break;
		}
		case SDL_EVENT_WINDOW_RESTORED:
		{
			EventDispatcher<WindowRestoreEvent>::Publish(WindowRestoreEvent(SDL_GetWindowFromID(windowEvent.windowID)));
			break;
		}
		case SDL_EVENT_WINDOW_MOUSE_ENTER:
		{
			EventDispatcher<WindowMouseEnterEvent>::Publish(WindowMouseEnterEvent(SDL_GetWindowFromID(windowEvent.windowID)));
			break;
		}
		case SDL_EVENT_WINDOW_MOUSE_LEAVE:
		{
			EventDispatcher<WindowMouseLeaveEvent>::Publish(WindowMouseLeaveEvent(SDL_GetWindowFromID(windowEvent.windowID)));
			break;
		}
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
		{
			EventDispatcher<WindowFocusEvent>::Publish(WindowFocusEvent(SDL_GetWindowFromID(windowEvent.windowID)));
			break;
		}
		case SDL_EVENT_WINDOW_FOCUS_LOST:
		{
			EventDispatcher<WindowLostFocusEvent>::Publish(WindowLostFocusEvent(SDL_GetWindowFromID(windowEvent.windowID)));
			break;
		}
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
		{
			EventDispatcher<WindowCloseEvent>::Publish(WindowCloseEvent(SDL_GetWindowFromID(windowEvent.windowID)));
			break;
		}
	}
}
