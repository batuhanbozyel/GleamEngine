#pragma once
#pragma warning(push, 0)
#include <SDL.h>
#pragma warning(pop)

namespace Gleam {

enum class WindowFlag : uint32_t
{
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
	BorderlessFullscreen = SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE,
	ExclusiveFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE,
#else
	BorderlessFullscreen = SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN,
	ExclusiveFullscreen = SDL_WINDOW_FULLSCREEN,
#endif
	MaximizedWindow = SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE,
	CustomWindow = SDL_WINDOW_RESIZABLE
};

struct DisplayMode
{
	uint32_t Width = 0, Height = 0;
	uint32_t RefreshRate = 0;
	uint32_t Monitor = 0;

	DisplayMode() = default;

	DisplayMode(uint32_t monitor)
		: Monitor(monitor)
	{
	}

	DisplayMode(uint32_t width, uint32_t height)
		: Width(width), Height(height)
	{
	}

	DisplayMode(uint32_t width, uint32_t height, uint32_t monitor)
		: Width(width), Height(height), Monitor(monitor)
	{
	}

	DisplayMode(uint32_t width, uint32_t height, uint32_t refreshRate, uint32_t monitor)
		: Width(width), Height(height), RefreshRate(refreshRate), Monitor(monitor)
	{
	}
};

struct WindowProperties
{
	TString Title = "Gleam Application";
	DisplayMode Display;
	WindowFlag Flag = WindowFlag::CustomWindow;

	WindowProperties(const TString& title, WindowFlag flag)
		: Title(title), Flag(flag)
	{
	}

	WindowProperties(const TString& title, WindowFlag flag, uint32_t monitor)
		: Title(title), Flag(flag), Display(monitor)
	{
	}

	WindowProperties(const TString& title, uint32_t width, uint32_t height, WindowFlag flag)
		: Title(title), Flag(flag), Display(width, height)
	{
	}

	WindowProperties(const TString& title, uint32_t width, uint32_t height, uint32_t monitor, WindowFlag flag)
		: Title(title), Display(width, height, monitor), Flag(flag)
	{
	}
};

class WindowConfig
{
public:

	static DisplayMode GetCurrentDisplayMode(uint32_t monitor)
	{
		SDL_DisplayMode currDisplay;
		SDL_GetCurrentDisplayMode(monitor, &currDisplay);
		return DisplayMode (
			currDisplay.w,
			currDisplay.h,
			currDisplay.refresh_rate,
			monitor
		);
	}

	static DisplayMode GetDisplayModeWithIndex(uint32_t monitor, uint32_t modeIndex)
	{
		SDL_DisplayMode currDisplay;
		SDL_GetDisplayMode(monitor, modeIndex, &currDisplay);
		return DisplayMode(
			currDisplay.w,
			currDisplay.h,
			currDisplay.refresh_rate,
			monitor
		);
	}

	static std::vector<DisplayMode> GetAvailableDisplayModes(uint32_t monitor)
	{
		uint32_t numDisplays = GetNumDisplayModes(monitor);
		TArray<DisplayMode> availableDisplayModes(numDisplays);
		for (uint32_t i = 0; i < numDisplays; i++)
		{
			availableDisplayModes[i] = GetDisplayModeWithIndex(monitor, i);
		}
		return availableDisplayModes;
	}

	static uint32_t GetNumDisplayModes(uint32_t monitor)
	{
		return SDL_GetNumDisplayModes(monitor);
	}

	static uint32_t GetNumVideoDisplays()
	{
		return SDL_GetNumVideoDisplays();
	}

};

}