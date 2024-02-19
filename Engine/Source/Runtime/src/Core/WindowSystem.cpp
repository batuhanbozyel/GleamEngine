#include "gpch.h"
#include "WindowSystem.h"

#include "Events/WindowEvent.h"

using namespace Gleam;

void WindowSystem::Initialize()
{
    int initSucess = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS | SDL_INIT_SENSOR);
    GLEAM_ASSERT(initSucess == 0, "Window subsystem initialization failed!");
}

void WindowSystem::Shutdown()
{
    if (mWindow) { SDL_DestroyWindow(mWindow); }
    SDL_Quit();
}

void WindowSystem::Configure(const WindowConfig& config)
{
    // destroy old window if exists
    if (mWindow) { SDL_DestroyWindow(mWindow); }
    
    mConfig = config;
    
	// query display info to create window if not provided by the user
	if (static_cast<uint32_t>(config.size.width) == 0 || static_cast<uint32_t>(config.size.height) == 0)
	{
		mConfig.windowFlag = WindowFlag::MaximizedWindow;
	}

	// create window
	mWindow = SDL_CreateWindow(mConfig.title.c_str(),
                               static_cast<int>(mConfig.size.width),
							   static_cast<int>(mConfig.size.height),
                               static_cast<uint32_t>(mConfig.windowFlag));
	GLEAM_ASSERT(mWindow, "Window creation failed!");
    
	EventDispatcher<WindowResizeEvent>::Subscribe([this](const WindowResizeEvent& e)
	{
		mConfig.size.width = static_cast<float>(e.GetWidth());
        mConfig.size.height = static_cast<float>(e.GetHeight());
	});
}

DisplayMode WindowSystem::GetCurrentDisplayMode(uint32_t monitor) const
{
    auto currDisplay = SDL_GetCurrentDisplayMode(monitor);
    return DisplayMode
    {
        static_cast<SDL_PixelFormatEnum>(currDisplay->format),
        static_cast<uint32_t>(currDisplay->w),
        static_cast<uint32_t>(currDisplay->h),
        static_cast<uint32_t>(currDisplay->refresh_rate),
        monitor
    };
}

TArray<DisplayMode> WindowSystem::GetAvailableDisplayModes() const
{
    TArray<DisplayMode> displayModes;
    
    int numDisplays = 0;
    auto displays = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &numDisplays);
    if (displays)
    {
        displayModes.reserve(numDisplays);
        uint32_t monitor = SDL_GetDisplayForWindow(mWindow);
        
        for (int i = 0; i < numDisplays; ++i)
        {
            auto display = displays[i];
            displayModes[i] = DisplayMode
            {
                static_cast<SDL_PixelFormatEnum>(display->format),
                static_cast<uint32_t>(display->w),
                static_cast<uint32_t>(display->h),
                static_cast<uint32_t>(display->refresh_rate),
                monitor
            };
        }
        SDL_free(displays);
    }
    return displayModes;
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
