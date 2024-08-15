#include "gpch.h"
#include "Engine.h"
#include "Globals.h"
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

	// create window
	auto newConfig = config;
	auto display = GetPrimaryDisplayMode();

	// if window size is not provided, use primary display mode
	if (static_cast<int>(newConfig.size.width) == 0 || static_cast<int>(newConfig.size.height) == 0)
	{
		newConfig.size.width = static_cast<float>(display.width);
		newConfig.size.height = static_cast<float>(display.height);
	}

	mWindow = SDL_CreateWindow(Globals::ProjectName.c_str(),
							   static_cast<int>(newConfig.size.width),
							   static_cast<int>(newConfig.size.height),
                               static_cast<uint32_t>(config.windowFlag));

	if (static_cast<int>(newConfig.size.width) != static_cast<int>(config.size.width) ||
		static_cast<int>(newConfig.size.height) != static_cast<int>(config.size.height))
	{
		WindowResizeEvent e(mWindow,
			static_cast<int>(newConfig.size.width),
			static_cast<int>(newConfig.size.height));

		EventDispatcher<WindowResizeEvent>::Publish(e);
	}

	// if refresh rate is not provided, use primary display mode
	if (newConfig.refreshRate == 0)
	{
		newConfig.refreshRate = display.refreshRate;
	}
	else if (newConfig.refreshRate != display.refreshRate) // if refresh rate config is different than the display, update display mode
	{
		uint32_t mode = 0;
		for (const auto& displayMode : GetAvailableDisplayModes())
		{
			if (displayMode.width == static_cast<uint32_t>(newConfig.size.width) &&
				displayMode.height == static_cast<uint32_t>(newConfig.size.height) &&
				displayMode.refreshRate == newConfig.refreshRate)
			{
				SetDisplayMode(mode);
				break;
			}
			++mode;
		}
	}

	Globals::Engine->UpdateConfig(newConfig);
	GLEAM_ASSERT(mWindow, "Window creation failed!");
}

void WindowSystem::SetDisplayMode(uint32_t mode) const
{
	auto displays = SDL_GetFullscreenDisplayModes(SDL_GetDisplayForWindow(mWindow), nullptr);
	SDL_SetWindowFullscreenMode(mWindow, displays[mode]);
	SDL_free(displays);
}

DisplayMode WindowSystem::GetPrimaryDisplayMode() const
{
	return GetDisplayMode(SDL_GetPrimaryDisplay());
}

DisplayMode WindowSystem::GetCurrentDisplayMode() const
{
	return GetDisplayMode(SDL_GetDisplayForWindow(mWindow));
}

DisplayMode WindowSystem::GetDisplayMode(uint32_t monitor) const
{
    auto currDisplay = SDL_GetCurrentDisplayMode(monitor);
    return DisplayMode
    {
        static_cast<SDL_PixelFormat>(currDisplay->format),
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
    auto displays = SDL_GetFullscreenDisplayModes(SDL_GetDisplayForWindow(mWindow), &numDisplays);
    if (displays)
    {
        displayModes.resize(numDisplays);
        uint32_t monitor = SDL_GetDisplayForWindow(mWindow);
        
        for (int i = 0; i < numDisplays; ++i)
        {
            auto display = displays[i];
            displayModes[i] = DisplayMode
            {
                static_cast<SDL_PixelFormat>(display->format),
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
