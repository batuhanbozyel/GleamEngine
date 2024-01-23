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

struct WindowConfig
{
	TString title = "Gleam Application";
	WindowFlag windowFlag = WindowFlag::CustomWindow;
    Size size = {1280.0f, 720.0f};
    float refreshRate = 60.0f;
};

} // namespace Gleam
