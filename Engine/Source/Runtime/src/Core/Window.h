#pragma once
#include "WindowConfig.h"

namespace Gleam {

class Window
{
public:

	Window(const WindowProperties& props);
	~Window();

	const WindowProperties& GetProperties() const
	{
		return mProperties;
	}

	SDL_Window* GetSDLWindow() const
	{
		return mWindow;
	}

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

private:

	SDL_Window* mWindow;
	WindowProperties mProperties;

};

} // namespace Gleam