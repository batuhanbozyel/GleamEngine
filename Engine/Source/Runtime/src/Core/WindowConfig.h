#pragma once
#include <SDL.h>

namespace Gleam {

#ifdef USE_VULKAN_RENDERER
#define GLEAM_WINDOW_RENDERER_API SDL_WINDOW_VULKAN
#else
#define GLEAM_WINDOW_RENDERER_API SDL_WINDOW_METAL
#endif 

enum class WindowFlag : uint32_t
{
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
	BorderlessFullscreen = SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE | GLEAM_WINDOW_RENDERER_API,
	ExclusiveFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE | GLEAM_WINDOW_RENDERER_API,
#else
	BorderlessFullscreen = SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN | GLEAM_WINDOW_RENDERER_API,
	ExclusiveFullscreen = SDL_WINDOW_FULLSCREEN | GLEAM_WINDOW_RENDERER_API,
#endif
	MaximizedWindow = SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | GLEAM_WINDOW_RENDERER_API,
	CustomWindow = SDL_WINDOW_RESIZABLE | GLEAM_WINDOW_RENDERER_API
};

struct DisplayMode
{
	uint32_t width = 0, height = 0;
	uint32_t refreshRate = 0;
	uint32_t monitor = 0;

	DisplayMode() = default;

	DisplayMode(uint32_t monitor)
		: monitor(monitor)
	{
	}

	DisplayMode(uint32_t width, uint32_t height)
		: width(width), height(height)
	{
	}

	DisplayMode(uint32_t width, uint32_t height, uint32_t monitor)
		: width(width), height(height), monitor(monitor)
	{
	}

	DisplayMode(uint32_t width, uint32_t height, uint32_t refreshRate, uint32_t monitor)
		: width(width), height(height), refreshRate(refreshRate), monitor(monitor)
	{
	}
};

struct WindowProperties
{
	TString title = "Gleam Application";
	WindowFlag windowFlag = WindowFlag::CustomWindow;
	DisplayMode display;

	WindowProperties() = default;

	WindowProperties(const TString& title, WindowFlag flag)
		: title(title), windowFlag(flag)
	{
	}

	WindowProperties(const TString& title, WindowFlag flag, uint32_t monitor)
		: title(title), windowFlag(flag), display(monitor)
	{
	}

	WindowProperties(const TString& title, uint32_t width, uint32_t height, WindowFlag flag)
		: title(title), windowFlag(flag), display(width, height)
	{
	}

	WindowProperties(const TString& title, uint32_t width, uint32_t height, uint32_t monitor, WindowFlag flag)
		: title(title), display(width, height, monitor), windowFlag(flag)
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

	static TArray<DisplayMode> GetAvailableDisplayModes(uint32_t monitor)
	{
		uint32_t numDisplayModes = GetNumDisplayModes(monitor);
		TArray<DisplayMode> availableDisplayModes(numDisplayModes);
		for (uint32_t i = 0; i < numDisplayModes; i++)
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

} // namespace Gleam