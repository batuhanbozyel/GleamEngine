#pragma once
#include "WindowConfig.h"

namespace Gleam {

class Window
{
public:

	Window(const WindowProperties& props);
	~Window();

	const WindowProperties& GetProps() const
	{
		return mProps;
	}

	SDL_Window* GetSDLWindow() const
	{
		return mWindow;
	}

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

private:

	SDL_Window* mWindow;
	WindowProperties mProps;

};

} // namespace Gleam