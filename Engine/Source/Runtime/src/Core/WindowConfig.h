#pragma once
#include <SDL3/SDL.h>

namespace Gleam {

#ifdef USE_VULKAN_RENDERER
#define GLEAM_WINDOW_RENDERER_API SDL_WINDOW_VULKAN
#else
#define GLEAM_WINDOW_RENDERER_API SDL_WINDOW_METAL
#endif 

enum class WindowFlag : uint32_t
{
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
	BorderlessFullscreen = SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | GLEAM_WINDOW_RENDERER_API,
	ExclusiveFullscreen = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | GLEAM_WINDOW_RENDERER_API,
#else
	BorderlessFullscreen = SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY| GLEAM_WINDOW_RENDERER_API,
	ExclusiveFullscreen = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY | GLEAM_WINDOW_RENDERER_API,
#endif
	MaximizedWindow = SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | GLEAM_WINDOW_RENDERER_API,
	CustomWindow = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | GLEAM_WINDOW_RENDERER_API
};

struct DisplayMode
{
	SDL_PixelFormatEnum format = SDL_PIXELFORMAT_UNKNOWN;
	uint32_t width = 0, height = 0;
	uint32_t refreshRate = 0;
	uint32_t monitor = 0;
};

struct WindowProperties
{
	TString title = "Gleam Application";
	WindowFlag windowFlag = WindowFlag::CustomWindow;
	DisplayMode display;
};

class WindowConfig
{
public:

	static DisplayMode GetCurrentDisplayMode(uint32_t monitor)
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

	static TArray<DisplayMode> GetAvailableDisplayModes()
	{
        SDL_DisplayID display = SDL_GetPrimaryDisplay();
        int num_modes = 0;
        auto modes = SDL_GetFullscreenDisplayModes(display, &num_modes);
        if (modes)
        {
            for (int i = 0; i < num_modes; ++i)
            {
                auto mode = modes[i];
                SDL_Log("Display %" SDL_PRIu32 " mode %d: %dx%d@%gx %gHz\n",
                        display, i, mode->w, mode->h, mode->pixel_density, mode->refresh_rate);
            }
            SDL_free(modes);
        }
	}

};

} // namespace Gleam
