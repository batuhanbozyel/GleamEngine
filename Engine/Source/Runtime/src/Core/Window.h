#pragma once
#include "Events/Event.h"

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

struct WindowProperties
{
	TString Title = "Gleam Application";
	uint32_t Width = 1280, Height = 720;
	WindowFlag Flag = WindowFlag::CustomWindow;
};

class Window
{
public:

	Window(const WindowProperties& props);

	~Window();

	void OnUpdate();

	void OnResize(uint32_t width, uint32_t height);

	const WindowProperties& GetProps() const
	{
		return m_Props;
	}

private:

	WindowProperties m_Props;
	SDL_Window* m_Window;
	SDL_Event m_Event;

};

}