#pragma once
#include <SDL3/SDL.h>

namespace Gleam {

#if defined(USE_DIRECTX_RENDERER)
#define GLEAM_WINDOW_RENDERER_API 0
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
	WindowFlag windowFlag = WindowFlag::MaximizedWindow;
    Size size = {0.0f, 0.0f};
    float refreshRate = 60.0f;
};

} // namespace Gleam

GLEAM_ENUM(Gleam::WindowFlag, Guid("1237B8F5-13B3-4166-809D-96F61DF19A4A"))
GLEAM_TYPE(Gleam::WindowConfig, Guid("5AA6CC17-C7FB-4A8E-86ED-A58C7092CFB1"))
	GLEAM_FIELD(windowFlag, Serializable())
	GLEAM_FIELD(size, Serializable())
	GLEAM_FIELD(refreshRate, Serializable())
GLEAM_END
