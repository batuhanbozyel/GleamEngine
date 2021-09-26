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
};

class Window
{
public:

	Window(const WindowProperties& props, WindowFlag flag);

	~Window();

	void OnUpdate();

	void OnResize(uint32_t width, uint32_t height);

	void SetEventCallback(EventCallbackFn&& callback)
	{
		m_EventCallback = std::move(callback);
	}

	const WindowProperties& GetProps() const
	{
		return m_Props;
	}

private:

	WindowProperties m_Props;
	EventCallbackFn m_EventCallback;

	SDL_Window* m_Window;
	SDL_Event m_Event;

};

}